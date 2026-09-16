#pragma once

#include "../math/vector3.hpp"
#include "../fdm/flight_state.hpp"
#include "../fdm/aircraft_forces.hpp"
#include "../fdm/mass_properties.hpp"
#include "../aircraft/aircraft_type.hpp"
#include "../aircraft/aircraft_config.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace fastjet {
namespace gear {

/**
 * @brief A single landing gear strut (oleo-pneumatic shock absorber + tyre).
 *
 * Modelled as a spring-damper acting along the body Z axis, with Coulomb
 * friction at the contact patch resolved into rolling (longitudinal) and
 * cornering (lateral) components.
 */
struct Strut {
    math::Vector3 pos_b{0.0, 0.0, 0.0}; // Attachment point in body frame [m]
    double rest_length{0.0};            // Uncompressed strut length [m]
    double max_stroke{0.0};             // Maximum compression travel [m]
    double spring_k{0.0};               // Strut spring rate [N/m]
    double damping_c{0.0};              // Strut damping coefficient [N/(m/s)]
    bool steerable{false};              // Nose gear castors/steers
    bool braked{false};                 // Main gear carries a brake

    // --- Live state ---
    double compression{0.0};            // Current stroke [m], 0 = fully extended
    bool in_contact{false};             // Tyre touching the ground this step

    /// Contact point in body frame with the strut at its current extension.
    [[nodiscard]] math::Vector3 contact_point_b() const noexcept {
        return math::Vector3(pos_b.x, pos_b.y, pos_b.z + rest_length - compression);
    }
};

/**
 * @brief Three-point landing gear and ground-reaction model supporting multiple aircraft.
 *
 * Replaces a hard altitude clamp with real strut dynamics, so the aircraft can
 * be taxied, rotated, flown off, and landed. Produces body-frame forces and
 * moments that are summed with aerodynamics and thrust before integration.
 *
 * Sign conventions follow the rest of the codebase: NED position (altitude
 * h = -pos_ned.z), body X forward, Y right, Z down.
 *
 * Zero dynamic heap allocation.
 */
class LandingGear {
public:
    // Tyre-to-surface friction coefficients (dry concrete).
    static constexpr double MU_ROLLING  = 0.03; // Free-rolling resistance
    static constexpr double MU_BRAKING  = 0.55; // Maximum braking
    static constexpr double MU_LATERAL  = 0.65; // Cornering / side-scrub

    // Maximum nosewheel steering deflection at low speed. Authority is scaled
    // down as ground speed rises.
    static constexpr double MAX_STEER_RAD = 32.0 * (M_PI / 180.0);
    static constexpr double STEER_FULL_SPEED = 5.0;  // [m/s] full authority below this
    static constexpr double STEER_MIN_GAIN   = 0.05; // Floor on steering authority

    // Below this ground speed the aircraft is held against creep.
    static constexpr double CREEP_SPEED = 0.35; // [m/s]

    // Tyre cornering stiffness
    static constexpr double CORNERING_STIFFNESS = 4.0; // [1/rad]
    static constexpr double SLIP_SPEED_FLOOR = 1.0;    // [m/s] slip-angle guard

    // Structural limit: sink rate above this on touchdown breaks the gear.
    static constexpr double GEAR_LIMIT_SINK_MPS = 4.5;

    std::array<Strut, 3> struts{};   // [0] = nose, [1] = left main, [2] = right main
    bool deployed{true};             // Gear down and locked
    bool collapsed{false};           // Gear failed from overload

    double steer_cmd{0.0};           // Nosewheel steering [-1, +1]
    double brake_left{0.0};          // Left main brake [0, 1]
    double brake_right{0.0};         // Right main brake [0, 1]

    double max_sink_rate{0.0};       // Peak sink rate seen at touchdown [m/s]

    LandingGear() noexcept { reset(); }

    explicit LandingGear(aircraft::AircraftType type) noexcept { configure(type); }

