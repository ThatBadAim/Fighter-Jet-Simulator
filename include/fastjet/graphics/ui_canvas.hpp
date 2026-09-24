#pragma once

#include "fastjet/graphics/gl_common.hpp"
#include "fastjet/graphics/menu_font.hpp"
#include "fastjet/graphics/shader.hpp"
#include "fastjet/ui/draw_list.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace fastjet::graphics {

/// @brief OpenGL backend for ui::DrawList.
///
/// A single shader draws every primitive analytically, so corners, borders,
/// soft shadows and lines stay crisp at any size or UI scale:
///   mode 0  rounded rectangle (signed distance field, optional border),
///   mode 1  glyph from the embedded font atlas,
///   mode 2  anti-aliased capsule line,
///   mode 3  blurred rounded-rectangle drop shadow.
/// Output is premultiplied alpha. Consecutive commands share one draw call;
/// only scissor changes split a batch.
class UiCanvas {
public:
    UiCanvas() = default;
    ~UiCanvas() { destroy(); }
    UiCanvas(const UiCanvas&) = delete;
    UiCanvas& operator=(const UiCanvas&) = delete;

    bool init() {
        if (initialized_) return true;
        if (!shader_.init_from_source(kVertexSrc, kFragmentSrc)) return false;

        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);
        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        const GLsizei stride = sizeof(Vertex);
        auto attrib = [stride](GLuint loc, GLint n, size_t offset) {
            glVertexAttribPointer(loc, n, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offset));
            glEnableVertexAttribArray(loc);
        };
        attrib(0, 2, offsetof(Vertex, pos));
        attrib(1, 2, offsetof(Vertex, local));
        attrib(2, 2, offsetof(Vertex, half));
        attrib(3, 4, offsetof(Vertex, params));
        attrib(4, 4, offsetof(Vertex, fill));
        attrib(5, 4, offsetof(Vertex, border));
        attrib(6, 2, offsetof(Vertex, uv));
        glBindVertexArray(0);

        std::vector<uint8_t> atlas(static_cast<size_t>(kMenuFontTexW) * kMenuFontTexH, 0);
        decode_menu_font_atlas(atlas.data());
        glGenTextures(1, &font_tex_);
        glBindTexture(GL_TEXTURE_2D, font_tex_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, kMenuFontTexW, kMenuFontTexH, 0, GL_RED, GL_UNSIGNED_BYTE, atlas.data());
        glBindTexture(GL_TEXTURE_2D, 0);

        initialized_ = true;
        return true;
    }

    void destroy() noexcept {
        if (font_tex_) { glDeleteTextures(1, &font_tex_); font_tex_ = 0; }
        if (vbo_) { glDeleteBuffers(1, &vbo_); vbo_ = 0; }
        if (vao_) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
        shader_.destroy();
        initialized_ = false;
    }

    /// @brief Draws the list over whatever is in the bound framebuffer.
    void render(const ui::DrawList& list, int fb_w, int fb_h) {
        if (!initialized_ || list.empty() || fb_w <= 0 || fb_h <= 0) return;
        fb_w_ = fb_w;
        fb_h_ = fb_h;

        glViewport(0, 0, fb_w, fb_h);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_STENCIL_TEST);
        glEnable(GL_BLEND);
        glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        shader_.use();
        const GLint vp_loc = shader_.get_uniform_loc("uViewport");
        glUniform2f(vp_loc, static_cast<float>(fb_w), static_cast<float>(fb_h));
        shader_.set_int("uFont", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, font_tex_);
        glBindVertexArray(vao_);

        clip_stack_.clear();
        verts_.clear();
        for (const ui::DrawCmd& c : list.commands()) {
            switch (c.type) {
                case ui::DrawCmdType::RECT: emit_rect(c); break;
                case ui::DrawCmdType::SHADOW: emit_shadow(c); break;
                case ui::DrawCmdType::LINE: emit_line(c); break;
                case ui::DrawCmdType::TEXT: emit_text(c); break;
                case ui::DrawCmdType::CLIP_PUSH: {
                    flush();
                    const ui::Rect r = clip_stack_.empty() ? c.rect : ui::Rect::intersect(clip_stack_.back(), c.rect);
                    clip_stack_.push_back(r);
                    apply_clip();
                    break;
                }
                case ui::DrawCmdType::CLIP_POP:
                    flush();
                    if (!clip_stack_.empty()) clip_stack_.pop_back();
                    apply_clip();
                    break;
            }
        }
        flush();

        glDisable(GL_SCISSOR_TEST);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_DEPTH_TEST);
    }

