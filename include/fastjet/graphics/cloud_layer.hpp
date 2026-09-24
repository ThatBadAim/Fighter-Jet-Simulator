#pragma once

#include "fastjet/graphics/gl_common.hpp"
#include "fastjet/graphics/shader.hpp"
#include "fastjet/graphics/shader_library.hpp"
#include "fastjet/graphics/frame_context.hpp"
#include <array>
#include <cmath>
#include <string>

namespace fastjet::graphics {

/// @brief Scattered cumulus deck drawn as a lit, camera-centred sheet.
///
/// The coverage field (glsl::CLOUDS) gives each point a density; the sheet
/// turns that into a slab of cloud with optical depth along the view ray, sun
/// transmittance through the neighbouring cloud (so sunlit edges glow and
/// cores go grey), a two-lobe scattering phase for the silver lining, and the
/// same aerial perspective as the terrain. It is depth-tested against the
/// terrain and airframe, so the jet can fly above, below and between clouds.
///
/// It also owns the baked cloud map that the sheet and every cloud shadow
/// read. The field is static in cloud space (the wind only offsets it), so
/// the map is rebuilt only when the aircraft has travelled far from its
/// centre or the coverage changes, not every frame.
class CloudLayer {
public:
    /// Half-size of the sheet around the eye [m]; it fades out before the edge.
    static constexpr float kExtent = 36000.0f;
    /// Cloud map: edge length [m] and resolution. At 47 m per texel it
    /// resolves every octave of the field, so the sheet can read the map
    /// instead of evaluating the noise per pixel.
    static constexpr float kMapSize = 96000.0f;
    static constexpr int kMapResolution = 2048;
    /// Distance from the map centre that triggers a rebuild [m]. The map
    /// then still covers every cloud the visible sheet can show.
    static constexpr float kMapRecenter = 12000.0f;
    static constexpr int kMapOctaves = 6;
    /// A rebuild is spread over this many frames (one strip each) into a
    /// back buffer, so recentring never stalls a frame.
    static constexpr int kMapStrips = 8;

    CloudLayer() = default;
    ~CloudLayer() { destroy(); }
    CloudLayer(const CloudLayer&) = delete;
    CloudLayer& operator=(const CloudLayer&) = delete;

    bool init() {
        if (vao_) return true;
        const char* vert = R"(
            #version 330 core
            uniform mat4  uViewProj;
            uniform vec3  uEyeNed;
            uniform float uLayerAlt;
            uniform float uExtent;
            out vec3 vWorld;
            void main() {
                // Two triangles from the vertex id, centred under the eye.
                const vec2 CORNERS[6] = vec2[6](vec2(-1, -1), vec2(1, 1), vec2(1, -1),
                                                vec2(-1, -1), vec2(-1, 1), vec2(1, 1));
                vec2 c = CORNERS[gl_VertexID];
                vWorld = vec3(uEyeNed.xy + c * uExtent, -uLayerAlt);
                gl_Position = uViewProj * vec4(vWorld, 1.0);
            }
        )";

