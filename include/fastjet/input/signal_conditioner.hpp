#pragma once

#include <algorithm>
#include <cmath>

namespace fastjet {
namespace input {

/**
 * @brief Configuration parameters for a single analog controller axis.
 */
struct AxisCalibration {
    int raw_min{-32768};           // Minimum raw integer value (e.g. SDL3/DirectInput 16-bit range)
    int raw_center{0};              // Neutral/center raw integer value
    int raw_max{32767};            // Maximum raw integer value
    double inner_deadband{0.03};   // Inner deadband fraction [0.0, 0.20] (removes center sensor noise)
    double outer_deadband{0.02};   // Outer deadband fraction [0.0, 0.20] (guarantees 100% deflection at stops)
    bool inverted{false};          // Reverses axis direction if true
    double curvature{0.50};        // Force-sensing polynomial curvature c in [0.0, 1.0]

    constexpr AxisCalibration() noexcept = default;

    constexpr AxisCalibration(
        int min_val, int center_val, int max_val,
        double inner_db, double outer_db, bool inv, double curve
    ) noexcept
        : raw_min(min_val), raw_center(center_val), raw_max(max_val),
          inner_deadband(inner_db), outer_deadband(outer_db),
          inverted(inv), curvature(curve) {}
};

/**
 * @brief Signal Conditioning and Transfer Function Engine.
 * Features:
 * - Full calibration offset normalization (min, center, max).
 * - Smooth inner and outer deadband transition without step discontinuities.
 * - F-16 rigid force-sensing side-stick emulation curve:
 *     y = (1 - c) * x + c * x^3
 *
 * Zero dynamic heap allocation, fully deterministic.
 */
class SignalConditioner {
public:
    /**
     * @brief Computes the F-16 force-sensing simulation curve:
     * y = sign(x) * [ (1 - c)*|x| + c*|x|^3 ]
     *
     * @param x Input normalized value in [-1.0, +1.0]
     * @param c Curvature parameter in [0.0, 1.0] (0 = Linear, 1 = Full Cubic)
     * @return double Shaped output in [-1.0, +1.0]
     */
    [[nodiscard]] static constexpr double evaluate_force_curve(double x, double c) noexcept {
        const double clamped_c = std::clamp(c, 0.0, 1.0);
        const double mag = x < 0.0 ? -x : x;
        if (mag < 1e-9) return 0.0;

        // y = (1 - c)*x + c*x^3
        const double shaped_mag = (1.0 - clamped_c) * mag + clamped_c * (mag * mag * mag);
        return (x >= 0.0) ? shaped_mag : -shaped_mag;
    }

    /**
     * @brief Applies inner and outer deadbands with smooth linear rescaling.
     * Between inner_db and (1.0 - outer_db), output scales smoothly from 0.0 to 1.0.
     *
     * @param x Input in [-1.0, 1.0]
     * @param inner_db Inner deadband [0.0, 0.5)
     * @param outer_db Outer deadband [0.0, 0.5)
     * @return double Deadband-conditioned output in [-1.0, 1.0]
     */
    [[nodiscard]] static constexpr double apply_deadbands(double x, double inner_db, double outer_db) noexcept {
        const double mag = x < 0.0 ? -x : x;
        const double i_db = std::clamp(inner_db, 0.0, 0.40);
        const double o_db = std::clamp(outer_db, 0.0, 0.40);

        if (mag <= i_db) {
            return 0.0;
        }

        const double active_range = (1.0 - o_db) - i_db;
        if (active_range <= 1e-6) {
            return (x >= 0.0) ? 1.0 : -1.0;
        }

        if (mag >= (1.0 - o_db)) {
            return (x >= 0.0) ? 1.0 : -1.0;
        }

        const double rescaled_mag = (mag - i_db) / active_range;
        return (x >= 0.0) ? rescaled_mag : -rescaled_mag;
    }

    /**
     * @brief Processes a raw bipolar axis (e.g. Pitch, Roll, Yaw) into conditioned [-1.0, +1.0].
     *
     * @param raw_value Integer reading from hardware (e.g. -32768 to 32767)
     * @param cal Axis calibration profile
     * @return double Fully conditioned, deadband-filtered, and curve-shaped output
     */
    [[nodiscard]] static constexpr double process_bipolar(int raw_value, const AxisCalibration& cal) noexcept {
        double x = 0.0;

        // 1. Normalize based on min, center, and max offsets
        if (raw_value < cal.raw_center) {
            const double span = static_cast<double>(cal.raw_center - cal.raw_min);
            x = (span > 1.0) ? static_cast<double>(raw_value - cal.raw_center) / span : 0.0;
        } else {
            const double span = static_cast<double>(cal.raw_max - cal.raw_center);
            x = (span > 1.0) ? static_cast<double>(raw_value - cal.raw_center) / span : 0.0;
        }
        x = std::clamp(x, -1.0, 1.0);

        // 2. Apply axis inversion toggle
        if (cal.inverted) {
            x = -x;
        }

        // 3. Apply inner and outer deadbands
        const double x_deadband = apply_deadbands(x, cal.inner_deadband, cal.outer_deadband);

        // 4. Apply force-sensing polynomial transfer curve
        return evaluate_force_curve(x_deadband, cal.curvature);
    }

    /**
     * @brief Processes a raw unipolar axis (e.g. Throttle, Toe Brakes) into [0.0, 1.0].
     *
     * @param raw_value Integer reading from hardware
     * @param cal Axis calibration profile
     * @return double Output in [0.0, 1.0]
     */
    [[nodiscard]] static constexpr double process_unipolar(int raw_value, const AxisCalibration& cal) noexcept {
        const double span = static_cast<double>(cal.raw_max - cal.raw_min);
        double u = (span > 1.0) ? static_cast<double>(raw_value - cal.raw_min) / span : 0.0;
        u = std::clamp(u, 0.0, 1.0);

        if (cal.inverted) {
            u = 1.0 - u;
        }

        // Apply deadbands on unipolar axis:
        // [0.0, inner_db] -> 0.0
        // [(1.0 - outer_db), 1.0] -> 1.0
        const double i_db = std::clamp(cal.inner_deadband, 0.0, 0.20);
        const double o_db = std::clamp(cal.outer_deadband, 0.0, 0.20);

        if (u <= i_db) return 0.0;
        if (u >= (1.0 - o_db)) return 1.0;

        const double active_range = (1.0 - o_db) - i_db;
        return (active_range > 1e-6) ? ((u - i_db) / active_range) : 0.0;
    }
};

} // namespace input
} // namespace fastjet
