#pragma once

#include "fastjet/fdm/aircraft_forces.hpp"
#include "fastjet/fdm/flight_state.hpp"
#include "fastjet/fdm/mass_properties.hpp"
#include "fastjet/fdm/six_dof_fdm.hpp"
#include "fastjet/math/vector3.hpp"
#include <algorithm>
#include <cmath>

namespace fastjet::sim {

/**
 * @brief Relative geometry between an attacker ("own") and a target, in the
 * terms fighter pilots use.
 *
 *  - Range and closure (Vc, positive when closing).
 *  - Antenna train angle (ATA): angle off the attacker's nose to the target.
 *  - Aspect angle (AA): angle off the target's tail to the attacker.
 *    0 deg = dead six, 180 deg = head-on.
 *  - Heading crossing angle (HCA): angle between the two velocity vectors.
 *
 * All angles in radians.
 */
struct AirCombatGeometry {
    math::Vector3 los_ned{};  ///< Target minus own position [m]
    math::Vector3 los_b{};    ///< Line of sight in own body axes [m]
    double range{0.0};
    double closure{0.0};
    double ata{0.0};
    double aspect{0.0};
    double hca{0.0};

    [[nodiscard]] static AirCombatGeometry compute(const fdm::FlightState& own,
                                                   const fdm::FlightState& target) noexcept {
        AirCombatGeometry g;
        g.los_ned = target.pos_ned - own.pos_ned;
        g.range = g.los_ned.norm();
        g.los_b = own.q_att.rotate_ned_to_body(g.los_ned);
        const math::Vector3 v_own = own.velocity_ned();
        const math::Vector3 v_tgt = target.velocity_ned();
        if (g.range < 1e-6) return g;

        const math::Vector3 u = g.los_ned / g.range;
        g.closure = -(v_tgt - v_own).dot(u);
        g.ata = angle_between(own.q_att.rotate_body_to_ned(math::Vector3(1.0, 0.0, 0.0)), u);
        // Aspect: from the target's tail (-v_tgt) to the line from target to attacker (-u).
        g.aspect = angle_between(-v_tgt, -u);
        g.hca = angle_between(v_own, v_tgt);
        return g;
    }

    [[nodiscard]] static double angle_between(const math::Vector3& a, const math::Vector3& b) noexcept {
        const double na = a.norm();
        const double nb = b.norm();
        if (na < 1e-9 || nb < 1e-9) return 0.0;
        return std::acos(std::clamp(a.dot(b) / (na * nb), -1.0, 1.0));
    }
};

/// @brief Own-ship energy and turn readouts.
struct EnergyState {
    double turn_rate{0.0};  ///< Rate of rotation of the velocity vector [rad/s]
    double ps{0.0};         ///< Specific excess power [m/s]
    double energy_height{0.0}; ///< h + V^2 / 2g [m]

    /// @param forces Aero + thrust (+ gear) forces in body axes, gravity excluded.
    [[nodiscard]] static EnergyState compute(const fdm::FlightState& s, const fdm::AircraftForces& forces,
                                             const fdm::MassProperties& mass) noexcept {
        constexpr double G = fdm::SixDoFFDM::GRAVITY_ACCEL;
        EnergyState e;
        const double V = s.airspeed();
        e.energy_height = s.altitude() + V * V / (2.0 * G);
        if (V < 1.0) return e;
        const double m = std::max(1.0, mass.mass_kg);
        // Excess power: work done by thrust minus drag, per unit weight.
        e.ps = forces.force_b.dot(s.vel_b) / (m * G);
        // Turn rate: acceleration normal to the velocity (gravity included) over V.
        const math::Vector3 a_ned = s.q_att.rotate_body_to_ned(forces.force_b / m) + math::Vector3(0.0, 0.0, G);
        const math::Vector3 v_ned = s.velocity_ned();
        const math::Vector3 u = v_ned / V;
        const math::Vector3 a_perp = a_ned - u * a_ned.dot(u);
        e.turn_rate = a_perp.norm() / V;
        return e;
    }
};

/// @brief Closest distance between segments p0-p1 and q0-q1; @p s and @p t
/// return the parameters of the closest points on each.
[[nodiscard]] inline double segment_segment_distance(const math::Vector3& p0, const math::Vector3& p1,
                                                     const math::Vector3& q0, const math::Vector3& q1,
                                                     double& s, double& t) noexcept {
    const math::Vector3 d1 = p1 - p0;
    const math::Vector3 d2 = q1 - q0;
    const math::Vector3 r = p0 - q0;
    const double a = d1.dot(d1);
    const double e = d2.dot(d2);
    const double f = d2.dot(r);
    constexpr double EPS = 1e-12;

    if (a <= EPS && e <= EPS) {
        s = t = 0.0;
        return r.norm();
    }
    if (a <= EPS) {
        s = 0.0;
        t = std::clamp(f / e, 0.0, 1.0);
    } else {
        const double c = d1.dot(r);
        if (e <= EPS) {
            t = 0.0;
            s = std::clamp(-c / a, 0.0, 1.0);
        } else {
            const double b = d1.dot(d2);
            const double denom = a * e - b * b;
            s = (denom > EPS) ? std::clamp((b * f - c * e) / denom, 0.0, 1.0) : 0.0;
            t = (b * s + f) / e;
            if (t < 0.0) {
                t = 0.0;
                s = std::clamp(-c / a, 0.0, 1.0);
            } else if (t > 1.0) {
                t = 1.0;
                s = std::clamp((b - c) / a, 0.0, 1.0);
            }
        }
    }
    const math::Vector3 c1 = p0 + d1 * s;
    const math::Vector3 c2 = q0 + d2 * t;
    return (c1 - c2).norm();
}

} // namespace fastjet::sim
