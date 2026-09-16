#include "../include/fastjet/aero/f16_aero_model.hpp"
#include "../include/fastjet/environment/atmosphere1976.hpp"
#include "../include/fastjet/fdm/rk4_integrator.hpp"
#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace fastjet::math;
using namespace fastjet::fdm;
using namespace fastjet::environment;
using namespace fastjet::aero;

void test_open_loop_pitch_divergence() {
    std::cout << "[Test] F-16 Open-Loop Pitch Divergence (Relaxed Static Stability, No FLCS)...\n";

    const MassProperties mass = MassProperties::create_clean_f16();
    const RK4Integrator integrator(0.005); // 200 Hz fixed timestep

    // Flight condition: 5,000 m altitude, 200 m/s airspeed
    constexpr double ALTITUDE = 5000.0;
    constexpr double AIRSPEED = 200.0;

    // Initial trim angle of attack ~ 2.5 deg (0.0436 rad)
    constexpr double alpha_trim_deg = 2.5;
    constexpr double alpha_trim_rad = alpha_trim_deg * (M_PI / 180.0);

    // Find elevator deflection to trim pitching moment to zero at alpha_trim:
    // Cm(alpha=2.5, de=0) ~ 0.011 > 0
    // de_pitch_effectiveness ~ -0.0165 -> de_trim ~ -0.011 / (-0.0165) ~ +0.67 deg (trailing edge down)
    ControlSurfaces controls(0.67, 0.0, 0.0, 0.0);

    // Setup initial state trimmed at alpha_trim
    FlightState state;
    state.pos_ned = Vector3(0.0, 0.0, -ALTITUDE);
    state.vel_b = Vector3(AIRSPEED * std::cos(alpha_trim_rad), 0.0, AIRSPEED * std::sin(alpha_trim_rad));
    state.omega_b = Vector3::zero();
    state.q_att = Quaternion::from_euler(0.0, alpha_trim_rad, 0.0);

    // Introduce a small pitch perturbation: dq = +0.02 rad/s (+1.15 deg/s)
    constexpr double PERTURBATION_Q = 0.02; // rad/s
    state.omega_b.y += PERTURBATION_Q;

    double time = 0.0;
    constexpr double SIM_DURATION = 2.5; // 2.5 seconds (500 steps @ 200 Hz)
    const int total_steps = static_cast<int>(SIM_DURATION / integrator.dt);

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "  Simulating open-loop dynamic response without flight controller...\n";
    std::cout << "  --------------------------------------------------------------------\n";
    std::cout << "  Time [s] | Alpha [deg] | Pitch Rate q [deg/s] | Pitch Theta [deg] | Cm [-]\n";
    std::cout << "  --------------------------------------------------------------------\n";

    double initial_alpha_deg = state.alpha() * (180.0 / M_PI);
    double initial_q_deg = state.omega_b.y * (180.0 / M_PI);

    std::cout << "   " << std::setw(6) << time
              << "  |  " << std::setw(9) << initial_alpha_deg
              << "  |  " << std::setw(18) << initial_q_deg
              << "  |  " << std::setw(17) << (state.euler_angles().pitch * 180.0 / M_PI)
              << "  |  " << std::setw(6) << "0.0000" << "\n";

    std::vector<double> history_time;
    std::vector<double> history_alpha;
    std::vector<double> history_q;

    for (int step = 0; step < total_steps; ++step) {
        // Step FDM with full NASA TP-1538 aerodynamics synthesized in body frame
        integrator.step(state, time, mass, [&](double, const FlightState& s) noexcept -> AircraftForces {
            // Update dynamic pressure at current altitude & airspeed
            const AirData ad = Atmosphere1976::compute(s.altitude(), s.airspeed());
            // Synthesize aero forces and moments
            return F16AeroModel::compute_forces_and_moments(s, controls, ad.dynamic_pressure);
        });

        const double cur_alpha_deg = state.alpha() * (180.0 / M_PI);
        const double cur_q_deg = state.omega_b.y * (180.0 / M_PI);

        history_time.push_back(time);
        history_alpha.push_back(cur_alpha_deg);
        history_q.push_back(cur_q_deg);

        // Print telemetry every 0.5 seconds
        if ((step + 1) % 100 == 0) {
            const AirData ad = Atmosphere1976::compute(state.altitude(), state.airspeed());
            const AeroCoefficients ac = F16AeroModel::compute_coefficients(state, controls, ad.dynamic_pressure);

            std::cout << "   " << std::setw(6) << time
                      << "  |  " << std::setw(9) << cur_alpha_deg
                      << "  |  " << std::setw(18) << cur_q_deg
                      << "  |  " << std::setw(17) << (state.euler_angles().pitch * 180.0 / M_PI)
                      << "  |  " << std::setw(6) << ac.Cm << "\n";
        }
    }
    std::cout << "  --------------------------------------------------------------------\n";

    const double final_alpha_deg = history_alpha.back();
    const double final_q_deg = history_q.back();

    // Verify open-loop pitch divergence:
    // Because dCm/dalpha > 0, the initial +0.02 rad/s (+1.15 deg/s) perturbation causes
    // alpha to increase, generating positive Cm, which drives further pitch-up!
    // Over 2.5s, alpha should diverge significantly (increase from ~2.5 deg to > 15 deg)!
    const double alpha_growth = final_alpha_deg - initial_alpha_deg;
    assert(alpha_growth > 8.0); // Alpha grew by more than 8 degrees unstably
    assert(final_q_deg > initial_q_deg); // Pitch rate accelerated upward

    std::cout << "  VERIFICATION SUCCESSFUL:\n"
              << "  - Initial Alpha: " << initial_alpha_deg << " deg\n"
              << "  - Final Alpha:   " << final_alpha_deg << " deg (Divergence: +"
              << alpha_growth << " deg)\n"
              << "  - Final Pitch Rate: " << final_q_deg << " deg/s (Unstable acceleration)\n"
              << "  - Correctly demonstrated F-16 relaxed static stability (open-loop unstable without FLCS)!\n";
}

int main() {
    std::cout << "=== F-16 Aerodynamic Pitch Divergence Verification ===\n";
    test_open_loop_pitch_divergence();
    std::cout << "All Pitch Divergence tests passed successfully!\n\n";
    return 0;
}
