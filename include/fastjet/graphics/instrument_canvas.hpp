#pragma once

#include "fastjet/graphics/gl_common.hpp"
#include "fastjet/graphics/shader.hpp"
#include "fastjet/graphics/menu_font.hpp"
#include <vector>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>

namespace fastjet::graphics {

/// @brief 2D vertex for instrument vector drawing. Untextured geometry
/// carries a negative U; glyphs sample the font atlas.
struct InstVertex {
    float pos[2];
    float uv[2];
    float color[4];
};

/// @brief Off-screen 2D vector canvas shared by the cockpit displays.
///
/// Owns one render-target texture and batches flat triangles, strokes and
/// font glyphs into it in painter's order. Each display draws into its own
/// cell of the target through set_panel_viewport(), in a local -1..1 space.
class InstrumentCanvas {
protected:
    GLuint fbo_ = 0;
    GLuint texture_ = 0;
    GLuint font_tex_ = 0;

    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    ShaderProgram shader_;
    bool initialized_ = false;
    int tex_w_ = 0;
    int tex_h_ = 0;

    std::vector<InstVertex> tri_verts_;
    std::vector<InstVertex> line_verts_;

    // Panel-local -> texture NDC transform. Each display on the instrument
    // panel draws in its own -1..1 space; these map that space into the
    // sub-rectangle of the shared atlas the display owns.
    float vp_cx_ = 0.0f, vp_cy_ = 0.0f, vp_sx_ = 1.0f, vp_sy_ = 1.0f;

    static constexpr float kPi = 3.14159265f;
    static constexpr float kNoTex = -1.0f;

    /// @brief Direct the following draw calls into one atlas cell.
    /// @param cx,cy Cell centre in texture NDC
    /// @param hw,hh Cell half-extents in texture NDC
    void set_panel_viewport(float cx, float cy, float hw, float hh) noexcept {
        vp_cx_ = cx; vp_cy_ = cy; vp_sx_ = hw; vp_sy_ = hh;
    }

    void reset_panel_viewport() noexcept {
        vp_cx_ = 0.0f; vp_cy_ = 0.0f; vp_sx_ = 1.0f; vp_sy_ = 1.0f;
    }

    float mx(float x) const noexcept { return vp_cx_ + x * vp_sx_; }
    float my(float y) const noexcept { return vp_cy_ + y * vp_sy_; }

    InstVertex vert(float x, float y, const Color4& col, float u = kNoTex, float v = kNoTex) const noexcept {
        return InstVertex{{mx(x), my(y)}, {u, v}, {col.r, col.g, col.b, col.a}};
    }

    void add_line(float x0, float y0, float x1, float y1, const Color4& col) {
        line_verts_.push_back(vert(x0, y0, col));
        line_verts_.push_back(vert(x1, y1, col));
    }

    void add_tri(float x0, float y0, float x1, float y1, float x2, float y2, const Color4& col) {
        tri_verts_.push_back(vert(x0, y0, col));
        tri_verts_.push_back(vert(x1, y1, col));
        tri_verts_.push_back(vert(x2, y2, col));
    }

    void add_quad(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, const Color4& col) {
        add_tri(x0, y0, x1, y1, x2, y2, col);
        add_tri(x0, y0, x2, y2, x3, y3, col);
    }

    void add_rect(float cx, float cy, float w, float h, const Color4& col) {
        const float l = cx - w * 0.5f;
        const float r = cx + w * 0.5f;
        const float b = cy - h * 0.5f;
        const float t = cy + h * 0.5f;
        add_quad(l, b, r, b, r, t, l, t, col);
    }

    void add_circle(float cx, float cy, float r, const Color4& col, int segs = 24) {
        for (int i = 0; i < segs; ++i) {
            const float a0 = i * (2.0f * kPi / segs);
            const float a1 = (i + 1) * (2.0f * kPi / segs);
            add_tri(cx, cy, cx + r * std::cos(a0), cy + r * std::sin(a0),
                    cx + r * std::cos(a1), cy + r * std::sin(a1), col);
        }
    }

