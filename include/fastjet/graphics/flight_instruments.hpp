#pragma once

#include "fastjet/graphics/gl_common.hpp"
#include "fastjet/graphics/shader.hpp"
#include "fastjet/graphics/cockpit_telemetry.hpp"
#include "fastjet/aircraft/aircraft_type.hpp"
#include "fastjet/fdm/flight_state.hpp"
#include "fastjet/environment/atmosphere1976.hpp"
#include <vector>
#include <cmath>
#include <cstdio>
#include <algorithm>

namespace fastjet::graphics {

/// @brief 2D Vertex for instrument panel vector drawing
struct InstVertex {
    float pos[2];
    float color[4];
};

/// @brief Cockpit dashboard flight instruments rendered to an off-screen FBO texture
class FlightInstruments {
public:
    static constexpr int TEX_WIDTH = 2048;
    static constexpr int TEX_HEIGHT = 2048;

private:
    GLuint fbo_ = 0;
    GLuint texture_ = 0;
    GLuint rbo_ = 0;

    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    ShaderProgram shader_;
    bool initialized_ = false;

    std::vector<InstVertex> tri_verts_;
    std::vector<InstVertex> line_verts_;

    // Panel-local -> texture NDC transform. Each display on the instrument
    // panel draws in its own -1..1 space; these map that space into the
    // sub-rectangle of the shared atlas the display owns.
    float vp_cx_ = 0.0f, vp_cy_ = 0.0f, vp_sx_ = 1.0f, vp_sy_ = 1.0f;

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

    void add_line(float x0, float y0, float x1, float y1, const Color4& col) {
        line_verts_.push_back(InstVertex{{mx(x0), my(y0)}, {col.r, col.g, col.b, col.a}});
        line_verts_.push_back(InstVertex{{mx(x1), my(y1)}, {col.r, col.g, col.b, col.a}});
    }

    void add_tri(float x0, float y0, float x1, float y1, float x2, float y2, const Color4& col) {
        tri_verts_.push_back(InstVertex{{mx(x0), my(y0)}, {col.r, col.g, col.b, col.a}});
        tri_verts_.push_back(InstVertex{{mx(x1), my(y1)}, {col.r, col.g, col.b, col.a}});
        tri_verts_.push_back(InstVertex{{mx(x2), my(y2)}, {col.r, col.g, col.b, col.a}});
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
            const float a0 = i * (2.0f * 3.14159265f / segs);
            const float a1 = (i + 1) * (2.0f * 3.14159265f / segs);
            add_tri(cx, cy, cx + r * std::cos(a0), cy + r * std::sin(a0),
                    cx + r * std::cos(a1), cy + r * std::sin(a1), col);
        }
    }

