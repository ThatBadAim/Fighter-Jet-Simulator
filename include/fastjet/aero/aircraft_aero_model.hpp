#pragma once

#include "f16_aero_model.hpp"
#include "../aircraft/aircraft_type.hpp"
#include "../aircraft/aircraft_config.hpp"
#include <cmath>
#include <algorithm>

namespace fastjet::aero {

/**
 * @brief Unified aerodynamic model supporting all 5 combat aircraft.
 * Delegates to NASA TP-1538 wind-tunnel lookup tables for F-16C, and utilizes
 * high-fidelity DATCOM / NASA TM dimensional polar synthesis with compressibility,
 * wave drag, damping derivatives, and high-alpha vortex lift for F-15EX, Typhoon, F-22, and A-10.
 *
 * Zero dynamic heap allocation.
 */
class AircraftAeroModel {
public:
    aircraft::AircraftType aircraft_type{aircraft::AircraftType::F16_FIGHTING_FALCON};
    aircraft::AeroConfig config{};

    constexpr AircraftAeroModel() noexcept {
        configure(aircraft::AircraftType::F16_FIGHTING_FALCON);
    }

    explicit constexpr AircraftAeroModel(aircraft::AircraftType type) noexcept {
        configure(type);
    }

    constexpr void configure(aircraft::AircraftType type) noexcept {
        aircraft_type = type;
        config = aircraft::AircraftConfig::get(type).aero;
    }

    /**
     * @brief Prandtl-Glauert compressibility correction factor.
     *
     * Continuous across the whole Mach range: subsonic PG up to the table limit,
     * held on that plateau through the transonic region until the supersonic
     * Ackeret factor falls below it, then Ackeret — floored at 1.0 so behaviour
     * well above M1.4 is unchanged.  This used to switch to an Ackeret branch
     * bounded at M^2-1 >= 0.1 above a hard-coded M1.05 while the caller gated on
     * each airframe's own mach_peak (1.10-1.15), multiplying lift by up to 2.84
     * in that band and then dropping it to 1.0: 10-15 G spikes near Mach 1.1.
     */
    [[nodiscard]] double prandtl_glauert(double mach) const noexcept {
        constexpr double MACH_TABLE_LIMIT = 0.80;

        if (mach < MACH_TABLE_LIMIT) {
            return 1.0 / std::sqrt((std::max)(0.05, 1.0 - mach * mach));
        }
        const double plateau = 1.0 / std::sqrt(1.0 - MACH_TABLE_LIMIT * MACH_TABLE_LIMIT);
        if (mach <= 1.0) {
            return plateau;
        }
        const double ackeret = 1.0 / std::sqrt((std::max)(mach * mach - 1.0, 1e-6));
        return std::clamp(ackeret, 1.0, plateau);
    }

    /**
     * @brief Transonic and supersonic wave drag increment.
     */
    [[nodiscard]] double wave_drag(double mach) const noexcept {
        if (mach <= config.mach_crit) return 0.0;

        if (mach <= config.mach_peak) {
            const double t = (mach - config.mach_crit) / (config.mach_peak - config.mach_crit);
            const double wave = config.cd_wave_peak * t * t * (3.0 - 2.0 * t);
            if (aircraft_type == aircraft::AircraftType::A10_THUNDERBOLT && mach > 0.70) {
                const double a10_rise = (mach - 0.70) / 0.15;
                return wave + 0.045 * a10_rise * a10_rise;
            }
            return wave;
        }

        const double decay = std::exp(-(mach - config.mach_peak) / 0.45);
        const double base = config.cd_wave_super + (config.cd_wave_peak - config.cd_wave_super) * decay;
        const double excess = (std::max)(0.0, mach - config.mach_peak);

        // A-10 is purely subsonic: wave drag climbs catastrophically past M 0.75
        if (aircraft_type == aircraft::AircraftType::A10_THUNDERBOLT) {
            return base + 0.15 * excess * excess;
        }

        return base + 0.006 * excess * excess;
    }