private:
    struct Vertex {
        float pos[2];
        float local[2];
        float half[2];
        float params[4]; ///< radius, border width, blur, mode
        float fill[4];
        float border[4];
        float uv[2];
    };

    enum Mode : int { RECT = 0, GLYPH = 1, LINE = 2, SHADOW = 3 };

    static constexpr float kAaPad = 1.0f; ///< Quad padding so the SDF edge can fade out

    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint font_tex_ = 0;
    ShaderProgram shader_;
    bool initialized_ = false;
    int fb_w_ = 0;
    int fb_h_ = 0;
    std::vector<Vertex> verts_;
    std::vector<ui::Rect> clip_stack_;

    static constexpr const char* kVertexSrc = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aLocal;
        layout (location = 2) in vec2 aHalf;
        layout (location = 3) in vec4 aParams;
        layout (location = 4) in vec4 aFill;
        layout (location = 5) in vec4 aBorder;
        layout (location = 6) in vec2 aUV;
        uniform vec2 uViewport;
        out vec2 vLocal;
        out vec2 vHalf;
        out vec4 vParams;
        out vec4 vFill;
        out vec4 vBorder;
        out vec2 vUV;
        void main() {
            vLocal = aLocal;
            vHalf = aHalf;
            vParams = aParams;
            vFill = aFill;
            vBorder = aBorder;
            vUV = aUV;
            gl_Position = vec4(aPos.x / uViewport.x * 2.0 - 1.0, 1.0 - aPos.y / uViewport.y * 2.0, 0.0, 1.0);
        }
    )";

    static constexpr const char* kFragmentSrc = R"(
        #version 330 core
        in vec2 vLocal;
        in vec2 vHalf;
        in vec4 vParams;
        in vec4 vFill;
        in vec4 vBorder;
        in vec2 vUV;
        uniform sampler2D uFont;
        out vec4 FragColor;

        float sd_round_box(vec2 p, vec2 b, float r) {
            vec2 q = abs(p) - b + vec2(r);
            return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
        }
        vec4 premul(vec4 c) { return vec4(c.rgb * c.a, c.a); }

        void main() {
            int mode = int(vParams.w + 0.5);
            if (mode == 1) {
                FragColor = premul(vFill) * texture(uFont, vUV).r;
                return;
            }
            if (mode == 2) {
                float d = length(vec2(max(abs(vLocal.x) - vHalf.x, 0.0), vLocal.y)) - vHalf.y;
                FragColor = premul(vFill) * clamp(0.5 - d, 0.0, 1.0);
                return;
            }
            float r = min(vParams.x, min(vHalf.x, vHalf.y));
            float d = sd_round_box(vLocal, vHalf, r);
            if (mode == 3) {
                float blur = max(vParams.z, 1.0);
                FragColor = premul(vFill) * (1.0 - smoothstep(-blur, blur, d));
                return;
            }
            vec4 col = premul(vFill);
            float bw = vParams.y;
            if (bw > 0.0) {
                col = mix(col, premul(vBorder), clamp(d + bw + 0.5, 0.0, 1.0));
            }
            FragColor = col * clamp(0.5 - d, 0.0, 1.0);
        }
    )";

    void apply_clip() {
        if (clip_stack_.empty()) {
            glDisable(GL_SCISSOR_TEST);
            return;
        }
        const ui::Rect& r = clip_stack_.back();
        glEnable(GL_SCISSOR_TEST);
        const int x0 = static_cast<int>(std::floor(r.x));
        const int y0 = static_cast<int>(std::floor(r.y));
        const int x1 = static_cast<int>(std::ceil(r.right()));
        const int y1 = static_cast<int>(std::ceil(r.bottom()));
        glScissor(x0, fb_h_ - y1, std::max(0, x1 - x0), std::max(0, y1 - y0));
    }

    void flush() {
        if (verts_.empty()) return;
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(verts_.size() * sizeof(Vertex)), verts_.data(), GL_STREAM_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(verts_.size()));
        verts_.clear();
    }

    /// @brief Appends a quad; corners are (x0,y0) top-left to (x1,y1) bottom-right.
    /// `lx0..ly1` are the matching SDF-local coordinates.
    void quad(float x0, float y0, float x1, float y1, float lx0, float ly0, float lx1, float ly1,
              float hw, float hh, const float params[4],
              const ui::Rgba& c_tl, const ui::Rgba& c_tr, const ui::Rgba& c_bl, const ui::Rgba& c_br,
              const ui::Rgba& border, float u0 = 0.0f, float v0 = 0.0f, float u1 = 0.0f, float v1 = 0.0f) {
        auto v = [&](float x, float y, float lx, float ly, const ui::Rgba& c, float u, float vv) {
            Vertex out{};
            out.pos[0] = x; out.pos[1] = y;
            out.local[0] = lx; out.local[1] = ly;
            out.half[0] = hw; out.half[1] = hh;
            for (int i = 0; i < 4; ++i) out.params[i] = params[i];
            out.fill[0] = c.r; out.fill[1] = c.g; out.fill[2] = c.b; out.fill[3] = c.a;
            out.border[0] = border.r; out.border[1] = border.g; out.border[2] = border.b; out.border[3] = border.a;
            out.uv[0] = u; out.uv[1] = vv;
            return out;
        };
        const Vertex tl = v(x0, y0, lx0, ly0, c_tl, u0, v0);
        const Vertex tr = v(x1, y0, lx1, ly0, c_tr, u1, v0);
        const Vertex bl = v(x0, y1, lx0, ly1, c_bl, u0, v1);
        const Vertex br = v(x1, y1, lx1, ly1, c_br, u1, v1);
        verts_.insert(verts_.end(), {tl, tr, br, tl, br, bl});
    }

    void emit_rect(const ui::DrawCmd& c) {
        const float hw = c.rect.w * 0.5f;
        const float hh = c.rect.h * 0.5f;
        const float p = kAaPad;
        const float params[4] = {c.radius, c.border_width, 0.0f, static_cast<float>(RECT)};
        const ui::Rgba& a = c.fill_top;
        const ui::Rgba& b = c.fill_bottom;
        quad(c.rect.x - p, c.rect.y - p, c.rect.right() + p, c.rect.bottom() + p,
             -hw - p, -hh - p, hw + p, hh + p, hw, hh, params,
             a, c.horizontal ? b : a, c.horizontal ? a : b, b, c.border);
    }

    void emit_shadow(const ui::DrawCmd& c) {
        const float hw = c.rect.w * 0.5f;
        const float hh = c.rect.h * 0.5f;
        const float p = c.blur * 1.5f + kAaPad;
        const float params[4] = {c.radius, 0.0f, c.blur, static_cast<float>(SHADOW)};
        quad(c.rect.x - p, c.rect.y - p, c.rect.right() + p, c.rect.bottom() + p,
             -hw - p, -hh - p, hw + p, hh + p, hw, hh, params,
             c.fill_top, c.fill_top, c.fill_top, c.fill_top, c.fill_top);
    }

    void emit_line(const ui::DrawCmd& c) {
        const float dx = c.x1 - c.x0;
        const float dy = c.y1 - c.y0;
        const float len = std::sqrt(dx * dx + dy * dy);
        if (len < 1e-4f) return;
        const float ux = dx / len, uy = dy / len; // along
        const float nx = -uy, ny = ux;            // across
        const float hl = len * 0.5f;
        const float ht = std::max(0.5f, c.thickness * 0.5f);
        const float el = hl + kAaPad;
        const float et = ht + kAaPad;
        const float cx = (c.x0 + c.x1) * 0.5f;
        const float cy = (c.y0 + c.y1) * 0.5f;
        const float params[4] = {0.0f, 0.0f, 0.0f, static_cast<float>(LINE)};
        // Corners in world space, walked in (along, across) order.
        auto corner = [&](float a, float t, float& x, float& y) { x = cx + ux * a + nx * t; y = cy + uy * a + ny * t; };
        float x_tl, y_tl, x_tr, y_tr, x_bl, y_bl, x_br, y_br;
        corner(-el, -et, x_tl, y_tl);
        corner(el, -et, x_tr, y_tr);
        corner(-el, et, x_bl, y_bl);
        corner(el, et, x_br, y_br);
        auto v = [&](float x, float y, float la, float lt) {
            Vertex out{};
            out.pos[0] = x; out.pos[1] = y;
            out.local[0] = la; out.local[1] = lt;
            out.half[0] = hl; out.half[1] = ht;
            for (int i = 0; i < 4; ++i) out.params[i] = params[i];
            out.fill[0] = c.fill_top.r; out.fill[1] = c.fill_top.g; out.fill[2] = c.fill_top.b; out.fill[3] = c.fill_top.a;
            return out;
        };
        const Vertex tl = v(x_tl, y_tl, -el, -et);
        const Vertex tr = v(x_tr, y_tr, el, -et);
        const Vertex bl = v(x_bl, y_bl, -el, et);
        const Vertex br = v(x_br, y_br, el, et);
        verts_.insert(verts_.end(), {tl, tr, br, tl, br, bl});
    }

    void emit_text(const ui::DrawCmd& c) {
        const float params[4] = {0.0f, 0.0f, 0.0f, static_cast<float>(GLYPH)};
        const float size = c.size;
        // Snap the baseline to a pixel row so small text stays sharp.
        const float baseline = std::round(c.y0);
        float pen = c.x0;
        for (const char ch : c.text) {
            const MenuGlyph* g = find_menu_glyph(static_cast<unsigned char>(ch));
            if (!g) continue;
            if (g->w > 0.0f && g->h > 0.0f) {
                const float x0 = std::round(pen + g->bearing_x * size);
                const float y0 = baseline - g->bearing_y * size;
                const float x1 = x0 + g->w * size;
                const float y1 = y0 + g->h * size;
                quad(x0, y0, x1, y1, 0, 0, 0, 0, 0, 0, params,
                     c.fill_top, c.fill_top, c.fill_top, c.fill_top, c.fill_top, g->u0, g->v0, g->u1, g->v1);
            }
            pen += g->advance * size + c.tracking;
        }
    }
};