    void add_circle_outline(float cx, float cy, float r, const Color4& col, int segs = 24) {
        for (int i = 0; i < segs; ++i) {
            const float a0 = i * (2.0f * kPi / segs);
            const float a1 = (i + 1) * (2.0f * kPi / segs);
            add_line(cx + r * std::cos(a0), cy + r * std::sin(a0),
                     cx + r * std::cos(a1), cy + r * std::sin(a1), col);
        }
    }

    /// @brief A stroke of given width built from triangles, so it keeps its
    /// weight regardless of the driver's wide-line support.
    void add_bar(float x0, float y0, float x1, float y1, float width, const Color4& col) {
        const float dx = x1 - x0, dy = y1 - y0;
        const float len = std::sqrt(dx * dx + dy * dy);
        if (len < 1e-6f) return;
        const float nx = -dy / len * width * 0.5f;
        const float ny =  dx / len * width * 0.5f;
        add_quad(x0 + nx, y0 + ny, x1 + nx, y1 + ny, x1 - nx, y1 - ny, x0 - nx, y0 - ny, col);
    }

    /// @brief Annulus sector. Angles are dial bearings: degrees clockwise
    /// from 12 o'clock, the convention every round gauge face uses.
    void add_ring_sector(float cx, float cy, float r0, float r1, float deg0, float deg1,
                         const Color4& col, int segs = 48) {
        const float step = (deg1 - deg0) / static_cast<float>(segs);
        for (int i = 0; i < segs; ++i) {
            const float a0 = (deg0 + step * static_cast<float>(i)) * kPi / 180.0f;
            const float a1 = (deg0 + step * static_cast<float>(i + 1)) * kPi / 180.0f;
            const float s0 = std::sin(a0), c0 = std::cos(a0), s1 = std::sin(a1), c1 = std::cos(a1);
            add_quad(cx + r0 * s0, cy + r0 * c0, cx + r1 * s0, cy + r1 * c0,
                     cx + r1 * s1, cy + r1 * c1, cx + r0 * s1, cy + r0 * c1, col);
        }
    }

    void add_ring(float cx, float cy, float r0, float r1, const Color4& col, int segs = 64) {
        add_ring_sector(cx, cy, r0, r1, 0.0f, 360.0f, col, segs);
    }

    /// @brief Convex polygon, fanned from its first vertex.
    void add_convex(const std::vector<float>& xy, const Color4& col) {
        const size_t n = xy.size() / 2;
        for (size_t i = 1; i + 1 < n; ++i) {
            add_tri(xy[0], xy[1], xy[i * 2], xy[i * 2 + 1], xy[(i + 1) * 2], xy[(i + 1) * 2 + 1], col);
        }
    }

    /// @brief Width of a string set in the panel typeface at `size` (em height).
    static float text_width(const char* str, float size) noexcept {
        float w = 0.0f;
        for (const char* p = str; *p; ++p) w += find_menu_glyph(static_cast<unsigned char>(*p))->advance * size;
        return w;
    }

