#include "fastjet/environment/ground_collision.hpp"
#include "fastjet/gear/landing_gear.hpp"
#include "fastjet/fdm/fuel_system.hpp"
#include "fastjet/aircraft/aircraft_type.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <iomanip>

using namespace fastjet;
using namespace fastjet::environment;

void test_airfield_flat_collision() {
    std::cout << "[Test] Ground Collision: Airfield Basin Collision Detection... ";

    fdm::FlightState s{};
    s.pos_ned = math::Vector3(0.0, 0.0, -100.0); // 100m altitude over runway
    s.q_att = math::Quaternion::identity();

    // 1. Airborne with gear down: no collision
    auto res = GroundCollision::check_collision(s, aircraft::AircraftType::F16_FIGHTING_FALCON, true, false);
    assert(!res.has_collided);

    // 2. Airborne with gear up: no collision
    res = GroundCollision::check_collision(s, aircraft::AircraftType::F16_FIGHTING_FALCON, false, false);
    assert(!res.has_collided);

    // 3. Resting stance on gear: no collision (wheels support airframe)
    s.pos_ned = math::Vector3(0.0, 0.0, -1.59);
    res = GroundCollision::check_collision(s, aircraft::AircraftType::F16_FIGHTING_FALCON, true, false);
    assert(!res.has_collided);

    // 4. Belly on ground with gear retracted: collision
    s.pos_ned = math::Vector3(0.0, 0.0, -0.50); // CG at 0.5m, belly extends to +0.9m -> 1.4m (> 0.0m ground)
    res = GroundCollision::check_collision(s, aircraft::AircraftType::F16_FIGHTING_FALCON, false, false);
    assert(res.has_collided);
    assert(res.contact_point == GroundContactPoint::BELLY);
    assert(res.penetration_depth > 0.0);

    // 5. Gear collapsed: immediate collision
    res = GroundCollision::check_collision(s, aircraft::AircraftType::F16_FIGHTING_FALCON, true, true);
    assert(res.has_collided);
    assert(res.contact_point == GroundContactPoint::GEAR_COLLAPSED);

    std::cout << "PASSED\n";
}

void test_mountain_elevation_collision() {
    std::cout << "[Test] Ground Collision: Mountain & Elevated Terrain Collision... ";

    // Search for a mountain location far from the airfield
    double peak_x = 30000.0;
    double peak_y = 0.0;
    double max_h = 0.0;

    for (double x = 20000.0; x <= 45000.0; x += 500.0) {
        for (double y = -20000.0; y <= 20000.0; y += 500.0) {
            const double h = GroundCollision::get_terrain_height(x, y);
            if (h > max_h) {
                max_h = h;
                peak_x = x;
                peak_y = y;
            }
        }
    }

    assert(max_h > 200.0 && "Must find significant mountain relief");

    fdm::FlightState s{};
    s.pos_ned = math::Vector3(peak_x, peak_y, -max_h - 100.0); // 100m above peak
    s.q_att = math::Quaternion::identity();

    // 1. Clear above mountain peak: no collision
    auto res = GroundCollision::check_collision(s, aircraft::AircraftType::F16_FIGHTING_FALCON, false, false);
    assert(!res.has_collided);
    assert(GroundCollision::get_agl(s) > 90.0);

    // 2. Flying into the mountain (altitude below mountain peak): collision!
    // Altitude is max_h - 20m, meaning the aircraft is 20m inside the mountain!
    s.pos_ned = math::Vector3(peak_x, peak_y, -max_h + 20.0);
    res = GroundCollision::check_collision(s, aircraft::AircraftType::F16_FIGHTING_FALCON, false, false);
    assert(res.has_collided);
    assert(res.penetration_depth >= 20.0);
    assert(std::abs(res.terrain_elev_m - max_h) < 0.01);

    std::cout << "PASSED (collided at peak " << max_h << " m MSL)\n";
}

void test_wingtip_strike_detection() {
    std::cout << "[Test] Ground Collision: Wingtip Strike During Steep Bank... ";

    // Place aircraft 2.5m above flat ground with gear up
    fdm::FlightState s{};
    s.pos_ned = math::Vector3(0.0, 0.0, -2.5);

    // Wings level: belly is at z = -2.5 + 0.9 = -1.6 (clear of ground z = 0.0)
    s.q_att = math::Quaternion::identity();
    auto res = GroundCollision::check_collision(s, aircraft::AircraftType::F16_FIGHTING_FALCON, false, false);
    assert(!res.has_collided);

    // Bank right 60 degrees: right wingtip drops by b_half * sin(60) = 4.572 * 0.866 = 3.96m
    // Wingtip NED z = -2.5 + 3.96 = +1.46m (penetrates ground by 1.46m!)
    s.q_att = math::Quaternion::from_euler(60.0 * (M_PI / 180.0), 0.0, 0.0);
    res = GroundCollision::check_collision(s, aircraft::AircraftType::F16_FIGHTING_FALCON, true, false);
    assert(res.has_collided);
    assert(res.contact_point == GroundContactPoint::RIGHT_WINGTIP);
    assert(res.penetration_depth > 1.0);

    // Bank left 60 degrees: left wingtip strikes ground
    s.q_att = math::Quaternion::from_euler(-60.0 * (M_PI / 180.0), 0.0, 0.0);
    res = GroundCollision::check_collision(s, aircraft::AircraftType::F16_FIGHTING_FALCON, true, false);
    assert(res.has_collided);
    assert(res.contact_point == GroundContactPoint::LEFT_WINGTIP);
    assert(res.penetration_depth > 1.0);

    std::cout << "PASSED (bank angle wingtip strike detected)\n";
}

