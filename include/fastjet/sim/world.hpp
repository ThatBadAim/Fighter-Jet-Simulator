#pragma once

#include "fastjet/environment/ground_collision.hpp"
#include "fastjet/sim/air_combat_geometry.hpp"
#include "fastjet/sim/aircraft.hpp"
#include "fastjet/sim/combat_specs.hpp"
#include "fastjet/sim/damage_model.hpp"
#include "fastjet/sim/gun.hpp"
#include <array>
#include <cmath>
#include <cstdint>

namespace fastjet::sim {

/// @brief Something that happened in the fight, for scoring, HUD cues and the debrief.
struct CombatEvent {
    enum class Kind : uint8_t { HIT, KILL, GROUND_IMPACT, MIDAIR };
    Kind kind{Kind::HIT};
    int victim{-1};
    int shooter{-1};   ///< Credited aircraft (-1 if none)
    HitZone zone{HitZone::FUSELAGE};
    double time{0.0};
};

/**
 * @brief Every jet, round and event in one sky, stepped deterministically.
 *
 * Fixed capacity and no heap use in step(). Order within a step is fixed:
 * aircraft (by index) -> guns -> rounds and hit tests -> mid-airs -> ground
 * impacts, and all randomness (dispersion, damage rolls) comes from one
 * seeded generator, so an engagement replays exactly from its inputs.
 */
class World {
public:
    static constexpr int kMaxAircraft = 8;
    static constexpr int kMaxRounds = 4096;
    static constexpr int kMaxEvents = 256;
    /// A jet that hits the ground this long after being shot is credited to the shooter.
    static constexpr double KILL_CREDIT_WINDOW_S = 30.0;

    std::array<Aircraft, kMaxAircraft> aircraft{};
    std::array<Projectile, kMaxRounds> rounds{};
    int count{0};
    double time{0.0};
    bool dispersion{true};

    World() noexcept = default;

    void clear(uint32_t seed = 1) noexcept {
        count = 0;
        time = 0.0;
        for (auto& r : rounds) r.active = false;
        next_round_ = 0;
        event_count_ = 0;
        rng_.state = seed ? seed : 1u;
        last_hit_by_.fill(-1);
        last_hit_time_.fill(-1e9);
        was_crashed_.fill(false);
        was_alive_.fill(true);
    }

    /// @brief Add a jet at @p start. Returns its index, or -1 when full.
    int add(aircraft::AircraftType type, const fdm::FlightState& start) noexcept {
        if (count >= kMaxAircraft) return -1;
        const int i = count++;
        Aircraft& a = aircraft[static_cast<size_t>(i)];
        if (a.type != type) a.configure(type);
        a.reset(start); // full tanks, systems reset, undamaged
        // The first jet keeps the default gust stream (identical to the
        // single-aircraft viewer); the others get their own.
        a.seed_turbulence(i == 0 ? 0u : 0x2545F491u * static_cast<uint32_t>(i + 1));
        was_crashed_[static_cast<size_t>(i)] = false;
        was_alive_[static_cast<size_t>(i)] = true;
        return i;
    }

    /// @brief One fixed step of the whole world.
    void step(double dt, const std::array<AircraftControls, kMaxAircraft>& controls) noexcept {
        for (int i = 0; i < count; ++i) {
            aircraft[static_cast<size_t>(i)].step(dt, controls[static_cast<size_t>(i)]);
        }
        time += dt;
        fire_guns(dt, controls);
        step_rounds(dt);
        check_midairs();
        check_ground_impacts();
        check_mission_kills();
    }

    // ---------------------------------------------------------------------
    // Events
    // ---------------------------------------------------------------------

    [[nodiscard]] int event_count() const noexcept { return event_count_; }
    [[nodiscard]] const CombatEvent& event(int i) const noexcept { return events_[static_cast<size_t>(i)]; }
    /// @brief Forget consumed events (the consumer keeps its own read cursor otherwise).
    void clear_events() noexcept { event_count_ = 0; }

    [[nodiscard]] int active_rounds() const noexcept {
        int n = 0;
        for (const auto& r : rounds) n += r.active ? 1 : 0;
        return n;
    }

    // ---------------------------------------------------------------------
    // Hit geometry (public for tests)
    // ---------------------------------------------------------------------