        const std::string frag = std::string(R"(
            #version 330 core
            in vec3 vWorld;
            out vec4 FragColor;
            uniform vec3  uEyeNed;
            uniform float uExtent;
        )") + glsl::COMMON + glsl::OUTPUT + glsl::ATMOSPHERE + glsl::LIGHTING + glsl::CLOUDS + R"(
            // Extinction per metre of full-density cloud, and the (smaller)
            // effective value for light arriving from the sun, which is
            // forward-scattered through the cloud rather than removed.
            const float SIGMA_VIEW = 0.012;
            const float SIGMA_SUN  = 0.004;

            float hg(float mu, float g) {
                float g2 = g * g;
                return (1.0 - g2) / (4.0 * PI * pow(1.0 + g2 - 2.0 * g * mu, 1.5));
            }

            void main() {
                vec2 xy = vWorld.xy;
                float d = cloud_density_map(xy);
                if (d <= 0.0) discard;
                // Erode the edges with fine detail so outlines break into puffs.
                float detail = vnoise(xy * (1.0 / 260.0)) * 0.6 + vnoise(xy * (1.0 / 90.0)) * 0.4;
                d = clamp(d - (1.0 - d) * detail * 0.55, 0.0, 1.0);
                if (d <= 0.004) discard;

                vec3 to_frag = vWorld - uEyeNed;
                float dist = length(to_frag);
                vec3 v = to_frag / dist;
                float thickness = (uCloudTop - uCloudBase) * d;

                // Optical depth through the slab along the view ray.
                float tau = SIGMA_VIEW * thickness / max(abs(v.z), 0.12);
                float alpha = 1.0 - exp(-tau);

                // Relief: the density gradient stands in for the surface
                // normal of the billows, so tops model like cauliflower and
                // not like a flat sheet.
                const float E = 60.0; // [m] gradient step
                vec2 grad = vec2(cloud_density_map(xy + vec2(E, 0.0)) - cloud_density_map(xy - vec2(E, 0.0)),
                                 cloud_density_map(xy + vec2(0.0, E)) - cloud_density_map(xy - vec2(0.0, E)));
                // Billows: two scales of noise with analytic gradients.
                vec2 billow = vnoise_d(xy * (1.0 / 380.0)).yz * 0.55 + vnoise_d(xy * (1.0 / 130.0)).yz * 0.3;
                vec3 n = normalize(vec3(-(grad * 2.2 + billow * d), -1.0)); // NED: up is -Z

                // Sun blocked by neighbouring cloud a few hundred metres sunward.
                vec2 toward_sun = uSunDir.xy / max(-uSunDir.z, 0.15) * 450.0;
                float d_sun = cloud_density_map(xy + toward_sun);

                float mu = dot(v, uSunDir);
                float phase = mix(hg(mu, 0.55), hg(mu, -0.25), 0.3);
                float bumps = 0.85 + 0.3 * detail;

                bool from_above = -uEyeNed.z > 0.5 * (uCloudBase + uCloudTop);
                vec3 lit;
                if (from_above) {
                    // Tops face the sun: bright multiple scattering, a little
                    // forward glow, shade on the lee side of each billow.
                    float facing = clamp(dot(n, uSunDir) * 0.8 + 0.2, 0.0, 1.0);
                    float shade = exp(-SIGMA_SUN * (uCloudTop - uCloudBase) * d_sun * 0.35);
                    // Kept below the tonemap's shoulder, or every top clips
                    // to the same flat white and the relief is lost.
                    lit = sun_radiance() * (0.42 * facing + phase * 0.45) * shade * bumps
                        + sky_ambient() * 0.55;
                } else {
                    // Bases see only what made it through the slab: grey
                    // cores, bright thin edges (the silver lining).
                    float sun_t = exp(-SIGMA_SUN * (uCloudTop - uCloudBase) * (0.5 * d + d_sun));
                    float base_t = exp(-SIGMA_SUN * thickness * 1.6);
                    lit = sun_radiance() * sun_t * base_t * (0.30 + phase * 2.0)
                        + (sky_ambient() * (0.45 + 0.25 * (1.0 - d)) + ground_ambient() * 0.8) * bumps;
                }
                lit *= vec3(1.0, 0.99, 0.98);
                lit = apply_haze(lit, vWorld, uEyeNed, uSunDir);

                // Fade near the sheet's edge, and when the eye is within the
                // slab so its zero thickness never shows as a hard plane.
                float horiz = length(vWorld.xy - uEyeNed.xy);
                alpha *= 1.0 - smoothstep(uExtent * 0.6, uExtent * 0.95, horiz);
                alpha *= smoothstep(30.0, 400.0, dist);
                FragColor = scene_output(lit * alpha, alpha);
            }
        )";

        if (!shader_.init_from_source(vert, frag.c_str())) return false;
        shader_.use();
        shader_.set_int("uCloudMap", CloudSettings::kMapUnit);
        glUseProgram(0);
        glGenVertexArrays(1, &vao_);
        return init_map();
    }

    void destroy() noexcept {
        if (vao_) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
        for (MapBuffer& m : maps_) {
            if (m.fbo) glDeleteFramebuffers(1, &m.fbo);
            if (m.tex) glDeleteTextures(1, &m.tex);
            m = MapBuffer{};
        }
        shader_.destroy();
        map_shader_.destroy();
        front_valid_ = false;
        building_ = false;
    }