    /**
     * @brief Dynamic pressure drag rise near structural airspeed limits (~800 KEAS).
     */
    [[nodiscard]] double dynamic_pressure_drag(double q_bar) const noexcept {
        constexpr double Q_DRAG_ONSET = 60000.0; // 60 kPa onset
        if (q_bar <= Q_DRAG_ONSET) return 0.0;
        const double excess = (q_bar - Q_DRAG_ONSET) / Q_DRAG_ONSET;
        return 0.095 * excess * excess;
    }

    /**
     * @brief Computes non-dimensional aerodynamic coefficients for the active aircraft.
     */
    [[nodiscard]] AeroCoefficients compute_coefficients(
        const fdm::FlightState& state,
        const ControlSurfaces& controls,
        double q_bar,
        double mach = 0.0
    ) const noexcept {
        // For F-16C, utilize the exact NASA TP-1538 tables
        if (aircraft_type == aircraft::AircraftType::F16_FIGHTING_FALCON) {
            return F16AeroModel::compute_coefficients(state, controls, q_bar, mach);
        }

        AeroCoefficients coeffs{};
        coeffs.q_bar = q_bar;
        coeffs.mach = mach;

        constexpr double RAD_TO_DEG = 180.0 / M_PI;
        const double V = state.airspeed();
        const double v_safe = (std::max)(V, 1.0);
        const double alpha_rad = state.alpha();
        const double beta_rad  = state.beta();

        coeffs.alpha_deg = alpha_rad * RAD_TO_DEG;
        coeffs.beta_deg  = beta_rad * RAD_TO_DEG;

        const ControlSurfaces ctrl = controls.clamped();

        // 1. Lift Coefficient (CL)
        // High-alpha vortex lift model for Typhoon (canard-delta) and F-22 (diamond wing + chine)
        double cl_linear = config.cl_alpha * alpha_rad;
        double cl_vortex = 0.0;

        if (aircraft_type == aircraft::AircraftType::EUROFIGHTER_TYPHOON ||
            aircraft_type == aircraft::AircraftType::F22_RAPTOR) {
            // Polhamus leading-edge suction analogy for slender/delta wings:
            // CL_vortex ~ K_v * sin^2(alpha) * cos(alpha)
            constexpr double K_V = 2.4;
            const double sin_a = std::sin(alpha_rad);
            const double cos_a = std::cos(alpha_rad);
            cl_vortex = K_V * sin_a * sin_a * cos_a;
        }

        // Elevator / Stabilator lift contribution
        constexpr double DEG_TO_RAD = M_PI / 180.0;
        const double cl_delta_e = 0.40 * (ctrl.delta_e * DEG_TO_RAD);

        double raw_cl = cl_linear + cl_vortex + cl_delta_e;
        // Clamp to aerodynamic stall limit
        const double max_cl = (aircraft_type == aircraft::AircraftType::F22_RAPTOR) ? 2.2 : config.cl_max;
        coeffs.CL = std::clamp(raw_cl, -max_cl, max_cl);

        // 2. Drag Coefficient (CD)
        // Polar: CD = CD0 + k * CL^2
        const double cd_induced = config.k_induced * (coeffs.CL * coeffs.CL);
        coeffs.CD = config.cd0 + cd_induced;

        // Compressibility and wave drag
        if (mach > 0.0) {
            coeffs.CD += wave_drag(mach);
            coeffs.CD += dynamic_pressure_drag(q_bar);
        }

        // Speedbrake increment
        if (ctrl.speedbrake > 0.0) {
            coeffs.CD += config.cd_speedbrake * ctrl.speedbrake;
        }

        // 3. Pitching Moment (Cm)
        // Static margin stability + pitch control surface authority
        // Subsonic delta-canard (Typhoon) and F-22 have relaxed static stability
        double static_margin = 0.05; // Standard 5% positive for F-15 and A-10
        if (aircraft_type == aircraft::AircraftType::EUROFIGHTER_TYPHOON ||
            aircraft_type == aircraft::AircraftType::F22_RAPTOR) {
            static_margin = (mach < 1.0) ? -0.08 : +0.06; // Transonic aerodynamic center shift
        }
        const double cm_alpha = -config.cl_alpha * static_margin;
        const double cm_delta_e = -0.90 * (ctrl.delta_e * DEG_TO_RAD); // Pitch control surface power
        const double lon_damping = config.c_bar / (2.0 * v_safe);
        const double cm_q = config.pitch_damping * (lon_damping * state.omega_b.y);

        coeffs.Cm = cm_alpha * alpha_rad + cm_delta_e + cm_q;

        // Speedbrake pitch increment
        if (ctrl.speedbrake > 0.0) {
            coeffs.Cm += config.cm_speedbrake * ctrl.speedbrake;
        }

        // 4. Lateral-Directional Coefficients (CY, Cl, Cn)
        const double lat_damping = config.b_span / (2.0 * v_safe);
        const double p_rate = state.omega_b.x;
        const double r_rate = state.omega_b.z;

        // Sideforce CY
        coeffs.CY = -0.75 * beta_rad + 0.18 * (ctrl.delta_r * DEG_TO_RAD);

        // Roll moment Cl
        // Dihedral effect + aileron/flaperon power + roll damping
        coeffs.Cl = -0.12 * beta_rad - 0.22 * (ctrl.delta_a * DEG_TO_RAD) +
                    config.roll_damping * (lat_damping * p_rate);

        // Yaw moment Cn
        // Directional stability + rudder power + yaw damping
        coeffs.Cn = +0.16 * beta_rad - 0.10 * (ctrl.delta_r * DEG_TO_RAD) +
                    config.yaw_damping * (lat_damping * r_rate);

        // Apply compressibility scaling to lift and moments (continuous in Mach)
        if (mach > 0.0) {
            const double pg = prandtl_glauert(mach);
            coeffs.CL *= pg;
            coeffs.CY *= pg;
            coeffs.Cl *= pg;
            coeffs.Cn *= pg;
            coeffs.Cm *= pg;
        }

        return coeffs;
    }

