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

void test_high_speed_9g_pull() {
    std::cout << "[Test] High-Speed Full Aft Stick (+1.0) 9.0G Limiter Verification...\n";

    const MassProperties mass = MassProperties::create_clean_f16();
    const RK4Integrator integrator(0.005); // 200 Hz

    F16FLCS flcs;
    flcs.reset(0.5, 0.0, 0.0);

    // High dynamic pressure: V = 280 m/s at 3,000 m (q_bar ~ 35 kPa)
    constexpr double ALTITUDE = 3000.0;
    constexpr double AIRSPEED = 280.0;

    FlightState state;
    state.pos_ned = Vector3(0.0, 0.0, -ALTITUDE);
    state.vel_b = Vector3(AIRSPEED, 0.0, 0.0);
    state.omega_b = Vector3::zero();
    state.q_att = Quaternion::identity();

    // Full aft stick command (+1.0)
    const PilotCommands pilot_full_aft(1.0, 0.0, 0.0);

    double time = 0.0;
    constexpr double DURATION = 2.0; // 2 seconds of high-g pull
    const int total_steps = static_cast<int>(DURATION / integrator.dt);

    double max_nz = 0.0;
    double max_alpha = 0.0;

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "  Simulating full aft stick pull at 280 m/s...\n";
    std::cout << "  --------------------------------------------------------------------\n";
    std::cout << "  Time [s] | Load Factor Nz [G] | Alpha [deg] | Pitch Rate q [deg/s] | Elev [deg]\n";
    std::cout << "  --------------------------------------------------------------------\n";

    for (int step = 0; step < total_steps; ++step) {
        const AirData ad = Atmosphere1976::compute(state.altitude(), state.airspeed());

        // 1. Measure aero forces with current surfaces
        ControlSurfaces current_surfaces(flcs.stabilator.position, flcs.flaperon.position, flcs.rudder.position);
        AircraftForces current_aero = F16AeroModel::compute_forces_and_moments(state, current_surfaces, ad.dynamic_pressure);

        // 2. Run FLCS control laws and actuators
        const ControlSurfaces commanded_surfaces = flcs.update(
            integrator.dt, state, pilot_full_aft, ad.dynamic_pressure, current_aero, mass
        );

        // 3. Step 6-DoF RK4
        integrator.step(state, time, mass, [&](double, const FlightState& s) noexcept -> AircraftForces {
            const AirData ad_stage = Atmosphere1976::compute(s.altitude(), s.airspeed());
            return F16AeroModel::compute_forces_and_moments(s, commanded_surfaces, ad_stage.dynamic_pressure);
        });

        const AirData ad_post = Atmosphere1976::compute(state.altitude(), state.airspeed());
        const AircraftForces aero_post = F16AeroModel::compute_forces_and_moments(state, commanded_surfaces, ad_post.dynamic_pressure);
        const IMUData imu = IMUData::read(state, aero_post, mass, ad_post.dynamic_pressure);

        if (imu.Nz > max_nz) max_nz = imu.Nz;
        if (imu.alpha_deg > max_alpha) max_alpha = imu.alpha_deg;

        if ((step + 1) % 80 == 0) { // Every 0.4s
            std::cout << "  " << std::setw(6) << time
                      << "  |  " << std::setw(17) << imu.Nz
                      << "  |  " << std::setw(10) << imu.alpha_deg
                      << "  |  " << std::setw(19) << imu.q_deg
                      << "  |  " << std::setw(9) << flcs.stabilator.position << "\n";
        }
    }
    std::cout << "  --------------------------------------------------------------------\n";

    // G-Limiter Verification:
    // 1. Must achieve strong high-G pull (> 8.5G)
    assert(max_nz > 8.5);
    // 2. Must strictly NOT exceed +9.0G structural limit (allow 0.05G transient margin)
    assert(max_nz <= 9.05);

    std::cout << "  9.0G PULL VERIFIED:\n"
              << "  - Peak Nz:    " << max_nz << " G (Strictly <= 9.05 G)\n"
              << "  - Peak Alpha: " << max_alpha << " deg\n"
              << "  - G-limiter successfully prevented structural over-G!\n\n";
}

