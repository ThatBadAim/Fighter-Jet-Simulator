/**
 * @file test_missiles.cpp
 * @brief Radar missiles, fire-control radar, chaff and the RWR.
 *
 * Physics first: motor and drag give a Mach 3 burnout and a coast that
 * decays; lateral acceleration is capped by G and by available lift, so a slow
 * missile cannot turn. Guidance: PN intercepts a non-manoeuvring target, and
 * the launch-zone predictor flies exactly what the live missile flies.
 * Sensors: the doppler notch needs both beaming and look-down; chaff only
 * works inside the doppler gate. World: locks, launches, RWR levels,
 * warhead damage and bit-exact replay.
 */
#include "fastjet/sim/world.hpp"
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <memory>

using namespace fastjet;
using sim::MissileSpec;

namespace {

constexpr double DT = 0.005;
constexpr double G0 = fdm::SixDoFFDM::GRAVITY_ACCEL;

fdm::FlightState level(const math::Vector3& pos, double heading, double v) {
    fdm::FlightState s{};
    s.pos_ned = pos;
    s.vel_b = math::Vector3(v, 0.0, 0.0);
    s.q_att = math::Quaternion::from_euler(0.0, 0.0, heading);
    return s;
}

void test_motor_and_drag() {
    std::cout << "[MISSILE] Boost to Mach 3+, then a decaying coast...\n";
    const MissileSpec sp;
    sim::MissileBody m;
    m.pos = {0.0, 0.0, -5000.0};
    m.vel = {250.0, 0.0, 0.0};
    double v_burnout = 0.0;
    for (double t = 0.0; t < 40.0; t += DT) {
        m.step(sp, DT, sim::hold_weight());
        if (v_burnout == 0.0 && !m.motor_burning(sp)) v_burnout = m.speed();
    }
    const double a = environment::Atmosphere1976::compute(5000.0).speed_of_sound;
    std::cout << "    burnout Mach " << v_burnout / a << ", after 40 s " << m.speed() / a << ", mass "
              << m.mass(sp) << " kg\n";
    assert(v_burnout / a > 2.8 && v_burnout / a < 3.8);
    assert(m.speed() < 0.6 * v_burnout);          // drag bleeds the coast
    assert(std::abs(m.mass(sp) - sp.burnout_mass_kg) < 1e-9);
    // Weight held up: after the autopilot's first lag it flies level (under 1 deg).
    assert(std::abs(m.vel.z) < std::sin(M_PI / 180.0) * m.speed());
    std::cout << "  -> PASSED\n";
}

void test_lateral_limits() {
    std::cout << "[MISSILE] Turn capability: 40 G when fast, lift-limited when slow...\n";
    const MissileSpec sp;
    for (const double v : {1000.0, 350.0}) {
        sim::MissileBody m;
        m.pos = {0.0, 0.0, -5000.0};
        m.vel = {v, 0.0, 0.0};
        m.tof = sp.burn_time_s + 1.0; // coasting
        const double a_max = m.max_lateral_accel(sp);
        for (int i = 0; i < 200; ++i) m.step(sp, DT, {0.0, 1000.0 * G0, 0.0});
        const double a_lat = m.a_lat.norm();
        std::cout << "    " << v << " m/s: limit " << a_max / G0 << " G, achieved " << a_lat / G0 << " G\n";
        assert(a_lat <= m.max_lateral_accel(sp) + 1e-6 + 0.5 * G0); // limit falls as it slows
        if (v > 900.0) assert(std::abs(a_max - sp.g_limit * G0) < 1e-6);
        else assert(a_max < 0.5 * sp.g_limit * G0);
    }
    // Pulling G costs speed: induced drag.
    sim::MissileBody straight, turning;
    straight.pos = turning.pos = {0.0, 0.0, -5000.0};
    straight.vel = turning.vel = {700.0, 0.0, 0.0};
    straight.tof = turning.tof = 10.0;
    for (int i = 0; i < 400; ++i) {
        straight.step(sp, DT, sim::hold_weight());
        turning.step(sp, DT, {0.0, 25.0 * G0, -G0});
    }
    std::cout << "    2 s coasting: " << straight.speed() << " m/s straight, " << turning.speed() << " m/s at 25 G\n";
    assert(turning.speed() < straight.speed() - 30.0);
    std::cout << "  -> PASSED\n";
}

void test_pro_nav_intercept() {
    std::cout << "[MISSILE] PN intercepts a crossing, non-manoeuvring target...\n";
    const MissileSpec sp;
    sim::MissileBody m;
    m.pos = {0.0, 0.0, -5000.0};
    m.vel = {250.0, 0.0, 0.0};
    math::Vector3 tp{9000.0, -3000.0, -5200.0};
    const math::Vector3 tv{0.0, 250.0, 0.0};
    double best = 1e9;
    for (double t = 0.0; t < 40.0; t += DT) {
        const math::Vector3 m0 = m.pos, t0 = tp;
        m.step(sp, DT, m.tof < sim::RAIL_CLEAR_S ? sim::hold_weight() : sim::pro_nav(m, tp, tv, sp.nav_gain));
        tp += tv * DT;
        double s = 0.0;
        best = std::min(best, sim::closest_approach(m0, m.pos, t0, tp, s));
    }
    std::cout << "    miss distance " << best << " m\n";
    assert(best < 2.0);
    std::cout << "  -> PASSED\n";
}

void test_predictor_matches_live_missile() {
    std::cout << "[MISSILE] Launch-zone flyout = the live missile's own guidance and flight...\n";
    const MissileSpec sp;
    const math::Vector3 lp{0.0, 0.0, -6000.0}, lv{280.0, 0.0, 0.0};
    const math::Vector3 tp0{12000.0, 2000.0, -5500.0}, tv{-200.0, -120.0, 0.0};
    const sim::FlyoutResult f = sim::predict_flyout(sp, lp, lv, tp0, tv, sim::FlyoutTarget::STRAIGHT, DT);

    sim::Missile live;
    live.spec = sp;
    live.body.pos = lp;
    live.body.vel = lv;
    live.est_pos = tp0;
    live.est_vel = tv;
    live.active = true;
    math::Vector3 tp = tp0;
    double t_hit = -1.0, miss = 1e9;
    while (live.body.tof < sp.max_tof_s && t_hit < 0.0) {
        const math::Vector3 m0 = live.body.pos, t0 = tp;
        live.update(DT, /*uplink=*/true, tp, tv);
        tp += tv * DT;
        double s = 0.0;
        const double d = sim::closest_approach(m0, live.body.pos, t0, tp, s);
        if (live.body.tof > sp.arm_time_s && d < 0.5 * sp.fuze_radius_m && s < 1.0) {
            t_hit = live.body.tof;
            miss = d;
        }
    }
    std::cout << "    flyout: hit " << f.hit << " at " << f.time_s << " s, miss " << f.miss_m << " m; live: "
              << t_hit << " s, miss " << miss << " m (" << sim::to_string(live.phase) << ")\n";
    assert(f.hit);
    assert(std::abs(t_hit - f.time_s) < 1e-9);
    assert(std::abs(miss - f.miss_m) < 1e-9);
    std::cout << "  -> PASSED\n";
}

void test_launch_zones() {
    std::cout << "[MISSILE] Launch zones: head-on > beam > tail, thin air > thick, Rne < Rmax...\n";
    const MissileSpec sp;
    auto zone = [&](double alt, math::Vector3 tv, sim::FlyoutTarget model) {
        double r_best = 0.0;
        for (double r = 2000.0; r < 70000.0; r += 1000.0) {
            const auto f = sim::predict_flyout(sp, {0.0, 0.0, -alt}, {250.0, 0.0, 0.0}, {r, 0.0, -alt}, tv, model);
            if (f.hit) r_best = r;
        }
        return r_best;
    };
    const double head = zone(5000.0, {-250.0, 0.0, 0.0}, sim::FlyoutTarget::STRAIGHT);
    const double beam = zone(5000.0, {0.0, 250.0, 0.0}, sim::FlyoutTarget::STRAIGHT);
    const double tail = zone(5000.0, {250.0, 0.0, 0.0}, sim::FlyoutTarget::STRAIGHT);
    const double tail_low = zone(1000.0, {250.0, 0.0, 0.0}, sim::FlyoutTarget::STRAIGHT);
    const double beam_rne = zone(5000.0, {0.0, 250.0, 0.0}, sim::FlyoutTarget::TURN_COLD);
    std::cout << "    5 km: head-on " << head / 1000 << " km, beam " << beam / 1000 << " km, tail " << tail / 1000
              << " km (1 km altitude: " << tail_low / 1000 << " km); beam Rne " << beam_rne / 1000 << " km\n";
    assert(head > beam && beam > tail);
    assert(tail_low < tail);
    assert(beam_rne < 0.8 * beam);
    assert(head > 30000.0 && tail > 10000.0 && tail < 30000.0);
    std::cout << "  -> PASSED\n";
}

void test_doppler_notch() {
    std::cout << "[SENSOR] The notch needs beaming AND ground in the beam...\n";
    sim::SensorLimits lim;
    const math::Vector3 sensor{0.0, 0.0, -6000.0};
    const math::Vector3 fwd{1.0, 0.0, 0.0};
    const math::Vector3 below{10000.0, 0.0, -4000.0}; // look-down
    const math::Vector3 above{10000.0, 0.0, -8000.0}; // look-up, 11 deg
    const math::Vector3 beaming{0.0, 250.0, 0.0};
    const math::Vector3 cold{250.0, 0.0, 0.0};
    assert(lim.check(sensor, fwd, below, beaming) == sim::TrackCheck::NOTCH);
    assert(lim.check(sensor, fwd, below, cold) == sim::TrackCheck::OK);
    assert(lim.check(sensor, fwd, above, beaming) == sim::TrackCheck::OK);
    assert(lim.check(sensor, fwd, {0.0, 10000.0, -6000.0}, cold) == sim::TrackCheck::GIMBAL);
    assert(lim.check(sensor, fwd, {40000.0, 0.0, -6000.0}, cold) == sim::TrackCheck::RANGE);
    std::cout << "  -> PASSED\n";
}

void test_chaff_gates() {
    std::cout << "[SENSOR] Chaff is in the gates only when the target has no radial velocity of its own...\n";
    const math::Vector3 sensor{0.0, 0.0, -5000.0};
    sim::ChaffCloud c;
    c.pos = {8000.0, 0.0, -5000.0};
    c.vel = {0.0, 0.0, 2.0};
    c.active = true;
    const math::Vector3 tgt{8000.0, 60.0, -5000.0};
    const double beam = 1.75 * M_PI / 180.0;
    assert(sim::chaff_in_gates(sensor, tgt, {0.0, 280.0, 0.0}, c, beam));   // beaming: stolen
    assert(!sim::chaff_in_gates(sensor, tgt, {280.0, 0.0, 0.0}, c, beam));  // running: doppler rejects
    assert(!sim::chaff_in_gates(sensor, {8000.0, 900.0, -5000.0}, {0.0, 280.0, 0.0}, c, beam)); // outside the beam
    std::cout << "  -> PASSED\n";
}

std::unique_ptr<sim::World> two_jets(double range, double target_heading) {
    auto w = std::make_unique<sim::World>();
    w->clear(11);
    w->add(aircraft::AircraftType::F16_FIGHTING_FALCON, level({0.0, 0.0, -5000.0}, 0.0, 250.0));
    w->add(aircraft::AircraftType::F16_FIGHTING_FALCON, level({range, 0.0, -5000.0}, target_heading, 250.0));
    for (int i = 0; i < 2; ++i) w->aircraft[static_cast<size_t>(i)].landing_gear.deployed = false;
    return w;
}

/// Straight and level at a steady lever, for both jets.
std::array<sim::AircraftControls, sim::World::kMaxAircraft> cruise() {
    std::array<sim::AircraftControls, sim::World::kMaxAircraft> c{};
    for (auto& k : c) k.throttle = 0.8;
    return c;
}

void test_radar_lock_and_rwr() {
    std::cout << "[RADAR] Acquire -> lock -> launch, heard on the target's RWR...\n";
    auto w = two_jets(12000.0, 0.0); // target 12 km ahead, flying away
    assert(w->rwr(1).level == sim::RwrStatus::Level::CLEAR);
    w->radars[0].enable(1, 1.0);
    w->missile_load[0] = 2;
    auto c = cruise();
    w->step(DT, c);
    assert(w->rwr(1).level == sim::RwrStatus::Level::SEARCH);
    for (int i = 0; i < static_cast<int>(1.1 / DT); ++i) w->step(DT, c);
    const auto locked = w->rwr(1);
    assert(w->radars[0].locked());
    assert(locked.level == sim::RwrStatus::Level::LOCK);
    // The lock is behind the target: bearing near 180 deg.
    std::cout << "    lock heard at bearing " << locked.emitter_bearing * 180.0 / M_PI << " deg, symbol "
              << locked.emitter_symbol << "\n";
    assert(std::abs(std::abs(locked.emitter_bearing) - M_PI) < 0.1);
    assert(std::strcmp(locked.emitter_symbol, "16") == 0);

    assert(w->launch_missile(0, 1) >= 0);
    assert(w->missile_load[0] == 1);
    w->step(DT, c);
    const auto launch = w->rwr(1);
    assert(launch.level == sim::RwrStatus::Level::LAUNCH && launch.missile_valid);
    // No lock, no launch: a missile needs a track to start from.
    w->radars[0].break_lock();
    assert(w->launch_missile(0, 1) < 0);
    std::cout << "  -> PASSED\n";
}

void test_lock_breaks_outside_gimbal() {
    std::cout << "[RADAR] A lock coasts on memory, then drops when the target leaves the gimbal...\n";
    auto w = two_jets(8000.0, 0.0);
    w->radars[0].enable(1, 0.5);
    auto c = cruise();
    for (int i = 0; i < static_cast<int>(1.0 / DT); ++i) w->step(DT, c);
    assert(w->radars[0].locked());
    // Put the target 90 deg off the nose.
    w->aircraft[1].state.pos_ned = w->aircraft[0].state.pos_ned + math::Vector3(0.0, 8000.0, 0.0);
    w->step(DT, c);
    assert(w->radars[0].locked() && w->radars[0].coasting);
    for (int i = 0; i < static_cast<int>((w->radars[0].spec.memory_s + 0.1) / DT); ++i) w->step(DT, c);
    assert(!w->radars[0].locked());
    assert(w->rwr(1).level == sim::RwrStatus::Level::CLEAR); // out of the scan volume too
    std::cout << "  -> PASSED\n";
}

void test_missile_kill_and_replay() {
    std::cout << "[WORLD] A missile at a straight-flying jet fuzes and kills it, bit-exactly on replay...\n";
    auto run = [] {
        auto w = two_jets(9000.0, 0.0);
        w->radars[0].enable(1, 0.5);
        w->missile_load[0] = 1;
        auto c = cruise();
        bool launched = false;
        for (int i = 0; i < static_cast<int>(40.0 / DT); ++i) {
            if (!launched && w->radars[0].locked()) launched = w->launch_missile(0, 1) >= 0;
            w->step(DT, c);
        }
        return w;
    };
    const auto a = run();
    const auto b = run();
    int det = -1;
    for (int k = 0; k < a->event_count(); ++k) {
        if (a->event(k).kind == sim::CombatEvent::Kind::MISSILE_DETONATION) det = k;
    }
    assert(det >= 0);
    std::cout << "    detonated " << a->event(det).miss_m << " m from the target at t=" << a->event(det).time
              << " s; target alive: " << a->aircraft[1].alive() << "\n";
    assert(a->event(det).miss_m < a->missiles[0].spec.fuze_radius_m);
    assert(!a->aircraft[1].alive());
    assert(a->missiles_in_flight() == 0);
    assert(a->event_count() == b->event_count());
    for (int i = 0; i < 2; ++i) {
        const auto& sa = a->aircraft[static_cast<size_t>(i)].state;
        const auto& sb = b->aircraft[static_cast<size_t>(i)].state;
        assert(std::memcmp(&sa.pos_ned, &sb.pos_ned, sizeof(sa.pos_ned)) == 0);
    }
    std::cout << "  -> PASSED\n";
}

void test_chaff_dispenser() {
    std::cout << "[WORLD] Chaff: two bundles per press, never more than the load...\n";
    auto w = two_jets(8000.0, 0.0);
    auto c = cruise();
    const int load = w->chaff_load[1];
    for (int press = 0; press < 3; ++press) {
        c[1].dispense = true;
        w->step(DT, c);
        c[1].dispense = false;
        for (int i = 0; i < static_cast<int>(0.5 / DT); ++i) w->step(DT, c);
    }
    std::cout << "    3 presses: " << load - w->chaff_load[1] << " bundles\n";
    assert(load - w->chaff_load[1] == 3 * sim::World::CHAFF_PER_PROGRAM);
    // Holding the switch is one press.
    c[1].dispense = true;
    for (int i = 0; i < static_cast<int>(1.0 / DT); ++i) w->step(DT, c);
    assert(load - w->chaff_load[1] == 4 * sim::World::CHAFF_PER_PROGRAM);
    w->chaff_load[1] = 1;
    c[1].dispense = false;
    w->step(DT, c);
    c[1].dispense = true;
    for (int i = 0; i < static_cast<int>(1.0 / DT); ++i) w->step(DT, c);
    assert(w->chaff_load[1] == 0);
    std::cout << "  -> PASSED\n";
}

void test_no_radar_on_a10() {
    std::cout << "[RADAR] The A-10 has no air-to-air radar: nothing to lock, nothing to hear...\n";
    auto w = std::make_unique<sim::World>();
    w->clear(3);
    w->add(aircraft::AircraftType::A10_THUNDERBOLT, level({0.0, 0.0, -3000.0}, 0.0, 150.0));
    w->add(aircraft::AircraftType::F16_FIGHTING_FALCON, level({5000.0, 0.0, -3000.0}, 0.0, 200.0));
    w->radars[0].enable(1, 0.5);
    auto c = cruise();
    for (int i = 0; i < static_cast<int>(2.0 / DT); ++i) w->step(DT, c);
    assert(!w->radars[0].locked());
    assert(w->rwr(1).level == sim::RwrStatus::Level::CLEAR);
    std::cout << "  -> PASSED\n";
}

} // namespace

int main() {
    std::cout << "=========================================================\n";
    std::cout << "  Radar Missiles, Radar, Chaff and RWR                   \n";
    std::cout << "=========================================================\n";
    test_motor_and_drag();
    test_lateral_limits();
    test_pro_nav_intercept();
    test_predictor_matches_live_missile();
    test_launch_zones();
    test_doppler_notch();
    test_chaff_gates();
    test_radar_lock_and_rwr();
    test_lock_breaks_outside_gimbal();
    test_missile_kill_and_replay();
    test_chaff_dispenser();
    test_no_radar_on_a10();
    std::cout << "\nAll missile and sensor tests passed successfully!\n";
    return 0;
}
