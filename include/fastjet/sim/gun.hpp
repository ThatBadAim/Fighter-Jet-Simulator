#pragma once

#include "fastjet/aircraft/aircraft_type.hpp"
#include "fastjet/environment/atmosphere1976.hpp"
#include "fastjet/fdm/flight_state.hpp"
#include "fastjet/fdm/six_dof_fdm.hpp"
#include "fastjet/math/vector3.hpp"
#include "fastjet/sim/combat_specs.hpp"
#include <algorithm>
#include <cmath>

namespace fastjet::sim {

/// @brief Gun line in body axes (unit vector).
[[nodiscard]] inline math::Vector3 gun_boresight_b(const GunSpec& g) noexcept {
    const double e = g.boresight_elev_deg * (M_PI / 180.0);
    return {std::cos(e), 0.0, -std::sin(e)};
}

/**
 * @brief Gun state for one aircraft: ammunition, barrel spin and the fractional
 * round accumulator that keeps the rate exact at a 200 Hz step.
 *
 * Rotary guns take time to reach full rate; the rate ramps with barrel speed
 * while the trigger is held and the barrels coast down when it is released.
 */
class GunSystem {
public:
    GunSpec spec{};
    int ammo{0};
    double spin{0.0};        ///< Barrel speed as a fraction of full rate [0, 1]
    double accumulator{0.0}; ///< Fractional rounds carried between steps
    int rounds_fired{0};

    constexpr GunSystem() noexcept = default;

    void configure(aircraft::AircraftType type) noexcept {
        spec = gun_spec(type);
        ammo = spec.rounds;
        spin = 0.0;
        accumulator = 0.0;
        rounds_fired = 0;
    }

    [[nodiscard]] bool firing() const noexcept { return trigger_ && ammo > 0; }

    /// @brief Advance one step. Returns the number of rounds leaving the muzzle.
    int update(double dt, bool trigger) noexcept {
        trigger_ = trigger;
        const double spin_rate = 1.0 / std::max(1e-3, spec.spin_up_s);
        if (trigger && ammo > 0) {
            spin = std::min(1.0, spin + spin_rate * dt);
        } else {
            spin = std::max(0.0, spin - 0.5 * spin_rate * dt);
            accumulator = 0.0;
            return 0;
        }
        accumulator += spin * spec.rate_rpm / 60.0 * dt;
        const int n = std::min(ammo, static_cast<int>(accumulator));
        accumulator -= n;
        ammo -= n;
        rounds_fired += n;
        return n;
    }

    [[nodiscard]] math::Vector3 boresight_b() const noexcept { return gun_boresight_b(spec); }

private:
    bool trigger_{false};
};

/// @brief One round in flight.
struct Projectile {
    math::Vector3 pos{};     ///< NED [m]
    math::Vector3 vel{};     ///< NED [m/s]
    double k_drag{0.0};      ///< 0.5 * Cd * A / m at sea-level density [1/m per (kg/m^3)]
    double lethality{1.0};
    double age{0.0};
    int owner{-1};
    bool active{false};
};

/**
 * @brief Exterior ballistics shared by live rounds and every sight or AI that
 * predicts them, so a pipper can never promise a hit the rounds do not deliver.
 *
 * Point-mass model: gravity plus quadratic drag in the local standard
 * atmosphere, integrated with a midpoint (RK2) step.
 */
struct Ballistics {
    static constexpr double MAX_TIME_OF_FLIGHT_S = 3.0; ///< Rounds self-destruct / become harmless
    static constexpr double G = fdm::SixDoFFDM::GRAVITY_ACCEL;

    [[nodiscard]] static double drag_factor(const GunSpec& g) noexcept {
        const double area = 0.25 * M_PI * g.calibre_m * g.calibre_m;
        return 0.5 * g.drag_coeff * area / g.projectile_mass_kg;
    }

    [[nodiscard]] static math::Vector3 accel(const math::Vector3& vel, double k_rho) noexcept {
        return vel * (-k_rho * vel.norm()) + math::Vector3(0.0, 0.0, G);
    }

    /// @brief One midpoint step. Density is taken once per step: a round climbs
    /// or falls a few metres in 5 ms, far too little to change it.
    static void step(math::Vector3& pos, math::Vector3& vel, double k_drag, double dt) noexcept {
        const double k_rho = k_drag * environment::Atmosphere1976::compute(-pos.z).density;
        const math::Vector3 a1 = accel(vel, k_rho);
        const math::Vector3 v_mid = vel + a1 * (0.5 * dt);
        const math::Vector3 a2 = accel(v_mid, k_rho);
        pos += v_mid * dt;
        vel += a2 * dt;
    }

