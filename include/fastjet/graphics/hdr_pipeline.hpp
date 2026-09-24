#pragma once

#include "fastjet/graphics/gl_common.hpp"
#include "fastjet/graphics/shader.hpp"
#include "fastjet/graphics/shader_library.hpp"
#include <algorithm>
#include <string>
#include <vector>

namespace fastjet::graphics {

/// @brief Post-processing controls for one frame.
struct PostParams {
    float exposure = 1.35f;       ///< Scene-referred exposure before the ACES curve
    bool  bloom = true;           ///< Energy-conserving bloom from the mip chain
    float bloom_strength = 0.05f; ///< Fraction of the frame replaced by its blur
    float vignette = 0.18f;       ///< Optical corner falloff, 0 = off
    float greyout = 0.0f;         ///< G-induced vision loss, 0 = clear, 1 = black
};

/// @brief HDR scene target, bloom and display transform.
///
/// The world renders as linear radiance into a multisampled floating-point
/// target. After the resolve, a mip chain of progressively blurred copies
/// (13-tap downsample, tent upsample) supplies bloom, and a single composite
/// pass applies exposure, the ACES curve, vignette, G-LOC greyout, sRGB
/// encoding and dither into the display framebuffer.
///
/// R11F_G11F_B10F is used throughout: HDR range at half the bandwidth of
/// RGBA16F, which matters on integrated GPUs, and the scene needs no alpha.
class HdrPipeline {
public:
    HdrPipeline() = default;
    ~HdrPipeline() { destroy(); }
    HdrPipeline(const HdrPipeline&) = delete;
    HdrPipeline& operator=(const HdrPipeline&) = delete;

    bool init() {
        if (initialized_) return true;
        if (!init_shaders()) return false;
        glGenVertexArrays(1, &empty_vao_);
        initialized_ = true;
        return true;
    }

    void destroy() noexcept {
        release_targets();
        if (empty_vao_) { glDeleteVertexArrays(1, &empty_vao_); empty_vao_ = 0; }
        down_shader_.destroy();
        up_shader_.destroy();
        composite_shader_.destroy();
        overlay_shader_.destroy();
        initialized_ = false;
    }

    /// @brief (Re)allocates the targets if the size or sample count changed.
    /// @return False if no usable target could be built.
    bool ensure(int width, int height, int samples) {
        if (!initialized_) return false;
        width = (std::max)(width, 1);
        height = (std::max)(height, 1);
        samples = (std::max)(samples, 0);
        // Compare against the request, not the clamped sample count, or a
        // request above GL_MAX_SAMPLES would rebuild the targets every frame.
        if (width == width_ && height == height_ && samples == requested_samples_ && scene_fbo_) return true;
        for (const GLenum format : {GLenum(GL_R11F_G11F_B10F), GLenum(GL_RGBA16F)}) {
            // Some drivers refuse packed float as a multisample target.
            release_targets();
            width_ = width;
            height_ = height;
            samples_ = samples;
            requested_samples_ = samples;
            if (build_targets(format)) return true;
        }
        release_targets();
        return false;
    }

    /// @brief Binds the scene target for drawing and sets the viewport.
    void begin_scene() const noexcept {
        glBindFramebuffer(GL_FRAMEBUFFER, samples_ > 0 ? msaa_fbo_ : scene_fbo_);
        glViewport(0, 0, width_, height_);
    }

    /// @brief Resolves the multisampled scene into the sampled texture.
    void resolve() const noexcept {
        if (samples_ > 0) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, msaa_fbo_);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, scene_fbo_);
            glBlitFramebuffer(0, 0, width_, height_, 0, 0, width_, height_, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    /// @brief Bloom and display transform into `dst_fbo`.
    void composite(const PostParams& p, GLuint dst_fbo = 0) {
        if (!scene_fbo_) return;
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glDepthMask(GL_FALSE);
        glBindVertexArray(empty_vao_);

        const bool bloom = p.bloom && !mips_.empty();
        if (bloom) build_bloom();

        glBindFramebuffer(GL_FRAMEBUFFER, dst_fbo);
        glViewport(0, 0, width_, height_);
        composite_shader_.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, scene_tex_);
        composite_shader_.set_int("uScene", 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, bloom ? mips_.front().tex : scene_tex_);
        composite_shader_.set_int("uBloom", 1);
        composite_shader_.set_float("uBloomStrength", bloom ? p.bloom_strength : 0.0f);
        composite_shader_.set_float("uExposure", p.exposure);
        composite_shader_.set_float("uVignette", p.vignette);
        composite_shader_.set_float("uGreyout", std::clamp(p.greyout, 0.0f, 1.0f));
        composite_shader_.set_vec2("uResolution", static_cast<float>(width_), static_cast<float>(height_));
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
    }

