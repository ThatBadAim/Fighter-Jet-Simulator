#pragma once

#include "pilot_commands.hpp"
#include "imu_sensor.hpp"
#include <algorithm>
#include <cmath>

namespace fastjet {
namespace flcs {

/**
 * @brief F-16 Lateral-Directional (Roll & Yaw) Control Laws.
 * Features:
 * - Roll-rate command (up to 300 deg/s) with high-AoA roll-rate attenuation.
 * - Aileron-Rudder Interconnect (ARI) for automatic coordinated turns.
 * - Yaw rate damper and sideslip suppression for Dutch roll damping.
 *
 * Zero dynamic heap allocation.
 */
/**
 * @brief Integral trim on a roll-rate command loop.
 *
 * Every roll law here was proportional-only, and roll damping then settled the
 * rate 20-35% short of what full stick commands (the F-22 managed 184 of its
 * 280 deg/s).  The integral closes that gap.  It bleeds away while the stick is
 * centred so no leftover trim rolls a hands-off jet, and freezes while the
 * surface is already at its stop.
 */
struct RollRateTrim {
    double integral{0.0};  ///< Accumulated roll-rate error [deg].

    constexpr void reset() noexcept { integral = 0.0; }

    /// @return Aileron increment [deg], same sign convention as da (negative = right roll).
    [[nodiscard]] double update(double dt, double p_cmd_dps, double p_dps,
                                double ki, double da_unsaturated, double da_limit) noexcept {
        if (dt <= 0.0) return -ki * integral;
        if (std::abs(p_cmd_dps) < 1.0) {
            integral *= std::max(0.0, 1.0 - 6.0 * dt);
        } else {
            const double err = p_cmd_dps - p_dps;
            const double da_total = da_unsaturated - ki * integral;
            // Anti-windup: stop integrating further into a saturated surface.
            const bool pushing_into_stop = (da_total <= -da_limit && err > 0.0)
                                        || (da_total >=  da_limit && err < 0.0);
            if (!pushing_into_stop) integral += err * dt;
        }
        return -ki * integral;
    }
};

class LateralDirectionalController {
public:
    static constexpr double P_CMD_MAX_DPS   = 300.0;  // Maximum roll rate command [deg/s]
    static constexpr double ALPHA_ATTEN_MIN = 15.0;   // AoA where roll attenuation starts [deg]
    static constexpr double ALPHA_ATTEN_MAX = 25.5;   // AoA where roll attenuation reaches minimum [deg]

    // Gains
    double K_roll{0.15};           // Roll rate error to flaperon gain [deg da / (deg/s)]
    double K_yaw_damper{0.60};     // Yaw rate to rudder gain [deg dr / (deg/s)] (damping sign > 0)
    double K_beta{0.25};           // Sideslip feedback to rudder [deg dr / deg beta]
    double Ki_roll{0.60};          // Roll rate integral gain [deg da / deg]

    RollRateTrim roll_trim{};

    constexpr LateralDirectionalController() noexcept = default;

    constexpr void reset() noexcept { roll_trim.reset(); }

    /**
     * @brief Computes commanded flaperon (da) and rudder (dr) deflections.
     *
     * @param pilot Pilot cockpit commands
     * @param imu IMU sensor data
     * @param[out] da_cmd Output flaperon command [deg]
     * @param[out] dr_cmd Output rudder command [deg]
     */
    void update(
        double dt,
        const PilotCommands& pilot,
        const IMUData& imu,
        double& da_cmd,
        double& dr_cmd
    ) noexcept {
        const double roll_in  = std::clamp(pilot.roll_stick, -1.0, 1.0);
        const double yaw_in   = std::clamp(pilot.rudder_pedal, -1.0, 1.0);

        // 1. High-AoA Roll Rate Attenuation:
        // Attenuates roll command linearly from 100% at alpha <= 15 deg down to 15% at alpha >= 25.5 deg
        double roll_attenuation = 1.0;
        if (imu.alpha_deg > ALPHA_ATTEN_MIN) {
            const double fraction = (imu.alpha_deg - ALPHA_ATTEN_MIN) / (ALPHA_ATTEN_MAX - ALPHA_ATTEN_MIN);
            roll_attenuation = std::clamp(1.0 - fraction * 0.85, 0.15, 1.0);
        }

        // Commanded roll rate [deg/s] (positive roll stick commands positive/right roll rate)
        const double p_cmd = roll_in * P_CMD_MAX_DPS * roll_attenuation;

        // 2. Closed-Loop Roll Rate Tracking:
        // Negative da produces positive roll (right wing down)
        // e_p = p_cmd - p_meas -> da = - K_roll * e_p
        const double err_p = p_cmd - imu.p_deg;
        const double da_p  = -K_roll * err_p;
        da_cmd = std::clamp(da_p + roll_trim.update(dt, p_cmd, imu.p_deg, Ki_roll, da_p, 20.0),
                            -20.0, 20.0);

        // 3. Aileron-Rudder Interconnect (ARI):
        // Automatically deflects rudder into turn to eliminate adverse yaw and coordinate roll
        // When rolling right (da < 0), dr_ari < 0 (trailing edge right -> nose right moment)
        const double K_ari = std::clamp(0.15 + 0.02 * std::max(0.0, imu.alpha_deg), 0.15, 0.65);
        const double dr_ari = K_ari * da_cmd;

        // 4. Yaw Rate Damper & Sideslip Feedback (Dutch Roll suppression):
        // NASA TP-1538: dr > 0 induces Cn < 0 (nose left).
        // For r > 0 (yawing right), dr must be > 0 to oppose yaw.
        // For beta > 0 (wind from right), dr must be < 0 to yaw nose right into relative wind.
        const double dr_damper = K_yaw_damper * imu.r_deg - K_beta * imu.beta_deg;

        // 5. Manual Pilot Pedal Command:
        // Positive pedal (right foot) commands nose-right yaw (dr < 0)
        const double dr_pedal = -yaw_in * 30.0;

        // Total Rudder Command:
        dr_cmd = std::clamp(dr_ari + dr_damper + dr_pedal, -30.0, 30.0);
    }
};

} // namespace flcs
} // namespace fastjet
