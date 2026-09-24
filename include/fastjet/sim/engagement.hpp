#pragma once

#include "fastjet/sim/bfm_pilot.hpp"
#include "fastjet/sim/world.hpp"
#include <array>
#include <cmath>
#include <cstdint>

namespace fastjet::sim {

/// @brief How the two jets are set up when the fight starts.
enum class StartGeometry : uint8_t {
    HEAD_ON_MERGE,    ///< Neutral: 2 nm apart, nose to nose, offset for a left-to-left pass
    OFFENSIVE_PERCH,  ///< Player 3,000 ft behind the bandit, 30 deg off his tail, 1,000 ft high
    DEFENSIVE_PERCH,  ///< The same with the roles reversed
    GUNNERY_RANGE,    ///< Scripted, non-shooting target for gun and pursuit practice
};

/// @brief What a gunnery-range target flies.
enum class RangeProfile : uint8_t { STRAIGHT_AND_LEVEL, CONSTANT_TURN, WEAVE };

[[nodiscard]] constexpr const char* to_string(StartGeometry g) noexcept {
    switch (g) {
        case StartGeometry::HEAD_ON_MERGE:   return "HEAD-ON MERGE";
        case StartGeometry::OFFENSIVE_PERCH: return "OFFENSIVE PERCH";
        case StartGeometry::DEFENSIVE_PERCH: return "DEFENSIVE PERCH";
        case StartGeometry::GUNNERY_RANGE:   return "GUNNERY RANGE";
    }
    return "?";
}

struct EngagementSetup {
    aircraft::AircraftType own_type{aircraft::AircraftType::F16_FIGHTING_FALCON};
    aircraft::AircraftType bandit_type{aircraft::AircraftType::F16_FIGHTING_FALCON};
    AiSkill skill{AiSkill::VETERAN};
    StartGeometry geometry{StartGeometry::HEAD_ON_MERGE};
    RangeProfile range_profile{RangeProfile::CONSTANT_TURN};
    double altitude_m{5000.0};
    double speed_mps{250.0};
    double hard_deck_agl_m{1000.0};
    double time_limit_s{300.0};
    uint32_t seed{1};
    bool own_is_ai{false};    ///< AI flies the player's jet too (tests, attract mode)
};

enum class EngagementOutcome : uint8_t { IN_PROGRESS, WIN, LOSS, MUTUAL_KILL, TIMEOUT };

[[nodiscard]] constexpr const char* to_string(EngagementOutcome o) noexcept {
    switch (o) {
        case EngagementOutcome::IN_PROGRESS: return "FIGHT'S ON";
        case EngagementOutcome::WIN:         return "SPLASH ONE - VICTORY";
        case EngagementOutcome::LOSS:        return "YOU WERE SHOT DOWN";
        case EngagementOutcome::MUTUAL_KILL: return "MUTUAL KILL";
        case EngagementOutcome::TIMEOUT:     return "KNOCK IT OFF - TIME";
    }
    return "?";
}

/// @brief Tallies for the debrief.
struct EngagementStats {
    int rounds_fired{0};
    int hits_scored{0};
    int hits_taken{0};
    double time_above_7g_s{0.0};
    double peak_g{0.0};
    double min_agl_m{1e9};
    double duration_s{0.0};
};

/**
 * @brief Straight, turning or weaving target for the gunnery range.
 *
 * Holds altitude with a bank-angle schedule; flies through the same FLCS as
 * everyone else, so its turn is a real one.
 */
class RangeTargetPilot {
public:
    RangeProfile profile{RangeProfile::CONSTANT_TURN};
    double target_altitude_m{5000.0};
    double bank_deg{60.0};         ///< Constant-turn bank (2 G level turn)
    double weave_period_s{12.0};
    double target_speed_mps{230.0};

