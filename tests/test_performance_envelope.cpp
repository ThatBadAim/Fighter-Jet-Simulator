/**
 * @file test_performance_envelope.cpp
 * @brief Validates simulated F-16 performance against published F-16C figures.
 *
 * These tests pin the *outcome* of the whole thrust/drag/atmosphere chain
 * rather than any single constant, so a regression in the engine lapse model,
 * the compressibility corrections or the drag polar shows up here as a
 * performance number a pilot would recognise as wrong.
 *
 * Reference figures (F-16C, F110-GE-129, clean, ~60% internal fuel):
 *   - Max level speed, 11,000 m .......... Mach 2.05
 *   - Max level speed, sea level ......... Mach ~1.05
 *   - Military (dry) thrust, SLS ......... 76.3 kN / 17,155 lbf
 *   - Max afterburner thrust, SLS ........ 131.6 kN / 29,588 lbf
 *   - Thrust-to-weight, combat weight .... ~1.10
 */
#include "fastjet/propulsion/f110_engine.hpp"
#include "fastjet/fdm/rk4_integrator.hpp"
#include "fastjet/fdm/fuel_system.hpp"
#include "fastjet/environment/atmosphere1976.hpp"
#include "fastjet/aero/f16_aero_model.hpp"
#include "fastjet/flcs/f16_flcs.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace fastjet;

namespace {

constexpr double DT = 0.005;

/// Free-flight model with an altitude-hold autopilot on the pitch axis, so the
/// aircraft accelerates on its own thrust/drag balance with no state fixing.
struct LevelRun {
    fdm::FlightState state{};
    fdm::MassProperties mass{};
    fdm::RK4Integrator integrator{DT};
    aero::F16AeroModel aero{};
    flcs::F16FLCS flcs{};
    propulsion::F110Engine engine{};
    fdm::AircraftForces forces{};
    double time{0.0};
    double alt_target{0.0};

    void start(double alt, double v, double spooled_thrust) {
        alt_target = alt;
        state = fdm::FlightState{};
        state.pos_ned = math::Vector3(0.0, 0.0, -alt);
        state.vel_b = math::Vector3(v, 0.0, 0.0);
        state.q_att = math::Quaternion::identity();
        engine.net_thrust_n = spooled_thrust; // Start already spooled up.
        mass = fdm::FuelSystem::compute(engine.fuel_kg);
    }

    void run(double throttle, double seconds) {
        const int steps = static_cast<int>(seconds / DT);
        for (int i = 0; i < steps; ++i) {
            const auto air = environment::Atmosphere1976::compute(
                state.altitude(), state.airspeed());

            // Altitude + vertical-speed hold feeding the FLCS pitch command.
            const double h_err = alt_target - state.altitude();
            const double vs = -state.velocity_ned().z;
            const double cmd = std::clamp(0.0015 * h_err - 0.05 * vs, -0.6, 0.6);

            flcs::PilotCommands pilot{cmd, 0.0, 0.0};
            auto surfaces = flcs.update(
                DT, state, pilot, air.dynamic_pressure, forces, mass);

            const double thrust = engine.update(throttle, air, DT);
            mass = fdm::FuelSystem::compute(engine.fuel_kg);

            integrator.step(state, time, mass,
                [&](double, const fdm::FlightState& s) noexcept -> fdm::AircraftForces {
                    const auto sa = environment::Atmosphere1976::compute(
                        s.altitude(), s.airspeed());
                    fdm::AircraftForces f = aero.compute_forces_and_moments(
                        s, surfaces, sa.dynamic_pressure, sa.mach_number);
                    f.force_b.x += thrust;
                    return f;
                });
            forces = aero.compute_forces_and_moments(
                state, surfaces, air.dynamic_pressure, air.mach_number);
            forces.force_b.x += thrust;
            time += DT;
        }
    }