    /**
     * @brief Configures landing gear geometry for a specified aircraft type.
     */
    void configure(aircraft::AircraftType type) noexcept {
        const auto cfg = aircraft::AircraftConfig::get(type).gear;

        // Nose gear
        struts[0] = Strut{
            cfg.nose_pos_b,
            cfg.rest_length,
            cfg.max_stroke * 0.91,
            cfg.nose_spring_k,
            cfg.nose_damping_c,
            true,
            false
        };

        // Left main gear
        struts[1] = Strut{
            cfg.main_l_pos_b,
            cfg.rest_length,
            cfg.max_stroke,
            cfg.main_spring_k,
            cfg.main_damping_c,
            false,
            true
        };

        // Right main gear
        struts[2] = Strut{
            cfg.main_r_pos_b,
            cfg.rest_length,
            cfg.max_stroke,
            cfg.main_spring_k,
            cfg.main_damping_c,
            false,
            true
        };

        for (auto& s : struts) {
            s.compression = 0.0;
            s.in_contact = false;
        }

        deployed = true;
        collapsed = false;
        steer_cmd = 0.0;
        brake_left = 0.0;
        brake_right = 0.0;
        max_sink_rate = 0.0;
    }

    /**
     * @brief Restores tricycle gear geometry (defaults to F-16C).
     */
    void reset(aircraft::AircraftType type = aircraft::AircraftType::F16_FIGHTING_FALCON) noexcept {
        configure(type);
    }

    /// True when any strut is currently loaded (weight-on-wheels).
    [[nodiscard]] bool weight_on_wheels() const noexcept {
        return struts[0].in_contact || struts[1].in_contact || struts[2].in_contact;
    }

    /// True when both main gear are loaded (a settled landing / taxi).
    [[nodiscard]] bool mains_on_ground() const noexcept {
        return struts[1].in_contact && struts[2].in_contact;
    }

