#include "fastjet/gear/landing_gear.hpp"
#include "fastjet/fdm/rk4_integrator.hpp"
#include "fastjet/fdm/fuel_system.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace fastjet;

namespace {

constexpr double GRAVITY = 9.80665;

/// Places the aircraft so its wheels rest exactly on the ground plane.
fdm::FlightState make_ground_state(double u_mps = 0.0) {
    fdm::FlightState s{};
    // Contact plane sits 1.70 m below the CG at full extension; the struts
    // compress ~0.11 m under static load, so level stance is ~1.59 m.
    s.pos_ned = math::Vector3(0.0, 0.0, -1.59);
    s.vel_b   = math::Vector3(u_mps, 0.0, 0.0);
    s.omega_b = math::Vector3::zero();
    s.q_att   = math::Quaternion::identity();
    return s;
}

void test_static_equilibrium() {
    std::cout << "[Test] Landing Gear: Static Three-Point Weight Support... ";

    gear::LandingGear lg;
    const auto mass = fdm::FuelSystem::compute(fdm::FuelSystem::EMPTY_MASS_KG * 0.0);
    const double weight_n = mass.mass_kg * GRAVITY;

    // Settle the aircraft onto its struts under gravity.
    fdm::FlightState state = make_ground_state();
    fdm::RK4Integrator integrator(0.002);
    double t = 0.0;

    for (int i = 0; i < 4000; ++i) {
        integrator.step(state, t, mass, [&](double, const fdm::FlightState& s) noexcept {
            return lg.compute(s, mass);
        });
    }

    const auto f = lg.compute(state, mass);

    // All three struts must be carrying load.
    assert(lg.struts[0].in_contact && "nose gear must be in contact");
    assert(lg.mains_on_ground() && "both mains must be in contact");
    assert(lg.weight_on_wheels());

    // Vertical reaction must balance weight (body Z is down, so force is negative).
    const double lift_n = -f.force_b.z;
    const double err = std::abs(lift_n - weight_n) / weight_n;
    assert(err < 0.02 && "ground reaction must balance aircraft weight to 2%");

    // The aircraft must have come to rest vertically, not sunk through.
    const double vel_ned_z = state.q_att.rotate_body_to_ned(state.vel_b).z;
    assert(std::abs(vel_ned_z) < 0.05 && "must settle, not sink through ground");

    // Struts must be compressed but not bottomed out.
    assert(lg.struts[1].compression > 0.0);
    assert(lg.struts[1].compression < lg.struts[1].max_stroke);

    std::cout << "PASSED (Reaction " << lift_n / 1000.0 << " kN vs weight "
              << weight_n / 1000.0 << " kN, main stroke "
              << lg.struts[1].compression * 100.0 << " cm)\n";
}

void test_no_force_when_airborne() {
    std::cout << "[Test] Landing Gear: Zero Force When Airborne... ";

    gear::LandingGear lg;
    const auto mass = fdm::FuelSystem::compute(1000.0);

    fdm::FlightState s{};
    s.pos_ned = math::Vector3(0.0, 0.0, -1500.0); // 1,500 m up
    s.vel_b   = math::Vector3(220.0, 0.0, 0.0);
    s.q_att   = math::Quaternion::identity();

    const auto f = lg.compute(s, mass);

    assert(f.force_b.norm() == 0.0 && "no ground force at altitude");
    assert(f.moment_b.norm() == 0.0);
    assert(!lg.weight_on_wheels());

    std::cout << "PASSED (Zero force and moment at 1,500 m)\n";
}

void test_retracted_gear_inert() {
    std::cout << "[Test] Landing Gear: Retracted Gear Produces No Reaction... ";

    gear::LandingGear lg;
    lg.deployed = false;
    const auto mass = fdm::FuelSystem::compute(1000.0);

    // Belly at ground level with the gear up.
    fdm::FlightState s = make_ground_state();
    const auto f = lg.compute(s, mass);

    assert(f.force_b.norm() == 0.0 && "retracted gear must not push");
    assert(!lg.weight_on_wheels());

    std::cout << "PASSED (Gear up: no ground reaction)\n";
}

void test_braking_decelerates() {
    std::cout << "[Test] Landing Gear: Wheel Braking Deceleration... ";

    const auto mass = fdm::FuelSystem::compute(1000.0);
    fdm::RK4Integrator integrator(0.002);

    // Roll out from 60 m/s with full braking.
    gear::LandingGear lg;
    fdm::FlightState state = make_ground_state(60.0);
    lg.brake_left = 1.0;
    lg.brake_right = 1.0;

    double t = 0.0;
    for (int i = 0; i < 5000; ++i) { // 10 s
        integrator.step(state, t, mass, [&](double, const fdm::FlightState& s) noexcept {
            return lg.compute(s, mass);
        });
    }
    const double braked_speed = state.vel_b.x;

    // Same rollout with brakes released.
    gear::LandingGear lg_free;
    fdm::FlightState free_state = make_ground_state(60.0);
    double t2 = 0.0;
    for (int i = 0; i < 5000; ++i) {
        integrator.step(free_state, t2, mass, [&](double, const fdm::FlightState& s) noexcept {
            return lg_free.compute(s, mass);
        });
    }
    const double rolling_speed = free_state.vel_b.x;

    assert(braked_speed < 60.0 && "braking must slow the aircraft");
    assert(braked_speed < rolling_speed && "braking must beat free rolling");
    assert(braked_speed >= -1.0 && "must not reverse through friction");

    // Only the mains are braked, and they carry ~88% of static weight, so the
    // achievable deceleration is ~0.88 * mu_braking * g ~= 4.7 m/s^2. Over 10 s
    // that is ~47 m/s. Allow a generous band around that physical expectation.
    const double decel = (60.0 - braked_speed) / 10.0;
    assert(decel > 3.0 && "full braking must exceed 3 m/s^2");
    assert(decel < 6.0 && "braking cannot exceed the tyre friction limit");

    std::cout << "PASSED (60 -> " << braked_speed << " m/s braked ("
              << decel << " m/s^2) vs " << rolling_speed << " m/s free-rolling)\n";
}

void test_gear_collapse_on_hard_landing() {
    std::cout << "[Test] Landing Gear: Structural Failure on Excessive Sink Rate... ";

    const auto mass = fdm::FuelSystem::compute(1000.0);

    // Gentle touchdown: well inside limits.
    gear::LandingGear soft;
    fdm::FlightState s_soft = make_ground_state();
    s_soft.pos_ned.z = -1.59;             // Settled on the struts
    s_soft.vel_b = math::Vector3(70.0, 0.0, 1.5); // 1.5 m/s sink
    (void)soft.compute(s_soft, mass);
    assert(!soft.collapsed && "1.5 m/s touchdown must not break the gear");

    // Hard arrival: beyond the structural limit.
    gear::LandingGear hard;
    fdm::FlightState s_hard = make_ground_state();
    s_hard.pos_ned.z = -1.59;
    s_hard.vel_b = math::Vector3(70.0, 0.0, 7.0); // 7 m/s sink
    (void)hard.compute(s_hard, mass);
    assert(hard.collapsed && "7 m/s touchdown must exceed gear limits");

    // A collapsed gear stops producing reaction force.
    const auto f_after = hard.compute(s_hard, mass);
    assert(f_after.force_b.norm() == 0.0 && "collapsed gear carries no load");

    std::cout << "PASSED (1.5 m/s survived, 7.0 m/s collapsed at "
              << gear::LandingGear::GEAR_LIMIT_SINK_MPS << " m/s limit)\n";
}

void test_nosewheel_steering() {
    std::cout << "[Test] Landing Gear: Nosewheel Steering Authority... ";

    const auto mass = fdm::FuelSystem::compute(1000.0);
    fdm::RK4Integrator integrator(0.002);

    gear::LandingGear lg;
    fdm::FlightState state = make_ground_state(12.0); // Taxi speed
    lg.steer_cmd = 1.0;                               // Full right

    double t = 0.0;
    for (int i = 0; i < 3000; ++i) { // 6 s
        integrator.step(state, t, mass, [&](double, const fdm::FlightState& s) noexcept {
            return lg.compute(s, mass);
        });
    }

    // Steering right must produce a right (positive) yaw displacement.
    assert(state.yaw() > 0.01 && "full right steer must yaw the aircraft right");
    assert(lg.struts[0].steerable);

    std::cout << "PASSED (Taxi turn: heading " << state.yaw() * 180.0 / M_PI
              << " deg after 6 s at full right steer)\n";
}

} // namespace

int main() {
    std::cout << "=== Landing Gear & Ground Reaction Verification ===\n";

    test_static_equilibrium();
    test_no_force_when_airborne();
    test_retracted_gear_inert();
    test_braking_decelerates();
    test_gear_collapse_on_hard_landing();
    test_nosewheel_steering();

    std::cout << "All Landing Gear tests passed successfully!\n";
    return 0;
}
