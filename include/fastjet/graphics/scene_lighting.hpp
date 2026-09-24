#pragma once

#include "fastjet/graphics/shader.hpp"
#include <algorithm>
#include <array>
#include <cmath>

/// @file
/// @brief Per-frame lighting environment shared by every scene shader.
///
/// The sky shader evaluates the scattering model per pixel. Surfaces need a
/// few integrals of it (the colour of the sun after the atmosphere, the
/// cosine-weighted sky radiance a surface sees), which are the same for every
/// pixel in the frame, so they are computed here once on the CPU and uploaded
/// as uniforms. AtmosphereModel mirrors glsl::ATMOSPHERE exactly.

namespace fastjet::graphics {

/// @brief Linear RGB triple.
struct Rgb {
    float r = 0.0f, g = 0.0f, b = 0.0f;

    constexpr Rgb operator+(const Rgb& o) const noexcept { return {r + o.r, g + o.g, b + o.b}; }
    constexpr Rgb operator*(float s) const noexcept { return {r * s, g * s, b * s}; }
    constexpr Rgb operator*(const Rgb& o) const noexcept { return {r * o.r, g * o.g, b * o.b}; }
    [[nodiscard]] constexpr float luminance() const noexcept { return 0.2126f * r + 0.7152f * g + 0.0722f * b; }
};

/// @brief CPU mirror of the GLSL single-scattering sky (glsl::ATMOSPHERE).
class AtmosphereModel {
public:
    static constexpr Rgb   RAYLEIGH{5.8e-6f, 13.5e-6f, 33.1e-6f};
    static constexpr float MIE       = 21e-6f;
    static constexpr float SCALE_H   = 8500.0f;
    static constexpr float EXPOSURE  = 1.6f;
    static constexpr Rgb   SUN_TINT{1.0f, 0.96f, 0.90f};
    static constexpr float MIE_G     = 0.76f;
    static constexpr float MIN_MU    = 0.015f;
    static constexpr float PI        = 3.14159265f;

    /// Sun illuminance scale: the value the shaders used before the lighting
    /// model existed, kept so the scene's reflectances stay calibrated.
    static constexpr float SUN_ILLUMINANCE = 3.0f;

    /// @brief Sky radiance along a unit NED direction from an altitude.
    [[nodiscard]] static Rgb sky_radiance(const std::array<float, 3>& ray, const std::array<float, 3>& sun,
                                          float altitude_m) noexcept {
        const float mu = (std::max)(-ray[2], MIN_MU);
        const float cos_sun = std::clamp(ray[0] * sun[0] + ray[1] * sun[1] + ray[2] * sun[2], -1.0f, 1.0f);
        const float density = std::exp(-altitude_m / SCALE_H);
        const float air_mass = 1.0f / mu;

        const Rgb tau_r = RAYLEIGH * (SCALE_H * density);
        const float tau_m = MIE * SCALE_H * density;
        const float phase_r = 0.75f * (1.0f + cos_sun * cos_sun);
        const float g2 = MIE_G * MIE_G;
        const float phase_m = (1.0f - g2) / (4.0f * PI * std::pow(1.0f + g2 - 2.0f * MIE_G * cos_sun, 1.5f));

        auto channel = [&](float tr) {
            const float od = (tr + tau_m) * air_mass;
            const float atten = (1.0f - std::exp(-od)) / (std::max)(od, 1e-6f);
            return (tr * phase_r + tau_m * phase_m) * air_mass * atten * EXPOSURE;
        };
        return Rgb{channel(tau_r.r), channel(tau_r.g), channel(tau_r.b)} * SUN_TINT;
    }

    /// @brief Direct sun radiance scale reaching an altitude, relative to sea
    /// level, times the calibrated illuminance. Climbing above the aerosol
    /// and much of the Rayleigh column makes the sun whiter and brighter.
    [[nodiscard]] static Rgb sun_radiance(const std::array<float, 3>& sun, float altitude_m) noexcept {
        const float mu = (std::max)(-sun[2], MIN_MU);
        const float air_mass = 1.0f / mu;
        const float lost = 1.0f - std::exp(-altitude_m / SCALE_H); // fraction of column below the eye
        auto channel = [&](float tr) {
            const float tau_sl = (tr + MIE) * SCALE_H;
            return std::exp(tau_sl * lost * air_mass);
        };
        const Rgb gain{channel(RAYLEIGH.r), channel(RAYLEIGH.g), channel(RAYLEIGH.b)};
        return SUN_TINT * gain * SUN_ILLUMINANCE;
    }