    /// @brief Text in the panel typeface: anti-aliased glyphs from the font
    /// atlas, as silk-screened on real instrument faces.
    /// @param x,y     Anchor, on the baseline
    /// @param size    Em height in local units (cap height is ~0.72 em)
    /// @param align   -1 left, 0 centre, +1 right, relative to the anchor
    /// @param rot_deg Rotation about the anchor, clockwise
    void draw_text(const char* str, float x, float y, float size, const Color4& col,
                   int align = -1, float rot_deg = 0.0f) {
        float pen = 0.0f;
        if (align == 0) pen = -text_width(str, size) * 0.5f;
        else if (align > 0) pen = -text_width(str, size);
        const float a = -rot_deg * kPi / 180.0f;
        const float ca = std::cos(a), sa = std::sin(a);
        auto place = [&](float lx, float ly, float u, float v) {
            return vert(x + lx * ca - ly * sa, y + lx * sa + ly * ca, col, u, v);
        };
        for (const char* p = str; *p; ++p) {
            const MenuGlyph* g = find_menu_glyph(static_cast<unsigned char>(*p));
            if (g->w > 0.0f && g->h > 0.0f) {
                const float l = pen + g->bearing_x * size;
                const float r = l + g->w * size;
                const float t = g->bearing_y * size;
                const float b = t - g->h * size;
                const InstVertex tl = place(l, t, g->u0, g->v0);
                const InstVertex tr = place(r, t, g->u1, g->v0);
                const InstVertex br = place(r, b, g->u1, g->v1);
                const InstVertex bl = place(l, b, g->u0, g->v1);
                tri_verts_.insert(tri_verts_.end(), {tl, tr, br, tl, br, bl});
            }
            pen += g->advance * size;
        }
    }

    /// @brief Text centred on a point (both axes), for dial numerals.
    void draw_text_centred(const char* str, float x, float y, float size, const Color4& col) {
        draw_text(str, x, y - size * 0.36f, size, col, 0);
    }

