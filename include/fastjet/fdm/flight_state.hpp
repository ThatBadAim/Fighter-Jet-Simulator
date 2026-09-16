#pragma once

#include "../math/vector3.hpp"
#include "../math/quaternion.hpp"
#include <cmath>

namespace fastjet {
namespace fdm {

/**
 * @brief Time derivative of the 6-DoF flight state vector.
 * Stack-allocated value type for zero memory allocations during integration.
 */
struct FlightStateDeriv {
    math::Vector3 d_pos_ned{0.0, 0.0, 0.0};  // Velocity in NED frame [m/s]
    math::Vector3 d_vel_b{0.0, 0.0, 0.0};    // Linear acceleration in body frame [m/s^2]
    math::Vector3 d_omega_b{0.0, 0.0, 0.0};  // Angular acceleration in body frame [rad/s^2]
    math::Quaternion d_q_att{0.0, 0.0, 0.0, 0.0}; // Rate of change of quaternion [1/s]

    constexpr FlightStateDeriv() noexcept = default;

    constexpr FlightStateDeriv(
        const math::Vector3& dp,
        const math::Vector3& dv,
        const math::Vector3& dw,
        const math::Quaternion& dq
    ) noexcept : d_pos_ned(dp), d_vel_b(dv), d_omega_b(dw), d_q_att(dq) {}

    [[nodiscard]] constexpr FlightStateDeriv operator+(const FlightStateDeriv& rhs) const noexcept {
        return {
            d_pos_ned + rhs.d_pos_ned,
            d_vel_b + rhs.d_vel_b,
            d_omega_b + rhs.d_omega_b,
            d_q_att + rhs.d_q_att
        };
    }

    [[nodiscard]] constexpr FlightStateDeriv operator*(double scalar) const noexcept {
        return {
            d_pos_ned * scalar,
            d_vel_b * scalar,
            d_omega_b * scalar,
            d_q_att * scalar
        };
    }
};

[[nodiscard]] constexpr inline FlightStateDeriv operator*(double scalar, const FlightStateDeriv& d) noexcept {
    return d * scalar;
}

/**
 * @brief Zero-allocation 6-DoF rigid-body flight state.
 *
 * Coordinates:
 * - pos_ned: Position vector in North-East-Down (NED) frame [m]. Altitude h = -pos_ned.z.
 * - vel_b: Linear velocity in aircraft body-fixed frame [u, v, w] [m/s].
 *          u: nose (+X), v: right wing (+Y), w: belly (+Z).
 * - omega_b: Angular velocity in body-fixed frame [p, q, r] [rad/s].
 *          p: roll rate, q: pitch rate, r: yaw rate.
 * - q_att: Unit attitude quaternion representing orientation relative to NED frame.
 */
struct FlightState {
    math::Vector3 pos_ned{0.0, 0.0, 0.0};    // [m]
    math::Vector3 vel_b{0.0, 0.0, 0.0};      // [u, v, w] [m/s]
    math::Vector3 omega_b{0.0, 0.0, 0.0};    // [p, q, r] [rad/s]
    math::Quaternion q_att{1.0, 0.0, 0.0, 0.0}; // [w, x, y, z]

    constexpr FlightState() noexcept = default;

    constexpr FlightState(
        const math::Vector3& pos,
        const math::Vector3& vel,
        const math::Vector3& omega,
        const math::Quaternion& q
    ) noexcept : pos_ned(pos), vel_b(vel), omega_b(omega), q_att(q) {}

    // State arithmetic for numerical integration
    [[nodiscard]] constexpr FlightState operator+(const FlightStateDeriv& deriv) const noexcept {
        return {
            pos_ned + deriv.d_pos_ned,
            vel_b + deriv.d_vel_b,
            omega_b + deriv.d_omega_b,
            q_att + deriv.d_q_att
        };
    }

    constexpr FlightState& operator+=(const FlightStateDeriv& deriv) noexcept {
        pos_ned += deriv.d_pos_ned;
        vel_b += deriv.d_vel_b;
        omega_b += deriv.d_omega_b;
        q_att += deriv.d_q_att;
        return *this;
    }

    /**
     * @brief Normalizes attitude quaternion to prevent numerical degradation.
     */
    void normalize_quaternion() noexcept {
        q_att.normalize();
    }

    // Kinematic helper queries
    [[nodiscard]] constexpr double altitude() const noexcept {
        return -pos_ned.z;
    }

    [[nodiscard]] double airspeed() const noexcept {
        return vel_b.norm();
    }

    [[nodiscard]] double alpha() const noexcept {
        // Angle of attack: atan2(w, u)
        if (std::abs(vel_b.x) < 1e-9 && std::abs(vel_b.z) < 1e-9) {
            return 0.0;
        }
        return std::atan2(vel_b.z, vel_b.x);
    }

    [[nodiscard]] double beta() const noexcept {
        // Sideslip angle: asin(v / V)
        const double V = airspeed();
        if (V < 1e-9) {
            return 0.0;
        }
        const double ratio = std::clamp(vel_b.y / V, -1.0, 1.0);
        return std::asin(ratio);
    }

    [[nodiscard]] constexpr math::Vector3 velocity_ned() const noexcept {
        return q_att.rotate_body_to_ned(vel_b);
    }

    [[nodiscard]] math::Quaternion::EulerAngles euler_angles() const noexcept {
        return q_att.to_euler();
    }

    [[nodiscard]] double roll() const noexcept {
        return q_att.to_euler().roll;
    }

    [[nodiscard]] double pitch() const noexcept {
        return q_att.to_euler().pitch;
    }

    [[nodiscard]] double yaw() const noexcept {
        return q_att.to_euler().yaw;
    }
};

} // namespace fdm
} // namespace fastjet
