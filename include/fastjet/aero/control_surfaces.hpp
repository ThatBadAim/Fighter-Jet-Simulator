#pragma once

#include <algorithm>

namespace fastjet {
namespace aero {

/**
 * @brief F-16 flight control surface positions [degrees].
 * Deflection sign conventions match NASA TP-1538 / Stevens & Lewis:
 * - delta_e: Elevator deflection [-25 deg, +25 deg] (trailing edge down is positive).
 * - delta_a: Aileron deflection [-20 deg, +20 deg] (differential flaperon).
 * - delta_r: Rudder deflection [-30 deg, +30 deg] (trailing edge left is positive).
 * - delta_lef: Leading-edge flap deflection [0 deg, 25 deg] (droop down is positive).
 * - speedbrake: Airbrake extension fraction [0.0, 1.0] (0 = closed, 1 = fully open).
 */
struct ControlSurfaces {
    double delta_e{0.0};    // Elevator deflection [deg]
    double delta_a{0.0};    // Aileron deflection [deg]
    double delta_r{0.0};    // Rudder deflection [deg]
    double delta_lef{0.0};  // Leading-edge flap deflection [deg]
    double speedbrake{0.0}; // Airbrake extension fraction [-]

    constexpr ControlSurfaces() noexcept = default;
    constexpr ControlSurfaces(double de, double da, double dr, double dlef = 0.0,
                              double sb = 0.0) noexcept
        : delta_e(de), delta_a(da), delta_r(dr), delta_lef(dlef), speedbrake(sb) {}

    [[nodiscard]] constexpr ControlSurfaces clamped() const noexcept {
        return {
            std::clamp(delta_e, -25.0, 25.0),
            std::clamp(delta_a, -20.0, 20.0),
            std::clamp(delta_r, -30.0, 30.0),
            std::clamp(delta_lef, 0.0, 25.0),
            std::clamp(speedbrake, 0.0, 1.0)
        };
    }
};

} // namespace aero
} // namespace fastjet
