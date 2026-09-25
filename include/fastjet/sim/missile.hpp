#pragma once

#include "fastjet/environment/atmosphere1976.hpp"
#include "fastjet/environment/ground_collision.hpp"
#include "fastjet/fdm/six_dof_fdm.hpp"
#include "fastjet/math/vector3.hpp"
#include "fastjet/sim/air_combat_geometry.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace fastjet::sim {

/**
 * @brief An active-radar, medium-range air-to-air missile.
 *
 * Figures are representative of the AIM-120C from open sources: a 157 kg
 * round on a 178 mm body with a ~5 s boost motor, Mach 3+ burnout
 * speed and a 40 G airframe. They are not exact.
 *
 * Aerodynamic parameters are referenced to the body cross-section. The
 * missile's lift (its turn capability) comes from dynamic pressure, so a missile
 * that has coasted slow cannot turn hard. That is how a missile is defeated
 * kinematically.
 */
struct MissileSpec {
    const char* name{"AIM-120C"};
    double launch_mass_kg{157.0};
    double burnout_mass_kg{111.0};
    double thrust_n{22000.0};       ///< Boost thrust (Isp ~245 s)
    double burn_time_s{5.0};
    double diameter_m{0.178};
    double k_induced{0.02};         ///< Induced drag: Cd_i = k CL^2
    double cl_max{12.0};            ///< Body + fin lift on the reference area
    double g_limit{40.0};           ///< Structural / autopilot limit
    double autopilot_tau_s{0.20};   ///< Lateral acceleration response lag
    double nav_gain{4.0};           ///< Proportional navigation constant
    // Seeker and guidance
    double seeker_range_m{14000.0};   ///< Active seeker acquisition range vs a fighter
    double seeker_gimbal_deg{50.0};
    double pitbull_range_m{11000.0};  ///< Goes active this far from the (estimated) target
    double seeker_basket_deg{8.0};    ///< Search basket around the predicted target
    double seeker_beam_deg{6.0};      ///< Beamwidth for chaff in the beam
    double notch_mps{28.0};           ///< Doppler clutter notch half-width
    double memory_s{1.2};             ///< Coast on the last track before searching again
    double chaff_seduction{0.45};     ///< Chance a bundle in the gates steals the track
    // Fuze, warhead and limits
    double arm_time_s{1.0};
    double fuze_radius_m{15.0};
    double lethal_radius_m{16.0};     ///< Fragments still do damage out to here
    double max_tof_s{75.0};
    double min_speed_mps{180.0};      ///< Below this, after burnout, the missile is spent
    // Launch envelope limits (the aircraft's launch constraints, not the flyout)
    double min_launch_range_m{900.0};
    double max_launch_ata_deg{45.0};

    [[nodiscard]] double ref_area() const noexcept { return 0.25 * M_PI * diameter_m * diameter_m; }

    /// @brief Zero-lift drag coefficient against Mach: a transonic rise, then a slow supersonic decay.
    [[nodiscard]] static double cd0(double mach) noexcept {
        struct P { double m, cd; };
        static constexpr P table[] = {{0.0, 0.30}, {0.8, 0.30}, {1.05, 0.58}, {1.3, 0.56},
                                      {2.0, 0.45}, {3.0, 0.36}, {4.0, 0.31}, {6.0, 0.28}};
        constexpr int n = static_cast<int>(sizeof(table) / sizeof(table[0]));
        if (mach <= table[0].m) return table[0].cd;
        for (int i = 1; i < n; ++i) {
            if (mach <= table[i].m) {
                const double t = (mach - table[i - 1].m) / (table[i].m - table[i - 1].m);
                return table[i - 1].cd + t * (table[i].cd - table[i - 1].cd);
            }
        }
        return table[n - 1].cd;
    }
};

/// @brief What a radar or seeker needs to hold a track, and why it can't.
enum class TrackCheck : uint8_t { OK, RANGE, GIMBAL, NOTCH };