void test_nose_and_tail_strike_detection() {
    std::cout << "[Test] Ground Collision: Nose & Tail Strike Detection... ";

    fdm::FlightState s{};
    s.pos_ned = math::Vector3(0.0, 0.0, -2.5);

    // Pitch down 35 degrees: nose dips by nose_x * sin(35) = 5.4 * 0.573 = 3.1m
    // Nose NED z = -2.5 + 3.1 = +0.6m (penetrates ground)
    s.q_att = math::Quaternion::from_euler(0.0, -35.0 * (M_PI / 180.0), 0.0);
    auto res = GroundCollision::check_collision(s, aircraft::AircraftType::F16_FIGHTING_FALCON, true, false);
    assert(res.has_collided);
    assert(res.contact_point == GroundContactPoint::NOSE);

    // Pitch up 35 degrees: tail strikes ground
    s.q_att = math::Quaternion::from_euler(0.0, 35.0 * (M_PI / 180.0), 0.0);
    res = GroundCollision::check_collision(s, aircraft::AircraftType::F16_FIGHTING_FALCON, true, false);
    assert(res.has_collided);
    assert(res.contact_point == GroundContactPoint::TAIL);

    std::cout << "PASSED\n";
}

static void find_mountain_point(double min_elevation, double& out_x, double& out_y, double& out_h) {
    for (double x = 20000.0; x <= 45000.0; x += 500.0) {
        for (double y = -20000.0; y <= 20000.0; y += 500.0) {
            const double h = GroundCollision::get_terrain_height(x, y);
            if (h >= min_elevation) {
                out_x = x;
                out_y = y;
                out_h = h;
                return;
            }
        }
    }
    out_x = 0.0;
    out_y = 0.0;
    out_h = 0.0;
}

void test_landing_gear_terrain_support() {
    std::cout << "[Test] Ground Collision: Landing Gear Support on Mountain Terrain... ";

    double hill_x = 0.0, hill_y = 0.0, hill_h = 0.0;
    find_mountain_point(200.0, hill_x, hill_y, hill_h);
    assert(hill_h >= 200.0 && "Terrain must have non-zero elevation");

    gear::LandingGear lg(aircraft::AircraftType::F16_FIGHTING_FALCON);
    const auto mass = fdm::FuelSystem::compute(0.0, aircraft::AircraftType::F16_FIGHTING_FALCON);

    // Place aircraft settled on gear on the mountain: z = -hill_h - 1.59
    fdm::FlightState s{};
    s.pos_ned = math::Vector3(hill_x, hill_y, -hill_h - 1.59);
    s.q_att = math::Quaternion::identity();

    const auto forces = lg.compute(s, mass);

    // Wheels must contact the elevated ground and generate upward normal force (-Z)
    assert(lg.weight_on_wheels());
    assert(forces.force_b.z < -50000.0 && "Ground reaction must push upward on mountain");

    std::cout << "PASSED (struts compressed against mountain at " << hill_h << " m MSL)\n";
}

void test_post_crash_clamping() {
    std::cout << "[Test] Ground Collision: Post-Crash Terrain Clamping... ";

    double hill_x = 0.0, hill_y = 0.0, hill_h = 0.0;
    find_mountain_point(250.0, hill_x, hill_y, hill_h);
    assert(hill_h >= 250.0);

    fdm::FlightState s{};
    // Penetrated 30m inside mountain
    s.pos_ned = math::Vector3(hill_x, hill_y, -hill_h + 30.0);
    s.vel_b = math::Vector3(150.0, 10.0, -5.0);
    s.omega_b = math::Vector3(0.5, -0.2, 0.1);

    GroundCollision::clamp_to_surface(s);

    // Must be clamped to the mountain surface minus BELLY_CLEARANCE
    const double expected_z = -hill_h - GroundCollision::BELLY_CLEARANCE;
    assert(std::abs(s.pos_ned.z - expected_z) < 1e-6);
    assert(s.vel_b.norm_squared() == 0.0);
    assert(s.omega_b.norm_squared() == 0.0);

    std::cout << "PASSED (aircraft rested on mountain surface at " << hill_h << " m MSL)\n";
}

int main() {
    std::cout << "=== Fast Jet Simulator: Comprehensive Ground Collision Tests ===\n";
    test_airfield_flat_collision();
    test_mountain_elevation_collision();
    test_wingtip_strike_detection();
    test_nose_and_tail_strike_detection();
    test_landing_gear_terrain_support();
    test_post_crash_clamping();
    std::cout << "All Ground Collision tests passed successfully!\n";
    return 0;
}