    /// @brief Cosine-weighted mean sky radiance over the upper hemisphere,
    /// i.e. sky irradiance / pi on a level surface.
    [[nodiscard]] static Rgb sky_irradiance(const std::array<float, 3>& sun, float altitude_m) noexcept {
        constexpr int kAzimuth = 12;
        constexpr int kElevation = 6;
        Rgb sum{};
        float weight = 0.0f;
        for (int e = 0; e < kElevation; ++e) {
            // Stratified in cos^2 so each band carries equal projected solid angle.
            const float u = (static_cast<float>(e) + 0.5f) / kElevation;
            const float cos_t = std::sqrt(1.0f - u);
            const float sin_t = std::sqrt(u);
            for (int a = 0; a < kAzimuth; ++a) {
                const float phi = 2.0f * PI * (static_cast<float>(a) + 0.5f) / kAzimuth;
                const std::array<float, 3> dir{sin_t * std::cos(phi), sin_t * std::sin(phi), -cos_t};
                sum = sum + sky_radiance(dir, sun, altitude_m);
                weight += 1.0f;
            }
        }
        return sum * (1.0f / weight);
    }
};

/// @brief Everything a lit scene shader needs about the frame's light.
struct SceneLighting {
    std::array<float, 3> sun_dir{0.0f, 0.0f, -1.0f}; ///< NED, toward the sun
    Rgb sun{};            ///< Direct sun, already divided by pi (albedo * sun * N.L)
    Rgb sky_ambient{};    ///< Cosine-weighted sky radiance
    Rgb ground_ambient{}; ///< Radiance bounced from the ground
    float exposure = 1.35f;
    float altitude_m = 0.0f;

    /// Mean ground reflectance used for the bounce term (grass and fields).
    static constexpr Rgb GROUND_ALBEDO{0.085f, 0.105f, 0.060f};
    /// Scales the physical sky integral into the calibrated ambient range the
    /// scene's reflectances were tuned under.
    static constexpr float SKY_AMBIENT_GAIN = 1.25f;

    /// @brief Builds the lighting for an eye altitude and sun direction.
    [[nodiscard]] static SceneLighting compute(const std::array<float, 3>& sun_dir, float eye_altitude_m,
                                               float exposure) noexcept {
        SceneLighting l;
        l.sun_dir = sun_dir;
        l.altitude_m = eye_altitude_m;
        l.exposure = exposure;
        l.sun = AtmosphereModel::sun_radiance(sun_dir, eye_altitude_m);
        l.sky_ambient = AtmosphereModel::sky_irradiance(sun_dir, eye_altitude_m) * SKY_AMBIENT_GAIN;
        const float sun_up = (std::max)(-sun_dir[2], 0.0f);
        // Ground lit by the sun at sea level (not the eye's), plus the sky.
        const Rgb sun_ground = AtmosphereModel::sun_radiance(sun_dir, 0.0f);
        l.ground_ambient = GROUND_ALBEDO * (sun_ground * sun_up + l.sky_ambient);
        return l;
    }

    /// @brief Uploads the glsl::LIGHTING uniforms.
    void upload(const ShaderProgram& s) const noexcept {
        s.set_vec3("uSunDir", sun_dir[0], sun_dir[1], sun_dir[2]);
        s.set_vec3("uSunRadiance", sun.r, sun.g, sun.b);
        s.set_vec3("uSkyAmbient", sky_ambient.r, sky_ambient.g, sky_ambient.b);
        s.set_vec3("uGroundAmbient", ground_ambient.r, ground_ambient.g, ground_ambient.b);
        s.set_float("uExposure", exposure);
    }
};

/// @brief Cumulus layer parameters, shared by the cloud renderer and every
/// shader that receives cloud shadows (glsl::CLOUDS).
struct CloudSettings {
    /// Texture unit of the baked cloud map in every receiver shader.
    static constexpr int kMapUnit = 6;

    float coverage = 0.0f;   ///< 0 = clear sky, 1 = overcast
    float base_m = 2300.0f;  ///< Cloud base altitude [m]
    float top_m = 3300.0f;   ///< Cloud top altitude [m]
    float offset_x = 0.0f;   ///< Wind drift of the coverage field [m]
    float offset_y = 0.0f;

    // Baked field for shadows (CloudLayer::update_map). No map = no shadows.
    GLuint map_texture = 0;
    float map_origin_x = 0.0f; ///< Map corner in cloud space [m]
    float map_origin_y = 0.0f;
    float map_inv_size = 0.0f;

    void upload(const ShaderProgram& s) const noexcept {
        s.set_float("uCloudCoverage", map_texture ? coverage : 0.0f);
        s.set_float("uCloudBase", base_m);
        s.set_float("uCloudTop", top_m);
        s.set_vec2("uCloudOffset", offset_x, offset_y);
        glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(kMapUnit));
        glBindTexture(GL_TEXTURE_2D, map_texture);
        glActiveTexture(GL_TEXTURE0);
        s.set_int("uCloudMap", kMapUnit);
        s.set_vec3("uCloudRect", map_origin_x, map_origin_y, map_inv_size);
    }
};

} // namespace fastjet::graphics
