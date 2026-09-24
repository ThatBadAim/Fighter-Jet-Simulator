#pragma once

#include "fastjet/graphics/gl_common.hpp"
#include "fastjet/graphics/shader.hpp"
#include <array>
#include <cmath>

namespace fastjet::graphics {

/// @brief Directional sun shadow map fitted around the aircraft.
///
/// The aircraft is the only shadow caster in the world (terrain relief is
/// gentle and the sun is high), so a single tight orthographic map around the
/// airframe gives centimetre-scale self-shadowing: the canopy frame across the
/// spine, the fin across the fuselage, the wings on the ground under a parked
/// jet. Because the projection runs along the sun direction, ground far below
/// an airborne jet still maps onto the airframe's texels; receivers fade that
/// case out themselves (uShadowStrength) since real penumbrae widen with height.
class ShadowMap {
public:
    /// Half-width of the fitted box [m]: the F-16 spans ~15 m nose to nozzle.
    static constexpr float kFocusRadius = 11.0f;
    /// Depth range either side of the focus along the light [m]. Deep enough
    /// to reach the ground under a jet until its shadow has faded out
    /// (ground_strength() is zero by 350 m), shallow enough to keep 24-bit
    /// depth precision far below a millimetre.
    static constexpr float kDepthRange = 600.0f;
    /// Receiver depth bias [m]: absorbs depth quantisation and the PCF
    /// kernel's reach across curved surfaces without detaching shadows.
    static constexpr float kReceiverBias = 0.025f;

    ShadowMap() = default;
    ~ShadowMap() { destroy(); }
    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;

    /// @brief Allocates the depth texture. Size 0 disables shadows.
    bool init(int size) {
        destroy();
        size_ = size;
        if (size_ <= 0) return true;

        glGenTextures(1, &depth_tex_);
        glBindTexture(GL_TEXTURE_2D, depth_tex_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, size_, size_, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        // Hardware depth comparison with bilinear filtering = free 2x2 PCF.
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        glBindTexture(GL_TEXTURE_2D, 0);

        glGenFramebuffers(1, &fbo_);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_tex_, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        const bool ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (!ok) destroy();
        return ok;
    }

    void destroy() noexcept {
        if (fbo_) { glDeleteFramebuffers(1, &fbo_); fbo_ = 0; }
        if (depth_tex_) { glDeleteTextures(1, &depth_tex_); depth_tex_ = 0; }
        size_ = 0;
        valid_ = false;
    }

    [[nodiscard]] bool enabled() const noexcept { return fbo_ != 0; }
    [[nodiscard]] int size() const noexcept { return size_; }

    /// @brief Fits the light projection around a focus point.
    /// @param sun_dir Unit NED vector toward the sun.
    /// @param focus_ned Centre of the region that casts shadows [m]. It also
    ///        becomes the map's origin: casters and receivers pass positions
    ///        relative to it (see caster_matrix()), which keeps millimetre
    ///        precision 100 km from the world origin.
    void fit(const std::array<float, 3>& sun_dir, const math::Vector3& focus_ned) noexcept {
        origin_ = focus_ned;
        light_view_proj_ = compute_light_view_proj(sun_dir, focus_ned, size_);
    }

    /// @brief Binds the map for the caster pass and clears it.
    void begin() const noexcept {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
        glViewport(0, 0, size_, size_);
        glDepthMask(GL_TRUE);
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        // Slope-scaled bias on the casters keeps lit faces from self-shadowing.
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(2.0f, 4.0f);
        glDisable(GL_CULL_FACE); // The model is double-sided
    }

    void end() noexcept {
        glDisable(GL_POLYGON_OFFSET_FILL);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        valid_ = true;
    }

    /// @brief Marks the map stale (e.g. nothing was rendered this frame).
    void invalidate() noexcept { valid_ = false; }

    /// @brief World position the map's coordinates are relative to.
    [[nodiscard]] const math::Vector3& origin() const noexcept { return origin_; }

    /// @brief Light clip matrix for positions expressed relative to `frame_origin`.
    ///
    /// The difference to the map origin is taken in double precision, so the
    /// resulting float matrix only ever carries small translations.
    [[nodiscard]] Mat4 caster_matrix(const math::Vector3& frame_origin) const noexcept {
        const math::Vector3 d = frame_origin - origin_;
        return light_view_proj_ * Mat4::translate(static_cast<float>(d.x), static_cast<float>(d.y),
                                                  static_cast<float>(d.z));
    }

    /// @brief Shadow texture matrix for receiver positions relative to `frame_origin`.
    [[nodiscard]] Mat4 receiver_matrix(const math::Vector3& frame_origin) const noexcept {
        Mat4 bias = Mat4::identity();
        bias(0, 0) = bias(1, 1) = bias(2, 2) = 0.5f;
        bias(0, 3) = bias(1, 3) = bias(2, 3) = 0.5f;
        return bias * caster_matrix(frame_origin);
    }

    /// @brief Binds the depth texture to a unit and sets the glsl::SHADOW
    /// uniforms on a receiver shader.
    /// @param receiver From receiver_matrix(), for the shader's position frame.
    /// @param strength Receiver fade in [0, 1] (penumbra with height).
    void bind_receiver(const ShaderProgram& s, int unit, float strength, const Mat4& receiver) const noexcept {
        const bool on = valid_ && enabled() && strength > 0.0f;
        glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(unit));
        glBindTexture(GL_TEXTURE_2D, on ? depth_tex_ : 0);
        glActiveTexture(GL_TEXTURE0);
        s.set_int("uShadowMap", unit);
        s.set_int("uShadowEnabled", on ? 1 : 0);
        s.set_mat4("uShadowMatrix", receiver);
        s.set_float("uShadowTexel", size_ > 0 ? 1.0f / static_cast<float>(size_) : 0.0f);
        // Metres to normalised depth over the map's 2 * kDepthRange span.
        s.set_float("uShadowBias", kReceiverBias / (2.0f * kDepthRange));
        s.set_float("uShadowStrength", strength);
    }