    /// @brief Test a round's swept path against one airframe.
    ///
    /// The sweep is done in the target's frame (round displacement minus the
    /// target's), so a head-on pass at 1,400 m/s closure, 7 m per step, cannot
    /// tunnel through the jet. Returns true with the hit point in body axes.
    [[nodiscard]] static bool sweep_hit(const math::Vector3& round_p0, const math::Vector3& round_p1,
                                        const fdm::FlightState& target_prev, const fdm::FlightState& target,
                                        const AirframeCombatSpec& spec, math::Vector3& hit_b) noexcept {
        const math::Vector3 r0 = target.q_att.rotate_ned_to_body(round_p0 - target_prev.pos_ned);
        const math::Vector3 r1 = target.q_att.rotate_ned_to_body(round_p1 - target.pos_ned);

        // Broad phase: bounding sphere against the swept segment.
        const double bound = 0.5 * std::max(spec.length_m, spec.span_m) + spec.fuselage_radius_m;
        double s = 0.0, t = 0.0;
        const math::Vector3 o{0.0, 0.0, 0.0};
        if (segment_segment_distance(r0, r1, o, o, s, t) > bound) return false;

        const double half_l = 0.5 * spec.length_m - spec.fuselage_radius_m;
        const math::Vector3 fus_a{half_l, 0.0, 0.0};
        const math::Vector3 fus_b{-half_l, 0.0, 0.0};
        const double wing_x = -0.05 * spec.length_m;
        const double half_b = 0.5 * spec.span_m - spec.wing_radius_m;
        const math::Vector3 wing_a{wing_x, -half_b, 0.0};
        const math::Vector3 wing_b{wing_x, half_b, 0.0};

        double best_s = 2.0;
        double sf = 0.0, tf = 0.0;
        if (segment_segment_distance(r0, r1, fus_a, fus_b, sf, tf) <= spec.fuselage_radius_m) best_s = sf;
        double sw = 0.0, tw = 0.0;
        if (segment_segment_distance(r0, r1, wing_a, wing_b, sw, tw) <= spec.wing_radius_m && sw < best_s) {
            best_s = sw;
        }
        if (best_s > 1.0) return false;
        hit_b = r0 + (r1 - r0) * best_s;
        return true;
    }

private:
    std::array<CombatEvent, kMaxEvents> events_{};
    int event_count_{0};
    int next_round_{0};
    DamageRng rng_{};
    std::array<int, kMaxAircraft> last_hit_by_{};
    std::array<double, kMaxAircraft> last_hit_time_{};
    std::array<bool, kMaxAircraft> was_crashed_{};
    std::array<bool, kMaxAircraft> was_alive_{};

    void push(CombatEvent e) noexcept {
        if (event_count_ < kMaxEvents) events_[static_cast<size_t>(event_count_++)] = e;
    }

    double gaussian() noexcept {
        const double u1 = std::max(1e-12, rng_.uniform());
        const double u2 = rng_.uniform();
        return std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * M_PI * u2);
    }

    void fire_guns(double dt, const std::array<AircraftControls, kMaxAircraft>& controls) noexcept {
        for (int i = 0; i < count; ++i) {
            Aircraft& a = aircraft[static_cast<size_t>(i)];
            const bool trigger = controls[static_cast<size_t>(i)].trigger && a.alive() && !a.crashed;
            const int n = a.gun.update(dt, trigger);
            if (n <= 0) continue;
            const double k = Ballistics::drag_factor(a.gun.spec);
            const double sigma = dispersion ? a.gun.spec.dispersion_mrad * 1e-3 : 0.0;
            for (int r = 0; r < n; ++r) {
                math::Vector3 dir = a.gun.boresight_b();
                if (sigma > 0.0) {
                    dir = math::Vector3(dir.x, dir.y + sigma * gaussian(), dir.z + sigma * gaussian()).normalized();
                }
                Projectile& p = rounds[static_cast<size_t>(next_round_)];
                next_round_ = (next_round_ + 1) % kMaxRounds;
                Ballistics::launch(a.state, a.gun.spec, dir, p.pos, p.vel);
                // Rounds leave evenly through the step rather than in a clump.
                const double lead = dt * (static_cast<double>(r) + 0.5) / static_cast<double>(n);
                p.pos += p.vel * (dt * 0.5 - lead);
                p.k_drag = k;
                p.lethality = a.gun.spec.lethality;
                p.age = 0.0;
                p.owner = i;
                p.active = true;
            }
        }
    }