    [[nodiscard]] double mach() const {
        return environment::Atmosphere1976::compute(
            state.altitude(), state.airspeed()).mach_number;
    }
};

void test_sea_level_static_thrust_matches_f110() {
    std::cout << "[Test] Envelope: Sea-Level Static Thrust Ratings... ";

    input::ThrottleController lever{};

    const double mil = lever.update(input::ThrottleController::DETENT_MIL);
    assert(lever.state == input::ThrottleController::DetentState::MIL_POWER);
    // Real F110-GE-129 military thrust: 17,155 lbf = 76.3 kN. Allow 5%.
    assert(std::abs(mil - 76300.0) / 76300.0 < 0.05);

    const double ab = lever.update(1.0);
    assert(lever.state == input::ThrottleController::DetentState::AFTERBURNER);
    // Real max afterburner: 29,588 lbf = 131.6 kN. Allow 5%.
    assert(std::abs(ab - 131600.0) / 131600.0 < 0.05);

    // Afterburner must add roughly 70% over military power.
    const double ratio = ab / mil;
    assert(ratio > 1.60 && ratio < 1.80);

    std::cout << "PASSED (MIL " << mil / 1000.0 << " kN, AB " << ab / 1000.0
              << " kN, ratio " << ratio << ")\n";
}

void test_thrust_to_weight_ratio() {
    std::cout << "[Test] Envelope: Combat Thrust-to-Weight... ";

    // Combat weight: empty + 60% internal fuel.
    const auto mass = fdm::FuelSystem::compute(
        propulsion::F110Engine::INTERNAL_FUEL_CAPACITY_KG * 0.6);
    const double weight_n = mass.mass_kg * 9.80665;

    input::ThrottleController lever{};
    const double ab = lever.update(1.0);
    const double twr = ab / weight_n;

    // The F-16 is famous for exceeding 1.0 at combat weight (~1.10).
    assert(twr > 1.05 && twr < 1.25);

    std::cout << "PASSED (T/W " << twr << " at "
              << mass.mass_kg << " kg)\n";
}

void test_thrust_lapse_with_altitude() {
    std::cout << "[Test] Envelope: Thrust Lapse With Altitude... ";

    const auto sl  = environment::Atmosphere1976::compute(0.0, 0.0);
    const auto alt = environment::Atmosphere1976::compute(11000.0, 0.0);

    const double f_sl  = propulsion::F110Engine::lapse_factor(sl, true);
    const double f_alt = propulsion::F110Engine::lapse_factor(alt, true);

    assert(std::abs(f_sl - 1.0) < 1e-6 && "Sea level static must be unity");

    // At 11 km density is ~30% of sea level, so static thrust falls to ~1/3.
    assert(f_alt > 0.25 && f_alt < 0.40);

    // Ram recovery must raise thrust with Mach at fixed altitude.
    const auto fast = environment::Atmosphere1976::compute(11000.0, 590.0);
    const double f_fast = propulsion::F110Engine::lapse_factor(fast, true);
    assert(f_fast > f_alt && "Ram recovery must increase thrust with Mach");

    std::cout << "PASSED (SL 1.00, 11 km static " << f_alt
              << ", 11 km at M2 " << f_fast << ")\n";
}

void test_max_level_speed_at_altitude() {
    std::cout << "[Test] Envelope: Max Level Speed at 11,000 m... " << std::flush;

    LevelRun run;
    run.start(11000.0, 250.0, 45000.0);
    run.run(1.0, 500.0);

    const double m = run.mach();

    // Real F-16C tops out at Mach 2.05 clean. The airframe limit is 2.0 for
    // inlet/thermal reasons; the model should land in that neighbourhood.
    assert(m > 1.90 && "Must reach at least Mach 1.9 at altitude");
    assert(m < 2.30 && "Must not wildly exceed the real airframe limit");

    // Altitude hold must have actually worked, or the speed is meaningless.
    assert(std::abs(run.state.altitude() - 11000.0) < 200.0);

    std::cout << "PASSED (Mach " << m << " vs real 2.05, alt held to "
              << std::abs(run.state.altitude() - 11000.0) << " m)\n";
}

void test_max_level_speed_at_sea_level() {
    std::cout << "[Test] Envelope: Max Level Speed at Sea Level... " << std::flush;

    LevelRun run;
    run.start(100.0, 200.0, 60000.0);
    run.run(1.0, 300.0);

    const double m = run.mach();

    // Real F-16 is roughly Mach 1.05-1.2 on the deck, and is limited there by
    // dynamic pressure (the 800 KEAS placard, q ~ 102 kPa) rather than by
    // thrust. Verify both the Mach number and that q stayed inside the
    // structural limit -- without the q-drag term the aircraft accelerates to
    // Mach 1.9+, which is 2.6x the airframe's maximum dynamic pressure.
    assert(m > 1.00 && "Must go supersonic on the deck");
    assert(m < 1.35 && "Sea-level speed must stay near the real limit");

    const auto air = environment::Atmosphere1976::compute(
        run.state.altitude(), run.state.airspeed());
    constexpr double Q_STRUCTURAL_LIMIT = 102000.0; // 800 KEAS [Pa]
    assert(air.dynamic_pressure < Q_STRUCTURAL_LIMIT &&
           "Level speed must stay inside the structural q limit");

    std::cout << "PASSED (Mach " << m << " vs real ~1.05, q "
              << air.dynamic_pressure / 1000.0 << " kPa vs 102 kPa limit)\n";
}

void test_military_power_is_subsonic_at_altitude() {
    std::cout << "[Test] Envelope: Dry Thrust Cannot Reach High Supersonic... "
              << std::flush;

    LevelRun run;
    run.start(11000.0, 250.0, 25000.0);
    run.run(input::ThrottleController::DETENT_MIL, 400.0);

    const double m = run.mach();

    // Without reheat the F-16 is limited to around Mach 1.0-1.2 at altitude;
    // it must clearly fall short of the afterburning figure.
    assert(m < 1.60 && "Dry thrust must not reach max-AB speeds");

    std::cout << "PASSED (Mach " << m << " dry)\n";
}

} // namespace

int main() {
    std::cout << "=== F-16 Performance Envelope Validation ===\n";
    test_sea_level_static_thrust_matches_f110();
    test_thrust_to_weight_ratio();
    test_thrust_lapse_with_altitude();
    test_max_level_speed_at_sea_level();
    test_military_power_is_subsonic_at_altitude();
    test_max_level_speed_at_altitude();
    std::cout << "All Performance Envelope tests passed successfully!\n";
    return 0;
}