    /// @brief 14-segment alphanumeric cell, as used on real MFD/UFC labels.
    ///
    /// Segment names follow the usual starburst convention: the four outer
    /// bars (t/b, and the four half-height verticals), a split middle bar, two
    /// diagonals per corner and a centre vertical. That is enough to draw the
    /// full upper-case alphabet legibly at panel size, which the previous
    /// 7-segment cell could not (it had no letters beyond 'M').
    void draw_char(char c, float cx, float cy, float scale, const Color4& col) {
        const float w = 0.6f * scale;
        const float h = 1.0f * scale;
        const float l = cx - w * 0.5f;
        const float r = cx + w * 0.5f;
        const float b = cy - h * 0.5f;
        const float m = cy;
        const float t = cy + h * 0.5f;

        auto seg_t  = [&]() { add_line(l, t, r, t, col); };   // top bar
        auto seg_tr = [&]() { add_line(r, m, r, t, col); };   // upper right
        auto seg_br = [&]() { add_line(r, b, r, m, col); };   // lower right
        auto seg_b  = [&]() { add_line(l, b, r, b, col); };   // bottom bar
        auto seg_bl = [&]() { add_line(l, b, l, m, col); };   // lower left
        auto seg_tl = [&]() { add_line(l, m, l, t, col); };   // upper left
        auto seg_ml = [&]() { add_line(l, m, cx, m, col); };  // middle left
        auto seg_mr = [&]() { add_line(cx, m, r, m, col); };  // middle right
        auto seg_m  = [&]() { seg_ml(); seg_mr(); };
        auto seg_cv_t = [&]() { add_line(cx, m, cx, t, col); };  // centre vert upper
        auto seg_cv_b = [&]() { add_line(cx, b, cx, m, col); };  // centre vert lower
        auto seg_d_tl = [&]() { add_line(l, t, cx, m, col); };   // diagonal \ upper-left
        auto seg_d_tr = [&]() { add_line(r, t, cx, m, col); };   // diagonal / upper-right
        auto seg_d_bl = [&]() { add_line(l, b, cx, m, col); };   // diagonal / lower-left
        auto seg_d_br = [&]() { add_line(r, b, cx, m, col); };   // diagonal \ lower-right

        switch (c) {
            // ---- digits ----
            case '0': seg_t(); seg_tr(); seg_br(); seg_b(); seg_bl(); seg_tl(); break;
            case '1': seg_tr(); seg_br(); break;
            case '2': seg_t(); seg_tr(); seg_m(); seg_bl(); seg_b(); break;
            case '3': seg_t(); seg_tr(); seg_m(); seg_br(); seg_b(); break;
            case '4': seg_tl(); seg_m(); seg_tr(); seg_br(); break;
            case '5': seg_t(); seg_tl(); seg_m(); seg_br(); seg_b(); break;
            case '6': seg_t(); seg_tl(); seg_m(); seg_bl(); seg_br(); seg_b(); break;
            case '7': seg_t(); seg_tr(); seg_br(); break;
            case '8': seg_t(); seg_tr(); seg_br(); seg_b(); seg_bl(); seg_tl(); seg_m(); break;
            case '9': seg_t(); seg_tl(); seg_tr(); seg_m(); seg_br(); seg_b(); break;

            // ---- letters ----
            case 'A': seg_t(); seg_tl(); seg_tr(); seg_m(); seg_bl(); seg_br(); break;
            case 'B': seg_t(); seg_tr(); seg_br(); seg_b(); seg_mr(); seg_cv_t(); seg_cv_b(); break;
            case 'C': seg_t(); seg_tl(); seg_bl(); seg_b(); break;
            case 'D': seg_t(); seg_tr(); seg_br(); seg_b(); seg_cv_t(); seg_cv_b(); break;
            case 'E': seg_t(); seg_tl(); seg_ml(); seg_bl(); seg_b(); break;
            case 'F': seg_t(); seg_tl(); seg_ml(); seg_bl(); break;
            case 'G': seg_t(); seg_tl(); seg_bl(); seg_b(); seg_br(); seg_mr(); break;
            case 'H': seg_tl(); seg_bl(); seg_m(); seg_tr(); seg_br(); break;
            case 'I': seg_t(); seg_b(); seg_cv_t(); seg_cv_b(); break;
            case 'J': seg_tr(); seg_br(); seg_b(); seg_bl(); break;
            case 'K': seg_tl(); seg_bl(); seg_ml(); seg_d_tr(); seg_d_br(); break;
            case 'L': seg_tl(); seg_bl(); seg_b(); break;
            case 'M': seg_tl(); seg_bl(); seg_d_tl(); seg_d_tr(); seg_tr(); seg_br(); break;
            case 'N': seg_tl(); seg_bl(); seg_d_tl(); seg_d_br(); seg_tr(); seg_br(); break;
            case 'O': seg_t(); seg_tr(); seg_br(); seg_b(); seg_bl(); seg_tl(); break;
            case 'P': seg_t(); seg_tl(); seg_tr(); seg_m(); seg_bl(); break;
            case 'Q': seg_t(); seg_tr(); seg_br(); seg_b(); seg_bl(); seg_tl(); seg_d_br(); break;
            case 'R': seg_t(); seg_tl(); seg_tr(); seg_m(); seg_bl(); seg_d_br(); break;
            case 'S': seg_t(); seg_tl(); seg_m(); seg_br(); seg_b(); break;
            case 'T': seg_t(); seg_cv_t(); seg_cv_b(); break;
            case 'U': seg_tl(); seg_bl(); seg_b(); seg_br(); seg_tr(); break;
            case 'V': seg_tl(); seg_bl(); seg_d_bl(); seg_d_tr(); break;
            case 'W': seg_tl(); seg_bl(); seg_d_bl(); seg_d_br(); seg_tr(); seg_br(); break;
            case 'X': seg_d_tl(); seg_d_tr(); seg_d_bl(); seg_d_br(); break;
            case 'Y': seg_d_tl(); seg_d_tr(); seg_cv_b(); break;
            case 'Z': seg_t(); seg_d_tr(); seg_d_bl(); seg_b(); break;

            // ---- punctuation ----
            case '.': add_line(cx - 0.05f * scale, b, cx + 0.05f * scale, b, col); break;
            case '-': seg_m(); break;
            case '/': add_line(l, b, r, t, col); break;
            case ':': add_line(cx - 0.04f * scale, m + 0.18f * scale, cx + 0.04f * scale, m + 0.18f * scale, col);
                      add_line(cx - 0.04f * scale, m - 0.18f * scale, cx + 0.04f * scale, m - 0.18f * scale, col); break;
            case '+': add_line(cx, m - 0.22f * scale, cx, m + 0.22f * scale, col); seg_m(); break;
            case ' ': break;
            default: break;
        }
    }

