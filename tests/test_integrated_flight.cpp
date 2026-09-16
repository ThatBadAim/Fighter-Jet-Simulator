#include "fastjet/fdm/rk4_integrator.hpp"
#include "fastjet/fdm/fuel_system.hpp"
#include "fastjet/environment/atmosphere1976.hpp"
#include "fastjet/aero/f16_aero_model.hpp"
#include "fastjet/flcs/f16_flcs.hpp"
#include "fastjet/propulsion/f110_engine.hpp"
#include "fastjet/gear/landing_gear.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace fastjet;

namespace {

constexpr double DT = 0.005;

/**
 * @brief Full simulation stack: atmosphere -> FLCS -> aero -> engine -> gear -> RK4.
 * Mirrors the integration order used by the viewer.
 */
struct Sim {
    fdm::FlightState state{};
    fdm::MassProperties mass{};
    fdm::RK4Integrator integrator{DT};
    aero::F16AeroModel aero{};
    flcs::F16FLCS flcs{};
    propulsion::F110Engine engine{};
    gear::LandingGear gear{};
    fdm::AircraftForces forces{};
    double time{0.0};

    double throttle{0.85};
    double speedbrake{0.0};

    Sim() { mass = fdm::FuelSystem::compute(engine.fuel_kg); }

    void step(const flcs::PilotCommands& pilot) {
        const auto air = environment::Atmosphere1976::compute(
            state.altitude(), state.airspeed());

        auto surfaces = flcs.update(DT, state, pilot, air.dynamic_pressure, forces, mass);
        surfaces.speedbrake = speedbrake;

        const double thrust = engine.update(throttle, air, DT);
        mass = fdm::FuelSystem::compute(engine.fuel_kg);

        gear.steer_cmd = pilot.rudder_pedal;

        integrator.step(state, time, mass,
            [&](double, const fdm::FlightState& s) noexcept -> fdm::AircraftForces {
                const auto sa = environment::Atmosphere1976::compute(
                    s.altitude(), s.airspeed());
                fdm::AircraftForces f = aero.compute_forces_and_moments(
                    s, surfaces, sa.dynamic_pressure, sa.mach_number);
                f.force_b.x += thrust;
                const auto g = gear.compute(s, mass);
                f.force_b = f.force_b + g.force_b;
                f.moment_b = f.moment_b + g.moment_b;
                return f;
            });

        forces = aero.compute_forces_and_moments(
            state, surfaces, air.dynamic_pressure, air.mach_number);
        forces.force_b.x += thrust;
    }

