#include "../include/fastjet/fdm/rk4_integrator.hpp"
#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>

using namespace fastjet::math;
using namespace fastjet::fdm;

void test_pitch_rate_response() {
    std::cout << "[Test] Rotational Dynamics: Uncoupled Pitch Moment Response... " << std::flush;

    const MassProperties mass = MassProperties::create_clean_f16();
    const RK4Integrator integrator(0.005); // 200 Hz

    FlightState state;
    state.omega_b = Vector3::zero();
    state.q_att = Quaternion::identity();

    // Apply constant pitch-up moment My = 50,000 N*m
    constexpr double MY = 50000.0;
    const AircraftForces forces(Vector3::zero(), Vector3(0.0, MY, 0.0));

    double time = 0.0;
    constexpr double DURATION = 1.0; // 1 second
    const int total_steps = static_cast<int>(DURATION / integrator.dt);

    for (int step = 0; step < total_steps; ++step) {
        integrator.step(state, time, mass, forces);
    }

    // Since Ixy = Iyz = 0, pitch acceleration is uncoupled: q_dot = My / Iyy
    const double expected_q = (MY / mass.Iyy) * DURATION;
    const double q_err = std::abs(state.omega_b.y - expected_q);

    assert(q_err < 1e-6);
    assert(std::abs(state.omega_b.x) < 1e-10); // Roll rate remains zero
    assert(std::abs(state.omega_b.z) < 1e-10); // Yaw rate remains zero

    std::cout << "PASSED (Final q: " << state.omega_b.y << " rad/s, Expected: "
              << expected_q << " rad/s, Error: " << q_err << " rad/s)\n";
}

void test_torque_free_angular_momentum_conservation() {
    std::cout << "[Test] Rotational Dynamics: Torque-Free Angular Momentum Conservation... " << std::flush;

    // Use symmetric inertia tensor (Ixz = 0) so torque-free motion is exact Euler top precession
    const MassProperties mass(9298.64, 12875.0, 75674.0, 85552.0, 0.0);
    const RK4Integrator integrator(0.001); // 1 kHz for high angular velocity accuracy

    FlightState state;
    // Initial high spin rate (e.g. roll p = 1.5 rad/s, pitch q = 0.8 rad/s, yaw r = 0.3 rad/s)
    state.omega_b = Vector3(1.5, 0.8, 0.3);
    state.q_att = Quaternion::identity();

    // Initial angular momentum in NED frame: H_ned = C_n_b * (I * omega_b)
    const Vector3 H_b_0 = mass.I * state.omega_b;
    const Vector3 H_ned_0 = state.q_att.rotate_body_to_ned(H_b_0);
    const double H_mag_0 = H_ned_0.norm();

    // Initial rotational kinetic energy: T_rot = 0.5 * omega_b . (I * omega_b)
    const double T_rot_0 = 0.5 * state.omega_b.dot(H_b_0);

    double time = 0.0;
    constexpr double DURATION = 10.0; // 10 seconds of tumbling motion (10,000 steps)
    const int total_steps = static_cast<int>(DURATION / integrator.dt);

    for (int step = 0; step < total_steps; ++step) {
        // Zero external moments
        integrator.step(state, time, mass, AircraftForces::zero());
    }

    // Final angular momentum in NED frame
    const Vector3 H_b_final = mass.I * state.omega_b;
    const Vector3 H_ned_final = state.q_att.rotate_body_to_ned(H_b_final);
    const double H_mag_final = H_ned_final.norm();

    // Final rotational kinetic energy
    const double T_rot_final = 0.5 * state.omega_b.dot(H_b_final);

    // Verify conservation of magnitude of angular momentum
    const double H_mag_err = std::abs(H_mag_final - H_mag_0) / H_mag_0;
    assert(H_mag_err < 1e-4);

    // Verify conservation of rotational kinetic energy
    const double T_rot_err = std::abs(T_rot_final - T_rot_0) / T_rot_0;
    assert(T_rot_err < 1e-4);

    // Verify conservation of vector direction of H_ned in inertial frame
    const double H_vec_err = (H_ned_final - H_ned_0).norm() / H_mag_0;
    assert(H_vec_err < 1e-3);

    std::cout << "PASSED\n"
              << "  H_mag Drift:    " << H_mag_err * 100.0 << " %\n"
              << "  T_rot Drift:    " << T_rot_err * 100.0 << " %\n"
              << "  H_vec Drift:    " << H_vec_err * 100.0 << " %\n";
}

int main() {
    std::cout << "=== 6-DoF Rotational Dynamics & Conservation Law Verification ===\n";
    test_pitch_rate_response();
    test_torque_free_angular_momentum_conservation();
    std::cout << "All Rotational Dynamics tests passed successfully!\n\n";
    return 0;
}