    /// @brief Muzzle position and launch velocity for a round fired now along @p dir_b.
    static void launch(const fdm::FlightState& shooter, const GunSpec& g, const math::Vector3& dir_b,
                       math::Vector3& pos, math::Vector3& vel) noexcept {
        pos = shooter.pos_ned + shooter.q_att.rotate_body_to_ned(g.muzzle_b);
        // Muzzle velocity adds to the gun's own velocity, including the part due to the jet's rotation.
        const math::Vector3 v_muzzle_b = shooter.vel_b + shooter.omega_b.cross(g.muzzle_b);
        vel = shooter.q_att.rotate_body_to_ned(v_muzzle_b + dir_b * g.muzzle_velocity_mps);
    }

    /// @brief Where a round fired now along @p dir_b will be after @p t seconds,
    /// stepped with exactly the integrator the live rounds use.
    [[nodiscard]] static math::Vector3 predict(const fdm::FlightState& shooter, const GunSpec& g,
                                               const math::Vector3& dir_b, double t,
                                               double dt = 0.005) noexcept {
        math::Vector3 pos, vel;
        launch(shooter, g, dir_b, pos, vel);
        const double k = drag_factor(g);
        const int n = static_cast<int>(t / dt + 0.5);
        for (int i = 0; i < n; ++i) step(pos, vel, k, dt);
        return pos;
    }
};

/**
 * @brief Director gunsight solution against a known target (radar or AI track).
 *
 * The round's flight is simulated with the live ballistics until it reaches
 * the target's range; the target is extrapolated over the same time of flight
 * with its current velocity and acceleration. The miss vector between the two
 * is what the pipper shows: put the pipper on the target and the miss is zero.
 */
struct GunSolution {
    bool valid{false};
    double time_of_flight{0.0};
    double range{0.0};
    math::Vector3 miss_ned{};       ///< Target minus round at the time of flight [m]
    math::Vector3 pipper_ned{};     ///< World point the pipper overlays (target position minus miss)
    math::Vector3 lead_dir_b{};     ///< Gun line that would zero the miss (body axes, unit)
    double miss_distance{1e9};

    static constexpr double MAX_RANGE_M = 2500.0;

    [[nodiscard]] static GunSolution compute(const fdm::FlightState& shooter, const GunSpec& g,
                                             const math::Vector3& target_pos, const math::Vector3& target_vel,
                                             const math::Vector3& target_acc, double dt = 0.005) noexcept {
        GunSolution sol;
        const math::Vector3 rel = target_pos - shooter.pos_ned;
        sol.range = rel.norm();
        if (sol.range > MAX_RANGE_M || sol.range < 1.0) return sol;

        const math::Vector3 dir_b = gun_boresight_b(g);
        math::Vector3 pos, vel;
        Ballistics::launch(shooter, g, dir_b, pos, vel);
        const double k = Ballistics::drag_factor(g);
        const math::Vector3 los = rel / sol.range;

        // Step the round until it has travelled past the target along the
        // line of sight (target moving with its own velocity meanwhile).
        double t = 0.0;
        math::Vector3 prev_pos = pos;
        double prev_gap = (target_pos - pos).dot(los);
        while (t < Ballistics::MAX_TIME_OF_FLIGHT_S) {
            prev_pos = pos;
            Ballistics::step(pos, vel, k, dt);
            t += dt;
            const math::Vector3 tgt = target_pos + target_vel * t + target_acc * (0.5 * t * t);
            const double gap = (tgt - pos).dot(los);
            if (gap <= 0.0) {
                // Interpolate to the crossing inside this step.
                const double f = prev_gap / std::max(1e-9, prev_gap - gap);
                t -= dt * (1.0 - f);
                pos = prev_pos + (pos - prev_pos) * f;
                break;
            }
            prev_gap = gap;
        }
        if (t >= Ballistics::MAX_TIME_OF_FLIGHT_S) return sol;

        const math::Vector3 tgt = target_pos + target_vel * t + target_acc * (0.5 * t * t);
        sol.valid = true;
        sol.time_of_flight = t;
        sol.miss_ned = tgt - pos;
        sol.miss_distance = sol.miss_ned.norm();
        sol.pipper_ned = target_pos - sol.miss_ned;

        // Gun line that zeroes the miss (small-angle correction of the boresight).
        const math::Vector3 bore_ned = shooter.q_att.rotate_body_to_ned(dir_b);
        const double travel = std::max(1.0, (pos - shooter.pos_ned).norm());
        const math::Vector3 lead_ned = (bore_ned * travel + sol.miss_ned).normalized();
        sol.lead_dir_b = shooter.q_att.rotate_ned_to_body(lead_ned);
        return sol;
    }
};

} // namespace fastjet::sim
