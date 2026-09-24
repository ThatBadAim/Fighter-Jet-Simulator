#pragma once

#include "fastjet/graphics/scene_lighting.hpp"
#include "fastjet/graphics/shadow_map.hpp"
#include "fastjet/math/vector3.hpp"

namespace fastjet::graphics {

/// @brief Everything the scene passes share for one frame.
///
/// Built once by RenderEngine and handed to each renderer, so the sky,
/// terrain, airfield, airframe, clouds and cockpit all light from the same
/// sun, clouds and shadow map.
struct FrameContext {
    /// Texture unit reserved for the sun shadow map in every receiver shader,
    /// clear of the material units (0-3) so sampler types never alias.
    static constexpr int kShadowUnit = 5;

    SceneLighting lighting{};
    CloudSettings clouds{};
    const ShadowMap* shadow = nullptr;
    math::Vector3 eye_ned{0.0, 0.0, 0.0};
    bool hdr_output = true;    ///< Scene target is HDR (post chain tonemaps)
    float time_s = 0.0f;       ///< Monotonic render time [s], for animated effects
    float ground_shadow = 1.0f; ///< ShadowMap::ground_strength() for this frame

    /// @brief Uploads output mode, lighting and cloud uniforms.
    void upload(const ShaderProgram& s) const noexcept {
        s.set_int("uHdrOutput", hdr_output ? 1 : 0);
        lighting.upload(s);
        clouds.upload(s);
        s.set_int("uFrameIsBody", 0);
    }

    /// @brief Binds the shadow map for a receiver.
    /// @param strength Receiver fade in [0, 1].
    /// @param frame_origin World position the shader's positions are relative
    ///        to (zero for shaders that work in absolute world coordinates).
    void bind_shadow(const ShaderProgram& s, float strength,
                     const math::Vector3& frame_origin = math::Vector3(0.0, 0.0, 0.0)) const noexcept {
        if (shadow) {
            shadow->bind_receiver(s, kShadowUnit, strength, shadow->receiver_matrix(frame_origin));
        } else {
            s.set_int("uShadowMap", kShadowUnit);
            s.set_int("uShadowEnabled", 0);
        }
    }
};

} // namespace fastjet::graphics
