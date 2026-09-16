#pragma once

#include <algorithm>
#include <cmath>

namespace fastjet {
namespace input {

/**
 * @brief Digital trim hat state for pitch and roll trim.
 */
class DigitalTrimHat {
public:
    double trim_pitch{0.0};     // Pitch trim offset [-0.5, +0.5] (+ = Nose Up)
    double trim_roll{0.0};      // Roll trim offset [-0.5, +0.5] (+ = Right Wing Down)
    double slew_rate{0.04};     // Slew rate per second

    constexpr DigitalTrimHat() noexcept = default;

    void step(double pitch_dir, double roll_dir, double dt) noexcept {
        if (dt <= 0.0) return;
        trim_pitch += std::clamp(pitch_dir, -1.0, 1.0) * slew_rate * dt;
        trim_roll  += std::clamp(roll_dir, -1.0, 1.0) * slew_rate * dt;

        trim_pitch = std::clamp(trim_pitch, -0.5, 0.5);
        trim_roll  = std::clamp(trim_roll, -0.5, 0.5);
    }

    void reset() noexcept {
        trim_pitch = 0.0;
        trim_roll = 0.0;
    }
};

/**
 * @brief F-16 Engine Throttle Controller with physical detents:
 * - Cutoff: u < 0.03
 * - Idle: 0.08
 * - Military (Dry) Power: 0.85
 * - Afterburner Zone: 0.85 to 1.00 (Wet thrust)
 */
class ThrottleController {
public:
    enum class DetentState {
        CUTOFF,
        IDLE,
        MIL_POWER,
        AFTERBURNER
    };

    static constexpr double DETENT_CUTOFF = 0.03;
    static constexpr double DETENT_IDLE   = 0.08;
    static constexpr double DETENT_MIL    = 0.85;

    // F-16 F110-GE-129 / F100-PW-229 Engine Thrust Ratings (SI units: Newtons)
    static constexpr double THRUST_IDLE_N = 4450.0;    // ~1,000 lbf
    static constexpr double THRUST_MIL_N  = 75600.0;   // ~17,000 lbf (Dry Max)
    static constexpr double THRUST_MAX_AB = 129000.0;  // ~29,000 lbf (Full Afterburner)

    DetentState state{DetentState::IDLE};
    double dry_power{0.0};       // [0.0, 1.0]
    double ab_power{0.0};        // [0.0, 1.0]
    double net_thrust_n{0.0};    // Net engine thrust [N]

    /**
     * @brief Evaluates engine thrust from physical throttle lever position in [0.0, 1.0].
     *
     * @param throttle_position Lever position in [0.0, 1.0]
     * @param ab_gate_open Boolean flag simulating physical afterburner detent gate pushthrough
     * @return double Net engine thrust in Newtons
     */
    double update(double throttle_position, bool ab_gate_open = true) noexcept {
        const double u = std::clamp(throttle_position, 0.0, 1.0);

        if (u < DETENT_CUTOFF) {
            state = DetentState::CUTOFF;
            dry_power = 0.0;
            ab_power = 0.0;
            net_thrust_n = 0.0;
            return net_thrust_n;
        }

        if (u <= DETENT_IDLE) {
            state = DetentState::IDLE;
            dry_power = 0.0;
            ab_power = 0.0;
            net_thrust_n = THRUST_IDLE_N;
            return net_thrust_n;
        }

        if (u <= DETENT_MIL || !ab_gate_open) {
            state = DetentState::MIL_POWER;
            const double mil_range = DETENT_MIL - DETENT_IDLE;
            dry_power = std::clamp((u - DETENT_IDLE) / mil_range, 0.0, 1.0);
            ab_power = 0.0;
            net_thrust_n = THRUST_IDLE_N + dry_power * (THRUST_MIL_N - THRUST_IDLE_N);
            return net_thrust_n;
        }

        // Afterburner Range (0.85 to 1.00)
        state = DetentState::AFTERBURNER;
        dry_power = 1.0;
        const double ab_range = 1.0 - DETENT_MIL;
        ab_power = std::clamp((u - DETENT_MIL) / ab_range, 0.0, 1.0);
        net_thrust_n = THRUST_MIL_N + ab_power * (THRUST_MAX_AB - THRUST_MIL_N);
        return net_thrust_n;
    }
};

/**
 * @brief F-16 Airbrake (Speedbrake) Actuator.
 * 3-Position HOTAS Switch: EXTEND, OFF, RETRACT.
 * Fully extends in 2.0 seconds (60 deg max deflection).
 */
class SpeedbrakeController {
public:
    enum class SwitchPosition {
        RETRACT = -1,
        OFF     =  0,
        EXTEND  = +1
    };

    double position{0.0};      // Position in [0.0, 1.0] (0 = closed, 1 = fully open)
    double slew_rate{0.50};    // 0.5 / s -> 2.0s full deployment time

    constexpr SpeedbrakeController() noexcept = default;

    void update(SwitchPosition sw, double dt) noexcept {
        if (dt <= 0.0) return;
        if (sw == SwitchPosition::EXTEND) {
            position += slew_rate * dt;
        } else if (sw == SwitchPosition::RETRACT) {
            position -= slew_rate * dt;
        }
        position = std::clamp(position, 0.0, 1.0);
    }

    [[nodiscard]] double get_drag_coefficient_increment() const noexcept {
        // Full extension adds Delta CD ~ +0.025
        return 0.025 * position;
    }
};

/**
 * @brief Differential Wheel Brakes for ground steering and rollout deceleration.
 */
struct WheelBrakes {
    double left_brake{0.0};    // [0.0, 1.0]
    double right_brake{0.0};   // [0.0, 1.0]
    bool parking_brake{false};

    [[nodiscard]] constexpr double effective_left() const noexcept {
        return parking_brake ? 1.0 : left_brake;
    }

    [[nodiscard]] constexpr double effective_right() const noexcept {
        return parking_brake ? 1.0 : right_brake;
    }
};

} // namespace input
} // namespace fastjet