[[nodiscard]] constexpr const char* to_string(TrackCheck c) noexcept {
    switch (c) {
        case TrackCheck::OK:     return "OK";
        case TrackCheck::RANGE:  return "RANGE";
        case TrackCheck::GIMBAL: return "GIMBAL";
        case TrackCheck::NOTCH:  return "NOTCH";
    }
    return "?";
}

/**
 * @brief Pulse-doppler tracking limits shared by fighter radars and missile seekers.
 *
 * A pulse-doppler set separates a target from ground clutter by its doppler
 * shift. Clutter from the ground behind the target sits at the doppler the
 * sensor's own motion gives stationary ground. The target stands out from it
 * only by its own velocity along the line of sight. A target flying at 90 deg
 * to the line of sight ("beaming") has almost no radial velocity and falls
 * into the clutter notch, but only when there is ground in the beam behind it
 * (look-down). Looking up against empty sky, a beaming target stays visible.
 */
struct SensorLimits {
    double max_range_m{18500.0};
    double gimbal_rad{60.0 * M_PI / 180.0};
    double notch_mps{25.0};
    double beam_half_rad{1.75 * M_PI / 180.0}; ///< Ground enters the beam this far above level

    [[nodiscard]] TrackCheck check(const math::Vector3& sensor_pos, const math::Vector3& boresight,
                                   const math::Vector3& tgt_pos, const math::Vector3& tgt_vel) const noexcept {
        const math::Vector3 r = tgt_pos - sensor_pos;
        const double range = r.norm();
        if (range > max_range_m) return TrackCheck::RANGE;
        if (range < 1.0) return TrackCheck::OK;
        if (AirCombatGeometry::angle_between(boresight, r) > gimbal_rad) return TrackCheck::GIMBAL;
        const math::Vector3 u = r / range;
        // NED: u.z > 0 is looking down. Ground is in the beam once the lower
        // edge of the beam points below the horizon.
        const bool clutter_behind = u.z > -std::sin(beam_half_rad);
        if (clutter_behind && std::abs(tgt_vel.dot(u)) < notch_mps) return TrackCheck::NOTCH;
        return TrackCheck::OK;
    }
};

/// @brief A chaff bundle: a cloud of dipoles that slows to the air mass almost at once.
struct ChaffCloud {
    static constexpr double BLOOM_S = 0.25;    ///< Time for the cloud to open to full size
    static constexpr double LIFE_S = 6.0;
    static constexpr double DECEL_TAU_S = 0.12; ///< Dipoles stop in the airflow almost at once

    math::Vector3 pos{};
    math::Vector3 vel{};
    double age{0.0};
    int owner{-1};
    bool active{false};
    bool bloomed{false}; ///< Its one chance to seduce a tracker has been rolled

    void step(double dt) noexcept {
        age += dt;
        vel = vel * std::exp(-dt / DECEL_TAU_S) + math::Vector3(0.0, 0.0, 2.0) * (1.0 - std::exp(-dt / DECEL_TAU_S));
        pos += vel * dt;
        if (age > LIFE_S) active = false;
    }
};

/**
 * @brief Is a chaff cloud inside a tracker's resolution cell around its target?
 *
 * A tracker holds its target in angle (the beam), range (the range gate) and
 * velocity (the doppler gate). A bundle that blooms inside all three
 * competes with the target for the track. Chaff stops dead in the air, so the
 * doppler gate rejects it unless the target has little radial velocity of its
 * own, which is why chaff works together with beaming and seldom alone.
 */
[[nodiscard]] inline bool chaff_in_gates(const math::Vector3& sensor_pos, const math::Vector3& tgt_pos,
                                         const math::Vector3& tgt_vel, const ChaffCloud& c,
                                         double beam_half_rad) noexcept {
    constexpr double RANGE_GATE_M = 150.0;
    constexpr double DOPPLER_GATE_MPS = 40.0;
    const math::Vector3 rt = tgt_pos - sensor_pos;
    const math::Vector3 rc = c.pos - sensor_pos;
    const double range_t = rt.norm();
    if (range_t < 1.0) return false;
    const math::Vector3 u = rt / range_t;
    if (AirCombatGeometry::angle_between(rt, rc) > beam_half_rad) return false;
    if (std::abs(rc.norm() - range_t) > RANGE_GATE_M) return false;
    return std::abs((tgt_vel - c.vel).dot(u)) < DOPPLER_GATE_MPS;
}

