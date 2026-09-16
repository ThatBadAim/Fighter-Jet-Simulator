#include "fastjet/graphics/terrain_field.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <vector>

using fastjet::graphics::TerrainField;

/// The airfield must be dead flat, or the runway decals float above or sink
/// into the terrain and takeoff/landing become unpredictable.
void test_airfield_basin_is_flat() {
    std::cout << "[Test] Terrain: Airfield Basin Is Exactly Flat... ";

    // Whole runway plus the apron margin.
    for (float x = -750.0f; x <= 3750.0f; x += 25.0f) {
        for (float y = -1000.0f; y <= 1000.0f; y += 25.0f) {
            const float h = TerrainField::height(x, y);
            assert(h == 0.0f && "Airfield footprint must be perfectly flat");
        }
    }

    std::cout << "PASSED (runway and apron at exactly 0 m)\n";
}

/// Relief must appear away from the field, otherwise the world is a plate.
void test_relief_grows_away_from_field() {
    std::cout << "[Test] Terrain: Relief Develops Away From The Airfield... ";

    // Inside the basin radius: no relief at all.
    assert(TerrainField::basin_weight(1500.0f, 0.0f) == 0.0f);
    assert(TerrainField::basin_weight(1500.0f, 2000.0f) == 0.0f);

    // Far away: full relief.
    assert(TerrainField::basin_weight(1500.0f, 40000.0f) == 1.0f);

    // The blend must be monotonic, so there is no visible ring or step where
    // the flat pad meets the hills.
    float prev = -1.0f;
    for (float d = 0.0f; d <= 12000.0f; d += 100.0f) {
        const float w = TerrainField::basin_weight(1500.0f + d, 0.0f);
        assert(w >= prev - 1e-6f && "Basin blend must be monotonic");
        assert(w >= 0.0f && w <= 1.0f);
        prev = w;
    }

    // Sample a wide area far from the field and require real variation.
    float min_h = 1e9f, max_h = -1e9f;
    for (int i = -80; i <= 80; ++i) {
        for (int j = -80; j <= 80; ++j) {
            const float h = TerrainField::height(30000.0f + static_cast<float>(i) * 400.0f,
                                                 static_cast<float>(j) * 400.0f);
            min_h = std::fmin(min_h, h);
            max_h = std::fmax(max_h, h);
        }
    }
    assert(max_h > 200.0f && "Distant terrain must have real hills");
    assert(min_h < 20.0f && "Distant terrain must also have low ground");

    std::printf("PASSED (relief %.0f - %.0f m away from field) ", min_h, max_h);
    std::cout << "\n";
}

/// The field is sampled by the mesh builder and could later be sampled by a
/// collision query. If it were not a pure function the two would disagree.
void test_field_is_deterministic() {
    std::cout << "[Test] Terrain: Height Field Is Deterministic... ";

    for (int i = 0; i < 500; ++i) {
        const float x = static_cast<float>(i) * 137.0f - 20000.0f;
        const float y = static_cast<float>(i) * 91.0f - 15000.0f;
        const float a = TerrainField::height(x, y);
        const float b = TerrainField::height(x, y);
        assert(a == b && "Height must be a pure function of position");
    }

    std::cout << "PASSED (repeated sampling is bit-identical)\n";
}

/// Elevation must never go below sea level: the renderer has no water, and
/// negative ground would appear as holes the aircraft could fly through.
void test_height_is_non_negative_and_bounded() {
    std::cout << "[Test] Terrain: Height Stays Within Physical Bounds... ";

    float max_h = 0.0f;
    for (int i = -150; i <= 150; ++i) {
        for (int j = -150; j <= 150; ++j) {
            const float h = TerrainField::height(static_cast<float>(i) * 700.0f,
                                                 static_cast<float>(j) * 700.0f);
            assert(h >= 0.0f && "Terrain must never dip below sea level");
            assert(std::isfinite(h) && "Terrain height must be finite");
            max_h = std::fmax(max_h, h);
        }
    }
    assert(max_h <= TerrainField::BASE_AMP + 1.0f && "Height must respect BASE_AMP");

    std::printf("PASSED (peak %.0f m, cap %.0f m) ", max_h, TerrainField::BASE_AMP);
    std::cout << "\n";
}

/// Normals drive the lighting. A non-unit or upside-down normal shows up
/// instantly as black or inverted hillsides.
void test_normals_are_unit_and_upward() {
    std::cout << "[Test] Terrain: Surface Normals Are Unit And Point Up... ";

    for (int i = 0; i < 400; ++i) {
        const float x = static_cast<float>(i) * 233.0f - 30000.0f;
        const float y = static_cast<float>(i) * 179.0f - 25000.0f;

        float nx, ny, nz;
        TerrainField::normal_ned(x, y, 60.0f, nx, ny, nz);

        const float len = std::sqrt(nx * nx + ny * ny + nz * nz);
        assert(std::fabs(len - 1.0f) < 1e-4f && "Normal must be unit length");

        // NED: up is -Z, so an upward normal has a negative z component.
        assert(nz < 0.0f && "Terrain normal must point upward (negative z in NED)");
    }

    // On the flat airfield the normal must be exactly straight up.
    float nx, ny, nz;
    TerrainField::normal_ned(1500.0f, 0.0f, 60.0f, nx, ny, nz);
    assert(std::fabs(nx) < 1e-5f && std::fabs(ny) < 1e-5f);
    assert(std::fabs(nz + 1.0f) < 1e-5f);

    std::cout << "PASSED (unit length, upward, flat over the runway)\n";
}

/// A steep slope must produce a normal tilted away from vertical, otherwise
/// the elevation palette's rock-on-slope rule never triggers.
void test_slope_produces_tilted_normal() {
    std::cout << "[Test] Terrain: Slopes Tilt The Normal... ";

    float max_tilt = 0.0f;
    for (int i = 0; i < 3000; ++i) {
        const float x = 25000.0f + static_cast<float>(i) * 37.0f;
        const float y = 9000.0f + static_cast<float>(i) * 23.0f;
        float nx, ny, nz;
        TerrainField::normal_ned(x, y, 60.0f, nx, ny, nz);
        max_tilt = std::fmax(max_tilt, 1.0f + nz); // 0 when vertical
    }

    assert(max_tilt > 0.02f && "Terrain must contain genuinely sloped ground");

    std::printf("PASSED (max tilt metric %.3f) ", max_tilt);
    std::cout << "\n";
}

int main() {
    std::cout << "=== Procedural Terrain Field Verification ===\n";
    test_airfield_basin_is_flat();
    test_relief_grows_away_from_field();
    test_field_is_deterministic();
    test_height_is_non_negative_and_bounded();
    test_normals_are_unit_and_upward();
    test_slope_produces_tilted_normal();
    std::cout << "All Terrain Field tests passed successfully!\n\n";
    return 0;
}
