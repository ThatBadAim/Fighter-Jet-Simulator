#pragma once

#include "../math/vector3.hpp"

namespace fastjet {
namespace fdm {

/**
 * @brief Total external forces and moments acting on the aircraft in the body frame.
 * Zero dynamic memory allocation.
 */
struct AircraftForces {
    math::Vector3 force_b{0.0, 0.0, 0.0};    // External force in body frame (Aero + Thrust) [N]
    math::Vector3 moment_b{0.0, 0.0, 0.0};   // External moment in body frame (Aero + Thrust) [N*m]

    constexpr AircraftForces() noexcept = default;
    constexpr AircraftForces(const math::Vector3& f, const math::Vector3& m) noexcept
        : force_b(f), moment_b(m) {}

    static constexpr AircraftForces zero() noexcept {
        return {math::Vector3::zero(), math::Vector3::zero()};
    }
};

} // namespace fdm
} // namespace fastjet