/**
 * @brief Point-mass missile flight: thrust, Mach-dependent drag, induced drag
 * from the lift it is pulling, gravity, and a first-order autopilot on the
 * lateral acceleration, limited by G and by available lift (dynamic pressure).
 *
 * The live missile and the launch-zone predictor both fly this function, so
 * the launch zone can't promise a shot that the missile can't make.
 */
struct MissileBody {
    math::Vector3 pos{};
    math::Vector3 vel{};
    math::Vector3 a_lat{}; ///< Achieved lateral acceleration [m/s^2, NED]
    double tof{0.0};

    [[nodiscard]] double mass(const MissileSpec& s) const noexcept {
        const double f = std::clamp(tof / s.burn_time_s, 0.0, 1.0);
        return s.launch_mass_kg - (s.launch_mass_kg - s.burnout_mass_kg) * f;
    }
    [[nodiscard]] bool motor_burning(const MissileSpec& s) const noexcept { return tof < s.burn_time_s; }
    [[nodiscard]] double speed() const noexcept { return vel.norm(); }

    /// @brief Lateral acceleration the airframe can make right now [m/s^2].
    [[nodiscard]] double max_lateral_accel(const MissileSpec& s) const noexcept {
        const auto air = environment::Atmosphere1976::compute(-pos.z, speed());
        const double lift_limit = s.cl_max * air.dynamic_pressure * s.ref_area() / mass(s);
        return std::min(s.g_limit * fdm::SixDoFFDM::GRAVITY_ACCEL, lift_limit);
    }

    /// @brief One step with commanded lateral acceleration @p a_cmd (NED; any
    /// component along the velocity is ignored).
    void step(const MissileSpec& s, double dt, const math::Vector3& a_cmd) noexcept {
        constexpr double G0 = fdm::SixDoFFDM::GRAVITY_ACCEL;
        const double V = std::max(1.0, speed());
        const math::Vector3 u = vel / V;
        const auto air = environment::Atmosphere1976::compute(-pos.z, V);
        const double qs = std::max(1.0, air.dynamic_pressure * s.ref_area());
        const double m = mass(s);
        const double a_max = std::min(s.g_limit * G0, s.cl_max * qs / m);

        auto lateral = [&](math::Vector3 a) {
            a = a - u * a.dot(u);
            const double n = a.norm();
            return n > a_max ? a * (a_max / n) : a;
        };
        const math::Vector3 cmd = lateral(a_cmd);
        a_lat = lateral(a_lat + (cmd - a_lat) * (1.0 - std::exp(-dt / s.autopilot_tau_s)));

        const double thrust = motor_burning(s) ? s.thrust_n : 0.0;
        const double lift = m * a_lat.norm();
        const double drag = qs * MissileSpec::cd0(air.mach_number) + s.k_induced * lift * lift / qs;
        const math::Vector3 acc = u * ((thrust - drag) / m) + a_lat + math::Vector3(0.0, 0.0, G0);
        vel += acc * dt;
        pos += vel * dt;
        tof += dt;
    }
};

/**
 * @brief Proportional navigation with gravity compensation.
 *
 * Commands a lateral acceleration of N * Vc times the rotation rate of the
 * line of sight. A target that holds a constant bearing is on a collision
 * course, and PN drives the line-of-sight rate to zero.
 */