/// @brief Full-screen colour-vision-deficiency correction ("daltonisation").
///
/// Resolves the (multisampled) default framebuffer into a texture, simulates
/// how the frame looks to the selected deficiency in LMS space, and shifts
/// the lost contrast into channels that viewer can still distinguish.
class ColorFilterPass {
public:
    ColorFilterPass() = default;
    ~ColorFilterPass() { destroy(); }
    ColorFilterPass(const ColorFilterPass&) = delete;
    ColorFilterPass& operator=(const ColorFilterPass&) = delete;

    bool init() {
        if (initialized_) return true;
        if (!shader_.init_from_source(kVertexSrc, kFragmentSrc)) return false;
        glGenVertexArrays(1, &vao_); // Core profile needs a VAO even for attribute-less draws
        initialized_ = true;
        return true;
    }

    void destroy() noexcept {
        release_target();
        if (vao_) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
        shader_.destroy();
        initialized_ = false;
    }

    /// @param mode 1 = protanopia, 2 = deuteranopia, 3 = tritanopia; 0 is a no-op.
    void apply(int mode, int fb_w, int fb_h) {
        if (!initialized_ || mode <= 0 || fb_w <= 0 || fb_h <= 0) return;
        if (!ensure_target(fb_w, fb_h)) return;

        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_);
        glBlitFramebuffer(0, 0, fb_w, fb_h, 0, 0, fb_w, fb_h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glViewport(0, 0, fb_w, fb_h);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_SCISSOR_TEST);
        glDisable(GL_CULL_FACE);
        shader_.use();
        shader_.set_int("uScene", 0);
        shader_.set_int("uMode", mode);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex_);
        glBindVertexArray(vao_);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glEnable(GL_DEPTH_TEST);
    }