    /**
     * @brief Computes total aerodynamic forces and moments in aircraft body frame.
     */
    [[nodiscard]] fdm::AircraftForces compute_forces_and_moments(
        const fdm::FlightState& state,
        const ControlSurfaces& controls,
        double q_bar,
        double mach = 0.0
    ) const noexcept {
        // Delegate F-16 directly
        if (aircraft_type == aircraft::AircraftType::F16_FIGHTING_FALCON) {
            return F16AeroModel::compute_forces_and_moments(state, controls, q_bar, mach);
        }

        const AeroCoefficients coeffs = compute_coefficients(state, controls, q_bar, mach);

        const double alpha_rad = state.alpha();
        const double sin_a = std::sin(alpha_rad);
        const double cos_a = std::cos(alpha_rad);

        const double qS = coeffs.q_bar * config.s_ref;

        // Forces in body frame
        const double Fx_b = qS * (coeffs.CL * sin_a - coeffs.CD * cos_a);
        const double Fy_b = qS * coeffs.CY;
        const double Fz_b = qS * (-coeffs.CL * cos_a - coeffs.CD * sin_a);

        // Moments in body frame
        const double Mx_b = qS * config.b_span * coeffs.Cl;
        const double My_b = qS * config.c_bar * coeffs.Cm;
        const double Mz_b = qS * config.b_span * coeffs.Cn;

        return {
            math::Vector3(Fx_b, Fy_b, Fz_b),
            math::Vector3(Mx_b, My_b, Mz_b)
        };
    }
};

} // namespace fastjet::aero
