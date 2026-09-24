#pragma once

#include "fastjet/ui/user_settings.hpp"
#include <array>
#include <cstddef>

namespace fastjet::graphics {

/// @brief Renderer feature levels for one quality preset.
///
/// Everything here applies live: the scene target, shadow map and effects
/// are owned by the renderer, unlike the window's own multisampling (which
/// only the cockpit and HUD use now, and which needs a restart).
struct RenderQuality {
    int scene_samples = 4;      ///< MSAA on the HDR scene target
    int shadow_map_size = 2048; ///< Sun shadow map edge [texels], 0 = no shadows
    bool bloom = true;
    bool clouds = true;         ///< Cumulus deck and its shadows
    bool cirrus = true;         ///< High ice cloud in the sky dome
    bool exhaust = true;        ///< Afterburner plume and nozzle glow

    [[nodiscard]] static RenderQuality from_preset(ui::QualityPreset preset) noexcept {
        // LOW keeps the lit, shadowed world but drops the costly extras;
        // ULTRA stops at 4x scene MSAA, where the float target's bandwidth
        // starts to cost more than the edges gain on integrated GPUs.
        constexpr std::array<RenderQuality, static_cast<size_t>(ui::QualityPreset::COUNT)> kPresets = {{
            {0, 1024, false, true, false, true},  // LOW
            {2, 1024, true, true, true, true},    // MEDIUM
            {4, 2048, true, true, true, true},    // HIGH
            {4, 4096, true, true, true, true},    // ULTRA
        }};
        const auto i = static_cast<size_t>(preset);
        return i < kPresets.size() ? kPresets[i] : kPresets.back();
    }

    constexpr bool operator==(const RenderQuality&) const noexcept = default;
};

/// @brief Sky conditions. One fixed scattered-cumulus day for now; kept as
/// data so weather can be driven by settings or missions later.
struct WeatherState {
    float cumulus_coverage = 0.32f; ///< Fraction of sky with cumulus
    float cumulus_base_m = 2300.0f;
    float cumulus_top_m = 3300.0f;
    float cirrus_cover = 0.45f;
    float wind_north_mps = 6.0f;    ///< Drift of the cloud field
    float wind_east_mps = 9.0f;
};

} // namespace fastjet::graphics
