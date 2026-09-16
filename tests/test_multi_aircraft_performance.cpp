/**
 * @file test_multi_aircraft_performance.cpp
 * @brief Dynamic flight testing and performance envelope validation across
 * all 5 combat aircraft (F-16C, F-15EX, Typhoon, F-22A, A-10C).
 */
#include "fastjet/aircraft/aircraft_type.hpp"
#include "fastjet/aircraft/aircraft_config.hpp"
#include "fastjet/fdm/rk4_integrator.hpp"
#include "fastjet/fdm/fuel_system.hpp"
#include "fastjet/environment/atmosphere1976.hpp"
#include "fastjet/aero/aircraft_aero_model.hpp"
#include "fastjet/flcs/aircraft_flcs.hpp"
#include "fastjet/propulsion/multi_engine.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace fastjet;
using namespace fastjet::aircraft;

namespace {

constexpr double DT = 0.005;

struct FlightSimHarness {
    AircraftType type;
    fdm::FlightState state{};
    fdm::MassProperties mass{};
    fdm::RK4Integrator integrator{DT};
    aero::AircraftAeroModel aero{};
    flcs::AircraftFLCS flcs{};
    propulsion::MultiEngine engine{};
    fdm::AircraftForces forces{};
    double sim_time{0.0};

    explicit FlightSimHarness(AircraftType t) : type(t), aero(t), flcs(t), engine(t) {
        mass = fdm::FuelSystem::compute(engine.fuel_kg, t);
    }

    void trim_level(double alt_m, double airspeed_mps) {
        state.pos_ned = math::Vector3(0.0, 0.0, -alt_m);
        state.vel_b = math::Vector3(airspeed_mps, 0.0, 0.0);
        state.omega_b = math::Vector3::zero();
        state.q_att = math::Quaternion::identity();
    }