[[nodiscard]] inline math::Vector3 pro_nav(const MissileBody& m, const math::Vector3& tgt_pos,
                                          const math::Vector3& tgt_vel, double nav_gain) noexcept {
    const math::Vector3 r = tgt_pos - m.pos;
    const double range = std::max(1.0, r.norm());
    const math::Vector3 u = r / range;
    const math::Vector3 v_rel = tgt_vel - m.vel;
    const math::Vector3 omega = r.cross(v_rel) / (range * range);
    const double vc = -r.dot(v_rel) / range;
    // Opening or barely closing: keep a floor so the missile still steers at the target.
    const double vc_eff = std::max(vc, 150.0);
    math::Vector3 a = omega.cross(u) * (nav_gain * vc_eff);
    // Hold up the missile's own weight so gravity does not become a standing guidance error.
    a += math::Vector3(0.0, 0.0, -fdm::SixDoFFDM::GRAVITY_ACCEL);
    return a;
}

/// @brief Time off the rail before the missile manoeuvres (clearing the launcher).
inline constexpr double RAIL_CLEAR_S = 0.4;

/// @brief The command that only holds the missile's weight up: straight flight.
[[nodiscard]] constexpr math::Vector3 hold_weight() noexcept {
    return {0.0, 0.0, -fdm::SixDoFFDM::GRAVITY_ACCEL};
}

/// @brief Closest approach of two points moving linearly across one step, in
/// the target's frame: @p s in [0, 1] is where along the step it happens.
[[nodiscard]] inline double closest_approach(const math::Vector3& m0, const math::Vector3& m1,
                                             const math::Vector3& t0, const math::Vector3& t1, double& s) noexcept {
    const math::Vector3 r0 = m0 - t0;
    const math::Vector3 d = (m1 - t1) - r0;
    const double dd = d.dot(d);
    s = dd > 1e-12 ? std::clamp(-r0.dot(d) / dd, 0.0, 1.0) : 0.0;
    return (r0 + d * s).norm();
}

/// @brief What the target is assumed to do in a launch-zone prediction.
enum class FlyoutTarget : uint8_t {
    STRAIGHT,  ///< Holds course and speed: the edge of the envelope (Rmax)
    TURN_COLD, ///< Turns away at 6 G and runs in full reheat: the no-escape zone (Rne)
};

struct FlyoutResult {
    bool hit{false};
    double time_s{0.0};
    double miss_m{1e9};
    double terminal_speed_mps{0.0};
};

/**
 * @brief Launch-zone prediction: fly the missile's own dynamics and
 * guidance against a modelled target.
 *
 * A "hit" also needs the missile to arrive with a speed margin over the
 * target. A missile that only just catches the target has no energy left to
 * follow a last-ditch turn.
 */
inline constexpr double RUN_ACCEL_MPS2 = 4.0;       ///< Reheat acceleration of a running fighter
inline constexpr double RUN_SPEED_GAIN_MPS = 180.0;  ///< How much faster it can get before the missile arrives
inline constexpr double RUN_DASH_MACH = 1.5;         ///< ...and no faster than a fighter's supersonic dash