    /**
     * @brief Computes ground-reaction forces and moments in the body frame.
     *
     * @param state       Current flight state.
     * @param mass        Mass properties (for static-load-aware damping).
     * @param ground_elev Ground plane elevation in NED down-coordinates (0 = MSL).
     * @return Body-frame force and moment from all struts combined.
     */
    [[nodiscard]] fdm::AircraftForces compute(
        const fdm::FlightState& state,
        const fdm::MassProperties& mass,
        double ground_elev = 0.0
    ) noexcept {
        fdm::AircraftForces out{};

        for (auto& s : struts) {
            s.in_contact = false;
            s.compression = 0.0;
        }

        if (!deployed || collapsed) {
            return out;
        }

        for (auto& s : struts) {
            // Contact point in body frame at full extension.
            const math::Vector3 cp_b = s.contact_point_b();

            // Its NED position: CG position plus the rotated offset.
            const math::Vector3 cp_ned_offset = state.q_att.rotate_body_to_ned(cp_b);
            const double cp_down = state.pos_ned.z + cp_ned_offset.z;

            // Penetration below the ground plane (positive when compressed).
            const double penetration = cp_down - ground_elev;
            if (penetration <= 0.0) {
                continue; // Wheel is clear of the surface.
            }

            s.in_contact = true;
            s.compression = std::min(penetration, s.max_stroke);

            // Velocity of this contact point: v_cp = v_cg + omega x r
            const math::Vector3 v_cp_b = state.vel_b + state.omega_b.cross(cp_b);
            const math::Vector3 v_cp_ned = state.q_att.rotate_body_to_ned(v_cp_b);

            // --- Normal (vertical) strut force -----------------------------
            const double sink = v_cp_ned.z; // + = descending
            double normal_n = s.spring_k * s.compression + s.damping_c * sink;

            // A strut can push but never pull the aircraft down.
            normal_n = std::max(0.0, normal_n);

            // Bottoming out: a fully compressed strut goes rigid.
            if (penetration > s.max_stroke) {
                const double over = penetration - s.max_stroke;
                normal_n += s.spring_k * 8.0 * over;
            }

            // Record touchdown severity and fail the gear if overloaded.
            if (sink > max_sink_rate) {
                max_sink_rate = sink;
            }
            if (sink > GEAR_LIMIT_SINK_MPS) {
                collapsed = true;
            }

            // Normal force acts upward in NED (negative Z).
            const math::Vector3 f_normal_ned(0.0, 0.0, -normal_n);

            // --- Tyre friction in the ground plane -------------------------
            // Ground-track velocity of the contact patch.
            const double vx = v_cp_ned.x;
            const double vy = v_cp_ned.y;

            // Heading of the wheel: aircraft yaw, plus steering on the nose gear.
            double wheel_hdg = state.yaw();
            if (s.steerable) {
                // Taper steering authority as 1/V^2 above taxi speed so the
                // commanded lateral acceleration stays within the rollover limit.
                const double gs = std::sqrt(vx * vx + vy * vy);
                double gain = 1.0;
                if (gs > STEER_FULL_SPEED) {
                    const double ratio = STEER_FULL_SPEED / gs;
                    gain = std::max(ratio * ratio, STEER_MIN_GAIN);
                }
                wheel_hdg += std::clamp(steer_cmd, -1.0, 1.0) * MAX_STEER_RAD * gain;
            }

            const double cos_h = std::cos(wheel_hdg);
            const double sin_h = std::sin(wheel_hdg);

            // Resolve ground velocity into rolling and lateral wheel axes.
            const double v_roll = vx * cos_h + vy * sin_h;
            const double v_side = -vx * sin_h + vy * cos_h;

            // Longitudinal: rolling resistance plus commanded braking.
            double mu_long = MU_ROLLING;
            if (s.braked) {
                const double brake = (s.pos_b.y < 0.0) ? brake_left : brake_right;
                mu_long += std::clamp(brake, 0.0, 1.0) * (MU_BRAKING - MU_ROLLING);
            }

            // Longitudinal friction opposes rolling motion; blend to zero
            // through the creep band so a stationary aircraft settles instead
            // of oscillating about zero.
            const double roll_scale = std::clamp(std::abs(v_roll) / CREEP_SPEED, 0.0, 1.0);
            const double f_roll = -std::copysign(mu_long * normal_n * roll_scale, v_roll);

            // Lateral force from tyre slip angle. Using a cornering-stiffness
            // model (force proportional to slip angle, saturating at the
            // friction limit) rather than pure Coulomb friction is what makes
            // ground handling stable: it provides progressive self-centring
            // damping instead of a full-magnitude force at any nonzero slip.
            const double slip_ref = std::max(std::abs(v_roll), SLIP_SPEED_FLOOR);
            const double slip_angle = std::atan2(v_side, slip_ref);

            const double f_side_linear = -CORNERING_STIFFNESS * slip_angle * normal_n;
            const double f_side_limit = MU_LATERAL * normal_n;
            const double f_side = std::clamp(f_side_linear, -f_side_limit, f_side_limit);

            // Back to NED axes.
            const double fx_ned = f_roll * cos_h - f_side * sin_h;
            const double fy_ned = f_roll * sin_h + f_side * cos_h;

            const math::Vector3 f_friction_ned(fx_ned, fy_ned, 0.0);

            // --- Accumulate in body frame ----------------------------------
            const math::Vector3 f_total_ned = f_normal_ned + f_friction_ned;
            const math::Vector3 f_total_b = state.q_att.rotate_ned_to_body(f_total_ned);

            out.force_b = out.force_b + f_total_b;
            out.moment_b = out.moment_b + cp_b.cross(f_total_b);
        }

        (void)mass; // Reserved for load-dependent strut tuning.
        return out;
    }
};

} // namespace gear
} // namespace fastjet
