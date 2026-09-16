#pragma once

#include <algorithm>

namespace fastjet {
namespace flcs {

/**
 * @brief Normalized pilot cockpit inceptor commands.
 * Zero allocation value struct.
 */
struct PilotCommands {
    double pitch_stick{0.0};   // Pitch stick input [-1.0, +1.0] (+1.0 = Full aft stick / pitch up)
    double roll_stick{0.0};    // Lateral stick input [-1.0, +1.0] (+1.0 = Full right roll)
    double rudder_pedal{0.0};  // Rudder pedal input [-1.0, +1.0] (+1.0 = Full right rudder)

    constexpr PilotCommands() noexcept = default;
    constexpr PilotCommands(double pitch, double roll, double yaw) noexcept
        : pitch_stick(pitch), roll_stick(roll), rudder_pedal(yaw) {}

    [[nodiscard]] constexpr PilotCommands clamped() const noexcept {
        return {
            std::clamp(pitch_stick, -1.0, 1.0),
            std::clamp(roll_stick, -1.0, 1.0),
            std::clamp(rudder_pedal, -1.0, 1.0)
        };
    }
};

} // namespace flcs
} // namespace fastjet