    [[nodiscard]] AircraftControls update(double dt, const Aircraft& self) noexcept {
        time_ += dt;
        AircraftControls out{};
        // Speed hold: a target that accelerates away through Mach 1 is no use for gunnery.
        const double v_err = target_speed_mps - self.state.airspeed();
        throttle_ = std::clamp(throttle_ + 0.01 * v_err * dt, 0.0, 1.0);
        out.throttle = std::clamp(throttle_ + 0.03 * v_err, 0.0, 1.0);
        const auto cfg = aircraft::AircraftConfig::get(self.type);
        double bank = 0.0;
        switch (profile) {
            case RangeProfile::STRAIGHT_AND_LEVEL: bank = 0.0; break;
            case RangeProfile::CONSTANT_TURN:      bank = bank_deg; break;
            case RangeProfile::WEAVE:
                bank = bank_deg * (std::sin(2.0 * M_PI * time_ / weave_period_s) >= 0.0 ? 1.0 : -1.0);
                break;
        }
        const double phi_des = bank * (M_PI / 180.0);
        const double phi = self.state.roll();
        out.stick.roll_stick = std::clamp(1.5 * (phi_des - phi) - 0.3 * self.state.omega_b.x, -1.0, 1.0);

        // Level-turn load factor plus an altitude / climb-rate hold.
        const double climb = -self.state.velocity_ned().z;
        const double alt_err = target_altitude_m - self.state.altitude();
        const double n_level = 1.0 / std::max(0.2, std::cos(phi));
        const double n_cmd = std::clamp(n_level + 0.002 * alt_err - 0.02 * climb, 0.0, 5.0);
        const bool fbw = cfg.flcs.law_type != aircraft::FLCSConfig::LawType::HYDRO_SAS_AUGMENTED;
        const double err = n_cmd - self.imu.Nz;
        trim_ = std::clamp(trim_ + (fbw ? 0.05 : 0.3) * err * dt, -0.4, 0.4);
        const double ff = fbw ? (n_cmd - 1.0) / std::max(1.0, cfg.flcs.max_g_positive - 1.0) : 0.05 * (n_cmd - 1.0);
        out.stick.pitch_stick = std::clamp(ff + trim_, -0.5, 1.0);
        return out;
    }

private:
    double time_{0.0};
    double trim_{0.0};
    double throttle_{0.7};
};

/**
 * @brief One dogfight: the world, the AI pilots, the rules and the score.
 *
 * Index 0 is always the player's jet. The engagement ends on a kill (or a
 * mutual kill), when anyone busts through the ground, or at the time limit.
 */
class Engagement {
public:
    static constexpr int OWN = 0;
    static constexpr int BANDIT = 1;

    World world{};
    EngagementSetup setup_data{};
    std::array<BfmPilot, World::kMaxAircraft> ai{};
    std::array<bool, World::kMaxAircraft> ai_controlled{};
    RangeTargetPilot range_pilot{};
    EngagementStats stats{};
    EngagementOutcome outcome{EngagementOutcome::IN_PROGRESS};
    double end_time{-1.0};

    /// @brief Start a new fight.
    void setup(const EngagementSetup& s) noexcept {
        setup_data = s;
        world.clear(s.seed);
        outcome = EngagementOutcome::IN_PROGRESS;
        stats = EngagementStats{};
        end_time = -1.0;
        events_read_ = 0;
        ai_controlled.fill(false);

        const double alt = s.altitude_m;
        const double v = s.speed_mps;
        fdm::FlightState own{}, bandit{};
        switch (s.geometry) {
            case StartGeometry::HEAD_ON_MERGE:
                own = make_state({-1852.0, -150.0, -alt}, 0.0, v);
                bandit = make_state({1852.0, 150.0, -alt}, M_PI, v);
                break;
            case StartGeometry::OFFENSIVE_PERCH:
            case StartGeometry::GUNNERY_RANGE:
                perch(own, bandit, alt, v);
                break;
            case StartGeometry::DEFENSIVE_PERCH:
                perch(bandit, own, alt, v);
                break;
        }

        world.add(s.own_type, own);
        world.add(s.bandit_type, bandit);
        for (int i = 0; i < world.count; ++i) {
            Aircraft& a = world.aircraft[static_cast<size_t>(i)];
            a.landing_gear.deployed = false;
            prespool(a, 0.9);
        }

        ai_controlled[OWN] = s.own_is_ai;
        ai_controlled[BANDIT] = s.geometry != StartGeometry::GUNNERY_RANGE;
        for (int i = 0; i < world.count; ++i) {
            ai[static_cast<size_t>(i)] = BfmPilot(s.skill, s.seed * 2654435761u + static_cast<uint32_t>(i) * 97u + 1u);
            ai[static_cast<size_t>(i)].hard_deck_agl_m = s.hard_deck_agl_m;
        }
        range_pilot = RangeTargetPilot{};
        range_pilot.profile = s.range_profile;
        range_pilot.target_altitude_m = alt;
        range_pilot.target_speed_mps = std::min(v, s.own_type == aircraft::AircraftType::A10_THUNDERBOLT ? 150.0 : 230.0);
    }

