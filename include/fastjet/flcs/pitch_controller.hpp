#pragma once

#include "pilot_commands.hpp"
#include "imu_sensor.hpp"
#include <algorithm>
#include <cmath>

namespace fastjet {
namespace flcs {

/**
 * @brief F-16 Pitch Axis Control Laws.
 * Features:
 * - Dynamic pressure blended pitch rate (q_cmd) and normal acceleration (Nz_cmd).
 * - Hard limiters: strictly clamped to +9.0G / -3.0G and max 25.5 deg angle of attack.
 * - Closed-loop PI controller with pitch rate damping and anti-windup integrator.
 *
 * Sign convention (NASA TP-1538):
 * - delta_e > 0 (trailing edge DOWN) produces nose-DOWN moment (Cm < 0, reduces G/q).
 * - delta_e < 0 (trailing edge UP) produces nose-UP moment (Cm > 0, increases G/q).
 */
class PitchController {
public:
    // Limits
    static constexpr double NZ_MAX         = +9.0;   // Maximum normal acceleration [G]
    static constexpr double NZ_MIN         = -3.0;   // Minimum normal acceleration [G]
    static constexpr double ALPHA_MAX_DEG  = 25.5;   // Maximum allowable angle of attack [deg]
    static constexpr double Q_CMD_MAX_DPS  = 25.0;   // Maximum pitch rate command [deg/s]
    static constexpr double ALPHA_LEAD_S   = 0.10;   // AoA limiter prediction horizon [s]

    // Tuned gains for 200 Hz digital loop
    double Kp{1.40};            // Proportional gain [deg elevator / G error]
    double Ki{0.70};            // Integral gain [deg elevator / (G * s)]
    double Kd{0.32};            // Pitch rate damping gain [deg elevator / (deg/s)]
    double K_alpha{2.20};       // AoA limiter direct feedforward gain
    double K_alpha_rate{0.30};  // AoA rate lead gain

    double integrator{0.0};     // Integrator state [G * s]
    double last_error{0.0};
    double last_de_cmd{0.0};

    constexpr PitchController() noexcept = default;

    void reset(double init_integrator = 0.0) noexcept {
        integrator = init_integrator;
        last_error = 0.0;
        last_de_cmd = 0.0;
    }

    /**
     * @brief Executes pitch control law for a discrete time step.
     *
     * @param dt Time step [s]
     * @param pilot Pilot cockpit commands
     * @param imu Current IMU sensor feedback
     * @param is_actuator_saturated Boolean flag if stabilator is currently saturated
     * @return double Commanded stabilator deflection [deg]
     */
    double update(
        double dt,
        const PilotCommands& pilot,
        const IMUData& imu,
        bool is_actuator_saturated = false
    ) noexcept {
        const double pitch_in = std::clamp(pilot.pitch_stick, -1.0, 1.0);

        // 1. Map Stick Input to Nz_cmd (+9.0G to -3.0G)
        double Nz_cmd = (pitch_in >= 0.0) ? (1.0 + pitch_in * (NZ_MAX - 1.0))
                                          : (1.0 + pitch_in * (1.0 - NZ_MIN));
        Nz_cmd = std::clamp(Nz_cmd, NZ_MIN, NZ_MAX);

        // 2. Map Stick Input to q_cmd for low dynamic pressure
        double q_cmd = pitch_in * Q_CMD_MAX_DPS;

        // 3. Dynamic Pressure Blending Weight:
        // q_bar < 2000 Pa: pure q_cmd (w_Nz = 0.0)
        // q_bar > 8000 Pa: pure Nz_cmd (w_Nz = 1.0)
        const double w_Nz = std::clamp((imu.q_bar - 2000.0) / 6000.0, 0.0, 1.0);

        // 4. Hard Angle of Attack (AoA) Limiter Command Restriction
        // As alpha exceeds 18 deg and approaches 25.5 deg, override pilot command.
        // Every tier acts on alpha *predicted* ALPHA_LEAD_S ahead.  On raw alpha the
        // limiter only reacted once the nose was already through the limit, and its
        // hard push-down then threw alpha back to single figures while the pilot
        // still held full aft stick: at low q (heavy jet, 150 m/s at 5 km) that grew
        // into a divergent -8..34 deg pitch oscillation.  The rate used is alpha's,
        // i.e. pitch rate less the flight-path turn rate the current Nz accounts
        // for — a sustained pull's steady pitch rate is not alpha rising.
        constexpr double G0 = 9.80665;
        const double path_rate_dps = (180.0 / M_PI) * G0 * (imu.Nz - 1.0)
                                   / std::max(imu.airspeed, 50.0);
        const double alpha_lim = imu.alpha_deg + ALPHA_LEAD_S * (imu.q_deg - path_rate_dps);
        double aoa_override_de = 0.0;
        if (alpha_lim > 18.0) {
            const double alpha_headroom = std::max(0.0, ALPHA_MAX_DEG - alpha_lim);
            const double max_allowed_G = 1.0 + (alpha_headroom / 7.5) * (NZ_MAX - 1.0) - K_alpha_rate * std::max(0.0, imu.q_deg);
            Nz_cmd = std::min(Nz_cmd, max_allowed_G);
            q_cmd  = std::min(q_cmd,  (alpha_headroom / 7.5) * Q_CMD_MAX_DPS);

            if (alpha_lim > 22.0) {
                const double excess = alpha_lim - 22.0;
                aoa_override_de = K_alpha * excess + 0.15 * std::max(0.0, imu.q_deg);
            }
            if (alpha_lim >= 25.0) {
                // Hard ceiling push-down
                aoa_override_de += (alpha_lim - 25.0) * 15.0;
                Nz_cmd = std::min(Nz_cmd, 0.5);
                q_cmd = std::min(q_cmd, -2.0);
            }
        }

        // 5. Commanded Pitch Error:
        constexpr double K_q_to_G = 0.04;
        const double err_Nz = Nz_cmd - imu.Nz;
        const double err_q  = (q_cmd - imu.q_deg) * K_q_to_G;
        const double error  = w_Nz * err_Nz + (1.0 - w_Nz) * err_q;

        // 6. Integrator Update with Anti-Windup
        bool freeze_integrator = false;
        if (is_actuator_saturated) {
            if ((last_de_cmd >= 25.0 && error < 0.0) || (last_de_cmd <= -25.0 && error > 0.0)) {
                freeze_integrator = true;
            }
        }
        if (imu.alpha_deg >= 24.5 && error > 0.0) {
            freeze_integrator = true; // Stop integrating pitch-up command near stall
        }

        if (!freeze_integrator && dt > 0.0) {
            integrator += error * dt;
            integrator = std::clamp(integrator, -15.0, 15.0);
        }

        // 7. Closed-loop PI Controller with Pitch Rate Damping:
        double de_cmd = -(Kp * error + Ki * integrator) + Kd * imu.q_deg + aoa_override_de;

        // Clamp to stabilator physical limits [-25, +25] deg
        de_cmd = std::clamp(de_cmd, -25.0, 25.0);

        last_error = error;
        last_de_cmd = de_cmd;

        return de_cmd;
    }
};

} // namespace flcs
} // namespace fastjet
