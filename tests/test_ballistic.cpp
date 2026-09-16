#include "../include/fastjet/fdm/rk4_integrator.hpp"
#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>

using namespace fastjet::math;
using namespace fastjet::fdm;

void test_horizontal_ballistic_flight() {
    std::cout << "[Test] 6-DoF Ballistic Flight under Gravity (Horizontal Launch)... " << std::flush;

    const MassProperties mass = MassProperties::create_clean_f16();
    const RK4Integrator integrator(0.005); // 200 Hz

    // Initial state: Level flight at 10,000 m altitude, 250 m/s forward speed
    FlightState state;
    state.pos_ned = Vector3(0.0, 0.0, -10000.0); // NED z = -10000m (altitude = +10000m)
    state.vel_b = Vector3(250.0, 0.0, 0.0);       // 250 m/s forward
    state.omega_b = Vector3::zero();              // Zero angular rate
    state.q_att = Quaternion::identity();         // Level attitude

    double time = 0.0;
    constexpr double DURATION = 20.0; // 20 seconds of free-fall (4,000 steps)
    const int total_steps = static_cast<int>(DURATION / integrator.dt);

    // Initial total specific energy: E/m = 0.5 * V^2 + g0 * altitude = 0.5 * V^2 - g0 * z_ned
    const double initial_specific_energy = 0.5 * state.airspeed() * state.airspeed() +
                                           SixDoFFDM::GRAVITY_ACCEL * state.altitude();

    for (int step = 0; step < total_steps; ++step) {
        integrator.step(state, time, mass, AircraftForces::zero());
    }

    // Analytical verification after t = DURATION:
    // x(t) = x0 + u0 * t
    const double expected_x = 250.0 * DURATION; // 5000 m
    // z(t) = z0 + 0.5 * g0 * t^2
    const double expected_z = -10000.0 + 0.5 * SixDoFFDM::GRAVITY_ACCEL * DURATION * DURATION;
    // v_x(t) = 250 m/s, v_z(t) = g0 * t
    const double expected_vz = SixDoFFDM::GRAVITY_ACCEL * DURATION;

    const Vector3 v_ned = state.velocity_ned();
    const double final_specific_energy = 0.5 * state.airspeed() * state.airspeed() +
                                         SixDoFFDM::GRAVITY_ACCEL * state.altitude();

    // Check position tolerances (< 1e-4 m after 4,000 steps)
    const double pos_err_x = std::abs(state.pos_ned.x - expected_x);
    const double pos_err_z = std::abs(state.pos_ned.z - expected_z);
    assert(pos_err_x < 1e-4);
    assert(pos_err_z < 1e-4);

    // Check velocity tolerances (< 1e-5 m/s)
    const double vel_err_x = std::abs(v_ned.x - 250.0);
    const double vel_err_z = std::abs(v_ned.z - expected_vz);
    assert(vel_err_x < 1e-5);
    assert(vel_err_z < 1e-5);

    // Check quaternion normalization
    assert(std::abs(state.q_att.norm() - 1.0) < 1e-12);

    // Check conservation of mechanical energy: delta_E / E < 1e-6
    const double energy_error = std::abs(final_specific_energy - initial_specific_energy);
    assert(energy_error < 1e-4);

    std::cout << "PASSED\n"
              << "  Simulated Time: " << time << " s (" << total_steps << " steps @ 200 Hz)\n"
              << "  Final Pos NED:  " << state.pos_ned << " m (Error: x=" << pos_err_x << " m, z=" << pos_err_z << " m)\n"
              << "  Final Vel NED:  " << v_ned << " m/s (Error: vz=" << vel_err_z << " m/s)\n"
              << "  Energy Drift:   " << energy_error << " J/kg\n";
}