void test_low_speed_alpha_limiter_capture() {
    std::cout << "[Test] Low-Speed Full Aft Stick (+1.0) 25.5 deg AoA Limiter Verification...\n";

    const MassProperties mass = MassProperties::create_clean_f16();
    const RK4Integrator integrator(0.005); // 200 Hz

    F16FLCS flcs;
    flcs.reset(0.5, 0.0, 0.0);

    // Moderate dynamic pressure: V = 160 m/s at 5,000 m (q_bar ~ 9.4 kPa)
    // 9G cannot be reached aerodynamically; AoA limiter will govern!
    constexpr double ALTITUDE = 5000.0;
    constexpr double AIRSPEED = 160.0;

    FlightState state;
    state.pos_ned = Vector3(0.0, 0.0, -ALTITUDE);
    state.vel_b = Vector3(AIRSPEED, 0.0, 0.0);
    state.omega_b = Vector3::zero();
    state.q_att = Quaternion::identity();

    const PilotCommands pilot_full_aft(1.0, 0.0, 0.0);

    double time = 0.0;
    constexpr double DURATION = 2.5; // 2.5 seconds
    const int total_steps = static_cast<int>(DURATION / integrator.dt);

    double max_alpha = 0.0;
    double max_nz = 0.0;

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "  Simulating full aft stick pull at 160 m/s (AoA limiting regime)...\n";
    std::cout << "  --------------------------------------------------------------------\n";
    std::cout << "  Time [s] | Alpha [deg] | Load Factor Nz [G] | Pitch Rate q [deg/s] | Elev [deg]\n";
    std::cout << "  --------------------------------------------------------------------\n";

    for (int step = 0; step < total_steps; ++step) {
        const AirData ad = Atmosphere1976::compute(state.altitude(), state.airspeed());

        ControlSurfaces current_surfaces(flcs.stabilator.position, flcs.flaperon.position, flcs.rudder.position);
        AircraftForces current_aero = F16AeroModel::compute_forces_and_moments(state, current_surfaces, ad.dynamic_pressure);

        const ControlSurfaces commanded_surfaces = flcs.update(
            integrator.dt, state, pilot_full_aft, ad.dynamic_pressure, current_aero, mass
        );

        integrator.step(state, time, mass, [&](double, const FlightState& s) noexcept -> AircraftForces {
            const AirData ad_stage = Atmosphere1976::compute(s.altitude(), s.airspeed());
            return F16AeroModel::compute_forces_and_moments(s, commanded_surfaces, ad_stage.dynamic_pressure);
        });

        const AirData ad_post = Atmosphere1976::compute(state.altitude(), state.airspeed());
        const AircraftForces aero_post = F16AeroModel::compute_forces_and_moments(state, commanded_surfaces, ad_post.dynamic_pressure);
        const IMUData imu = IMUData::read(state, aero_post, mass, ad_post.dynamic_pressure);

        if (imu.alpha_deg > max_alpha) max_alpha = imu.alpha_deg;
        if (imu.Nz > max_nz) max_nz = imu.Nz;

        if ((step + 1) % 100 == 0) { // Every 0.5s
            std::cout << "  " << std::setw(6) << time
                      << "  |  " << std::setw(10) << imu.alpha_deg
                      << "  |  " << std::setw(17) << imu.Nz
                      << "  |  " << std::setw(19) << imu.q_deg
                      << "  |  " << std::setw(9) << flcs.stabilator.position << "\n";
        }
    }
    std::cout << "  --------------------------------------------------------------------\n";

    // AoA Limiter Verification:
    // 1. Must pull high angle of attack (> 22 deg)
    assert(max_alpha > 22.0);
    // 2. Must strictly NOT exceed 25.5 deg AoA (allow 0.05 deg numerical overshoot margin)
    assert(max_alpha <= 25.55);

    std::cout << "  25.5 DEG AoA LIMITER VERIFIED:\n"
              << "  - Peak Alpha: " << max_alpha << " deg (Strictly <= 25.55 deg)\n"
              << "  - Peak Nz:    " << max_nz << " G\n"
              << "  - AoA limiter smoothly captured maximum alpha with zero stall departure!\n\n";
}

int main() {
    std::cout << "=== FLCS Maneuver Limiter Verification (+9G & 25.5 deg AoA) ===\n";
    test_high_speed_9g_pull();
    test_low_speed_alpha_limiter_capture();
    std::cout << "All Maneuver Limiter tests passed successfully!\n\n";
    return 0;
}