    void draw_string(const char* str, float start_x, float y, float scale, const Color4& col) {
        float x = start_x;
        const float spacing = scale * 0.85f;
        while (*str) {
            draw_char(*str, x, y, scale, col);
            x += spacing;
            ++str;
        }
    }

    /// @brief Creates the render target, shader and font atlas.
    bool init_canvas(int width, int height) {
        if (initialized_) return true;
        tex_w_ = width;
        tex_h_ = height;

        glGenFramebuffers(1, &fbo_);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

        glGenTextures(1, &texture_);
        glBindTexture(GL_TEXTURE_2D, texture_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tex_w_, tex_h_, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_, 0);


        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "[InstrumentCanvas Error] FBO incomplete!\n";
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Flat colour, or coverage from the font atlas where U is set.
        const char* vert_src = R"(
            #version 330 core
            layout (location = 0) in vec2 aPos;
            layout (location = 1) in vec2 aUV;
            layout (location = 2) in vec4 aColor;
            out vec2 vUV;
            out vec4 vColor;
            void main() {
                vUV = aUV;
                vColor = aColor;
                gl_Position = vec4(aPos, 0.0, 1.0);
            }
        )";

        const char* frag_src = R"(
            #version 330 core
            in vec2 vUV;
            in vec4 vColor;
            out vec4 FragColor;
            uniform sampler2D uFont;
            void main() {
                float cover = vUV.x < 0.0 ? 1.0 : texture(uFont, vUV).r;
                FragColor = vec4(vColor.rgb, vColor.a * cover);
            }
        )";