    /// @brief Advances the cloud map and records the current one in `clouds`.
    ///
    /// The first map is built at once. After that, when the eye drifts
    /// kMapRecenter from the centre, a recentred map is built into the back
    /// buffer one strip per frame while the front one stays in use.
    void update_map(CloudSettings& clouds, const math::Vector3& eye_ned) {
        if (!maps_[0].tex || clouds.coverage <= 0.0f) return;
        // The eye in cloud space.
        const double cx = eye_ned.x + clouds.offset_x;
        const double cy = eye_ned.y + clouds.offset_y;
        const double half = 0.5 * kMapSize;

        MapBuffer& front = maps_[front_];
        MapBuffer& back = maps_[1 - front_];
        if (!front_valid_ || clouds.coverage != front.coverage) {
            // Nothing usable yet (or the weather changed): build in full now.
            front.origin_x = cx - half;
            front.origin_y = cy - half;
            front.coverage = clouds.coverage;
            render_map(front, 0, kMapResolution);
            front_valid_ = true;
            building_ = false;
        } else if (building_) {
            const int rows = kMapResolution / kMapStrips;
            render_map(back, build_strip_ * rows, (build_strip_ + 1) * rows);
            if (++build_strip_ == kMapStrips) {
                front_ = 1 - front_;
                building_ = false;
            }
        } else if (std::abs(cx - (front.origin_x + half)) > kMapRecenter ||
                   std::abs(cy - (front.origin_y + half)) > kMapRecenter) {
            back.origin_x = cx - half;
            back.origin_y = cy - half;
            back.coverage = clouds.coverage;
            building_ = true;
            build_strip_ = 0;
        }

        const MapBuffer& current = maps_[front_];
        clouds.map_texture = current.tex;
        clouds.map_origin_x = static_cast<float>(current.origin_x);
        clouds.map_origin_y = static_cast<float>(current.origin_y);
        clouds.map_inv_size = 1.0f / kMapSize;
    }

    /// @brief Draws the deck. Depth test on, depth writes off, premultiplied.
    void render(const Mat4& view_proj, const FrameContext& ctx) {
        if (!vao_ || ctx.clouds.coverage <= 0.0f) return;
        shader_.use();
        ctx.upload(shader_);
        shader_.set_mat4("uViewProj", view_proj);
        shader_.set_vec3("uEyeNed", static_cast<float>(ctx.eye_ned.x), static_cast<float>(ctx.eye_ned.y),
                         static_cast<float>(ctx.eye_ned.z));
        shader_.set_float("uLayerAlt", 0.5f * (ctx.clouds.base_m + ctx.clouds.top_m));
        shader_.set_float("uExtent", kExtent);

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glBindVertexArray(vao_);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glDisable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_TRUE);
    }

private:
    GLuint vao_ = 0;
    ShaderProgram shader_;

    struct MapBuffer {
        GLuint tex = 0;
        GLuint fbo = 0;
        double origin_x = 0.0; ///< Corner in cloud space [m]
        double origin_y = 0.0;
        float coverage = -1.0f;
    };
    std::array<MapBuffer, 2> maps_{};
    int front_ = 0;
    bool front_valid_ = false;
    bool building_ = false;
    int build_strip_ = 0;
    ShaderProgram map_shader_;

    bool init_map() {
        const char* vert = R"(
            #version 330 core
            out vec2 vUV;
            void main() {
                vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
                vUV = p;
                gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
            }
        )";
        const std::string frag = std::string(R"(
            #version 330 core
            in vec2 vUV;
            out vec4 FragColor;
            uniform vec2  uMapOrigin;
            uniform float uMapSize;
            uniform vec2  uRows;      // V range of the strip being filled
            uniform int   uOctaves;
        )") + glsl::COMMON + glsl::CLOUDS + R"(
            void main() {
                vec2 uv = vec2(vUV.x, mix(uRows.x, uRows.y, vUV.y));
                FragColor = vec4(cloud_field(uMapOrigin + uv * uMapSize, uOctaves), 0.0, 0.0, 1.0);
            }
        )";
        if (!map_shader_.init_from_source(vert, frag.c_str())) return false;

        bool ok = true;
        for (MapBuffer& m : maps_) {
            glGenTextures(1, &m.tex);
            glBindTexture(GL_TEXTURE_2D, m.tex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, kMapResolution, kMapResolution, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glGenFramebuffers(1, &m.fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, m.fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m.tex, 0);
            ok = ok && glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
        }
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return ok;
    }

    /// Fills rows [row0, row1) of a map buffer from the procedural field.
    void render_map(const MapBuffer& m, int row0, int row1) {
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        glBindFramebuffer(GL_FRAMEBUFFER, m.fbo);
        glViewport(0, row0, kMapResolution, row1 - row0);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_STENCIL_TEST);
        map_shader_.use();
        map_shader_.set_vec2("uMapOrigin", static_cast<float>(m.origin_x), static_cast<float>(m.origin_y));
        map_shader_.set_float("uMapSize", kMapSize);
        map_shader_.set_vec2("uRows", static_cast<float>(row0) / kMapResolution,
                             static_cast<float>(row1) / kMapResolution);
        map_shader_.set_float("uCloudCoverage", m.coverage);
        map_shader_.set_int("uOctaves", kMapOctaves);
        glBindVertexArray(vao_);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
        glEnable(GL_DEPTH_TEST);
    }
};

} // namespace fastjet::graphics