void test_inclined_ballistic_flight_arbitrary_attitude() {
    std::cout << "[Test] 6-DoF Ballistic Flight with Arbitrary Attitude & 3D Velocity... " << std::flush;

    const MassProperties mass = MassProperties::create_clean_f16();
    const RK4Integrator integrator(0.005);

    // Pitch 30 deg, roll 20 deg, yaw 45 deg
    constexpr double roll = 20.0 * M_PI / 180.0;
    constexpr double pitch = 30.0 * M_PI / 180.0;
    constexpr double yaw = 45.0 * M_PI / 180.0;

    FlightState state;
    state.pos_ned = Vector3(100.0, -200.0, -5000.0); // 5000 m altitude
    state.q_att = Quaternion::from_euler(roll, pitch, yaw);
    state.vel_b = Vector3(220.0, 15.0, -8.0);
    state.omega_b = Vector3::zero(); // No rotation for pure ballistic center-of-mass trajectory

    const Vector3 initial_pos_ned = state.pos_ned;
    const Vector3 initial_vel_ned = state.velocity_ned();

    double time = 0.0;
    constexpr double DURATION = 15.0; // 3,000 steps
    const int total_steps = static_cast<int>(DURATION / integrator.dt);

    for (int step = 0; step < total_steps; ++step) {
        integrator.step(state, time, mass, AircraftForces::zero());
    }

    // Analytical trajectory in NED frame:
    // p_ned(t) = p_ned(0) + v_ned(0)*t + 0.5 * [0, 0, g0] * t^2
    const Vector3 expected_pos(
        initial_pos_ned.x + initial_vel_ned.x * DURATION,
        initial_pos_ned.y + initial_vel_ned.y * DURATION,
        initial_pos_ned.z + initial_vel_ned.z * DURATION + 0.5 * SixDoFFDM::GRAVITY_ACCEL * DURATION * DURATION
    );
    const Vector3 expected_vel(
        initial_vel_ned.x,
        initial_vel_ned.y,
        initial_vel_ned.z + SixDoFFDM::GRAVITY_ACCEL * DURATION
    );

    const Vector3 final_vel_ned = state.velocity_ned();

    const double max_pos_err = std::max({
        std::abs(state.pos_ned.x - expected_pos.x),
        std::abs(state.pos_ned.y - expected_pos.y),
        std::abs(state.pos_ned.z - expected_pos.z)
    });
    const double max_vel_err = std::max({
        std::abs(final_vel_ned.x - expected_vel.x),
        std::abs(final_vel_ned.y - expected_vel.y),
        std::abs(final_vel_ned.z - expected_vel.z)
    });

    assert(max_pos_err < 1e-4);
    assert(max_vel_err < 1e-5);
    assert(std::abs(state.q_att.norm() - 1.0) < 1e-12);

    std::cout << "PASSED (Max pos err: " << max_pos_err << " m, Max vel err: " << max_vel_err << " m/s)\n";
}

void test_steady_level_trim_flight() {
    std::cout << "[Test] Steady-State Level Flight Equilibrium (Lift = Weight)... " << std::flush;

    const MassProperties mass = MassProperties::create_clean_f16();
    const RK4Integrator integrator(0.005);

    FlightState state;
    state.pos_ned = Vector3(0.0, 0.0, -10000.0); // 10,000 m altitude
    state.vel_b = Vector3(200.0, 0.0, 0.0);       // 200 m/s cruise
    state.omega_b = Vector3::zero();
    state.q_att = Quaternion::identity();

    // In level flight with q = identity, body Z aligns with NED Down Z.
    // Gravity pulls with +m*g0 along body Z.
    // Aerodynamic lift acts upward along -Z (negative belly): L = m * g0.
    const double lift_force = mass.mass_kg * SixDoFFDM::GRAVITY_ACCEL;
    const AircraftForces trim_forces(Vector3(0.0, 0.0, -lift_force), Vector3::zero());

    double time = 0.0;
    constexpr double DURATION = 60.0; // 1 minute of flight (12,000 steps)
    const int total_steps = static_cast<int>(DURATION / integrator.dt);

    for (int step = 0; step < total_steps; ++step) {
        integrator.step(state, time, mass, trim_forces);
    }

    // Altitude should remain exactly 10,000 m
    const double alt_drift = std::abs(state.altitude() - 10000.0);
    assert(alt_drift < 1e-8);

    // Forward speed should remain exactly 200.0 m/s
    const double vel_drift = std::abs(state.vel_b.x - 200.0);
    assert(vel_drift < 1e-8);

    // Distance covered should be exactly 200 m/s * 60 s = 12,000 m
    const double dist_err = std::abs(state.pos_ned.x - 12000.0);
    assert(dist_err < 1e-6);

    std::cout << "PASSED (Altitude drift: " << alt_drift << " m, Speed drift: "
              << vel_drift << " m/s over 12,000 steps)\n";
}

int main() {
    std::cout << "=== 6-DoF Ballistic & Equilibrium Flight Dynamics Verification ===\n";
    test_horizontal_ballistic_flight();
    test_inclined_ballistic_flight_arbitrary_attitude();
    test_steady_level_trim_flight();
    std::cout << "All Ballistic Flight tests passed successfully!\n\n";
    return 0;
}
