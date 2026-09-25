/**
 * @file test_radar_scope.cpp
 * @brief The pilot's fire-control radar (FCR page): RWS search and STT.
 *
 * RWS: the stabilised 4-bar raster takes 4 x 120 deg / 65 deg/s per frame;
 * it paints one brick per jet per frame, where the jet was; it shows only
 * what the radar can see (scan volume, detection range, doppler notch).
 * STT: designating a brick locks the jet through the world's radar, so the
 * target's RWR hears it; a broken lock drops back to RWS without relocking
 * by itself, and a stale brick cannot be locked.
 */
#include "fastjet/sim/radar_scope.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>

using namespace fastjet;
using sim::RadarScope;

namespace {

constexpr double DT = 0.005;
constexpr double DEG = M_PI / 180.0;

fdm::FlightState level(const math::Vector3& pos, double heading, double v) {
    fdm::FlightState s{};
    s.pos_ned = pos;
    s.vel_b = math::Vector3(v, 0.0, 0.0);
    s.q_att = math::Quaternion::from_euler(0.0, 0.0, heading);
    return s;
}

/// Ownship at 5 km heading north at 250 m/s; the other jet at @p offset from it.
std::unique_ptr<sim::World> two_jets(const math::Vector3& offset, double target_heading,
                                     aircraft::AircraftType own_type = aircraft::AircraftType::F16_FIGHTING_FALCON) {
    auto w = std::make_unique<sim::World>();
    w->clear(7);
    const math::Vector3 own_pos(0.0, 0.0, -5000.0);
    w->add(own_type, level(own_pos, 0.0, 250.0));
    w->add(aircraft::AircraftType::F16_FIGHTING_FALCON, level(own_pos + offset, target_heading, 250.0));
    for (int i = 0; i < 2; ++i) w->aircraft[static_cast<size_t>(i)].landing_gear.deployed = false;
    return w;
}

std::array<sim::AircraftControls, sim::World::kMaxAircraft> cruise() {
    std::array<sim::AircraftControls, sim::World::kMaxAircraft> c{};
    for (auto& k : c) k.throttle = 0.8;
    return c;
}

void run(sim::World& w, RadarScope& scope, double seconds) {
    const auto c = cruise();
    for (int i = 0; i < static_cast<int>(seconds / DT + 0.5); ++i) {
        w.step(DT, c);
        scope.step(DT, w, 0);
    }
}

constexpr double FRAME_S = RadarScope::BARS * 2.0 * RadarScope::AZ_LIMIT_RAD / RadarScope::SCAN_RATE_RAD_S;

/// Bricks painted after one full frame (plus a margin) of scanning.
int bricks_after_a_frame(const math::Vector3& offset, double target_heading) {
    auto w = two_jets(offset, target_heading);
    RadarScope scope;
    run(*w, scope, FRAME_S + 0.2);
    return scope.hit_count;
}

void test_rws_paints_where_the_jet_was() {
    std::cout << "[FCR] RWS: one brick per frame, where the beam crossed the jet...\n";
    auto w = two_jets({30000.0, 0.0, 0.0}, M_PI); // head-on, closing at 500 m/s
    RadarScope scope;
    scope.sync(*w, 0);
    assert(scope.fitted() && scope.mode == RadarScope::Mode::RWS);

    // Time the first frame.
    const auto c = cruise();
    double t = 0.0;
    while (scope.frame == 0) {
        w->step(DT, c);
        scope.step(DT, *w, 0);
        t += DT;
        assert(t < 20.0);
    }
    std::cout << "    frame time " << t << " s (expected " << FRAME_S << ")\n";
    assert(std::abs(t - FRAME_S) < 0.02);
    assert(scope.hit_count == 1);
    const sim::RadarHit& h = scope.hits[0];
    std::cout << "    brick at az " << h.az_rad / DEG << " deg, " << h.range_m << " m, frame " << h.frame << "\n";
    assert(h.target == 1 && h.frame == 0);
    assert(std::abs(h.az_rad) < 1.0 * DEG);
    assert(h.range_m < 30000.0 && h.range_m > 30000.0 - 500.0 * FRAME_S);
    assert(std::abs(h.alt_m - 5000.0) < 100.0);

    // In trail at 15 km (the same jet flying the same way, so it holds its
    // range): one brick per frame, history ageing out after three frames.
    w = two_jets({15000.0, 0.0, 0.0}, 0.0);
    run(*w, scope, 4.9 * FRAME_S);
    std::cout << "    in trail after 4.9 frames: frame " << scope.frame << ", " << scope.hit_count << " bricks\n";
    assert(scope.frame == 4);
    assert(scope.hit_count == RadarScope::HISTORY_FRAMES);
    const sim::RadarHit* cur = scope.cursor_hit();
    assert(cur && cur->frame == scope.frame);
    for (int i = 0; i < scope.hit_count; ++i) {
        const sim::RadarHit& o = scope.hits[static_cast<size_t>(i)];
        std::cout << "      f" << o.frame << " " << o.range_m << " m, az " << o.az_rad / DEG << " deg" << std::endl;
        assert(o.frame >= scope.frame - 2 && std::abs(o.range_m - 15000.0) < 100.0);
    }
    std::cout << "  -> PASSED\n";
}

void test_rws_sees_only_what_the_radar_can() {
    std::cout << "[FCR] RWS shows only the scan volume, detection range and out-of-notch jets...\n";
    const int ahead = bricks_after_a_frame({15000.0, 0.0, 0.0}, M_PI);
    const int behind = bricks_after_a_frame({-10000.0, 0.0, 0.0}, 0.0);
    const int far = bricks_after_a_frame({60000.0, 0.0, 0.0}, M_PI);
    // 300 m below at 10 km: look-down, clutter in the beam. The beaming jet
    // crosses the nose mid-frame, so its radial speed stays inside the notch.
    const double cross = -0.5 * 250.0 * FRAME_S;
    const int beaming_low = bricks_after_a_frame({10000.0, cross, 300.0}, M_PI / 2.0);
    const int hot_low = bricks_after_a_frame({10000.0, 0.0, 300.0}, M_PI);
    // Beaming but 700 m above: no ground behind it, no notch.
    const int beaming_high = bricks_after_a_frame({10000.0, cross, -700.0}, M_PI / 2.0);
    std::cout << "    ahead " << ahead << ", behind " << behind << ", 60 km " << far << ", beaming look-down "
              << beaming_low << ", hot look-down " << hot_low << ", beaming look-up " << beaming_high << std::endl;
    assert(ahead == 1 && behind == 0 && far == 0);
    assert(beaming_low == 0 && hot_low == 1 && beaming_high == 1);

    // 4 km above at 10 km, in trail, is 22 deg up: outside the 4-bar block until the
    // pilot raises the antenna.
    auto w = two_jets({10000.0, 0.0, -4000.0}, 0.0);
    RadarScope scope;
    run(*w, scope, FRAME_S + 0.2);
    assert(scope.hit_count == 0);
    std::cout << "    block at 10 km: " << (5000.0 + 10000.0 * std::sin(scope.coverage_bottom_rad())) / 0.3048
              << " - " << (5000.0 + 10000.0 * std::sin(scope.coverage_top_rad())) / 0.3048 << " ft\n";
    scope.slew_elevation(1.0, 22.0 * DEG / RadarScope::ELEV_SLEW_RAD_S);
    run(*w, scope, FRAME_S + 0.2);
    std::cout << "    antenna up " << scope.el_centre / DEG << " deg: " << scope.hit_count << " brick\n";
    assert(scope.hit_count == 1);
    std::cout << "  -> PASSED\n";
}

void test_stt_is_a_lock_the_target_hears() {
    std::cout << "[FCR] Designate -> STT through the jet's radar; the target's RWR hears the lock...\n";
    auto w = two_jets({15000.0, 0.0, 0.0}, M_PI);
    RadarScope scope;
    scope.sync(*w, 0);
    assert(!scope.designate(*w)); // nothing on the scope yet
    run(*w, scope, FRAME_S + 0.2);
    assert(w->rwr(1).level == sim::RwrStatus::Level::CLEAR);
    assert(scope.designate(*w));
    run(*w, scope, 0.15);
    assert(scope.mode == RadarScope::Mode::STT && w->radars[0].locked());
    assert(scope.locked_target(*w) == 1 && scope.hit_count == 0);
    assert(w->rwr(1).level == sim::RwrStatus::Level::LOCK);
    // The antenna follows the target.
    double az = 0.0, el = 0.0;
    RadarScope::stabilised_angles(w->aircraft[0].state, w->aircraft[1].position(), az, el);
    assert(std::abs(scope.ant_az - az) < 0.1 * DEG && std::abs(scope.ant_el - el) < 0.1 * DEG);

    scope.undesignate(*w);
    assert(scope.mode == RadarScope::Mode::RWS && !w->radars[0].locked());
    run(*w, scope, 0.05);
    assert(w->rwr(1).level == sim::RwrStatus::Level::CLEAR);
    std::cout << "  -> PASSED\n";
}

void test_broken_lock_needs_the_pilot() {
    std::cout << "[FCR] A broken lock drops to RWS and stays off until the pilot designates again...\n";
    auto w = two_jets({12000.0, 0.0, 0.0}, M_PI);
    RadarScope scope;
    run(*w, scope, FRAME_S + 0.2);
    assert(scope.designate(*w));
    run(*w, scope, 0.15);
    assert(scope.mode == RadarScope::Mode::STT);
    // 90 deg off the nose: memory, then the lock breaks.
    const math::Vector3 own = w->aircraft[0].position();
    w->aircraft[1].state.pos_ned = own + math::Vector3(0.0, 8000.0, 0.0);
    run(*w, scope, 0.15);
    assert(scope.mode == RadarScope::Mode::STT && w->radars[0].coasting);
    run(*w, scope, w->radars[0].spec.memory_s + 0.1);
    assert(scope.mode == RadarScope::Mode::RWS);
    assert(w->radars[0].mode == sim::FireControlRadar::Mode::OFF);
    // Back in front of the nose: no lock without a new designation.
    w->aircraft[1].state.pos_ned = w->aircraft[0].position() + math::Vector3(10000.0, 0.0, 0.0);
    run(*w, scope, 1.0);
    assert(!w->radars[0].locked() && w->rwr(1).level == sim::RwrStatus::Level::CLEAR);

    // A stale brick: the jet has gone behind since it was painted.
    run(*w, scope, FRAME_S + 0.2);
    assert(scope.hit_count >= 1);
    w->aircraft[1].state.pos_ned = w->aircraft[0].position() + math::Vector3(-10000.0, 0.0, 0.0);
    assert(scope.designate(*w));
    run(*w, scope, RadarScope::LOCK_TIMEOUT_S + 0.05);
    assert(scope.mode == RadarScope::Mode::RWS && scope.lock_failed_cue);
    assert(w->radars[0].mode == sim::FireControlRadar::Mode::OFF);
    std::cout << "  -> PASSED\n";
}

void test_range_scales() {
    std::cout << "[FCR] Range scale: off-scale bricks cannot be designated; STT auto-ranges...\n";
    auto w = two_jets({26000.0, 0.0, 0.0}, 0.0); // 14 nm, going away
    RadarScope scope;
    run(*w, scope, FRAME_S + 0.2);
    assert(scope.hit_count == 1);
    scope.range_down(); // 10 nm
    assert(scope.cursor_hit() == nullptr && !scope.designate(*w));
    scope.range_up();   // 20 nm
    assert(scope.designate(*w));
    run(*w, scope, 0.15);
    assert(scope.mode == RadarScope::Mode::STT);
    scope.range_down();
    run(*w, scope, DT);
    assert(scope.range_index == 1); // bumped back out to 20 nm
    for (int i = 0; i < 6; ++i) scope.range_up();
    assert(scope.range_index == 3);
    run(*w, scope, 2.0 * DT);
    assert(scope.range_index == 1); // 14 nm is under 40% of 80 and 40 nm: stepped down to 20
    std::cout << "  -> PASSED\n";
}

void test_no_radar() {
    std::cout << "[FCR] The A-10 has no radar: no scope, nothing to lock...\n";
    auto w = two_jets({12000.0, 0.0, 0.0}, M_PI, aircraft::AircraftType::A10_THUNDERBOLT);
    RadarScope scope;
    run(*w, scope, FRAME_S + 0.2);
    assert(!scope.fitted() && scope.mode == RadarScope::Mode::OFF && scope.hit_count == 0);
    assert(!scope.designate(*w));
    std::cout << "  -> PASSED\n";
}

void test_new_world_resets_the_scope() {
    std::cout << "[FCR] A new fight or a reset gives a cold scope...\n";
    auto w = two_jets({12000.0, 0.0, 0.0}, M_PI);
    RadarScope scope;
    run(*w, scope, FRAME_S + 0.2);
    assert(scope.designate(*w));
    run(*w, scope, 0.15);
    assert(scope.mode == RadarScope::Mode::STT);
    scope.range_up();
    const int range = scope.range_index;
    w = two_jets({12000.0, 0.0, 0.0}, M_PI);
    scope.sync(*w, 0);
    assert(scope.mode == RadarScope::Mode::RWS && scope.hit_count == 0 && scope.frame == 0);
    assert(scope.range_index == range); // the pilot's settings stay
    std::cout << "  -> PASSED\n";
}

} // namespace

int main() {
    std::cout << "=========================================================\n";
    std::cout << "  Fire-Control Radar Scope                               \n";
    std::cout << "=========================================================\n";
    test_rws_paints_where_the_jet_was();
    test_rws_sees_only_what_the_radar_can();
    test_stt_is_a_lock_the_target_hears();
    test_broken_lock_needs_the_pilot();
    test_range_scales();
    test_no_radar();
    test_new_world_resets_the_scope();
    std::cout << "\nAll radar scope tests passed successfully!\n";
    return 0;
}
