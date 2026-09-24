/**
 * @file test_bfm_ai.cpp
 * @brief Closed-loop behaviour of the BFM AI through the full engagement:
 * gunnery against scripted targets on every airframe, AI-vs-AI safety (no
 * terrain impacts, no mid-airs, every fight ends), skill ordering, bounded
 * G, and bit-exact replay from a seed.
 *
 * Fights are chaotic, so the assertions are on properties that must hold for
 * every seed (safety, termination, determinism) or on aggregate outcomes with
 * wide margins (skill ordering).
 */
#include "fastjet/sim/engagement.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>

using namespace fastjet;
using aircraft::AircraftType;

namespace {

constexpr double DT = 0.005;

constexpr AircraftType ALL_AIRCRAFT[] = {
    AircraftType::F16_FIGHTING_FALCON, AircraftType::F15EX_EAGLE_II, AircraftType::EUROFIGHTER_TYPHOON,
    AircraftType::F22_RAPTOR, AircraftType::A10_THUNDERBOLT};

struct FightResult {
    sim::EngagementOutcome outcome{};
    double time{0.0};
    int unforced_ground_impacts{0};
    int midairs{0};
    double peak_nz[2]{0.0, 0.0};
    int hits_scored{0};
};

/// Fly a whole engagement with the AI in both cockpits.
FightResult fly(sim::EngagementSetup s, sim::AiSkill own_skill, double time_limit = 180.0) {
    s.own_is_ai = true;
    s.time_limit_s = time_limit;
    auto e = std::make_unique<sim::Engagement>();
    e->setup(s);
    e->ai[0] = sim::BfmPilot(own_skill, s.seed * 31u + 5u);
    e->ai[0].hard_deck_agl_m = s.hard_deck_agl_m;
    FightResult r;
    const sim::AircraftControls none{};
    while (!e->finished()) {
        e->step(DT, none);
        for (int i = 0; i < 2; ++i) {
            const auto& a = e->world.aircraft[static_cast<size_t>(i)];
            if (a.alive()) r.peak_nz[i] = std::max(r.peak_nz[i], a.imu.Nz);
        }
    }
    r.outcome = e->outcome;
    r.time = e->world.time;
    r.hits_scored = e->stats.hits_scored;
    for (int i = 0; i < 2; ++i) {
        const auto& a = e->world.aircraft[static_cast<size_t>(i)];
        if (a.crashed && a.damage.hits_taken == 0) ++r.unforced_ground_impacts;
    }
    for (int k = 0; k < e->world.event_count(); ++k) {
        if (e->world.event(k).kind == sim::CombatEvent::Kind::MIDAIR) ++r.midairs;
    }
    return r;
}

void test_gunnery_range_every_airframe() {
    std::cout << "[BFM] Gunnery range: a veteran AI kills straight and turning targets in every airframe...\n";
    for (AircraftType t : ALL_AIRCRAFT) {
        for (auto profile : {sim::RangeProfile::STRAIGHT_AND_LEVEL, sim::RangeProfile::CONSTANT_TURN}) {
            sim::EngagementSetup s;
            s.geometry = sim::StartGeometry::GUNNERY_RANGE;
            s.range_profile = profile;
            s.own_type = s.bandit_type = t;
            s.skill = sim::AiSkill::VETERAN;
            s.seed = 3;
            if (t == AircraftType::A10_THUNDERBOLT) s.speed_mps = 170.0;
            const FightResult r = fly(s, sim::AiSkill::VETERAN, 120.0);
            std::cout << "    " << aircraft::to_short_string(t)
                      << (profile == sim::RangeProfile::STRAIGHT_AND_LEVEL ? " straight: " : " 2 G turn: ")
                      << sim::to_string(r.outcome) << " at " << r.time << " s (" << r.hits_scored << " hits)\n";
            assert(r.outcome == sim::EngagementOutcome::WIN);
            assert(r.unforced_ground_impacts == 0 && r.midairs == 0);
        }
    }
    std::cout << "  -> PASSED\n";
}

void test_ai_vs_ai_is_safe_and_terminates() {
    std::cout << "[BFM] AI vs AI from every set-up: no terrain impacts, no mid-airs, bounded G...\n";
    int fights = 0, decided = 0;
    for (auto geo : {sim::StartGeometry::HEAD_ON_MERGE, sim::StartGeometry::OFFENSIVE_PERCH,
                     sim::StartGeometry::DEFENSIVE_PERCH}) {
        for (uint32_t seed = 1; seed <= 4; ++seed) {
            for (auto pair : {std::pair{sim::AiSkill::VETERAN, sim::AiSkill::VETERAN},
                              std::pair{sim::AiSkill::ACE, sim::AiSkill::NOVICE}}) {
                sim::EngagementSetup s;
                s.geometry = geo;
                s.skill = pair.second;
                s.seed = seed;
                const FightResult r = fly(s, pair.first);
                ++fights;
                decided += r.outcome != sim::EngagementOutcome::TIMEOUT;
                const double g_limit = aircraft::AircraftConfig::get(s.own_type).flcs.max_g_positive;
                if (r.unforced_ground_impacts || r.midairs || r.peak_nz[0] > g_limit + 2.5 || r.peak_nz[1] > g_limit + 2.5) {
                    std::cerr << "    " << sim::to_string(geo) << " seed " << seed << ": impacts "
                              << r.unforced_ground_impacts << " midairs " << r.midairs << " peak G " << r.peak_nz[0]
                              << " / " << r.peak_nz[1] << "\n";
                }
                assert(r.unforced_ground_impacts == 0);
                assert(r.midairs == 0);
                assert(r.time <= 180.0 + DT);
                // FBW transient overshoot above the limit is small; the AI never asks past it.
                assert(r.peak_nz[0] <= g_limit + 2.5 && r.peak_nz[1] <= g_limit + 2.5);
            }
        }
    }
    std::cout << "    " << fights << " fights, " << decided << " decided before the time limit\n";
    std::cout << "  -> PASSED\n";
}

void test_skill_ordering() {
    std::cout << "[BFM] Skill tiers: an ace converts the perch on a novice; a novice never beats an ace...\n";
    int ace_wins = 0, ace_losses = 0, novice_wins = 0;
    for (uint32_t seed = 1; seed <= 12; ++seed) {
        sim::EngagementSetup s;
        s.geometry = sim::StartGeometry::OFFENSIVE_PERCH;
        s.seed = seed;
        s.skill = sim::AiSkill::NOVICE;
        const FightResult a = fly(s, sim::AiSkill::ACE);
        ace_wins += a.outcome == sim::EngagementOutcome::WIN;
        ace_losses += a.outcome == sim::EngagementOutcome::LOSS || a.outcome == sim::EngagementOutcome::MUTUAL_KILL;
        s.skill = sim::AiSkill::ACE;
        const FightResult b = fly(s, sim::AiSkill::NOVICE);
        novice_wins += b.outcome == sim::EngagementOutcome::WIN;
    }
    std::cout << "    ace attacking novice: " << ace_wins << " wins, " << ace_losses << " losses of 12; "
              << "novice attacking ace: " << novice_wins << " wins of 12\n";
    assert(ace_wins >= 2);
    assert(ace_losses == 0);
    assert(novice_wins == 0);
    std::cout << "  -> PASSED\n";
}

void test_engagement_replays_bit_exactly() {
    std::cout << "[BFM] Same seed, same inputs -> bit-identical engagement (replay / debrief)...\n";
    sim::EngagementSetup s;
    s.geometry = sim::StartGeometry::OFFENSIVE_PERCH;
    s.seed = 42;
    s.own_is_ai = true;
    auto run = [&] {
        auto e = std::make_unique<sim::Engagement>();
        e->setup(s);
        const sim::AircraftControls none{};
        for (int i = 0; i < static_cast<int>(40.0 / DT); ++i) e->step(DT, none);
        return e;
    };
    const auto a = run();
    const auto b = run();
    for (int i = 0; i < 2; ++i) {
        const auto& sa = a->world.aircraft[static_cast<size_t>(i)].state;
        const auto& sb = b->world.aircraft[static_cast<size_t>(i)].state;
        assert(std::memcmp(&sa.pos_ned, &sb.pos_ned, sizeof(sa.pos_ned)) == 0);
        assert(std::memcmp(&sa.q_att, &sb.q_att, sizeof(sa.q_att)) == 0);
        assert(a->world.aircraft[static_cast<size_t>(i)].gun.ammo == b->world.aircraft[static_cast<size_t>(i)].gun.ammo);
    }
    assert(a->world.event_count() == b->world.event_count());
    std::cout << "  -> PASSED\n";
}

void test_human_slot_is_not_flown_by_ai() {
    std::cout << "[BFM] The player's jet only moves by the player's controls...\n";
    sim::EngagementSetup s;
    s.geometry = sim::StartGeometry::HEAD_ON_MERGE;
    auto e = std::make_unique<sim::Engagement>();
    e->setup(s);
    sim::AircraftControls hands_off{};
    hands_off.throttle = 0.9;
    for (int i = 0; i < static_cast<int>(3.0 / DT); ++i) e->step(DT, hands_off);
    // Hands off, a FBW jet holds 1 G: wings stay level, no AI roll input.
    const double roll = std::abs(e->own().state.roll()) * 180.0 / M_PI;
    std::cout << "    own roll after 3 s hands-off: " << roll << " deg\n";
    assert(roll < 5.0);
    assert(e->own().gun.rounds_fired == 0);
    std::cout << "  -> PASSED\n";
}

} // namespace

int main() {
    std::cout << "=========================================================\n";
    std::cout << "  BFM AI and Engagement                                  \n";
    std::cout << "=========================================================\n";
    test_human_slot_is_not_flown_by_ai();
    test_engagement_replays_bit_exactly();
    test_gunnery_range_every_airframe();
    test_ai_vs_ai_is_safe_and_terminates();
    test_skill_ordering();
    std::cout << "\nAll BFM AI tests passed successfully!\n";
    return 0;
}
