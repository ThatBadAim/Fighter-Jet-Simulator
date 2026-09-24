#pragma once

#include "fastjet/fdm/flight_state.hpp"
#include "fastjet/aircraft/aircraft_type.hpp"
#include "fastjet/aircraft/aircraft_config.hpp"
#include "fastjet/graphics/terrain_field.hpp"
#include "fastjet/math/vector3.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace fastjet::environment {

/// @brief Ground strike point identifier.
enum class GroundContactPoint {
    NONE = 0,
    BELLY,
    NOSE,
    TAIL,
    LEFT_WINGTIP,
    RIGHT_WINGTIP,
    GEAR_COLLAPSED
};

/// @brief Result of a ground collision query.
struct CollisionResult {
    bool has_collided{false};
    GroundContactPoint contact_point{GroundContactPoint::NONE};
    double penetration_depth{0.0};
    math::Vector3 contact_pos_ned{0.0, 0.0, 0.0};
    double terrain_elev_m{0.0};
};

/**
 * @brief High-fidelity full-terrain ground collision detection system.
 *
 * Checks airframe extremities (fuselage belly, nose cone, tail assembly,
 * left and right wingtips) against the deterministic procedural terrain field
 * (TerrainField) across the entire world map.
 *
 * Ensures collision happens everywhere in the world—including mountains,
 * valleys, hills, ridges, and the sea-level airfield basin.
 */
class GroundCollision {
public:
    static constexpr double BELLY_CLEARANCE = 0.90; // Fuselage belly clearance from CG [m]

    /// @brief Return terrain elevation above sea level [m] at horizontal position (x, y) [NED].
    [[nodiscard]] static double get_terrain_height(double x_ned, double y_ned) noexcept {
        return static_cast<double>(graphics::TerrainField::height(
            static_cast<float>(x_ned), static_cast<float>(y_ned)));
    }

    /// @brief Return terrain elevation in NED down coordinates (z is negative above sea level).
    [[nodiscard]] static double get_ground_z(double x_ned, double y_ned) noexcept {
        return -get_terrain_height(x_ned, y_ned);
    }

    /// @brief Return altitude Above Ground Level (AGL) [m].
    [[nodiscard]] static double get_agl(const fdm::FlightState& state) noexcept {
        const double h_terr = get_terrain_height(state.pos_ned.x, state.pos_ned.y);
        return std::max(0.0, state.altitude() - h_terr);
    }

    /// @brief Checks airframe collision against all terrain.
    ///
    /// @param state           Current flight state (position, attitude).
    /// @param type            Aircraft type (for dimensions and geometry).
    /// @param gear_deployed   Whether landing gear is down and locked.
    /// @param gear_collapsed  Whether landing gear has collapsed from overload.
    /// @return CollisionResult detailing if a strike occurred, which extremity struck, and penetration.
    [[nodiscard]] static CollisionResult check_collision(
        const fdm::FlightState& state,
        aircraft::AircraftType type,
        bool gear_deployed,
        bool gear_collapsed
    ) noexcept {
        CollisionResult res{};

        if (gear_collapsed) {
            res.has_collided = true;
            res.contact_point = GroundContactPoint::GEAR_COLLAPSED;
            res.terrain_elev_m = get_terrain_height(state.pos_ned.x, state.pos_ned.y);
            res.contact_pos_ned = state.pos_ned;
            return res;
        }

        const auto cfg = aircraft::AircraftConfig::get(type);

        // Define airframe key extremity points in body frame [m]
        // [0] Belly: bottom of the fuselage
        // [1] Nose: forward tip of radome
        // [2] Tail: engine nozzle / tailcone
        // [3] Left wingtip
        // [4] Right wingtip
        const double b_half = cfg.aero.b_span * 0.5;
        const double nose_x = cfg.gear.nose_pos_b.x + 1.8;
        const double tail_x = cfg.gear.main_l_pos_b.x - 5.5;

        struct TestPoint {
            GroundContactPoint id;
            math::Vector3 pos_b;
        };

        const std::array<TestPoint, 5> test_points = {{
            { GroundContactPoint::BELLY,         math::Vector3(0.0, 0.0, BELLY_CLEARANCE) },
            { GroundContactPoint::NOSE,          math::Vector3(nose_x, 0.0, 0.0) },
            { GroundContactPoint::TAIL,          math::Vector3(tail_x, 0.0, 0.0) },
            { GroundContactPoint::LEFT_WINGTIP,  math::Vector3(0.0, -b_half, 0.0) },
            { GroundContactPoint::RIGHT_WINGTIP, math::Vector3(0.0,  b_half, 0.0) }
        }};

        // If gear is retracted, airframe belly/wheels strike surface directly
        // If gear is deployed, wheels take ground loads, but airframe extremities
        // (belly, wingtips, nose, tail) must not penetrate the terrain.
        for (const auto& pt : test_points) {
            const math::Vector3 pt_ned_offset = state.q_att.rotate_body_to_ned(pt.pos_b);
            const math::Vector3 pt_ned = state.pos_ned + pt_ned_offset;

            const double terr_h = get_terrain_height(pt_ned.x, pt_ned.y);
            const double ground_z = -terr_h;

            // In NED, z is positive downwards. If pt_ned.z >= ground_z, the point is at or below ground.
            const double penetration = pt_ned.z - ground_z;
            if (penetration >= 0.0) {
                if (penetration > res.penetration_depth) {
                    res.has_collided = true;
                    res.contact_point = pt.id;
                    res.penetration_depth = penetration;
                    res.contact_pos_ned = pt_ned;
                    res.terrain_elev_m = terr_h;
                }
            }
        }

        // Retracted gear belly check at CG location
        if (!gear_deployed) {
            const double terr_h = get_terrain_height(state.pos_ned.x, state.pos_ned.y);
            const double ground_z = -terr_h;
            const double belly_z = state.pos_ned.z + BELLY_CLEARANCE;
            const double penetration = belly_z - ground_z;
            if (penetration >= 0.0 && penetration > res.penetration_depth) {
                res.has_collided = true;
                res.contact_point = GroundContactPoint::BELLY;
                res.penetration_depth = penetration;
                res.contact_pos_ned = math::Vector3(state.pos_ned.x, state.pos_ned.y, belly_z);
                res.terrain_elev_m = terr_h;
            }
        }

        return res;
    }

    /// @brief Clamps a crashed aircraft state so it cleanly rests on top of the terrain at its position.
    static void clamp_to_surface(fdm::FlightState& state) noexcept {
        const double terr_h = get_terrain_height(state.pos_ned.x, state.pos_ned.y);
        const double ground_z = -terr_h;
        // In NED, resting on top of ground means z = ground_z - BELLY_CLEARANCE.
        const double resting_z = ground_z - BELLY_CLEARANCE;
        if (state.pos_ned.z > resting_z) {
            state.pos_ned.z = resting_z;
        }
        state.vel_b = math::Vector3::zero();
        state.omega_b = math::Vector3::zero();
    }
};

} // namespace fastjet::environment
