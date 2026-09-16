#include "../include/fastjet/flcs/f16_flcs.hpp"
#include "../include/fastjet/aero/f16_aero_model.hpp"
#include "../include/fastjet/environment/atmosphere1976.hpp"
#include "../include/fastjet/fdm/rk4_integrator.hpp"
#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>

using namespace fastjet::math;
using namespace fastjet::fdm;
using namespace fastjet::environment;
using namespace fastjet::aero;
using namespace fastjet::flcs;

struct SimRunner {
    FlightState state{};
    MassProperties mass = MassProperties::create_clean_f16();
    RK4Integrator integrator{0.005};
    F16AeroModel aero_model;
    F16FLCS flcs;
    AircraftForces current_forces{};
    double sim_time{0.0};

    void init(double alt = 1500.0, double spd = 220.0) {
        state = FlightState{};
        state.pos_ned = Vector3(0.0, 0.0, -alt);
        state.vel_b   = Vector3(spd, 0.0, 0.0);
        state.omega_b = Vector3::zero();
        state.q_att   = Quaternion::identity();
        flcs.reset();
        current_forces = AircraftForces{};
        sim_time = 0.0;
    }

    void step(const PilotCommands& pilot, double throttle_thrust = 70000.0) {
        constexpr double SIM_DT = 0.005;
        const auto air = Atmosphere1976::compute(state.altitude(), state.airspeed());
        
        const auto surfaces = flcs.update(
            SIM_DT, state, pilot, air.dynamic_pressure, current_forces, mass
        );

        current_forces = aero_model.compute_forces_and_moments(state, surfaces, air.dynamic_pressure);
        current_forces.force_b.x += throttle_thrust;

        integrator.step(state, sim_time, mass, [&](double, const FlightState&) noexcept {
            return current_forces;
        });

        sim_time += SIM_DT;
    }
};

void test_rudder_pedal_response() {
    std::cout << "[Test 1] Right Rudder Pedal Sign & Response Verification...\n";
    SimRunner sim;
    sim.init();

    PilotCommands right_pedal{};
    right_pedal.rudder_pedal = 1.0;

    double max_r_dps = 0.0;
    for (int i = 0; i < 200; ++i) { // 1 second
        sim.step(right_pedal);
        double r_dps = sim.state.omega_b.z * (180.0 / M_PI);
        if (std::abs(r_dps) > std::abs(max_r_dps)) max_r_dps = r_dps;
    }

    std::cout << "  Peak Yaw Rate r = " << max_r_dps << " deg/s (expected > 0 for right yaw)\n";
    assert(max_r_dps > 5.0 && "Right rudder pedal must produce positive yaw rate (nose right)!");
    std::cout << "  -> PASSED: Rudder responds correctly into commanded direction.\n\n";
}

void test_roll_damping_and_stability() {
    std::cout << "[Test 2] Roll Rate Tracking & Hands-Off Roll Damping Verification...\n";
    SimRunner sim;
    sim.init();

    PilotCommands right_roll{};
    right_roll.roll_stick = 1.0;

    double peak_p_dps = 0.0;
    for (int i = 0; i < 200; ++i) { // 1 second full roll
        sim.step(right_roll);
        double p_dps = sim.state.omega_b.x * (180.0 / M_PI);
        if (std::abs(p_dps) > std::abs(peak_p_dps)) peak_p_dps = p_dps;
    }

    PilotCommands neutral{};
    for (int i = 0; i < 400; ++i) { // 2 seconds hands-off
        sim.step(neutral);
    }
    double final_p_dps = sim.state.omega_b.x * (180.0 / M_PI);
    double final_r_dps = sim.state.omega_b.z * (180.0 / M_PI);

    std::cout << "  Peak Roll Rate p = " << peak_p_dps << " deg/s\n";
    std::cout << "  Residual Roll Rate p after 2s hands-off = " << final_p_dps << " deg/s\n";
    std::cout << "  Residual Yaw Rate r after 2s hands-off  = " << final_r_dps << " deg/s\n";

    assert(peak_p_dps > 150.0 && "Roll rate must achieve agile high performance (>150 deg/s)!");
    assert(std::abs(final_p_dps) < 5.0 && "Roll rate must damp to near zero when stick is released!");
    assert(std::abs(final_r_dps) < 5.0 && "Yaw rate must damp to near zero without spiral divergence!");
    std::cout << "  -> PASSED: Clean roll and stable hands-off settling verified.\n\n";
}