    /// @brief G-LOC tunnel vision over everything drawn so far, cockpit and
    /// HUD included: the periphery darkens first, then the centre.
    void vision_overlay(float greyout, int width, int height) {
        if (greyout <= 0.0f || !overlay_shader_.is_valid()) return;
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glViewport(0, 0, width, height);
        overlay_shader_.use();
        overlay_shader_.set_float("uGreyout", (std::min)(greyout, 1.0f));
        overlay_shader_.set_vec2("uResolution", static_cast<float>(width), static_cast<float>(height));
        glBindVertexArray(empty_vao_);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }

    [[nodiscard]] int width() const noexcept { return width_; }
    [[nodiscard]] int height() const noexcept { return height_; }
    [[nodiscard]] int samples() const noexcept { return samples_; }
    [[nodiscard]] GLuint scene_texture() const noexcept { return scene_tex_; }
    [[nodiscard]] GLenum color_format() const noexcept { return color_format_; }
    [[nodiscard]] size_t bloom_levels() const noexcept { return mips_.size(); }
    [[nodiscard]] bool is_initialized() const noexcept { return initialized_; }

private:
    struct Mip {
        GLuint fbo = 0;
        GLuint tex = 0;
        int w = 0;
        int h = 0;
    };

    /// Smallest bloom level edge; below this the blur adds nothing visible.
    static constexpr int kMinBloomSize = 8;
    static constexpr int kMaxBloomLevels = 6;

    bool initialized_ = false;
    int width_ = 0;
    int height_ = 0;
    int samples_ = 0;
    int requested_samples_ = -1;
    GLenum color_format_ = GL_R11F_G11F_B10F;

    GLuint msaa_fbo_ = 0;
    GLuint msaa_color_rb_ = 0;
    GLuint msaa_depth_rb_ = 0;
    GLuint scene_fbo_ = 0;
    GLuint scene_tex_ = 0;
    GLuint scene_depth_rb_ = 0; // only when not multisampled
    std::vector<Mip> mips_;
    GLuint empty_vao_ = 0;

    ShaderProgram down_shader_;
    ShaderProgram up_shader_;
    ShaderProgram composite_shader_;
    ShaderProgram overlay_shader_;