[[nodiscard]] inline FlyoutResult predict_flyout(const MissileSpec& spec, const math::Vector3& launch_pos,
                                                 const math::Vector3& launch_vel, math::Vector3 tgt_pos,
                                                 math::Vector3 tgt_vel, FlyoutTarget model,
                                                 double dt = 0.02) noexcept {
    constexpr double G0 = fdm::SixDoFFDM::GRAVITY_ACCEL;
    MissileBody m;
    m.pos = launch_pos;
    m.vel = launch_vel;
    FlyoutResult out;
    const double v_tgt0 = std::max(50.0, tgt_vel.norm());
    const double a_sound = environment::Atmosphere1976::compute(-tgt_pos.z).speed_of_sound;
    const double v_run = std::max(v_tgt0, std::min(v_tgt0 + RUN_SPEED_GAIN_MPS, RUN_DASH_MACH * a_sound));
    double best = (tgt_pos - m.pos).norm();
    for (double t = 0.0; t < spec.max_tof_s; t += dt) {
        const math::Vector3 m0 = m.pos;
        const math::Vector3 t0 = tgt_pos;
        m.step(spec, dt, m.tof < RAIL_CLEAR_S ? hold_weight() : pro_nav(m, tgt_pos, tgt_vel, spec.nav_gain));

        if (model == FlyoutTarget::TURN_COLD && t > 1.0) {
            // Level turn to put the missile dead astern, then run, accelerating
            // as a fighter in reheat does toward its supersonic dash speed.
            math::Vector3 h{tgt_vel.x, tgt_vel.y, 0.0};
            const double v = std::min(h.norm() + RUN_ACCEL_MPS2 * dt, v_run);
            math::Vector3 away{tgt_pos.x - m.pos.x, tgt_pos.y - m.pos.y, 0.0};
            away = away.normalized();
            h = h.normalized();
            const double max_turn = 6.0 * G0 / std::max(100.0, v) * dt;
            const double ang = AirCombatGeometry::angle_between(h, away);
            if (ang > 1e-6) {
                const double f = std::min(1.0, max_turn / ang);
                h = (h * (1.0 - f) + away * f).normalized();
            }
            tgt_vel = h * v;
        }
        tgt_pos += tgt_vel * dt;

        double s = 0.0;
        const double d = closest_approach(m0, m.pos, t0, tgt_pos, s);
        best = std::min(best, d);
        if (m.tof > spec.arm_time_s && d < 0.5 * spec.fuze_radius_m && s < 1.0) {
            out.hit = m.speed() > 1.3 * tgt_vel.norm();
            out.time_s = m.tof;
            out.miss_m = d;
            out.terminal_speed_mps = m.speed();
            return out;
        }
        const bool spent = !m.motor_burning(spec) && m.speed() < spec.min_speed_mps;
        const bool opening = (tgt_pos - m.pos).dot(tgt_vel - m.vel) > 0.0 && m.tof > spec.burn_time_s;
        if (spent || opening || m.pos.z > 0.0) break;
    }
    out.miss_m = best;
    out.time_s = m.tof;
    out.terminal_speed_mps = m.speed();
    return out;
}

/**
 * @brief One missile in flight: body, guidance phase and fuze.
 *
 * Guidance phases, as flown by an active-radar missile:
 *  1. DATALINK: the launcher's radar track is uplinked while it holds lock.
 *  2. INERTIAL: lock lost, so the missile flies to where the target should be.
 *  3. ACTIVE_SEARCH: inside pitbull range its own seeker searches a basket
 *     around the predicted target.
 *  4. ACTIVE_TRACK: guides on its own seeker. Notching, chaff or the
 *     gimbal can break the track. It then coasts on memory and searches again.
 *  5. LOST: nothing in the basket and the target outside the seeker. It flies on unguided.
 */
struct Missile {
    enum class Phase : uint8_t { DATALINK, INERTIAL, ACTIVE_SEARCH, ACTIVE_TRACK, LOST };

    MissileSpec spec{};
    MissileBody body{};
    Phase phase{Phase::DATALINK};
    int shooter{-1};
    int target{-1};
    bool active{false};
    math::Vector3 est_pos{}; ///< Where guidance believes the target is
    math::Vector3 est_vel{};
    double memory_timer{0.0};
    double decoy_timer{0.0}; ///< Tracking a chaff cloud: blind to the target until it runs out

    [[nodiscard]] bool seeker_active() const noexcept {
        return phase == Phase::ACTIVE_SEARCH || phase == Phase::ACTIVE_TRACK;
    }
    [[nodiscard]] bool guided() const noexcept { return phase != Phase::LOST; }

    [[nodiscard]] SensorLimits seeker_limits() const noexcept {
        SensorLimits l;
        l.max_range_m = spec.seeker_range_m;
        l.gimbal_rad = spec.seeker_gimbal_deg * M_PI / 180.0;
        l.notch_mps = spec.notch_mps;
        l.beam_half_rad = 0.5 * spec.seeker_beam_deg * M_PI / 180.0;
        return l;
    }

