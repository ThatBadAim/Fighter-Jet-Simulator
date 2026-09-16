#pragma once

#include "../math/vector3.hpp"
#include "../math/matrix3x3.hpp"
#include "../math/quaternion.hpp"
#include "flight_state.hpp"
#include "mass_properties.hpp"
#include "aircraft_forces.hpp"

namespace fastjet {
namespace fdm {

/**
 * @brief 6-Degrees-of-Freedom (6-DoF) Rigid-Body Flight Dynamics Evaluator.
 * Computes exact state time derivatives for translation, rotation, and kinematics.
 * Zero heap allocation.
 */
class SixDoFFDM {
public:
    static constexpr double GRAVITY_ACCEL = 9.80665; // [m/s^2]

    /**
     * @brief Computes the state derivative \dot{x} = f(x, forces, mass_properties).
     *
     * @param state Current flight state (pos_ned, vel_b, omega_b, q_att).
     * @param forces External forces and moments in body frame (Aerodynamic + Thrust).
     * @param mass Mass properties including precomputed I and I_inv.
     * @return FlightStateDeriv containing d_pos_ned, d_vel_b, d_omega_b, d_q_att.
     */
    [[nodiscard]] static constexpr FlightStateDeriv compute_derivatives(
        const FlightState& state,
        const AircraftForces& forces,
        const MassProperties& mass
    ) noexcept {
        FlightStateDeriv deriv;

        // 1. Position Kinematics: \dot{p}_NED = C_n_b * v_b
        deriv.d_pos_ned = state.q_att.rotate_body_to_ned(state.vel_b);

        // 2. Gravity vector in NED frame: g_ned = [0, 0, g0]
        constexpr math::Vector3 g_ned{0.0, 0.0, GRAVITY_ACCEL};

        // Gravity transformed into body frame: g_b = C_b_n * g_ned
        const math::Vector3 g_b = state.q_att.rotate_ned_to_body(g_ned);

        // 3. Linear Accelerations in body frame (Newton's 2nd Law in rotating frame):
        // \dot{v}_b = (F_ext / m) + g_b - (\omega_b \times v_b)
        const math::Vector3 f_ext_accel = forces.force_b * mass.inv_mass;
        const math::Vector3 coriolis_accel = state.omega_b.cross(state.vel_b);
        deriv.d_vel_b = f_ext_accel + g_b - coriolis_accel;

        // 4. Angular Accelerations in body frame (Euler's Rotational Equations):
        // I * \dot{\omega}_b + \omega_b \times (I * \omega_b) = M_ext
        // \dot{\omega}_b = I^-1 * (M_ext - \omega_b \times (I * \omega_b))
        const math::Vector3 I_omega = mass.I * state.omega_b;
        const math::Vector3 gyro_coupling = state.omega_b.cross(I_omega);
        const math::Vector3 net_moment = forces.moment_b - gyro_coupling;
        deriv.d_omega_b = mass.I_inv * net_moment;

        // 5. Attitude Kinematics (Quaternion rate of change):
        // \dot{q} = 0.5 * q \otimes [0, p, q, r]
        deriv.d_q_att = state.q_att.derivative(state.omega_b);

        return deriv;
    }
};

} // namespace fdm
} // namespace fastjet