void test_dutch_roll_damping() {
    std::cout << "[Test 3] Dutch Roll Yaw Damper Verification...\n";
    SimRunner sim;
    sim.init();

    // 0.5s rudder kick
    PilotCommands kick{};
    kick.rudder_pedal = 1.0;
    for (int i = 0; i < 100; ++i) sim.step(kick);

    // 5s hands-off recovery
    PilotCommands neutral{};
    for (int i = 0; i < 1000; ++i) sim.step(neutral);

    double final_r = sim.state.omega_b.z * (180.0 / M_PI);
    double final_beta = sim.state.beta() * (180.0 / M_PI);

    std::cout << "  Residual Yaw Rate r after 5s = " << final_r << " deg/s\n";
    std::cout << "  Residual Sideslip beta after 5s = " << final_beta << " deg\n";

    assert(std::abs(final_r) < 2.0 && "Yaw damper must damp Dutch roll oscillations!");
    assert(std::abs(final_beta) < 2.0 && "Sideslip must damp to near zero!");
    std::cout << "  -> PASSED: Dutch roll successfully damped by FLCS.\n\n";
}

void test_spin_entry_and_recovery() {
    std::cout << "[Test 4] Spin Entry & Automatic Recovery Verification...\n";
    SimRunner sim;
    sim.init();

    // 4s sustained aft stick + full rudder
    PilotCommands spin_input{};
    spin_input.pitch_stick = 1.0;
    spin_input.rudder_pedal = 1.0;

    for (int i = 0; i < 800; ++i) sim.step(spin_input);

    double p_release = sim.state.omega_b.x * (180.0 / M_PI);
    double r_release = sim.state.omega_b.z * (180.0 / M_PI);
    double a_release = sim.state.alpha() * (180.0 / M_PI);
    double b_release = sim.state.beta() * (180.0 / M_PI);

    std::cout << "  State at controls release: alpha=" << a_release << " deg, beta=" << b_release
              << " deg, p=" << p_release << " deg/s, r=" << r_release << " deg/s\n";

    // 6s hands-off recovery
    PilotCommands neutral{};
    bool safe_recovery = true;
    for (int i = 0; i < 1200; ++i) {
        sim.step(neutral);
        double a = std::abs(sim.state.alpha() * (180.0 / M_PI));
        double p = std::abs(sim.state.omega_b.x * (180.0 / M_PI));
        double r = std::abs(sim.state.omega_b.z * (180.0 / M_PI));
        if (a > 60.0 || p > 800.0 || r > 400.0 || std::isnan(a)) {
            safe_recovery = false;
            break;
        }
    }

    double final_alpha = sim.state.alpha() * (180.0 / M_PI);
    double final_p = sim.state.omega_b.x * (180.0 / M_PI);
    double final_r = sim.state.omega_b.z * (180.0 / M_PI);

    std::cout << "  State after 6s recovery: alpha=" << final_alpha << " deg, p=" << final_p
              << " deg/s, r=" << final_r << " deg/s\n";

    assert(safe_recovery && "Aircraft must not enter an uncontrolled divergent spin!");
    assert(std::abs(final_r) < 5.0 && "Yaw rate must recover to near zero!");
    assert(std::abs(final_p) < 5.0 && "Roll rate must recover to near zero!");
    std::cout << "  -> PASSED: Full spin recovery verified.\n\n";
}

int main() {
    std::cout << "=================================================================\n";
    std::cout << "      F-16 FLCS LATERAL-DIRECTIONAL STABILITY VERIFICATION       \n";
    std::cout << "=================================================================\n\n";

    test_rudder_pedal_response();
    test_roll_damping_and_stability();
    test_dutch_roll_damping();
    test_spin_entry_and_recovery();

    std::cout << "All FLCS Lateral-Directional Stability Tests Passed Successfully!\n";
    return 0;
}