private:
    GLuint fbo_ = 0;
    GLuint tex_ = 0;
    GLuint vao_ = 0;
    int w_ = 0;
    int h_ = 0;
    bool initialized_ = false;
    ShaderProgram shader_;

    static constexpr const char* kVertexSrc = R"(
        #version 330 core
        void main() {
            vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
            gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);
        }
    )";

    // LMS matrices and deficiency projections after Vienot, Brettel & Mollon (1999).
    static constexpr const char* kFragmentSrc = R"(
        #version 330 core
        uniform sampler2D uScene;
        uniform int uMode;
        out vec4 FragColor;

        vec3 rgb_to_lms(vec3 c) {
            return vec3(17.8824 * c.r + 43.5161 * c.g + 4.11935 * c.b,
                        3.45565 * c.r + 27.1554 * c.g + 3.86714 * c.b,
                        0.0299566 * c.r + 0.184309 * c.g + 1.46709 * c.b);
        }
        vec3 lms_to_rgb(vec3 l) {
            return vec3( 0.0809444479 * l.x - 0.130504409 * l.y + 0.116721066 * l.z,
                        -0.0102485335 * l.x + 0.0540193266 * l.y - 0.113614708 * l.z,
                        -0.000365296938 * l.x - 0.00412161469 * l.y + 0.693511405 * l.z);
        }

        void main() {
            vec3 c = texelFetch(uScene, ivec2(gl_FragCoord.xy), 0).rgb;
            vec3 l = rgb_to_lms(c);
            vec3 s;
            if (uMode == 1)      s = vec3(2.02344 * l.y - 2.52581 * l.z, l.y, l.z);   // protanopia
            else if (uMode == 2) s = vec3(l.x, 0.494207 * l.x + 1.24827 * l.z, l.z);  // deuteranopia
            else                 s = vec3(l.x, l.y, -0.395913 * l.x + 0.801109 * l.y); // tritanopia
            vec3 err = c - lms_to_rgb(s);
            vec3 shift = vec3(0.0, 0.7 * err.r + err.g, 0.7 * err.r + err.b);
            FragColor = vec4(clamp(c + shift, 0.0, 1.0), 1.0);
        }
    )";

    bool ensure_target(int w, int h) {
        if (fbo_ && w == w_ && h == h_) return true;
        release_target();
        glGenTextures(1, &tex_);
        glBindTexture(GL_TEXTURE_2D, tex_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);
        glGenFramebuffers(1, &fbo_);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_, 0);
        const bool ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (!ok) {
            release_target();
            return false;
        }
        w_ = w;
        h_ = h;
        return true;
    }

    void release_target() noexcept {
        if (fbo_) { glDeleteFramebuffers(1, &fbo_); fbo_ = 0; }
        if (tex_) { glDeleteTextures(1, &tex_); tex_ = 0; }
        w_ = h_ = 0;
    }
};

} // namespace fastjet::graphics
