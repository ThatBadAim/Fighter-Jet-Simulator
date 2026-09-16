#include "../include/fastjet/aero/f16_aero_model.hpp"
#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>

using namespace fastjet::math;
using namespace fastjet::fdm;
using namespace fastjet::aero;

void test_baseline_lookups() {
    std::cout << "[Test] NASA TP-1538 Baseline Aerodynamic Coefficients... " << std::flush;

    FlightState state;
    state.vel_b = Vector3(200.0, 0.0, 0.0); // alpha = 0, beta = 0, V = 200 m/s
    state.omega_b = Vector3::zero();

    const ControlSurfaces controls(0.0, 0.0, 0.0, 0.0); // clean, neutral controls
    constexpr double q_bar = 10000.0; // 10 kPa dynamic pressure

    const AeroCoefficients coeffs = F16AeroModel::compute_coefficients(state, controls, q_bar);

    // Baseline alpha=0 values
    assert(std::abs(coeffs.CL - 0.050) < 1e-3);
    assert(std::abs(coeffs.CD - 0.021) < 1e-3);
    assert(std::abs(coeffs.Cm - (-0.005)) < 1e-3);
    assert(std::abs(coeffs.CY) < 1e-6);
    assert(std::abs(coeffs.Cl) < 1e-6);
    assert(std::abs(coeffs.Cn) < 1e-6);

    std::cout << "PASSED (CL0=" << coeffs.CL << ", CD0=" << coeffs.CD
              << ", Cm0=" << coeffs.Cm << ")\n";
}

void test_relaxed_static_stability_derivative() {
    std::cout << "[Test] Relaxed Longitudinal Static Stability (dCm/dalpha > 0)... " << std::flush;

    const ControlSurfaces controls(0.0, 0.0, 0.0, 0.0);
    constexpr double q_bar = 10000.0;

    // Evaluate Cm at alpha = 0 deg
    FlightState s0;
    s0.vel_b = Vector3(200.0, 0.0, 0.0); // alpha = 0 deg
    const AeroCoefficients c0 = F16AeroModel::compute_coefficients(s0, controls, q_bar);

    // Evaluate Cm at alpha = 5 deg (u = 200*cos(5 deg), w = 200*sin(5 deg))
    constexpr double alpha_5deg = 5.0 * M_PI / 180.0;
    FlightState s5;
    s5.vel_b = Vector3(200.0 * std::cos(alpha_5deg), 0.0, 200.0 * std::sin(alpha_5deg));
    const AeroCoefficients c5 = F16AeroModel::compute_coefficients(s5, controls, q_bar);

    // Evaluate Cm at alpha = 10 deg
    constexpr double alpha_10deg = 10.0 * M_PI / 180.0;
    FlightState s10;
    s10.vel_b = Vector3(200.0 * std::cos(alpha_10deg), 0.0, 200.0 * std::sin(alpha_10deg));
    const AeroCoefficients c10 = F16AeroModel::compute_coefficients(s10, controls, q_bar);

    // Pitch stiffness: dCm/dalpha
    const double dCm_dalpha_0_5 = (c5.Cm - c0.Cm) / 5.0;     // per degree
    const double dCm_dalpha_5_10 = (c10.Cm - c5.Cm) / 5.0;   // per degree

    // IN A CONVENTIONAL STABLE AIRCRAFT: dCm/dalpha < 0 (nose pitches down as alpha increases)
    // IN THE F-16 (RELAXED STATIC STABILITY): dCm/dalpha > 0 (nose pitches up, destabilizing!)
    assert(dCm_dalpha_0_5 > 0.0);
    assert(dCm_dalpha_5_10 > 0.0);

    const double dCm_dalpha_rad = dCm_dalpha_0_5 * (180.0 / M_PI); // per radian

    std::cout << "PASSED\n"
              << "  Cm(0 deg)  = " << c0.Cm << "\n"
              << "  Cm(5 deg)  = " << c5.Cm << " (Delta Cm = +" << (c5.Cm - c0.Cm) << ")\n"
              << "  Cm(10 deg) = " << c10.Cm << " (Delta Cm = +" << (c10.Cm - c5.Cm) << ")\n"
              << "  dCm/dalpha = +" << dCm_dalpha_0_5 << " deg^-1 (+"
              << dCm_dalpha_rad << " rad^-1) > 0 (UNSTABLE)\n";
}

void test_elevator_control_power() {
    std::cout << "[Test] Pitch Control Effectiveness (dCm/ddelta_e < 0)... " << std::flush;

    FlightState state;
    state.vel_b = Vector3(200.0, 0.0, 0.0);
    constexpr double q_bar = 10000.0;

    // Positive elevator (trailing edge DOWN) produces negative pitch moment (nose DOWN)
    const ControlSurfaces de_neutral(0.0, 0.0, 0.0, 0.0);
    const ControlSurfaces de_down(+10.0, 0.0, 0.0, 0.0);
    const ControlSurfaces de_up(-10.0, 0.0, 0.0, 0.0);

    const AeroCoefficients c_neutral = F16AeroModel::compute_coefficients(state, de_neutral, q_bar);
    const AeroCoefficients c_down    = F16AeroModel::compute_coefficients(state, de_down, q_bar);
    const AeroCoefficients c_up      = F16AeroModel::compute_coefficients(state, de_up, q_bar);

    // Negative slope: trailing edge down pushes nose down (Cm decreases)
    assert(c_down.Cm < c_neutral.Cm);
    assert(c_up.Cm > c_neutral.Cm);

    const double dCm_dde = (c_down.Cm - c_neutral.Cm) / 10.0;
    assert(dCm_dde < 0.0);

    std::cout << "PASSED (dCm/dde = " << dCm_dde << " deg^-1)\n";
}

void test_dimensional_force_moment_synthesis() {
    std::cout << "[Test] Dimensional Force & Moment Synthesis... " << std::flush;

    FlightState state;
    // Cruise at alpha = 5 deg, V = 200 m/s
    constexpr double alpha = 5.0 * M_PI / 180.0;
    state.vel_b = Vector3(200.0 * std::cos(alpha), 0.0, 200.0 * std::sin(alpha));
    state.omega_b = Vector3(0.0, 0.05, 0.0); // pitch rate q = 0.05 rad/s

    const ControlSurfaces controls(0.0, 0.0, 0.0, 0.0);
    constexpr double q_bar = 12000.0; // 12 kPa

    const AircraftForces forces = F16AeroModel::compute_forces_and_moments(state, controls, q_bar);

    // Lift pushes upward (negative Z in body frame)
    assert(forces.force_b.z < 0.0);
    // Drag pushes backward (negative X in body frame at small alpha)
    assert(forces.force_b.x < 0.0);

    // Pitching moment incorporates static instability (positive) minus pitch damping (negative)
    std::cout << "PASSED\n"
              << "  Aero Force Body:  " << forces.force_b << " N (Lift = "
              << -forces.force_b.z << " N, Drag = " << -forces.force_b.x << " N)\n"
              << "  Aero Moment Body: " << forces.moment_b << " N*m (Pitch Moment = "
              << forces.moment_b.y << " N*m)\n";
}

int main() {
    std::cout << "=== F-16 Aerodynamic Coefficients & Synthesis Verification ===\n";
    test_baseline_lookups();
    test_relaxed_static_stability_derivative();
    test_elevator_control_power();
    test_dimensional_force_moment_synthesis();
    std::cout << "All Aerodynamic Model tests passed successfully!\n\n";
    return 0;
}