    [[nodiscard]] bool finished() const noexcept { return outcome != EngagementOutcome::IN_PROGRESS; }
    [[nodiscard]] Aircraft& own() noexcept { return world.aircraft[OWN]; }
    [[nodiscard]] const Aircraft& own() const noexcept { return world.aircraft[OWN]; }
    [[nodiscard]] const Aircraft& bandit() const noexcept { return world.aircraft[BANDIT]; }

    /// @brief One physics step. @p player drives the own jet unless it is AI-flown.
    void step(double dt, const AircraftControls& player) noexcept {
        std::array<AircraftControls, World::kMaxAircraft> controls{};
        controls[OWN] = player;
        for (int i = 0; i < world.count; ++i) {
            const auto idx = static_cast<size_t>(i);
            const Aircraft& self = world.aircraft[idx];
            const Aircraft& foe = world.aircraft[i == OWN ? BANDIT : OWN];
            if (ai_controlled[idx]) {
                controls[idx] = ai[idx].update(dt, self, foe);
            } else if (i == BANDIT && setup_data.geometry == StartGeometry::GUNNERY_RANGE) {
                controls[idx] = range_pilot.update(dt, self);
            }
        }
        // Once the fight is decided the survivors hold fire.
        if (finished()) {
            for (auto& c : controls) c.trigger = false;
        }

        world.step(dt, controls);
        tally(dt);
        judge();
    }

    /// @brief Seconds since the fight ended (0 while in progress).
    [[nodiscard]] double time_since_end() const noexcept {
        return end_time < 0.0 ? 0.0 : world.time - end_time;
    }

private:
    int events_read_{0};

    static fdm::FlightState make_state(const math::Vector3& pos, double heading, double v) noexcept {
        fdm::FlightState s{};
        s.pos_ned = pos;
        s.vel_b = math::Vector3(v, 0.0, 0.0);
        s.q_att = math::Quaternion::from_euler(0.0, 0.0, heading);
        return s;
    }

    /// @brief Attacker 3,000 ft (914 m) from the target, 30 deg off its tail,
    /// 1,000 ft (305 m) above; the target heading north, the attacker parallel.
    static void perch(fdm::FlightState& attacker, fdm::FlightState& target, double alt, double v) noexcept {
        constexpr double RANGE = 914.0;
        constexpr double OFF_TAIL = 30.0 * M_PI / 180.0;
        target = make_state({0.0, 0.0, -alt}, 0.0, v);
        const math::Vector3 offset{-RANGE * std::cos(OFF_TAIL), RANGE * std::sin(OFF_TAIL), -305.0};
        attacker = make_state(target.pos_ned + offset, 0.0, v);
    }

    /// @brief Bring the engine to its running setting before the fight, so
    /// neither jet starts with a spooled-down core. Fuel burnt is refilled.
    static void prespool(Aircraft& a, double lever) noexcept {
        const auto air = environment::Atmosphere1976::compute(a.state.altitude(), a.state.airspeed());
        for (int i = 0; i < 1600; ++i) (void)a.engine.update(lever, air, Aircraft::DT);
        a.engine.fuel_kg = aircraft::AircraftConfig::get(a.type).mass.internal_fuel_capacity_kg;
        a.mass = fdm::FuelSystem::compute(a.engine.fuel_kg, a.type);
    }

    void tally(double dt) noexcept {
        const Aircraft& me = world.aircraft[OWN];
        if (!finished()) {
            stats.duration_s += dt;
            stats.peak_g = std::max(stats.peak_g, me.imu.Nz);
            if (me.imu.Nz > 7.0) stats.time_above_7g_s += dt;
            stats.min_agl_m = std::min(stats.min_agl_m, environment::GroundCollision::get_agl(me.state));
        }
        stats.rounds_fired = me.gun.rounds_fired;
        for (; events_read_ < world.event_count(); ++events_read_) {
            const CombatEvent& e = world.event(events_read_);
            if (e.kind != CombatEvent::Kind::HIT) continue;
            if (e.shooter == OWN) ++stats.hits_scored;
            if (e.victim == OWN) ++stats.hits_taken;
        }
    }

    void judge() noexcept {
        if (finished()) return;
        const bool own_out = !world.aircraft[OWN].alive();
        const bool bandit_out = !world.aircraft[BANDIT].alive();
        if (own_out && bandit_out) outcome = EngagementOutcome::MUTUAL_KILL;
        else if (bandit_out) outcome = EngagementOutcome::WIN;
        else if (own_out) outcome = EngagementOutcome::LOSS;
        else if (world.time >= setup_data.time_limit_s) outcome = EngagementOutcome::TIMEOUT;
        if (finished()) end_time = world.time;
    }
};

} // namespace fastjet::sim