    void add_circle_outline(float cx, float cy, float r, const Color4& col, int segs = 24) {
        for (int i = 0; i < segs; ++i) {
            const float a0 = i * (2.0f * 3.14159265f / segs);
            const float a1 = (i + 1) * (2.0f * 3.14159265f / segs);
            add_line(cx + r * std::cos(a0), cy + r * std::sin(a0),
                     cx + r * std::cos(a1), cy + r * std::sin(a1), col);
        }
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

public:
    FlightInstruments() = default;

    ~FlightInstruments() {
        destroy();
    }

    bool init() {
        if (initialized_) return true;

        // 1. Create Framebuffer and Texture
        glGenFramebuffers(1, &fbo_);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

        glGenTextures(1, &texture_);
        glBindTexture(GL_TEXTURE_2D, texture_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TEX_WIDTH, TEX_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_, 0);

        glGenRenderbuffers(1, &rbo_);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo_);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, TEX_WIDTH, TEX_HEIGHT);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo_);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "[FlightInstruments Error] FBO incomplete!\n";
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 2. Shaders for 2D UI geometry
        const char* vert_src = R"(
            #version 330 core
            layout (location = 0) in vec2 aPos;
            layout (location = 1) in vec4 aColor;
            out vec4 vColor;
            void main() {
                vColor = aColor;
                gl_Position = vec4(aPos, 0.0, 1.0);
            }
        )";

        const char* frag_src = R"(
            #version 330 core
            in vec4 vColor;
            out vec4 FragColor;
            void main() {
                FragColor = vColor;
            }
        )";

        if (!shader_.init_from_source(vert_src, frag_src)) {
            return false;
        }

        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);

        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(InstVertex), (void*)offsetof(InstVertex, pos));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(InstVertex), (void*)offsetof(InstVertex, color));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);

        initialized_ = true;
        return true;
    }

    void destroy() noexcept {
        if (fbo_) { glDeleteFramebuffers(1, &fbo_); fbo_ = 0; }
        if (texture_) { glDeleteTextures(1, &texture_); texture_ = 0; }
        if (rbo_) { glDeleteRenderbuffers(1, &rbo_); rbo_ = 0; }
        if (vao_) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
        if (vbo_) { glDeleteBuffers(1, &vbo_); vbo_ = 0; }
        shader_.destroy();
        initialized_ = false;
    }

    /// @brief Left MFD page: AoA indexer, ADI ball, Mach tape, plus Landing Gear & Surface status panel
    void draw_left_mfd(const fdm::FlightState& state, const AvionicsTelemetry& tel) {
        // Background: MFD dark screen bezel
        add_rect(0.0f, 0.0f, 2.0f, 2.0f, Color4{0.04f, 0.05f, 0.06f, 1.0f});

        const double alpha_deg = state.alpha() * (180.0 / 3.14159265);
        const double pitch_rad = state.pitch();
        const double roll_rad  = state.roll();

        // =========================================================================
        // 1. Three-Light Angle-of-Attack (AoA) Indexer (Left side: x in [-0.90, -0.65])
        // =========================================================================
        {
            const float ix = -0.78f;
            const Color4 off_col{0.12f, 0.12f, 0.12f, 1.0f};

            // Outer housing box
            add_rect(ix, 0.15f, 0.22f, 0.60f, Color4{0.08f, 0.09f, 0.10f, 1.0f});

            // Upper chevron (pointing down: \/)
            const bool high_aoa = (alpha_deg > 14.0);
            const Color4 top_col = high_aoa ? Color4::amber() : off_col;
            add_tri(ix, 0.30f, ix - 0.08f, 0.40f, ix + 0.08f, 0.40f, top_col);

            // Center donut
            const bool on_speed = (alpha_deg >= 12.0 && alpha_deg <= 14.0);
            const Color4 mid_col = on_speed ? Color4::hud_green() : off_col;
            add_circle(ix, 0.15f, 0.055f, mid_col, 20);
            add_circle(ix, 0.15f, 0.025f, Color4{0.08f, 0.09f, 0.10f, 1.0f}, 16);

            // Lower chevron (pointing up: /\)
            const bool low_aoa = (alpha_deg < 12.0);
            const Color4 bot_col = low_aoa ? Color4::hud_green() : off_col;
            add_tri(ix, 0.00f, ix - 0.08f, -0.10f, ix + 0.08f, -0.10f, bot_col);
        }

        // =========================================================================
        // 2. Attitude Director Indicator (ADI) Ball (Center: x in [-0.45, +0.45])
        // =========================================================================
        {
            const float adi_x = 0.0f;
            const float adi_y = 0.15f;
            const float adi_r = 0.44f;

            // ADI Outer Bezel ring
            add_circle(adi_x, adi_y, adi_r + 0.03f, Color4{0.18f, 0.19f, 0.20f, 1.0f}, 32);

            const float pitch_offset = static_cast<float>(pitch_rad / (3.14159265 * 0.5)) * adi_r * 0.9f;
            const float cos_r = static_cast<float>(std::cos(-roll_rad));
            const float sin_r = static_cast<float>(std::sin(-roll_rad));

            constexpr int ADI_SEGS = 36;
            for (int i = 0; i < ADI_SEGS; ++i) {
                const float a0 = i * (2.0f * 3.14159265f / ADI_SEGS);
                const float a1 = (i + 1) * (2.0f * 3.14159265f / ADI_SEGS);

                const float lx0 = adi_r * std::cos(a0);
                const float ly0 = adi_r * std::sin(a0);
                const float lx1 = adi_r * std::cos(a1);
                const float ly1 = adi_r * std::sin(a1);

                const float mid_y = (ly0 + ly1) * 0.5f - pitch_offset;
                const Color4 hemisphere_col = (mid_y >= 0.0f) ? Color4::sky_blue() : Color4::earth_brown();

                const float rx0 = adi_x + (lx0 * cos_r - ly0 * sin_r);
                const float ry0 = adi_y + (lx0 * sin_r + ly0 * cos_r);
                const float rx1 = adi_x + (lx1 * cos_r - ly1 * sin_r);
                const float ry1 = adi_y + (lx1 * sin_r + ly1 * cos_r);

                add_tri(adi_x, adi_y, rx0, ry0, rx1, ry1, hemisphere_col);
            }

            // White Horizon line across the ADI
            const float hx0 = -adi_r * cos_r - pitch_offset * (-sin_r);
            const float hy0 = -adi_r * sin_r + pitch_offset * (cos_r);
            const float hx1 =  adi_r * cos_r - pitch_offset * (-sin_r);
            const float hy1 =  adi_r * sin_r + pitch_offset * (cos_r);
            add_line(adi_x + hx0, adi_y + hy0, adi_x + hx1, adi_y + hy1, Color4::white());

            // Miniature Aircraft Symbol
            const Color4 symbol_col{1.0f, 0.85f, 0.0f, 1.0f};
            add_circle(adi_x, adi_y, 0.02f, symbol_col, 12);
            add_rect(adi_x - 0.12f, adi_y, 0.12f, 0.02f, symbol_col);
            add_rect(adi_x - 0.06f, adi_y - 0.03f, 0.02f, 0.04f, symbol_col);
            add_rect(adi_x + 0.12f, adi_y, 0.12f, 0.02f, symbol_col);
            add_rect(adi_x + 0.06f, adi_y - 0.03f, 0.02f, 0.04f, symbol_col);
        }

        // =========================================================================
        // 3. Mach Number Tape (Right side: x in [+0.60, +0.90])
        // =========================================================================
        {
            const float mach_x = 0.75f;
            const auto air = environment::Atmosphere1976::compute(state.altitude(), state.airspeed());
            const float mach = static_cast<float>(air.mach_number);

            add_rect(mach_x, 0.15f, 0.24f, 0.65f, Color4{0.08f, 0.09f, 0.10f, 1.0f});
            draw_string("M", mach_x - 0.04f, 0.40f, 0.08f, Color4::hud_green());

            char m_str[8];
            std::snprintf(m_str, sizeof(m_str), "%1.2f", std::clamp(mach, 0.0f, 9.99f));
            draw_string(m_str, mach_x - 0.09f, 0.28f, 0.07f, Color4::white());

            const float bar_bottom = -0.12f;
            const float bar_max_h  = 0.35f;
            const float fill_frac  = std::clamp(mach / 2.5f, 0.0f, 1.0f);
            const float bar_h      = bar_max_h * fill_frac;

            add_rect(mach_x, bar_bottom + bar_max_h * 0.5f, 0.08f, bar_max_h, Color4{0.15f, 0.16f, 0.18f, 1.0f});
            if (bar_h > 0.005f) {
                const Color4 fill_col = (mach >= 1.0f) ? Color4::amber() : Color4::hud_green();
                add_rect(mach_x, bar_bottom + bar_h * 0.5f, 0.07f, bar_h, fill_col);
            }
        }

        // =========================================================================
        // 4. Landing Gear, Speedbrakes & Wheel Brakes Panel (Lower Quadrant)
        // =========================================================================
        {
            const float py = -0.65f;
            add_rect(0.0f, py, 1.94f, 0.55f, Color4{0.06f, 0.07f, 0.09f, 1.0f});
            add_line(-0.97f, py + 0.275f, 0.97f, py + 0.275f, Color4{0.25f, 0.28f, 0.30f, 1.0f});

            // 4a. Three-Green Landing Gear Indicator Box
            const float gx = -0.62f;
            add_rect(gx, py, 0.58f, 0.46f, Color4{0.03f, 0.03f, 0.04f, 1.0f});
            draw_string("GEAR", gx - 0.12f, py + 0.16f, 0.065f, Color4{0.6f, 0.7f, 0.8f, 1.0f});

            Color4 nose_col = Color4{0.18f, 0.20f, 0.22f, 1.0f};
            Color4 left_col = nose_col;
            Color4 right_col = nose_col;

            if (tel.gear_collapsed) {
                nose_col = left_col = right_col = Color4::red();
            } else if (tel.gear_deployed && tel.gear_transit_pos >= 0.98) {
                nose_col = left_col = right_col = Color4::hud_green();
            } else if (tel.gear_deployed && tel.gear_transit_pos < 0.98) {
                nose_col = left_col = right_col = Color4::amber();
            }

            // Nose gear lamp
            add_circle(gx, py + 0.04f, 0.045f, nose_col, 16);
            draw_string("N", gx - 0.02f, py + 0.04f, 0.050f, Color4::black());

            // Left main gear lamp
            add_circle(gx - 0.14f, py - 0.07f, 0.045f, left_col, 16);
            draw_string("L", gx - 0.16f, py - 0.07f, 0.050f, Color4::black());

            // Right main gear lamp
            add_circle(gx + 0.14f, py - 0.07f, 0.045f, right_col, 16);
            draw_string("R", gx + 0.12f, py - 0.07f, 0.050f, Color4::black());

            if (tel.gear_collapsed) {
                draw_string("COLLAPSE", gx - 0.22f, py - 0.17f, 0.055f, Color4::red());
            } else if (!tel.gear_deployed) {
                draw_string("UP / LOCKED", gx - 0.25f, py - 0.17f, 0.055f, Color4{0.5f, 0.5f, 0.5f, 1.0f});
            } else {
                draw_string("DOWN / LOCKED", gx - 0.26f, py - 0.17f, 0.055f, Color4::hud_green());
            }

            // 4b. Speedbrake Status Box (Center)
            const float sx = 0.0f;
            add_rect(sx, py, 0.58f, 0.46f, Color4{0.03f, 0.03f, 0.04f, 1.0f});
            draw_string("SPDBRK", sx - 0.18f, py + 0.16f, 0.065f, Color4{0.6f, 0.7f, 0.8f, 1.0f});

            const bool sb_out = (tel.speedbrake_pos > 0.02);
            const Color4 sb_col = sb_out ? Color4::amber() : Color4::hud_green();
            char sb_buf[16];
            std::snprintf(sb_buf, sizeof(sb_buf), "%3.0f%%", tel.speedbrake_pos * 100.0);
            draw_string(sb_out ? "EXT" : "RET", sx - 0.18f, py + 0.03f, 0.070f, sb_col);
            draw_string(sb_buf, sx + 0.02f, py + 0.03f, 0.065f, sb_col);

            add_rect(sx, py - 0.10f, 0.46f, 0.055f, Color4{0.15f, 0.16f, 0.18f, 1.0f});
            if (tel.speedbrake_pos > 0.02) {
                const float bar_w = 0.46f * static_cast<float>(tel.speedbrake_pos);
                add_rect(sx - 0.23f + bar_w * 0.5f, py - 0.10f, bar_w, 0.045f, Color4::amber());
            }

            // 4c. Wheel Brakes & Caution/Warning Box (Right side)
            const float bx = 0.62f;
            add_rect(bx, py, 0.58f, 0.46f, Color4{0.03f, 0.03f, 0.04f, 1.0f});
            draw_string("SYSTEMS", bx - 0.18f, py + 0.16f, 0.065f, Color4{0.6f, 0.7f, 0.8f, 1.0f});

            const bool braking = (tel.brake_left > 0.05 || tel.brake_right > 0.05);
            add_rect(bx, py + 0.03f, 0.48f, 0.08f, braking ? Color4::amber() : Color4{0.15f, 0.16f, 0.18f, 1.0f});
            draw_string("WHEEL BRAKE", bx - 0.22f, py + 0.03f, 0.055f, braking ? Color4::black() : Color4{0.5f, 0.5f, 0.5f, 1.0f});

            if (tel.is_crashed) {
                add_rect(bx, py - 0.10f, 0.48f, 0.08f, Color4::red());
                draw_string("IMPACT / CRASH", bx - 0.22f, py - 0.10f, 0.055f, Color4::white());
            } else if (tel.over_g_alert) {
                add_rect(bx, py - 0.10f, 0.48f, 0.08f, Color4::amber());
                draw_string("OVER-G WARNING", bx - 0.22f, py - 0.10f, 0.055f, Color4::black());
            } else if (tel.high_aoa_alert) {
                add_rect(bx, py - 0.10f, 0.48f, 0.08f, Color4::amber());
                draw_string("HIGH AOA / STALL", bx - 0.23f, py - 0.10f, 0.050f, Color4::black());
            } else if (tel.bingo_fuel_alert) {
                add_rect(bx, py - 0.10f, 0.48f, 0.08f, Color4::amber());
                draw_string("LOW FUEL BINGO", bx - 0.22f, py - 0.10f, 0.050f, Color4::black());
            } else {
                add_rect(bx, py - 0.10f, 0.48f, 0.08f, Color4{0.08f, 0.18f, 0.10f, 1.0f});
                draw_string("FLCS NORMAL", bx - 0.20f, py - 0.10f, 0.055f, Color4::hud_green());
            }
        }
    }

    /// @brief Right MFD page: Authentic F-16 F110 Engine & Fuel Management Display (ENG/FUEL)
    void draw_right_mfd([[maybe_unused]] const fdm::FlightState& state, const AvionicsTelemetry& tel) {
        // Background: MFD dark screen bezel
        add_rect(0.0f, 0.0f, 2.0f, 2.0f, Color4{0.03f, 0.04f, 0.05f, 1.0f});

        // 1. Header & Title Bar with Detent Indicator
        // The powerplant is named from the active airframe: this page is shared
        // by all five aircraft, only one of which flies behind an F110.
        add_rect(0.0f, 0.88f, 1.94f, 0.16f, Color4{0.06f, 0.08f, 0.11f, 1.0f});
        char eng_hdr[48];
        std::snprintf(eng_hdr, sizeof(eng_hdr), "%.*s ENG/FUEL",
                      static_cast<int>(aircraft::engine_designation(tel.aircraft_type).size()),
                      aircraft::engine_designation(tel.aircraft_type).data());
        draw_string(eng_hdr, -0.92f, 0.88f, 0.070f, Color4::hud_green());

        Color4 detent_col = Color4::hud_green();
        if (std::string(tel.detent_str) == "AFTERBURNER") detent_col = Color4::amber();
        else if (std::string(tel.detent_str) == "CUTOFF") detent_col = Color4::red();
        else if (std::string(tel.detent_str) == "IDLE") detent_col = Color4::sky_blue();

        char det_buf[32];
        std::snprintf(det_buf, sizeof(det_buf), "[ %s ]", tel.detent_str);
        draw_string(det_buf, 0.20f, 0.88f, 0.070f, detent_col);

        // 2. Engine Primary Parameters (Left column)
        const float col1_x = -0.48f;

        // Core RPM %
        {
            const float y = 0.65f;
            draw_string("RPM %", col1_x - 0.42f, y, 0.065f, Color4{0.7f, 0.75f, 0.8f, 1.0f});
            char buf[16];
            std::snprintf(buf, sizeof(buf), "%5.1f", tel.engine_rpm_pct);
            const Color4 rpm_col = (tel.engine_rpm_pct > 100.0) ? Color4::amber() : Color4::hud_green();
            draw_string(buf, col1_x + 0.10f, y, 0.075f, rpm_col);

            add_rect(col1_x, y - 0.08f, 0.84f, 0.04f, Color4{0.12f, 0.13f, 0.15f, 1.0f});
            const float rpm_frac = std::clamp(static_cast<float>(tel.engine_rpm_pct / 110.0), 0.0f, 1.0f);
            if (rpm_frac > 0.01f) {
                add_rect(col1_x - 0.42f + (0.84f * rpm_frac) * 0.5f, y - 0.08f, 0.84f * rpm_frac, 0.035f, rpm_col);
            }
        }

        // Fan Turbine Inlet Temp (FTIT C)
        {
            const float y = 0.40f;
            draw_string("FTIT C", col1_x - 0.42f, y, 0.065f, Color4{0.7f, 0.75f, 0.8f, 1.0f});
            char buf[16];
            std::snprintf(buf, sizeof(buf), "%4.0f", tel.engine_ftit_deg_c);
            const Color4 ftit_col = (tel.engine_ftit_deg_c > 920.0) ? Color4::amber() : Color4::hud_green();
            draw_string(buf, col1_x + 0.12f, y, 0.075f, ftit_col);

            add_rect(col1_x, y - 0.08f, 0.84f, 0.04f, Color4{0.12f, 0.13f, 0.15f, 1.0f});
            const float ftit_frac = std::clamp(static_cast<float>(tel.engine_ftit_deg_c / 1100.0), 0.0f, 1.0f);
            if (ftit_frac > 0.01f) {
                add_rect(col1_x - 0.42f + (0.84f * ftit_frac) * 0.5f, y - 0.08f, 0.84f * ftit_frac, 0.035f, ftit_col);
            }
        }

        // Net Thrust & Exhaust Nozzle Position
        {
            const float y = 0.16f;
            draw_string("THRUST", col1_x - 0.42f, y, 0.065f, Color4{0.7f, 0.75f, 0.8f, 1.0f});
            char buf[20];
            std::snprintf(buf, sizeof(buf), "%4.1f kN", tel.net_thrust_n / 1000.0);
            draw_string(buf, col1_x + 0.02f, y, 0.070f, Color4::hud_green());

            draw_string("NOZZLE", col1_x - 0.42f, y - 0.12f, 0.065f, Color4{0.7f, 0.75f, 0.8f, 1.0f});
            std::snprintf(buf, sizeof(buf), "%3.0f %%", tel.nozzle_pos_pct);
            draw_string(buf, col1_x + 0.08f, y - 0.12f, 0.070f, Color4::hud_green());
        }

        // 3. Secondary Parameters & Pressures (Right column)
        const float col2_x = 0.52f;
        {
            const float y = 0.65f;
            draw_string("OIL PRESS", col2_x - 0.38f, y, 0.060f, Color4{0.7f, 0.75f, 0.8f, 1.0f});
            char buf[20];
            std::snprintf(buf, sizeof(buf), "%2.0f PSI", tel.oil_pressure_psi);
            draw_string(buf, col2_x + 0.10f, y, 0.065f, Color4::hud_green());

            draw_string("HYD SYS A", col2_x - 0.38f, y - 0.12f, 0.060f, Color4{0.7f, 0.75f, 0.8f, 1.0f});
            std::snprintf(buf, sizeof(buf), "%4.0f", tel.hyd_press_a_psi);
            draw_string(buf, col2_x + 0.10f, y - 0.12f, 0.065f, Color4::hud_green());

            draw_string("HYD SYS B", col2_x - 0.38f, y - 0.24f, 0.060f, Color4{0.7f, 0.75f, 0.8f, 1.0f});
            std::snprintf(buf, sizeof(buf), "%4.0f", tel.hyd_press_b_psi);
            draw_string(buf, col2_x + 0.10f, y - 0.24f, 0.065f, Color4::hud_green());

            draw_string("THROTTLE", col2_x - 0.38f, y - 0.38f, 0.060f, Color4{0.7f, 0.75f, 0.8f, 1.0f});
            std::snprintf(buf, sizeof(buf), "%3.0f %%", tel.throttle_input * 100.0);
            draw_string(buf, col2_x + 0.10f, y - 0.38f, 0.065f, Color4::hud_green());
            add_rect(col2_x, y - 0.46f, 0.76f, 0.035f, Color4{0.12f, 0.13f, 0.15f, 1.0f});
            const float th_frac = std::clamp(static_cast<float>(tel.throttle_input), 0.0f, 1.0f);
            if (th_frac > 0.01f) {
                add_rect(col2_x - 0.38f + (0.76f * th_frac) * 0.5f, y - 0.46f, 0.76f * th_frac, 0.030f, detent_col);
            }
        }

        // 4. Fuel Subsystem Section (Lower half)
        {
            const float fy = -0.32f;
            add_line(-0.95f, fy + 0.20f, 0.95f, fy + 0.20f, Color4{0.20f, 0.22f, 0.25f, 1.0f});

            draw_string("INTERNAL FUEL SYSTEM", -0.90f, fy + 0.10f, 0.065f, Color4{0.6f, 0.7f, 0.8f, 1.0f});

            char buf[32];
            std::snprintf(buf, sizeof(buf), "TOTAL: %4.0f KG", tel.fuel_remaining_kg);
            const Color4 fuel_col = (tel.fuel_remaining_kg < 800.0) ? Color4::amber() : Color4::hud_green();
            draw_string(buf, -0.90f, fy - 0.04f, 0.075f, fuel_col);

            std::snprintf(buf, sizeof(buf), "FLOW: %4.0f KG/H", tel.fuel_flow_kg_hr);
            draw_string(buf, 0.08f, fy - 0.04f, 0.070f, Color4::hud_green());

            // Fuel quantity bar
            add_rect(0.0f, fy - 0.16f, 1.80f, 0.06f, Color4{0.12f, 0.13f, 0.15f, 1.0f});
            const float f_frac = std::clamp(static_cast<float>(tel.fuel_fraction), 0.0f, 1.0f);
            if (f_frac > 0.01f) {
                add_rect(-0.90f + (1.80f * f_frac) * 0.5f, fy - 0.16f, 1.80f * f_frac, 0.05f, fuel_col);
            }

            // Inceptor control mode indicator box
            add_rect(0.0f, -0.74f, 1.84f, 0.16f, Color4{0.06f, 0.07f, 0.09f, 1.0f});
            draw_string("CONTROL INCEPTOR:", -0.88f, -0.72f, 0.055f, Color4{0.5f, 0.6f, 0.7f, 1.0f});
            char ctrl_buf[48];
            std::snprintf(ctrl_buf, sizeof(ctrl_buf), "%s", tel.input_name);
            draw_string(ctrl_buf, -0.22f, -0.72f, 0.060f, tel.is_hardware_hotas ? Color4::hud_green() : Color4::sky_blue());

            draw_string("[R] RESET SIM   [T] TRIM ZERO   [I] INV THROTTLE   [B] SPDBRK", -0.88f, -0.80f, 0.042f, Color4{0.45f, 0.50f, 0.55f, 1.0f});
        }
    }

    /// @brief Centre display (CPD): horizontal situation, compass rose, and DED scratchpad
    void draw_center_cpd(const fdm::FlightState& state, const AvionicsTelemetry& tel) {
        add_rect(0.0f, 0.0f, 2.0f, 2.0f, Color4{0.03f, 0.04f, 0.05f, 1.0f});

        const Color4 hsd_line{0.15f, 0.75f, 0.35f, 1.0f};
        const Color4 dim_line{0.10f, 0.40f, 0.20f, 1.0f};
        const Color4 label{0.75f, 0.80f, 0.75f, 1.0f};

        const float heading = static_cast<float>(state.yaw());

        // Compass rose
        const float rose_r = 0.55f;
        const float rose_y = -0.15f;
        add_circle_outline(0.0f, rose_y, rose_r, hsd_line, 48);
        add_circle_outline(0.0f, rose_y, rose_r * 0.66f, dim_line, 40);
        add_circle_outline(0.0f, rose_y, rose_r * 0.33f, dim_line, 32);

        for (int deg = 0; deg < 360; deg += 10) {
            const float rel = static_cast<float>(deg) * 3.14159265f / 180.0f - heading;
            const float ux = std::sin(rel);
            const float uy = std::cos(rel);
            const bool major = (deg % 30 == 0);
            const float t_in = major ? 0.86f : 0.93f;
            add_line(ux * rose_r * t_in, rose_y + uy * rose_r * t_in,
                     ux * rose_r, rose_y + uy * rose_r,
                     major ? hsd_line : dim_line);
        }

        struct { const char* s; int deg; } cardinals[] = {
            {"N", 0}, {"E", 90}, {"S", 180}, {"W", 270}
        };
        for (const auto& c : cardinals) {
            const float rel = static_cast<float>(c.deg) * 3.14159265f / 180.0f - heading;
            const float lx = std::sin(rel) * rose_r * 0.76f;
            const float ly = std::cos(rel) * rose_r * 0.76f + rose_y;
            draw_string(c.s, lx - 0.03f, ly, 0.085f, label);
        }

        // Ownship symbol
        const Color4 own{1.0f, 0.85f, 0.0f, 1.0f};
        add_tri(0.0f, rose_y + 0.07f, -0.05f, rose_y - 0.05f, 0.05f, rose_y - 0.05f, own);

        // Steerpoint route
        {
            const float leg_bearing = 0.6f;
            const float rel = leg_bearing - heading;
            const float ex = std::sin(rel) * rose_r * 0.9f;
            const float ey = std::cos(rel) * rose_r * 0.9f + rose_y;
            add_line(0.0f, rose_y, ex, ey, Color4{0.95f, 0.55f, 0.10f, 1.0f});
            add_circle_outline(ex, ey, 0.045f, Color4{0.95f, 0.55f, 0.10f, 1.0f}, 12);
        }

        // Data Entry Display (DED) Scratchpad at the top
        {
            const float ded_y = 0.68f;
            add_rect(0.0f, ded_y, 1.84f, 0.32f, Color4{0.04f, 0.10f, 0.05f, 1.0f});
            add_rect(0.0f, ded_y, 1.84f, 0.32f, Color4::hud_green()); // outline

            draw_string("DED: NAV/TRK   STPT: 01", -0.84f, ded_y + 0.09f, 0.060f, Color4::hud_green());

            char b_buf[48];
            std::snprintf(b_buf, sizeof(b_buf), "BINGO: 800 KG   QTY: %4.0f KG", tel.fuel_remaining_kg);
            draw_string(b_buf, -0.84f, ded_y - 0.01f, 0.060f, (tel.fuel_remaining_kg < 800.0) ? Color4::amber() : Color4::hud_green());

            draw_string("WIND: 045/10 KT   BARO: 29.92", -0.84f, ded_y - 0.11f, 0.055f, Color4::hud_green());
        }

        // Ground speed and altitude blocks along the bottom edge
        {
            char buf[24];
            const double gs_kt = state.airspeed() * 1.94384;
            std::snprintf(buf, sizeof(buf), "%3.0f", std::clamp(gs_kt, 0.0, 999.0));
            draw_string("GS", -0.92f, -0.86f, 0.07f, label);
            draw_string(buf, -0.74f, -0.86f, 0.085f, Color4::hud_green());

            const double alt_ft = state.altitude() * 3.28084;
            std::snprintf(buf, sizeof(buf), "%5.0f", std::clamp(alt_ft, -999.0, 99999.0));
            draw_string("ALT", 0.34f, -0.86f, 0.07f, label);
            draw_string(buf, 0.60f, -0.86f, 0.085f, Color4::hud_green());
        }
    }

    /// @brief Update every instrument display and render off-screen to the atlas texture
    void update_and_render(const fdm::FlightState& state, const AvionicsTelemetry& telemetry = AvionicsTelemetry{}) {
        if (!initialized_) return;

        tri_verts_.clear();
        line_verts_.clear();

        set_panel_viewport(-0.5f, 0.5f, 0.5f, 0.5f);
        draw_left_mfd(state, telemetry);

        set_panel_viewport(0.5f, 0.5f, 0.5f, 0.5f);
        draw_right_mfd(state, telemetry);

        set_panel_viewport(0.0f, -0.5f, 0.5f, 0.5f);
        draw_center_cpd(state, telemetry);

        reset_panel_viewport();

        // =========================================================================
        // Render pass to FBO
        // =========================================================================
        GLint prev_fbo = 0;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);
        GLint prev_viewport[4];
        glGetIntegerv(GL_VIEWPORT, prev_viewport);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
        glViewport(0, 0, TEX_WIDTH, TEX_HEIGHT);
        glClearColor(0.04f, 0.05f, 0.06f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // This is flat 2D geometry drawn in painter's order, and it runs in the
        // middle of a frame whose depth, stencil and blend state belong to the
        // 3D passes. Without neutralising that state here, the panel pages draw
        // correctly on the first frame and then vanish once the cockpit passes
        // have left their own depth and stencil settings behind.
        const GLboolean had_depth   = glIsEnabled(GL_DEPTH_TEST);
        const GLboolean had_stencil = glIsEnabled(GL_STENCIL_TEST);
        const GLboolean had_blend   = glIsEnabled(GL_BLEND);
        const GLboolean had_cull    = glIsEnabled(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);

        shader_.use();
        glBindVertexArray(vao_);

        // 1. Draw solid filled triangles
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
        if (had_depth)   glEnable(GL_DEPTH_TEST);
        if (had_stencil) glEnable(GL_STENCIL_TEST);
        if (had_blend)   glEnable(GL_BLEND);
        if (had_cull)    glEnable(GL_CULL_FACE);

        glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo);
        glViewport(prev_viewport[0], prev_viewport[1], prev_viewport[2], prev_viewport[3]);

        // Generate mipmaps for smooth anti-aliased minification on the 3D dashboard
        glBindTexture(GL_TEXTURE_2D, texture_);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    /// @brief Bind the instrument texture to an active texture unit
    void bind_texture(GLenum unit = GL_TEXTURE0) const noexcept {
        glActiveTexture(unit);
        glBindTexture(GL_TEXTURE_2D, texture_);
    }

    GLuint texture_id() const noexcept { return texture_; }
    GLuint fbo_id() const noexcept { return fbo_; }
    bool is_initialized() const noexcept { return initialized_; }
};

} // namespace fastjet::graphics