    /**
     * @brief Advance guidance and flight one step.
     * @param uplink   The launcher's radar holds a live track this step.
     * @param tgt_pos  True target state (used only through the phase's sensor).
     */
    void update(double dt, bool uplink, const math::Vector3& tgt_pos, const math::Vector3& tgt_vel) noexcept {
        // Predicted target: extrapolate the last estimate.
        est_pos += est_vel * dt;

        const double est_range = (est_pos - body.pos).norm();
        const math::Vector3 boresight = body.vel.normalized();
        const SensorLimits lim = seeker_limits();
        const TrackCheck seen = lim.check(body.pos, boresight, tgt_pos, tgt_vel);

        switch (phase) {
            case Phase::DATALINK:
            case Phase::INERTIAL:
                if (uplink) {
                    phase = Phase::DATALINK;
                    est_pos = tgt_pos;
                    est_vel = tgt_vel;
                } else {
                    phase = Phase::INERTIAL;
                }
                if (est_range < spec.pitbull_range_m) {
                    phase = Phase::ACTIVE_SEARCH;
                    memory_timer = 0.0;
                }
                break;
            case Phase::ACTIVE_SEARCH: {
                if (decoy_timer > 0.0) {
                    // Following the chaff: the target is outside the gates.
                    decoy_timer -= dt;
                    break;
                }
                const bool in_basket = AirCombatGeometry::angle_between(est_pos - body.pos, tgt_pos - body.pos) <
                                       spec.seeker_basket_deg * M_PI / 180.0;
                if (seen == TrackCheck::OK && in_basket) {
                    phase = Phase::ACTIVE_TRACK;
                    est_pos = tgt_pos;
                    est_vel = tgt_vel;
                } else {
                    // Uplink still refines the basket while the launcher holds lock.
                    if (uplink) {
                        est_pos = tgt_pos;
                        est_vel = tgt_vel;
                    }
                    memory_timer += dt;
                    const bool basket_behind = AirCombatGeometry::angle_between(boresight, est_pos - body.pos) >
                                               lim.gimbal_rad;
                    if (basket_behind || memory_timer > 6.0) phase = Phase::LOST;
                }
                break;
            }
            case Phase::ACTIVE_TRACK:
                if (seen == TrackCheck::OK) {
                    est_pos = tgt_pos;
                    est_vel = tgt_vel;
                    memory_timer = 0.0;
                } else {
                    memory_timer += dt;
                    if (memory_timer > spec.memory_s) {
                        phase = Phase::ACTIVE_SEARCH;
                        memory_timer = 0.0;
                    }
                }
                break;
            case Phase::LOST: break;
        }

        // Guidance: PN on the estimate. A lost missile only holds up its own weight and flies straight.
        const math::Vector3 a_cmd = phase == Phase::LOST ? hold_weight() : pro_nav(body, est_pos, est_vel, spec.nav_gain);
        // The rail and first moments of flight: no manoeuvring until clear of the launcher.
        body.step(spec, dt, body.tof < RAIL_CLEAR_S ? hold_weight() : a_cmd);
    }

    /// @brief Chaff stole the track. The seeker follows the cloud until it
    /// resolves as a decoy, then has to search again around where the cloud was.
    void seduce(const ChaffCloud& c) noexcept {
        est_pos = c.pos;
        est_vel = c.vel;
        phase = Phase::ACTIVE_SEARCH;
        memory_timer = 0.0;
        decoy_timer = DECOY_TRACK_S;
    }

    static constexpr double DECOY_TRACK_S = 1.5;

    /// @brief Out of energy, time or sky.
    [[nodiscard]] bool expired() const noexcept {
        const bool spent = !body.motor_burning(spec) && body.speed() < spec.min_speed_mps;
        return spent || body.tof > spec.max_tof_s ||
               body.pos.z > environment::GroundCollision::get_ground_z(body.pos.x, body.pos.y);
    }
};

[[nodiscard]] constexpr const char* to_string(Missile::Phase p) noexcept {
    switch (p) {
        case Missile::Phase::DATALINK:      return "DATALINK";
        case Missile::Phase::INERTIAL:      return "INERTIAL";
        case Missile::Phase::ACTIVE_SEARCH: return "ACTIVE SEARCH";
        case Missile::Phase::ACTIVE_TRACK:  return "ACTIVE TRACK";
        case Missile::Phase::LOST:          return "LOST";
    }
    return "?";
}

} // namespace fastjet::sim
