/**
 * @file test_air_combat_geometry.cpp
 * @brief Air-combat geometry and energy readouts against analytic values:
 * range, closure, antenna train angle, aspect, heading crossing angle, turn
 * rate and specific excess power for hand-placed aircraft.
 */
#include "fastjet/sim/air_combat_geometry.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace fastjet;

namespace {

constexpr double DEG = M_PI / 180.0;
constexpr double G = fdm::SixDoFFDM::GRAVITY_ACCEL;

fdm::FlightState jet(double n, double e, double alt, double heading_deg, double v) {
    fdm::FlightState s{};
    s.pos_ned = math::Vector3(n, e, -alt);
    s.vel_b = math::Vector3(v, 0.0, 0.0);
    s.q_att = math::Quaternion::from_euler(0.0, 0.0, heading_deg * DEG);
    return s;
}

bool near(double a, double b, double tol) { return std::abs(a - b) <= tol; }

void test_dead_six() {
    std::cout << "[GEOMETRY] Dead six, same speed: ATA 0, AA 0, no closure...\n";
    const auto g = sim::AirCombatGeometry::compute(jet(0, 0, 5000, 0, 250), jet(1000, 0, 5000, 0, 250));
    assert(near(g.range, 1000.0, 1e-9));
    assert(near(g.ata, 0.0, 1e-9) && near(g.aspect, 0.0, 1e-9) && near(g.hca, 0.0, 1e-9));
    assert(near(g.closure, 0.0, 1e-9));
    std::cout << "  -> PASSED\n";
}

void test_head_on() {
    std::cout << "[GEOMETRY] Head-on: AA 180, HCA 180, closure = sum of speeds...\n";
    const auto g = sim::AirCombatGeometry::compute(jet(0, 0, 5000, 0, 250), jet(3000, 0, 5000, 180, 300));
    assert(near(g.aspect, M_PI, 1e-9) && near(g.hca, M_PI, 1e-9) && near(g.ata, 0.0, 1e-9));
    assert(near(g.closure, 550.0, 1e-6));
    std::cout << "  -> PASSED\n";
}

void test_beam() {
    std::cout << "[GEOMETRY] Bandit on the right beam, heading north: ATA 90, AA 90, HCA 0...\n";
    const auto g = sim::AirCombatGeometry::compute(jet(0, 0, 5000, 0, 250), jet(0, 2000, 5000, 0, 250));
    assert(near(g.ata, 90.0 * DEG, 1e-9) && near(g.aspect, 90.0 * DEG, 1e-9) && near(g.hca, 0.0, 1e-9));
    assert(g.los_b.y > 1999.0); // on the right in body axes
    std::cout << "  -> PASSED\n";
}

void test_quarter_aspect() {
    std::cout << "[GEOMETRY] 30 deg off the tail at 914 m (the offensive perch)...\n";
    const double r = 914.0;
    const auto tgt = jet(0, 0, 5000, 0, 250);
    const auto own = jet(-r * std::cos(30 * DEG), r * std::sin(30 * DEG), 5000, 0, 250);
    const auto g = sim::AirCombatGeometry::compute(own, tgt);
    std::cout << "    range " << g.range << " m, AA " << g.aspect / DEG << " deg, ATA " << g.ata / DEG << " deg\n";
    assert(near(g.range, r, 1e-6));
    assert(near(g.aspect, 30.0 * DEG, 1e-9));
    assert(near(g.ata, 30.0 * DEG, 1e-9));
    std::cout << "  -> PASSED\n";
}

void test_turn_rate_and_ps() {
    std::cout << "[ENERGY] Level turn at n G: omega = g sqrt(n^2 - 1) / V; Ps = (T - D) V / W...\n";
    fdm::MassProperties m{};
    m.mass_kg = 12000.0;
    for (double n : {2.0, 5.0, 9.0}) {
        const double V = 200.0;
        const double bank = std::acos(1.0 / n);
        fdm::FlightState s = jet(0, 0, 5000, 0, V);
        s.q_att = math::Quaternion::from_euler(bank, 0.0, 0.0);
        fdm::AircraftForces f{};
        f.force_b = math::Vector3(0.0, 0.0, -n * m.mass_kg * G); // lift only, thrust = drag
        const auto e = sim::EnergyState::compute(s, f, m);
        const double expected = G * std::sqrt(n * n - 1.0) / V;
        std::cout << "    n=" << n << ": turn rate " << e.turn_rate / DEG << " deg/s (analytic " << expected / DEG << ")\n";
        assert(near(e.turn_rate, expected, 1e-9));
        assert(near(e.ps, 0.0, 1e-9));
    }
    // 20 kN excess thrust at 250 m/s on a 12 t jet.
    fdm::FlightState s = jet(0, 0, 5000, 0, 250.0);
    fdm::AircraftForces f{};
    f.force_b = math::Vector3(20000.0, 0.0, -m.mass_kg * G);
    const auto e = sim::EnergyState::compute(s, f, m);
    const double expected_ps = 20000.0 * 250.0 / (m.mass_kg * G);
    std::cout << "    Ps " << e.ps << " m/s (analytic " << expected_ps << ")\n";
    assert(near(e.ps, expected_ps, 1e-9));
    std::cout << "  -> PASSED\n";
}

void test_segment_distance() {
    std::cout << "[GEOMETRY] Segment-segment distance for hit sweeps...\n";
    double s = 0, t = 0;
    // Crossing segments 2 m apart vertically.
    const double d = sim::segment_segment_distance({-5, 0, 0}, {5, 0, 0}, {0, -5, 2}, {0, 5, 2}, s, t);
    assert(near(d, 2.0, 1e-12) && near(s, 0.5, 1e-12) && near(t, 0.5, 1e-12));
    // Parallel, offset: distance is the offset.
    const double p = sim::segment_segment_distance({0, 0, 0}, {10, 0, 0}, {2, 3, 0}, {8, 3, 0}, s, t);
    assert(near(p, 3.0, 1e-12));
    std::cout << "  -> PASSED\n";
}

} // namespace

int main() {
    std::cout << "=========================================================\n";
    std::cout << "  Air Combat Geometry and Energy                         \n";
    std::cout << "=========================================================\n";
    test_dead_six();
    test_head_on();
    test_beam();
    test_quarter_aspect();
    test_turn_rate_and_ps();
    test_segment_distance();
    std::cout << "\nAll air combat geometry tests passed successfully!\n";
    return 0;
}