        if (!shader_.init_from_source(vert_src, frag_src)) {
            return false;
        }
        shader_.use();
        shader_.set_int("uFont", 0);
        glUseProgram(0);

        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);

        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(InstVertex), (void*)offsetof(InstVertex, pos));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(InstVertex), (void*)offsetof(InstVertex, uv));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(InstVertex), (void*)offsetof(InstVertex, color));
        glEnableVertexAttribArray(2);
        glBindVertexArray(0);

        std::vector<uint8_t> atlas(static_cast<size_t>(kMenuFontTexW) * kMenuFontTexH, 0);
        decode_menu_font_atlas(atlas.data());
        glGenTextures(1, &font_tex_);
        glBindTexture(GL_TEXTURE_2D, font_tex_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GLint prev_align = 4;
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &prev_align);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, kMenuFontTexW, kMenuFontTexH, 0, GL_RED, GL_UNSIGNED_BYTE, atlas.data());
        glPixelStorei(GL_UNPACK_ALIGNMENT, prev_align);
        glBindTexture(GL_TEXTURE_2D, 0);

        initialized_ = true;
        return true;
    }

    void destroy_canvas() noexcept {
        if (fbo_) { glDeleteFramebuffers(1, &fbo_); fbo_ = 0; }
        if (texture_) { glDeleteTextures(1, &texture_); texture_ = 0; }
        if (font_tex_) { glDeleteTextures(1, &font_tex_); font_tex_ = 0; }
        if (vao_) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
        if (vbo_) { glDeleteBuffers(1, &vbo_); vbo_ = 0; }
        shader_.destroy();
        initialized_ = false;
    }

    void begin_canvas() {
        tri_verts_.clear();
        line_verts_.clear();
        reset_panel_viewport();
    }

    /// @brief Clears the target to `clear` and draws the batched geometry.
    void render_canvas(const Color4& clear) {
        reset_panel_viewport();

        GLint prev_fbo = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);
        GLint prev_viewport[4];
        glGetIntegerv(GL_VIEWPORT, prev_viewport);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
        glViewport(0, 0, tex_w_, tex_h_);
        glClearColor(clear.r, clear.g, clear.b, clear.a);
        // Colour only: the canvas is drawn in painter's order and never
        // depth-tests, so it carries no depth or stencil buffer to clear.
        glClear(GL_COLOR_BUFFER_BIT);

        // This is flat 2D geometry drawn in painter's order, and it runs in the
        // middle of a frame whose depth, stencil and blend state belong to the
        // 3D passes. Without neutralising that state here, the panel pages draw
        // correctly on the first frame and then vanish once the cockpit passes
        // have left their own depth and stencil settings behind.
        const GLboolean had_depth   = glIsEnabled(GL_DEPTH_TEST);
        const GLboolean had_stencil = glIsEnabled(GL_STENCIL_TEST);
        const GLboolean had_blend   = glIsEnabled(GL_BLEND);
        const GLboolean had_cull    = glIsEnabled(GL_CULL_FACE);
        GLint blend_func[4] = {GL_ONE, GL_ZERO, GL_ONE, GL_ZERO};
        glGetIntegerv(GL_BLEND_SRC_RGB, &blend_func[0]);
        glGetIntegerv(GL_BLEND_DST_RGB, &blend_func[1]);
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &blend_func[2]);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &blend_func[3]);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_CULL_FACE);
        // Glyph edges blend over what is beneath them. Destination alpha
        // accumulates coverage, so cells left clear stay transparent.
        glEnable(GL_BLEND);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        shader_.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, font_tex_);
        glBindVertexArray(vao_);

        // 1. Draw solid filled triangles and glyphs
        if (!tri_verts_.empty()) {
            glBindBuffer(GL_ARRAY_BUFFER, vbo_);
            glBufferData(GL_ARRAY_BUFFER, tri_verts_.size() * sizeof(InstVertex), tri_verts_.data(), GL_DYNAMIC_DRAW);
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(tri_verts_.size()));
        }

        // 2. Draw crisp wireframe lines / text
        if (!line_verts_.empty()) {
            glLineWidth(3.0f);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_);
            glBufferData(GL_ARRAY_BUFFER, line_verts_.size() * sizeof(InstVertex), line_verts_.data(), GL_DYNAMIC_DRAW);
            glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(line_verts_.size()));
            glLineWidth(1.0f);
        }

        glBindVertexArray(0);

        // Restore previous framebuffer and viewport before touching the texture.
        // Mipmaps must be generated with the texture NOT attached to the bound
        // framebuffer: generating them while it is still this FBO's colour
        // attachment is undefined, and in practice leaves the whole chain
        // black, so the panel displays render as dead glass.
        // Hand the caller's state back exactly as it was found.
        glBlendFuncSeparate(static_cast<GLenum>(blend_func[0]), static_cast<GLenum>(blend_func[1]),
                            static_cast<GLenum>(blend_func[2]), static_cast<GLenum>(blend_func[3]));
        if (!had_blend)  glDisable(GL_BLEND);
        if (had_depth)   glEnable(GL_DEPTH_TEST);
        if (had_stencil) glEnable(GL_STENCIL_TEST);
        if (had_cull)    glEnable(GL_CULL_FACE);

        glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo);
        glViewport(prev_viewport[0], prev_viewport[1], prev_viewport[2], prev_viewport[3]);

        // Generate mipmaps for smooth anti-aliased minification on the 3D dashboard
        glBindTexture(GL_TEXTURE_2D, texture_);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

public:
    InstrumentCanvas() = default;
    InstrumentCanvas(const InstrumentCanvas&) = delete;
    InstrumentCanvas& operator=(const InstrumentCanvas&) = delete;

    /// @brief Bind the canvas texture to an active texture unit
    void bind_texture(GLenum unit = GL_TEXTURE0) const noexcept {
        glActiveTexture(unit);
        glBindTexture(GL_TEXTURE_2D, texture_);
    }

    GLuint texture_id() const noexcept { return texture_; }
    GLuint fbo_id() const noexcept { return fbo_; }
    bool is_initialized() const noexcept { return initialized_; }
};

} // namespace fastjet::graphics