    void set_airborne(double alt_m, double speed_mps) {
        state = fdm::FlightState{};
        state.pos_ned = math::Vector3(0.0, 0.0, -alt_m);
        state.vel_b = math::Vector3(speed_mps, 0.0, 0.0);
        state.q_att = math::Quaternion::identity();
        gear.deployed = false;
    }
};

void test_takeoff_roll() {
    std::cout << "[Test] Integrated: Ground Roll & Rotation to Flight... ";

    Sim sim;
    // Start stationary on the runway, gear down, full afterburner.
    sim.state.pos_ned = math::Vector3(0.0, 0.0, -1.59);
    sim.state.q_att = math::Quaternion::identity();
    sim.throttle = 1.0;

    assert(sim.gear.deployed);

    double max_speed = 0.0;
    bool got_airborne = false;
    flcs::PilotCommands pilot{0.0, 0.0, 0.0};

    // Accelerate for 30 s, rotating once past ~70 m/s.
    for (int i = 0; i < 6000; ++i) {
        const double v = sim.state.airspeed();
        max_speed = std::max(max_speed, v);
        pilot.pitch_stick = (v > 70.0) ? 0.45 : 0.0; // Rotate
        sim.step(pilot);
        if (sim.state.altitude() > 15.0) { got_airborne = true; break; }
    }

    assert(max_speed > 70.0 && "aircraft must accelerate on the runway");
    assert(got_airborne && "aircraft must rotate and climb away");
    assert(!sim.gear.collapsed && "gear must survive a normal takeoff");
    assert(sim.engine.fuel_kg < propulsion::F110Engine::INTERNAL_FUEL_CAPACITY_KG &&
           "takeoff must consume fuel");

    std::cout << "PASSED (Unstuck at " << max_speed << " m/s, climbing through "
              << sim.state.altitude() << " m)\n";
}

void test_speedbrake_decelerates_in_flight() {
    std::cout << "[Test] Integrated: Speedbrake Deceleration in Flight... ";

    flcs::PilotCommands level{0.0, 0.0, 0.0};

    Sim clean, braked;
    clean.set_airborne(5000.0, 250.0);
    braked.set_airborne(5000.0, 250.0);
    clean.throttle = 0.60;
    braked.throttle = 0.60;
    braked.speedbrake = 1.0;

    for (int i = 0; i < 4000; ++i) { // 20 s
        clean.step(level);
        braked.step(level);
    }

    assert(braked.state.airspeed() < clean.state.airspeed() &&
           "extended speedbrake must slow the aircraft");

    std::cout << "PASSED (Clean " << clean.state.airspeed()
              << " m/s vs speedbrake " << braked.state.airspeed() << " m/s after 20 s)\n";
}

void test_fuel_burn_changes_mass() {
    std::cout << "[Test] Integrated: Fuel Burn Reduces Gross Mass in Flight... ";

    Sim sim;
    sim.set_airborne(8000.0, 250.0);
    sim.throttle = 1.0; // Afterburner burns fast

    const double m0 = sim.mass.mass_kg;
    const double f0 = sim.engine.fuel_kg;
    const double Iyy0 = sim.mass.Iyy;

    flcs::PilotCommands level{0.0, 0.0, 0.0};
    for (int i = 0; i < 12000; ++i) sim.step(level); // 60 s

    assert(sim.engine.fuel_kg < f0 && "fuel must be consumed");
    assert(sim.mass.mass_kg < m0 && "gross mass must fall as fuel burns");
    assert(sim.mass.Iyy < Iyy0 && "inertia must fall with mass");

    // Mass loss must exactly equal fuel burned.
    const double burned = f0 - sim.engine.fuel_kg;
    assert(std::abs((m0 - sim.mass.mass_kg) - burned) < 1e-6);

    std::cout << "PASSED (Burned " << burned << " kg in 60 s AB, mass "
              << m0 << " -> " << sim.mass.mass_kg << " kg)\n";
}

void test_altitude_thrust_lapse_in_flight() {
    std::cout << "[Test] Integrated: Altitude Robs Thrust in Flight... ";

    flcs::PilotCommands level{0.0, 0.0, 0.0};

    Sim low, high;
    low.set_airborne(1000.0, 250.0);
    high.set_airborne(13000.0, 250.0);
    low.throttle = 0.85;
    high.throttle = 0.85;

    for (int i = 0; i < 1000; ++i) { low.step(level); high.step(level); }

    assert(high.engine.net_thrust_n < low.engine.net_thrust_n &&
           "thrust at altitude must be lower than down low");

    std::cout << "PASSED (1 km: " << low.engine.net_thrust_n / 1000.0
              << " kN vs 13 km: " << high.engine.net_thrust_n / 1000.0 << " kN)\n";
}

void test_gear_survives_gentle_landing() {
    std::cout << "[Test] Integrated: Gentle Touchdown Survives, Hard One Fails... ";

    flcs::PilotCommands level{0.0, 0.0, 0.0};

    // Gentle approach. A full-fuel F-16 stalls below ~90 m/s, so the approach
    // is flown at 125 m/s with power on: fast enough that the wing carries the
    // aircraft, slow enough to be a realistic threshold speed.
    Sim soft;
    soft.state.pos_ned = math::Vector3(0.0, 0.0, -2.2);
    soft.state.vel_b = math::Vector3(125.0, 0.0, 1.0); // ~1 m/s sink
    soft.state.q_att = math::Quaternion::identity();
    soft.gear.deployed = true;
    soft.throttle = 0.62;

    for (int i = 0; i < 2000; ++i) {
        soft.step(level);
        if (soft.gear.collapsed) break;
        if (soft.gear.mains_on_ground() && soft.state.altitude() < 1.7) break;
    }
    assert(soft.gear.weight_on_wheels() && "aircraft must actually touch down");
    assert(!soft.gear.collapsed && "a gentle touchdown must not break the gear");
    assert(soft.gear.max_sink_rate < gear::LandingGear::GEAR_LIMIT_SINK_MPS);

    // Hard arrival: well beyond the structural limit.
    Sim hard;
    hard.state.pos_ned = math::Vector3(0.0, 0.0, -3.0);
    hard.state.vel_b = math::Vector3(125.0, 0.0, 8.0); // 8 m/s sink
    hard.state.q_att = math::Quaternion::identity();
    hard.gear.deployed = true;

    for (int i = 0; i < 600; ++i) {
        hard.step(level);
        if (hard.gear.collapsed) break;
    }
    assert(hard.gear.collapsed && "an 8 m/s arrival must collapse the gear");

    std::cout << "PASSED (touchdown at " << soft.gear.max_sink_rate
              << " m/s survived, 8.0 m/s collapsed)\n";
}

void test_energy_sanity_in_cruise() {
    std::cout << "[Test] Integrated: Stable Cruise Without Divergence... ";

    Sim sim;
    sim.set_airborne(6000.0, 240.0);
    sim.throttle = 0.75;

    flcs::PilotCommands level{0.0, 0.0, 0.0};
    for (int i = 0; i < 8000; ++i) sim.step(level); // 40 s

    // The FLCS must hold the aircraft in controlled flight: no NaNs, no
    // runaway attitude, no descent into the ground.
    assert(std::isfinite(sim.state.airspeed()));
    assert(std::isfinite(sim.state.altitude()));
    assert(sim.state.altitude() > 1000.0 && "must not descend into terrain");
    assert(sim.state.altitude() < 20000.0 && "must not zoom-climb uncontrollably");
    assert(std::abs(sim.state.roll()) < M_PI / 2.0 && "must stay upright");
    assert(sim.state.airspeed() > 50.0 && sim.state.airspeed() < 700.0);

    std::cout << "PASSED (40 s cruise: " << sim.state.altitude() << " m at "
              << sim.state.airspeed() << " m/s, bank "
              << sim.state.roll() * 180.0 / M_PI << " deg)\n";
}

} // namespace

int main() {
    std::cout << "=== Integrated Flight Stack Verification ===\n";

    test_takeoff_roll();
    test_speedbrake_decelerates_in_flight();
    test_fuel_burn_changes_mass();
    test_altitude_thrust_lapse_in_flight();
    test_gear_survives_gentle_landing();
    test_energy_sanity_in_cruise();

    std::cout << "All Integrated Flight tests passed successfully!\n";
    return 0;
}