    /// @brief Ground receivers: the shadow of a jet high above the ground is
    /// all penumbra (the sun is 0.53 deg wide), so it fades out with height.
    [[nodiscard]] static float ground_strength(double height_above_ground_m) noexcept {
        constexpr double kFullBelow = 25.0;  // [m] crisp shadow up to here
        constexpr double kGoneAbove = 350.0; // [m] no discernible shadow
        if (height_above_ground_m <= kFullBelow) return 1.0f;
        if (height_above_ground_m >= kGoneAbove) return 0.0f;
        const double t = (height_above_ground_m - kFullBelow) / (kGoneAbove - kFullBelow);
        return static_cast<float>(1.0 - t * t * (3.0 - 2.0 * t));
    }

    /// @brief Light view-projection for positions relative to `focus`.
    ///
    /// The light-space position of the focus is snapped to whole shadow
    /// texels (the fractional remainder becomes the translation), so the map
    /// does not crawl as the aircraft moves.
    [[nodiscard]] static Mat4 compute_light_view_proj(const std::array<float, 3>& sun_dir,
                                                      const math::Vector3& focus, int size) noexcept {
        // Light looks down the sun ray.
        const math::Vector3 f(-sun_dir[0], -sun_dir[1], -sun_dir[2]);
        // Any up hint not parallel to the sun: north unless the sun is near it.
        math::Vector3 up_hint(1.0, 0.0, 0.0);
        if (std::abs(f.dot(up_hint)) > 0.95) up_hint = math::Vector3(0.0, 1.0, 0.0);
        const math::Vector3 s = f.cross(up_hint).normalized();
        const math::Vector3 u = s.cross(f);

        double rx = 0.0;
        double ry = 0.0;
        if (size > 0) {
            const double texel = 2.0 * kFocusRadius / size;
            const double fx = focus.dot(s);
            const double fy = focus.dot(u);
            rx = fx - std::floor(fx / texel) * texel;
            ry = fy - std::floor(fy / texel) * texel;
        }

        Mat4 v = Mat4::identity();
        v(0, 0) = static_cast<float>(s.x); v(0, 1) = static_cast<float>(s.y); v(0, 2) = static_cast<float>(s.z);
        v(1, 0) = static_cast<float>(u.x); v(1, 1) = static_cast<float>(u.y); v(1, 2) = static_cast<float>(u.z);
        v(2, 0) = static_cast<float>(-f.x); v(2, 1) = static_cast<float>(-f.y); v(2, 2) = static_cast<float>(-f.z);
        v(0, 3) = static_cast<float>(rx);
        v(1, 3) = static_cast<float>(ry);

        const Mat4 p = Mat4::ortho(-kFocusRadius, kFocusRadius, -kFocusRadius, kFocusRadius,
                                   -kDepthRange, kDepthRange);
        return p * v;
    }

private:
    GLuint fbo_ = 0;
    GLuint depth_tex_ = 0;
    int size_ = 0;
    bool valid_ = false;
    Mat4 light_view_proj_ = Mat4::identity();
    math::Vector3 origin_{0.0, 0.0, 0.0};
};

} // namespace fastjet::graphics