    void step_rounds(double dt) noexcept {
        for (auto& p : rounds) {
            if (!p.active) continue;
            const math::Vector3 p0 = p.pos;
            Ballistics::step(p.pos, p.vel, p.k_drag, dt);
            p.age += dt;
            if (p.age > Ballistics::MAX_TIME_OF_FLIGHT_S ||
                p.pos.z > environment::GroundCollision::get_ground_z(p.pos.x, p.pos.y)) {
                p.active = false;
                continue;
            }
            for (int j = 0; j < count; ++j) {
                if (j == p.owner) continue;
                Aircraft& t = aircraft[static_cast<size_t>(j)];
                if (t.crashed) continue;
                math::Vector3 hit_b;
                if (!sweep_hit(p0, p.pos, t.prev_state, t.state, airframe_combat_spec(t.type), hit_b)) continue;

                const HitZone zone = classify_hit(hit_b, airframe_combat_spec(t.type));
                t.damage.apply_hit(zone, p.lethality, rng_);
                last_hit_by_[static_cast<size_t>(j)] = p.owner;
                last_hit_time_[static_cast<size_t>(j)] = time;
                push({CombatEvent::Kind::HIT, j, p.owner, zone, time});
                p.active = false;
                break;
            }
        }
    }

    void check_midairs() noexcept {
        for (int i = 0; i < count; ++i) {
            for (int j = i + 1; j < count; ++j) {
                Aircraft& a = aircraft[static_cast<size_t>(i)];
                Aircraft& b = aircraft[static_cast<size_t>(j)];
                if (a.crashed || b.crashed || a.damage.destroyed || b.damage.destroyed) continue;
                const auto sa = airframe_combat_spec(a.type);
                const auto sb = airframe_combat_spec(b.type);
                // Bounding capsules along each fuselage, swept over the step in a's frame of reference.
                const double reach = 0.5 * (std::max(sa.length_m, sa.span_m) + std::max(sb.length_m, sb.span_m));
                if ((a.state.pos_ned - b.state.pos_ned).norm() > reach + 20.0) continue;
                const math::Vector3 ax = a.state.q_att.rotate_body_to_ned(math::Vector3(0.45 * sa.length_m, 0, 0));
                const math::Vector3 bx = b.state.q_att.rotate_body_to_ned(math::Vector3(0.45 * sb.length_m, 0, 0));
                double s = 0.0, t = 0.0;
                const double d = segment_segment_distance(a.state.pos_ned - ax, a.state.pos_ned + ax,
                                                          b.state.pos_ned - bx, b.state.pos_ned + bx, s, t);
                // Fuselages plus a share of the wings: jets whose axes pass this
                // close are overlapping somewhere unless perfectly stacked.
                const double contact = sa.fuselage_radius_m + sb.fuselage_radius_m + 0.25 * (sa.span_m + sb.span_m);
                if (d < contact) {
                    a.damage.destroyed = true;
                    b.damage.destroyed = true;
                    push({CombatEvent::Kind::MIDAIR, i, j, HitZone::FUSELAGE, time});
                    push({CombatEvent::Kind::MIDAIR, j, i, HitZone::FUSELAGE, time});
                    was_alive_[static_cast<size_t>(i)] = false;
                    was_alive_[static_cast<size_t>(j)] = false;
                }
            }
        }
    }

    [[nodiscard]] int credited_shooter(int victim) const noexcept {
        const auto v = static_cast<size_t>(victim);
        return (time - last_hit_time_[v] <= KILL_CREDIT_WINDOW_S) ? last_hit_by_[v] : -1;
    }

    void check_ground_impacts() noexcept {
        for (int i = 0; i < count; ++i) {
            const auto idx = static_cast<size_t>(i);
            const Aircraft& a = aircraft[idx];
            if (a.crashed && !was_crashed_[idx]) {
                was_crashed_[idx] = true;
                push({CombatEvent::Kind::GROUND_IMPACT, i, credited_shooter(i), HitZone::FUSELAGE, time});
                if (was_alive_[idx]) {
                    was_alive_[idx] = false;
                    push({CombatEvent::Kind::KILL, i, credited_shooter(i), HitZone::FUSELAGE, time});
                }
            }
        }
    }

    /// @brief A jet shot out of the fight (destroyed or pilot down) is a kill the
    /// moment it happens, not when the wreck reaches the ground.
    void check_mission_kills() noexcept {
        for (int i = 0; i < count; ++i) {
            const auto idx = static_cast<size_t>(i);
            if (was_alive_[idx] && !aircraft[idx].alive()) {
                was_alive_[idx] = false;
                if (!aircraft[idx].crashed) {
                    push({CombatEvent::Kind::KILL, i, credited_shooter(i), HitZone::FUSELAGE, time});
                }
            }
        }
    }
};

} // namespace fastjet::sim