    void step(double throttle, const flcs::PilotCommands& cmd) {
        const auto air = environment::Atmosphere1976::compute(state.altitude(), state.airspeed());
        const double thrust = engine.update(throttle, air, DT);
        engine.tvc_pitch_cmd = flcs.tvc_pitch_cmd;

        const auto surfaces = flcs.update(DT, state, cmd, air.dynamic_pressure, forces, mass);
        mass = fdm::FuelSystem::compute(engine.fuel_kg, type);

        forces = aero.compute_forces_and_moments(state, surfaces, air.dynamic_pressure, air.mach_number);
        forces.force_b.x += thrust;
        forces.moment_b.y += engine.pitch_moment();

        integrator.step(state, sim_time, mass,
            [&](double, const fdm::FlightState& s) noexcept -> fdm::AircraftForces {
                const auto stage_air = environment::Atmosphere1976::compute(s.altitude(), s.airspeed());
                auto f = aero.compute_forces_and_moments(s, surfaces, stage_air.dynamic_pressure, stage_air.mach_number);
                f.force_b.x += thrust;
                f.moment_b.y += engine.pitch_moment();
                return f;
            });
    }
};

void test_f15ex_high_speed_dash() {
    std::cout << "[PERF] Testing F-15EX Eagle II high-speed envelope...\n";
    FlightSimHarness sim(AircraftType::F15EX_EAGLE_II);
    // Trim at 12,000 m (tropopause) at Mach 1.5
    const auto air = environment::Atmosphere1976::compute(12000.0, 440.0);
    sim.trim_level(12000.0, 440.0);

    // Full afterburner acceleration
    flcs::PilotCommands cmd{};
    for (int i = 0; i < 2000; ++i) { // 10 seconds of flight
        sim.step(1.0, cmd);
    }

    // F-15EX twin F110s deliver massive excess thrust: aircraft accelerates rapidly
    const double final_mach = sim.state.airspeed() / air.speed_of_sound;
    assert(sim.engine.net_thrust_n > 100000.0); // High net thrust at altitude
    assert(final_mach > 1.50);
    std::cout << "  -> F-15EX high-speed dash passed (Mach: " << final_mach << ").\n";
}

void test_typhoon_supercruise() {
    std::cout << "[PERF] Testing Eurofighter Typhoon dry supercruise...\n";
    FlightSimHarness sim(AircraftType::EUROFIGHTER_TYPHOON);
    // Trim at 11,000 m at Mach 1.40
    const auto air = environment::Atmosphere1976::compute(11000.0, 413.0);
    sim.trim_level(11000.0, 413.0);

    // Military (dry) power: lever at 0.85 (no afterburner)
    flcs::PilotCommands cmd{};
    for (int i = 0; i < 2000; ++i) {
        sim.step(0.85, cmd);
    }

    assert(sim.engine.ab_power == 0.0); // Confirm dry power
    const double mach = sim.state.airspeed() / air.speed_of_sound;
    // Typhoon sustains supersonic flight on dry power alone
    assert(mach > 1.30);
    std::cout << "  -> Typhoon dry supercruise passed (Mach: " << mach << ").\n";
}

void test_f22_thrust_vectoring() {
    std::cout << "[PERF] Testing F-22A Raptor 2D thrust vectoring pitch authority...\n";
    FlightSimHarness sim(AircraftType::F22_RAPTOR);
    // Low-speed high-altitude regime: 80 m/s at 5,000 m
    sim.trim_level(5000.0, 80.0);

    flcs::PilotCommands cmd{};
    cmd.pitch_stick = -1.0; // Command aggressive pitch up

    for (int i = 0; i < 200; ++i) { // 1.0 second
        sim.step(0.85, cmd);
    }

    // Verify TVC was commanded and generated substantial pitching moment
    assert(sim.engine.config.has_thrust_vectoring == true);
    assert(std::abs(sim.flcs.tvc_pitch_cmd) > 0.3);
    assert(std::abs(sim.engine.pitch_moment()) > 10000.0); // Powerful TVC pitch moment
    std::cout << "  -> F-22A TVC pitch authority verified (Moment: "
              << sim.engine.pitch_moment() << " N*m).\n";
}

void test_a10_nacelle_pitch_coupling() {
    std::cout << "[PERF] Testing A-10C high-mounted nacelle pitch coupling...\n";
    FlightSimHarness sim(AircraftType::A10_THUNDERBOLT);
    sim.trim_level(1500.0, 150.0); // 150 m/s (~300 knots)

    // At idle, thrust is low
    flcs::PilotCommands cmd{};
    for (int i = 0; i < 200; ++i) sim.step(0.08, cmd);
    const double idle_moment = sim.engine.pitch_moment();

    // Slam throttle to maximum dry power (1.0)
    for (int i = 0; i < 400; ++i) sim.step(1.0, cmd);
    const double full_moment = sim.engine.pitch_moment();

    // A-10's high engine placement creates a nose-down moment on throttle application
    // (negative My in FastJet body axes where pitch-up is positive)
    assert(sim.engine.config.thrust_z_offset_m < 0.0);
    assert(full_moment < idle_moment); // Substantial nose-down moment from high thrust
    std::cout << "  -> A-10C pitch-thrust coupling verified (Nacelle moment: "
              << full_moment << " N*m).\n";
}

void test_a10_subsonic_drag_barrier() {
    std::cout << "[PERF] Testing A-10C transonic wave drag barrier...\n";
    aero::AircraftAeroModel aero(AircraftType::A10_THUNDERBOLT);
    fdm::FlightState s{};
    aero::ControlSurfaces ctrl{};

    // Below critical Mach (M = 0.50): drag is normal
    const auto c_sub = aero.compute_coefficients(s, ctrl, 20000.0, 0.50);
    // Transonic (M = 0.80): drag climbs steeply on the thick straight wing
    const auto c_trans = aero.compute_coefficients(s, ctrl, 20000.0, 0.80);

    assert(c_trans.CD > c_sub.CD * 2.5);
    std::cout << "  -> A-10C subsonic barrier passed (CD at M0.5: "
              << c_sub.CD << ", CD at M0.8: " << c_trans.CD << ").\n";
}

} // namespace

int main() {
    std::cout << "=========================================================\n";
    std::cout << "  Fast Jet Multi-Aircraft Flight Performance Test Suite  \n";
    std::cout << "=========================================================\n";

    test_f15ex_high_speed_dash();
    test_typhoon_supercruise();
    test_f22_thrust_vectoring();
    test_a10_nacelle_pitch_coupling();
    test_a10_subsonic_drag_barrier();

    std::cout << "\nAll aircraft flight performance tests successfully passed!\n";
    return 0;
}
