#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace fastjet::graphics {

/// @brief Deterministic procedural terrain heightfield.
///
/// Fractional Brownian motion over a hash-based value-noise lattice. The field
/// is a pure function of position, so it needs no storage, is identical on every
/// machine and every run, and can be evaluated by the mesh builder and by any
/// future ground-collision query without the two drifting apart.
///
/// The airfield sits in a deliberately flat basin around the runway so that
/// takeoff and landing stay usable; relief grows with distance from the field.
class TerrainField {
public:
    // Runway occupies X = [0, 3000] m, Y = [-30, +30] m in NED. The basin is
    // centred on the midpoint and blends out over BASIN_FADE metres.
    static constexpr float FIELD_CENTER_X = 1500.0f;
    static constexpr float FIELD_CENTER_Y = 0.0f;
    static constexpr float BASIN_RADIUS   = 2600.0f; // [m] fully flat inside
    static constexpr float BASIN_FADE     = 5200.0f; // [m] blend to full relief

    // Relief shaping.
    static constexpr float BASE_FREQ   = 1.0f / 9000.0f; // [1/m] largest landform
    static constexpr float BASE_AMP    = 620.0f;         // [m] peak-to-trough scale
    static constexpr int   OCTAVES     = 5;
    static constexpr float LACUNARITY  = 2.13f; // non-integer avoids axis-aligned repeats
    static constexpr float GAIN        = 0.47f;
    // Fraction of the ridged range treated as low ground (sea-level plain).
    static constexpr float PLAIN_BIAS  = 0.45f;

    /// @brief Terrain elevation above mean sea level [m], as a function of
    ///        horizontal NED position. Always >= 0 near the airfield.
    [[nodiscard]] static float height(float x, float y) noexcept {
        const float relief = fbm(x * BASE_FREQ, y * BASE_FREQ);

        // Ridged transform: sharpens ridges into plausible mountain spines
        // rather than the rolling blobs plain fBm produces. Cubing alone biases
        // the whole field upward (a high plateau), so bias back down and clamp:
        // most of the map should be low ground with peaks standing out of it.
        const float ridged = 1.0f - std::fabs(relief);
        const float shaped = ridged * ridged * ridged;
        const float floored = (std::max)(0.0f, shaped - PLAIN_BIAS) / (1.0f - PLAIN_BIAS);

        return floored * BASE_AMP * basin_weight(x, y);
    }

    /// @brief Analytic-ish surface normal in NED (points up, so z is negative).
    /// Central differences over a fixed span; cheap and stable at mesh scale.
    static void normal_ned(float x, float y, float span, float& nx, float& ny, float& nz) noexcept {
        const float hl = height(x - span, y);
        const float hr = height(x + span, y);
        const float hd = height(x, y - span);
        const float hu = height(x, y + span);

        // Gradient of elevation. NED z is down, so "up" carries a negative z.
        const float dzdx = (hr - hl) / (2.0f * span);
        const float dzdy = (hu - hd) / (2.0f * span);

        nx = -dzdx;
        ny = -dzdy;
        nz = -1.0f;

        const float inv_len = 1.0f / std::sqrt(nx * nx + ny * ny + 1.0f);
        nx *= inv_len;
        ny *= inv_len;
        nz *= inv_len;
    }

    /// @brief Weight in [0, 1] scaling relief away from the airfield basin.
    [[nodiscard]] static float basin_weight(float x, float y) noexcept {
        const float dx = x - FIELD_CENTER_X;
        const float dy = y - FIELD_CENTER_Y;
        const float dist = std::sqrt(dx * dx + dy * dy);

        if (dist <= BASIN_RADIUS) return 0.0f;
        if (dist >= BASIN_RADIUS + BASIN_FADE) return 1.0f;

        const float t = (dist - BASIN_RADIUS) / BASIN_FADE;
        return smoothstep(t);
    }

private:
    [[nodiscard]] static constexpr float smoothstep(float t) noexcept {
        return t * t * (3.0f - 2.0f * t);
    }

    /// @brief Integer hash to [-1, 1]. Deterministic across platforms because it
    /// works entirely in uint32 with defined wrapping.
    [[nodiscard]] static float hash2(int32_t ix, int32_t iy) noexcept {
        uint32_t h = static_cast<uint32_t>(ix) * 0x8DA6B343u ^
                     static_cast<uint32_t>(iy) * 0xD8163841u;
        h ^= h >> 15;
        h *= 0x2C1B3C6Du;
        h ^= h >> 12;
        h *= 0x297A2D39u;
        h ^= h >> 15;
        // Map to [-1, 1]
        return static_cast<float>(h & 0x00FFFFFFu) / 8388608.0f - 1.0f;
    }

    /// @brief Smooth value noise on the integer lattice.
    [[nodiscard]] static float value_noise(float x, float y) noexcept {
        const float fx = std::floor(x);
        const float fy = std::floor(y);
        const auto ix = static_cast<int32_t>(fx);
        const auto iy = static_cast<int32_t>(fy);

        const float tx = smoothstep(x - fx);
        const float ty = smoothstep(y - fy);

        const float c00 = hash2(ix,     iy);
        const float c10 = hash2(ix + 1, iy);
        const float c01 = hash2(ix,     iy + 1);
        const float c11 = hash2(ix + 1, iy + 1);

        const float a = c00 + (c10 - c00) * tx;
        const float b = c01 + (c11 - c01) * tx;
        return a + (b - a) * ty;
    }

    /// @brief Fractional Brownian motion: octaves of value noise, normalised to
    /// roughly [-1, 1] regardless of octave count.
    [[nodiscard]] static float fbm(float x, float y) noexcept {
        float sum = 0.0f;
        float amp = 1.0f;
        float norm = 0.0f;
        float px = x;
        float py = y;

        for (int i = 0; i < OCTAVES; ++i) {
            sum += amp * value_noise(px, py);
            norm += amp;
            amp *= GAIN;
            px *= LACUNARITY;
            py *= LACUNARITY;
            // Offset each octave so lattices do not align at the origin.
            px += 71.3f;
            py += ula_offset(i);
        }

        return sum / norm;
    }

    [[nodiscard]] static constexpr float ula_offset(int i) noexcept {
        return 13.7f + static_cast<float>(i) * 29.1f;
    }
};

} // namespace fastjet::graphics