    static GLuint make_texture(GLenum format, int w, int h) {
        GLuint t = 0;
        glGenTextures(1, &t);
        glBindTexture(GL_TEXTURE_2D, t);
        glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format), w, h, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);
        return t;
    }

    bool build_targets(GLenum format) {
        color_format_ = format;
        GLint max_samples = 0;
        glGetIntegerv(GL_MAX_SAMPLES, &max_samples);
        samples_ = (std::min)(samples_, static_cast<int>(max_samples));

        // Resolved (sampled) scene target.
        scene_tex_ = make_texture(format, width_, height_);
        glGenFramebuffers(1, &scene_fbo_);
        glBindFramebuffer(GL_FRAMEBUFFER, scene_fbo_);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, scene_tex_, 0);
        if (samples_ == 0) {
            glGenRenderbuffers(1, &scene_depth_rb_);
            glBindRenderbuffer(GL_RENDERBUFFER, scene_depth_rb_);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width_, height_);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, scene_depth_rb_);
        }
        bool ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

        // Multisampled render target.
        if (ok && samples_ > 0) {
            glGenRenderbuffers(1, &msaa_color_rb_);
            glBindRenderbuffer(GL_RENDERBUFFER, msaa_color_rb_);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples_, format, width_, height_);
            glGenRenderbuffers(1, &msaa_depth_rb_);
            glBindRenderbuffer(GL_RENDERBUFFER, msaa_depth_rb_);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples_, GL_DEPTH24_STENCIL8, width_, height_);
            glGenFramebuffers(1, &msaa_fbo_);
            glBindFramebuffer(GL_FRAMEBUFFER, msaa_fbo_);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msaa_color_rb_);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, msaa_depth_rb_);
            ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
        }

        // Bloom chain, starting at half resolution.
        if (ok) {
            int w = width_ / 2;
            int h = height_ / 2;
            while (static_cast<int>(mips_.size()) < kMaxBloomLevels && (std::min)(w, h) >= kMinBloomSize) {
                Mip m;
                m.w = w;
                m.h = h;
                m.tex = make_texture(format, w, h);
                glGenFramebuffers(1, &m.fbo);
                glBindFramebuffer(GL_FRAMEBUFFER, m.fbo);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m.tex, 0);
                mips_.push_back(m);
                if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                    ok = false;
                    break;
                }
                w /= 2;
                h /= 2;
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        return ok;
    }

    void release_targets() noexcept {
        for (auto& m : mips_) {
            if (m.fbo) glDeleteFramebuffers(1, &m.fbo);
            if (m.tex) glDeleteTextures(1, &m.tex);
        }
        mips_.clear();
        if (msaa_fbo_) { glDeleteFramebuffers(1, &msaa_fbo_); msaa_fbo_ = 0; }
        if (msaa_color_rb_) { glDeleteRenderbuffers(1, &msaa_color_rb_); msaa_color_rb_ = 0; }
        if (msaa_depth_rb_) { glDeleteRenderbuffers(1, &msaa_depth_rb_); msaa_depth_rb_ = 0; }
        if (scene_fbo_) { glDeleteFramebuffers(1, &scene_fbo_); scene_fbo_ = 0; }
        if (scene_tex_) { glDeleteTextures(1, &scene_tex_); scene_tex_ = 0; }
        if (scene_depth_rb_) { glDeleteRenderbuffers(1, &scene_depth_rb_); scene_depth_rb_ = 0; }
        width_ = height_ = 0;
    }

    /// Downsample the scene through the chain, then upsample back, adding
    /// each coarser level into the next finer one.
    void build_bloom() {
        down_shader_.use();
        down_shader_.set_int("uSource", 0);
        glActiveTexture(GL_TEXTURE0);
        GLuint src = scene_tex_;
        int src_w = width_, src_h = height_;
        for (size_t i = 0; i < mips_.size(); ++i) {
            const Mip& m = mips_[i];
            glBindFramebuffer(GL_FRAMEBUFFER, m.fbo);
            glViewport(0, 0, m.w, m.h);
            glBindTexture(GL_TEXTURE_2D, src);
            down_shader_.set_vec2("uSourceTexel", 1.0f / static_cast<float>(src_w), 1.0f / static_cast<float>(src_h));
            // The first level de-weights isolated bright pixels (Karis
            // average) so a single sub-pixel sun glint cannot flicker.
            down_shader_.set_int("uKarisAverage", i == 0 ? 1 : 0);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            src = m.tex;
            src_w = m.w;
            src_h = m.h;
        }

        up_shader_.use();
        up_shader_.set_int("uSource", 0);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        for (size_t i = mips_.size() - 1; i > 0; --i) {
            const Mip& from = mips_[i];
            const Mip& to = mips_[i - 1];
            glBindFramebuffer(GL_FRAMEBUFFER, to.fbo);
            glViewport(0, 0, to.w, to.h);
            glBindTexture(GL_TEXTURE_2D, from.tex);
            up_shader_.set_vec2("uSourceTexel", 1.0f / static_cast<float>(from.w), 1.0f / static_cast<float>(from.h));
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
        glDisable(GL_BLEND);
    }

    bool init_shaders() {
        // Fullscreen triangle generated from the vertex id; no vertex buffer.
        const char* vert = R"(
            #version 330 core
            out vec2 vUV;
            void main() {
                vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
                vUV = p;
                gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
            }
        )";

        // Call of Duty: Advanced Warfare 13-tap downsample.
        const char* down = R"(
            #version 330 core
            in vec2 vUV;
            out vec4 FragColor;
            uniform sampler2D uSource;
            uniform vec2 uSourceTexel;
            uniform int uKarisAverage;

            vec3 tap(vec2 o) { return texture(uSource, vUV + o * uSourceTexel).rgb; }
            float karis(vec3 c) { return 1.0 / (1.0 + dot(c, vec3(0.2126, 0.7152, 0.0722))); }

            void main() {
                vec3 a = tap(vec2(-2.0,  2.0)), b = tap(vec2(0.0,  2.0)), c = tap(vec2(2.0,  2.0));
                vec3 d = tap(vec2(-2.0,  0.0)), e = tap(vec2(0.0,  0.0)), f = tap(vec2(2.0,  0.0));
                vec3 g = tap(vec2(-2.0, -2.0)), h = tap(vec2(0.0, -2.0)), i = tap(vec2(2.0, -2.0));
                vec3 j = tap(vec2(-1.0,  1.0)), k = tap(vec2(1.0,  1.0));
                vec3 l = tap(vec2(-1.0, -1.0)), m = tap(vec2(1.0, -1.0));
                vec3 outc;
                if (uKarisAverage == 1) {
                    // Weight each 2x2 group by inverse luminance before summing.
                    vec3 g0 = (a + b + d + e) * 0.25, g1 = (b + c + e + f) * 0.25;
                    vec3 g2 = (d + e + g + h) * 0.25, g3 = (e + f + h + i) * 0.25;
                    vec3 g4 = (j + k + l + m) * 0.25;
                    float w0 = karis(g0), w1 = karis(g1), w2 = karis(g2), w3 = karis(g3), w4 = karis(g4);
                    outc = (g0 * w0 * 0.125 + g1 * w1 * 0.125 + g2 * w2 * 0.125 + g3 * w3 * 0.125 + g4 * w4 * 0.5)
                         / (w0 * 0.125 + w1 * 0.125 + w2 * 0.125 + w3 * 0.125 + w4 * 0.5);
                } else {
                    outc = e * 0.125 + (a + c + g + i) * 0.03125 + (b + d + f + h) * 0.0625 + (j + k + l + m) * 0.125;
                }
                FragColor = vec4(max(outc, vec3(0.0)), 1.0);
            }
        )";

        // 3x3 tent upsample.
        const char* up = R"(
            #version 330 core
            in vec2 vUV;
            out vec4 FragColor;
            uniform sampler2D uSource;
            uniform vec2 uSourceTexel;
            vec3 tap(vec2 o) { return texture(uSource, vUV + o * uSourceTexel).rgb; }
            void main() {
                vec3 s = tap(vec2(0.0)) * 4.0;
                s += (tap(vec2(-1.0, 0.0)) + tap(vec2(1.0, 0.0)) + tap(vec2(0.0, -1.0)) + tap(vec2(0.0, 1.0))) * 2.0;
                s += tap(vec2(-1.0, -1.0)) + tap(vec2(1.0, -1.0)) + tap(vec2(-1.0, 1.0)) + tap(vec2(1.0, 1.0));
                FragColor = vec4(s / 16.0, 1.0);
            }
        )";

        const std::string composite = std::string(R"(
            #version 330 core
            in vec2 vUV;
            out vec4 FragColor;
            uniform sampler2D uScene;
            uniform sampler2D uBloom;
            uniform float uBloomStrength;
            uniform float uVignette;
            uniform float uGreyout;
            uniform vec2  uResolution;
        )") + glsl::COMMON + glsl::OUTPUT + R"(
            void main() {
                vec3 c = texture(uScene, vUV).rgb;
                c = mix(c, texture(uBloom, vUV).rgb, uBloomStrength);

                // Natural lens vignetting (cos^4 falloff, softened).
                vec2 d = (vUV - 0.5) * vec2(uResolution.x / uResolution.y, 1.0);
                float r2 = dot(d, d);
                c *= mix(1.0, 1.0 / (1.0 + r2 * 1.8), uVignette);

                // G-LOC greyout: colour drains before vision closes in (the
                // tunnel itself is vision_overlay(), drawn over the cockpit too).
                if (uGreyout > 0.0) {
                    float lum = dot(c, vec3(0.2126, 0.7152, 0.0722));
                    c = mix(c, vec3(lum), smoothstep(0.0, 0.45, uGreyout));
                }

                vec3 outc = display_encode(c);
                // Half-LSB dither hides banding in the sky gradient.
                outc += (ign(gl_FragCoord.xy) - 0.5) / 255.0;
                FragColor = vec4(outc, 1.0);
            }
        )";

        const char* overlay = R"(
            #version 330 core
            in vec2 vUV;
            out vec4 FragColor;
            uniform float uGreyout;
            uniform vec2  uResolution;
            void main() {
                vec2 d = (vUV - 0.5) * vec2(uResolution.x / uResolution.y, 1.0);
                float r = length(d) * 1.4;
                float tunnel = smoothstep(1.05 - uGreyout, 1.35 - uGreyout * 1.2, r);
                float dark = max(tunnel, smoothstep(0.75, 1.0, uGreyout));
                FragColor = vec4(0.0, 0.0, 0.0, dark);
            }
        )";

        return down_shader_.init_from_source(vert, down) &&
               up_shader_.init_from_source(vert, up) &&
               composite_shader_.init_from_source(vert, composite.c_str()) &&
               overlay_shader_.init_from_source(vert, overlay);
    }
};

} // namespace fastjet::graphics
