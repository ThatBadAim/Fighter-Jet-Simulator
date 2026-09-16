#pragma once

#include "fastjet/graphics/gl_common.hpp"
#include "fastjet/graphics/shader.hpp"
#include "fastjet/aircraft/aircraft_type.hpp"
#include "fastjet/aircraft/aircraft_config.hpp"
#include <vector>
#include <string>
#include <cctype>
#include <cstring>
#include <algorithm>

namespace fastjet::graphics {

/// @brief Vertex structure for tactical 2D menu overlay
struct MenuVertex {
    float pos[3];
    float color[4];
};

/// @brief Interactive tactical in-flight Aircraft Selection Menu
class AircraftMenu {
private:
    GLuint vao_triangles_ = 0;
    GLuint vbo_triangles_ = 0;
    GLuint vao_lines_     = 0;
    GLuint vbo_lines_     = 0;
    ShaderProgram shader_;
    bool initialized_ = false;

    /// Number of selectable airframes, matching aircraft::AircraftType.
    static constexpr int kAircraftCount = 5;

    bool is_open_ = false;
    int selected_index_ = 0; // [0, kAircraftCount)

    std::vector<MenuVertex> triangle_verts_;
    std::vector<MenuVertex> line_verts_;

    void add_triangle(float x0, float y0, float x1, float y1, float x2, float y2, const Color4& col) {
        triangle_verts_.push_back({{x0, y0, 0.0f}, {col.r, col.g, col.b, col.a}});
        triangle_verts_.push_back({{x1, y1, 0.0f}, {col.r, col.g, col.b, col.a}});
        triangle_verts_.push_back({{x2, y2, 0.0f}, {col.r, col.g, col.b, col.a}});
    }

    void add_quad(float x0, float y0, float x1, float y1, const Color4& col) {
        add_triangle(x0, y0, x1, y0, x1, y1, col);
        add_triangle(x0, y0, x1, y1, x0, y1, col);
    }

    void add_line(float x0, float y0, float x1, float y1, const Color4& col) {
        line_verts_.push_back({{x0, y0, 0.0f}, {col.r, col.g, col.b, col.a}});
        line_verts_.push_back({{x1, y1, 0.0f}, {col.r, col.g, col.b, col.a}});
    }

    void add_rect_outline(float x0, float y0, float x1, float y1, const Color4& col) {
        add_line(x0, y0, x1, y0, col);
        add_line(x1, y0, x1, y1, col);
        add_line(x1, y1, x0, y1, col);
        add_line(x0, y1, x0, y0, col);
    }

    void draw_char(char raw_c, float cx, float cy, float scale, const Color4& col) {
        const char c = static_cast<char>(std::toupper(static_cast<unsigned char>(raw_c)));
        const float w = 0.65f * scale;
        const float h = 1.0f * scale;
        const float l = cx - w * 0.5f;
        const float r = cx + w * 0.5f;
        const float b = cy - h * 0.5f;
        const float m = cy;
        const float t = cy + h * 0.5f;

        auto h_top  = [&]() { add_line(l, t, r, t, col); };
        auto h_midl = [&]() { add_line(l, m, cx, m, col); };
        auto h_midr = [&]() { add_line(cx, m, r, m, col); };
        auto h_mid  = [&]() { add_line(l, m, r, m, col); };
        auto h_bot  = [&]() { add_line(l, b, r, b, col); };

        auto v_tl = [&]() { add_line(l, m, l, t, col); };
        auto v_tr = [&]() { add_line(r, m, r, t, col); };
        auto v_bl = [&]() { add_line(l, b, l, m, col); };
        auto v_br = [&]() { add_line(r, b, r, m, col); };
        auto v_l  = [&]() { add_line(l, b, l, t, col); };
        auto v_r  = [&]() { add_line(r, b, r, t, col); };

        auto v_c  = [&]() { add_line(cx, b, cx, t, col); };
        auto v_bc = [&]() { add_line(cx, b, cx, m, col); };

        auto d_tlc = [&]() { add_line(l, t, cx, m, col); };
        auto d_trc = [&]() { add_line(r, t, cx, m, col); };
        auto d_blc = [&]() { add_line(l, b, cx, m, col); };
        auto d_brc = [&]() { add_line(r, b, cx, m, col); };

        switch (c) {
            case '0': h_top(); h_bot(); v_l(); v_r(); add_line(r, t, l, b, col); break;
            case '1': add_line(cx - 0.25f * w, t - 0.25f * h, cx, t, col); v_c(); add_line(cx - 0.35f * w, b, cx + 0.35f * w, b, col); break;
            case '2': h_top(); v_tr(); h_mid(); v_bl(); h_bot(); break;
            case '3': h_top(); v_tr(); h_midr(); v_br(); h_bot(); break;
            case '4': v_tl(); h_mid(); v_r(); break;
            case '5': h_top(); v_tl(); h_mid(); v_br(); h_bot(); break;
            case '6': h_top(); v_l(); h_mid(); v_br(); h_bot(); break;
            case '7': h_top(); v_tr(); v_br(); break;
            case '8': h_top(); h_mid(); h_bot(); v_l(); v_r(); break;
            case '9': h_top(); v_tl(); h_mid(); v_r(); h_bot(); break;

            case 'A': h_top(); v_l(); v_r(); h_mid(); break;
            case 'B': v_l(); h_top(); h_mid(); h_bot(); add_line(r, m + 0.1f * h, r, t - 0.1f * h, col); add_line(r, b + 0.1f * h, r, m - 0.1f * h, col); break;
            case 'C': h_top(); v_l(); h_bot(); break;
            case 'D': v_l(); h_top(); h_bot(); add_line(r, b + 0.15f * h, r, t - 0.15f * h, col); break;
            case 'E': h_top(); v_l(); h_midl(); h_bot(); break;
            case 'F': h_top(); v_l(); h_midl(); break;
            case 'G': h_top(); v_l(); h_bot(); v_br(); h_midr(); break;
            case 'H': v_l(); v_r(); h_mid(); break;
            case 'I': h_top(); h_bot(); v_c(); break;
            case 'J': v_r(); h_bot(); v_bl(); break;
            case 'K': v_l(); d_trc(); d_brc(); break;
            case 'L': v_l(); h_bot(); break;
            case 'M': v_l(); v_r(); d_tlc(); d_trc(); break;
            case 'N': v_l(); v_r(); add_line(l, t, r, b, col); break;
            case 'O': h_top(); h_bot(); v_l(); v_r(); break;
            case 'P': v_l(); h_top(); v_tr(); h_mid(); break;
            case 'Q': h_top(); h_bot(); v_l(); v_r(); add_line(cx, m - 0.1f * h, r, b - 0.1f * h, col); break;
            case 'R': v_l(); h_top(); v_tr(); h_mid(); d_brc(); break;
            case 'S': h_top(); v_tl(); h_mid(); v_br(); h_bot(); break;
            case 'T': h_top(); v_c(); break;
            case 'U': v_l(); v_r(); h_bot(); break;
            case 'V': add_line(l, t, cx, b, col); add_line(r, t, cx, b, col); break;
            case 'W': v_l(); v_r(); d_blc(); d_brc(); break;
            case 'X': add_line(l, t, r, b, col); add_line(l, b, r, t, col); break;
            case 'Y': d_tlc(); d_trc(); v_bc(); break;
            case 'Z': h_top(); add_line(r, t, l, b, col); h_bot(); break;

            case '-': h_mid(); break;
            case '+': h_mid(); v_c(); break;
            case '.': add_line(cx - 0.05f * scale, b, cx + 0.05f * scale, b, col); break;
            case ':':
                add_line(cx - 0.04f * scale, m + 0.2f * scale, cx + 0.04f * scale, m + 0.2f * scale, col);
                add_line(cx - 0.04f * scale, m - 0.2f * scale, cx + 0.04f * scale, m - 0.2f * scale, col);
                break;
            case '/': add_line(l, b, r, t, col); break;
            case '\\': add_line(l, t, r, b, col); break;
            case '[': h_top(); h_bot(); v_l(); break;
            case ']': h_top(); h_bot(); v_r(); break;
            case '>': add_line(l, t, r, m, col); add_line(r, m, l, b, col); break;
            case '<': add_line(r, t, l, m, col); add_line(l, m, r, b, col); break;
            case '(': add_line(r, t, l, m, col); add_line(l, m, r, b, col); break;
            case ')': add_line(l, t, r, m, col); add_line(r, m, l, b, col); break;
            case '=':
                add_line(l, m + 0.15f * scale, r, m + 0.15f * scale, col);
                add_line(l, m - 0.15f * scale, r, m - 0.15f * scale, col);
                break;
            case ',': add_line(cx, b + 0.05f * scale, cx - 0.08f * scale, b - 0.1f * scale, col); break;
            case '|': v_c(); break;
            case '_': h_bot(); break;
            case ' ':
            default:
                break;
        }
    }

    /// @brief Per-character advance width, as a multiple of the cell scale.
    ///
    /// The glyph cell is a fixed-width skeleton, but advancing every character
    /// by the same amount leaves narrow characters swimming in white space and
    /// makes the text colour visibly uneven. Narrow glyphs therefore claim less
    /// of the line than wide ones, which is what proportional spacing buys.
    [[nodiscard]] static float char_advance(char raw_c) noexcept {
        const char c = static_cast<char>(std::toupper(static_cast<unsigned char>(raw_c)));
        switch (c) {
            case 'I': case '1': case '|': case ':': case '.': case ',':
                return 0.38f;
            case 'J': case 'L': case 'T': case 'Y': case '[': case ']':
            case '(': case ')': case '/': case '\\': case '-':
                return 0.68f;
            case 'M': case 'W':
                return 1.02f;
            case ' ':
                return 0.55f;
            default:
                return 0.82f;
        }
    }

    /// @brief Width of a rendered string, for centring and right-alignment.
    [[nodiscard]] static float measure_string(const char* str, float scale) noexcept {
        if (!str || !*str) return 0.0f;
        float w = 0.0f;
        for (const char* p = str; *p; ++p) {
            w += char_advance(*p) * scale;
        }
        return w;
    }

    void draw_string(const char* str, float start_x, float y, float scale, const Color4& col) {
        if (!str) return;
        float x = start_x;
        while (*str) {
            const float advance = char_advance(*str) * scale;
            // Centre each glyph inside the slice of the line it claims, so a
            // narrow character sits evenly between its neighbours.
            draw_char(*str, x + advance * 0.5f, y, scale, col);
            x += advance;
            ++str;
        }
    }

    void draw_string_centered(const char* str, float center_x, float y, float scale, const Color4& col) {
        if (!str || !*str) return;
        draw_string(str, center_x - measure_string(str, scale) * 0.5f, y, scale, col);
    }

    /// @brief Draws a string ending at right_x, for right-aligned badges.
    void draw_string_right(const char* str, float right_x, float y, float scale, const Color4& col) {
        if (!str || !*str) return;
        draw_string(str, right_x - measure_string(str, scale), y, scale, col);
    }

public:
    AircraftMenu() = default;
    ~AircraftMenu() { destroy(); }

    bool init() {
        if (initialized_) return true;

        const char* vert_src = R"(
            #version 330 core
            layout (location = 0) in vec3 aPos;
            layout (location = 1) in vec4 aColor;
            uniform mat4 uProjection;
            out vec4 vColor;
            void main() {
                vColor = aColor;
                gl_Position = uProjection * vec4(aPos, 1.0);
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

        // Triangle mesh setup
        glGenVertexArrays(1, &vao_triangles_);
        glGenBuffers(1, &vbo_triangles_);
        glBindVertexArray(vao_triangles_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_triangles_);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MenuVertex), (void*)offsetof(MenuVertex, pos));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(MenuVertex), (void*)offsetof(MenuVertex, color));
        glEnableVertexAttribArray(1);

        // Line mesh setup
        glGenVertexArrays(1, &vao_lines_);
        glGenBuffers(1, &vbo_lines_);
        glBindVertexArray(vao_lines_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_lines_);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MenuVertex), (void*)offsetof(MenuVertex, pos));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(MenuVertex), (void*)offsetof(MenuVertex, color));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
        initialized_ = true;
        return true;
    }

    void destroy() {
        if (vbo_triangles_) { glDeleteBuffers(1, &vbo_triangles_); vbo_triangles_ = 0; }
        if (vao_triangles_) { glDeleteVertexArrays(1, &vao_triangles_); vao_triangles_ = 0; }
        if (vbo_lines_)     { glDeleteBuffers(1, &vbo_lines_); vbo_lines_ = 0; }
        if (vao_lines_)     { glDeleteVertexArrays(1, &vao_lines_); vao_lines_ = 0; }
        shader_.destroy();
        initialized_ = false;
    }

    bool is_open() const noexcept { return is_open_; }

    void open(aircraft::AircraftType current) noexcept {
        is_open_ = true;
        selected_index_ = static_cast<int>(current);
        if (selected_index_ < 0 || selected_index_ >= kAircraftCount) selected_index_ = 0;
    }

    void close() noexcept {
        is_open_ = false;
    }

    void toggle(aircraft::AircraftType current) noexcept {
        if (is_open_) {
            close();
        } else {
            open(current);
        }
    }

    void move_up() noexcept {
        selected_index_ = (selected_index_ + kAircraftCount - 1) % kAircraftCount;
    }

    void move_down() noexcept {
        selected_index_ = (selected_index_ + 1) % kAircraftCount;
    }

    void select_index(int idx) noexcept {
        if (idx >= 0 && idx < kAircraftCount) {
            selected_index_ = idx;
        }
    }

    int selected_index() const noexcept { return selected_index_; }

    aircraft::AircraftType get_selected_type() const noexcept {
        return static_cast<aircraft::AircraftType>(selected_index_);
    }

    void render(float aspect, aircraft::AircraftType active_aircraft) {
        if (!initialized_ || !is_open_) return;

        triangle_verts_.clear();
        line_verts_.clear();

        // ---------------------------------------------------------------------
        // Color Palette (Tactical Glass & Avionics HUD)
        // ---------------------------------------------------------------------
        const Color4 col_bg_glass     {0.04f, 0.07f, 0.11f, 0.88f}; // Dark semi-translucent glass
        const Color4 col_card_bg     {0.02f, 0.05f, 0.08f, 0.75f}; // Sub-card backdrop
        const Color4 col_highlight_bg{0.08f, 0.22f, 0.18f, 0.75f}; // Highlighted entry fill

        const Color4 col_border_outer{0.15f, 0.50f, 0.60f, 0.90f}; // Outer cyan bezel
        const Color4 col_border_inner{0.10f, 0.35f, 0.45f, 0.60f}; // Inner bezel
        const Color4 col_title       {0.25f, 1.00f, 0.80f, 1.00f}; // Cyan/mint glowing title
        const Color4 col_text_dim    {0.45f, 0.65f, 0.65f, 0.85f}; // Dim cyan text
        const Color4 col_text_bright {0.20f, 1.00f, 0.40f, 1.00f}; // Bright tactical green
        const Color4 col_active_badge{1.00f, 0.75f, 0.10f, 1.00f}; // Amber active badge
        const Color4 col_spec_label  {0.35f, 0.80f, 0.95f, 1.00f}; // Cyan spec field labels
        const Color4 col_spec_val    {0.90f, 0.95f, 0.95f, 1.00f}; // Crisp white/ice values

        // Modal Frame Coordinates
        const float modal_w = std::min(1.80f, aspect * 1.70f);
        const float modal_h = 1.48f;
        const float x0 = -modal_w * 0.5f;
        const float x1 =  modal_w * 0.5f;
        const float y0 = -modal_h * 0.5f;
        const float y1 =  modal_h * 0.5f;

        // 1. Modal Glass Backdrop
        add_quad(x0, y0, x1, y1, col_bg_glass);

        // 2. Bezel and Double Frame Outlines
        add_rect_outline(x0, y0, x1, y1, col_border_outer);
        add_rect_outline(x0 + 0.012f, y0 + 0.012f, x1 - 0.012f, y1 - 0.012f, col_border_inner);

        // Corner Chamfer / Tactical Brackets
        const float c_len = 0.05f;
        add_line(x0 - 0.02f, y1, x0 + c_len, y1, col_title);
        add_line(x0, y1 + 0.02f, x0, y1 - c_len, col_title);
        add_line(x1 + 0.02f, y1, x1 - c_len, y1, col_title);
        add_line(x1, y1 + 0.02f, x1, y1 - c_len, col_title);
        add_line(x0 - 0.02f, y0, x0 + c_len, y0, col_title);
        add_line(x0, y0 - 0.02f, x0, y0 + c_len, col_title);
        add_line(x1 + 0.02f, y0, x1 - c_len, y0, col_title);
        add_line(x1, y0 - 0.02f, x1, y0 + c_len, col_title);

        // 3. Header Bar
        const float header_y = y1 - 0.12f;
        add_line(x0, header_y, x1, header_y, col_border_outer);
        add_line(x0, header_y - 0.005f, x1, header_y - 0.005f, col_border_inner);
        draw_string_centered("FAST JET SIMULATOR : AIRCRAFT SELECT", 0.0f, y1 - 0.065f, 0.030f, col_title);

        // 4. Footer Bar
        const float footer_y = y0 + 0.11f;
        add_line(x0, footer_y, x1, footer_y, col_border_outer);
        add_line(x0, footer_y + 0.005f, x1, footer_y + 0.005f, col_border_inner);
        draw_string_centered("UP/DOWN OR W/S: NAVIGATE  |  ENTER OR 1-5: FLY  |  ESC/M: CLOSE", 
                             0.0f, y0 + 0.055f, 0.020f, col_title);

        // 5. Column Layout
        // Split Left (Aircraft List) and Right (Spec Sheet)
        const float split_x = x0 + modal_w * 0.44f;
        add_line(split_x, header_y, split_x, footer_y, col_border_inner);

        // Shared spacing scale. Hoisted so the panel rhythm can be tuned in one
        // place rather than by hunting inline offsets.
        const float pad_s = 0.015f;
        const float pad_m = 0.030f;
        const float pad_l = 0.055f;

        // Type ramp. Each step is a clear size change: a label and its value
        // differing by a thousandth of a unit reads as an accident, not a
        // hierarchy.
        const float type_item     = 0.023f;
        const float type_subtitle = 0.014f;
        const float type_badge    = 0.016f;
        const float type_spec_lbl = 0.013f;
        const float type_spec_val = 0.018f;

        // --- LEFT COLUMN: AIRCRAFT LIST ---
        const char* aircraft_names[kAircraftCount] = {
            "F-16C FIGHTING FALCON",
            "F-15EX EAGLE II",
            "EUROFIGHTER TYPHOON",
            "F-22A RAPTOR",
            "A-10C THUNDERBOLT II"
        };

        const char* aircraft_subtitles[kAircraftCount] = {
            "BLOCK 50 MULTIROLE FIGHTER",
            "HEAVY AIR SUPERIORITY DFBW",
            "DELTA-CANARD CAREFREE FBW",
            "5TH-GEN STEALTH AIR DOMINANCE",
            "CLOSE AIR SUPPORT ATTACK JET"
        };

        const float item_h = (header_y - footer_y - 0.06f) / static_cast<float>(kAircraftCount);
        const float list_top = header_y - 0.03f;

        for (int i = 0; i < kAircraftCount; ++i) {
            const float item_y1 = list_top - static_cast<float>(i) * item_h;
            const float item_y0 = item_y1 - item_h;
            const float item_cy = (item_y0 + item_y1) * 0.5f;

            const bool is_selected = (i == selected_index_);
            const bool is_active   = (i == static_cast<int>(active_aircraft));

            const float row_l = x0 + pad_m;
            const float row_r = split_x - pad_m;

            if (is_selected) {
                // Highlight box with bright green border and tint
                add_quad(row_l, item_y0 + pad_s, row_r, item_y1 - pad_s, col_highlight_bg);
                add_rect_outline(row_l, item_y0 + pad_s, row_r, item_y1 - pad_s, col_text_bright);

                // A solid accent bar on the leading edge. The filled highlight
                // and border already carry the selection; paired cursor arrows
                // on top of them only added clutter.
                add_quad(row_l, item_y0 + pad_s, row_l + 0.008f, item_y1 - pad_s, col_text_bright);
            }

            // Whole unselected rows recede together. Previously the title and
            // subtitle of an unselected row were different hues, so three
            // colours competed inside one entry.
            const Color4 title_col = is_selected ? col_text_bright : col_text_dim;
            const Color4 sub_col   = is_selected ? col_spec_label  : col_text_dim;

            // Entry number & Name
            char title_str[64];
            std::snprintf(title_str, sizeof(title_str), "[%d] %s", i + 1, aircraft_names[i]);
            draw_string(title_str, row_l + pad_m, item_cy + pad_m, type_item, title_col);

            // Subtitle
            draw_string(aircraft_subtitles[i], row_l + pad_l, item_cy - 0.018f,
                        type_subtitle, sub_col);

            // Active Badge, right-aligned against the column edge so a longer
            // airframe name can never run into it.
            if (is_active) {
                draw_string_right("[ACTIVE]", row_r - pad_s, item_cy + pad_m,
                                  type_badge, col_active_badge);
            }
        }

        // --- RIGHT COLUMN: SPECIFICATION CARD ---
        const float spec_x0 = split_x + 0.03f;
        const float spec_x1 = x1 - 0.03f;
        const float spec_top = header_y - 0.035f;

        // Card backdrop
        add_quad(spec_x0, footer_y + 0.02f, spec_x1, header_y - 0.02f, col_card_bg);
        add_rect_outline(spec_x0, footer_y + 0.02f, spec_x1, header_y - 0.02f, col_border_inner);

        // Header for Card
        char header_card[64];
        std::snprintf(header_card, sizeof(header_card), "AIRFRAME SPECIFICATION : %s", aircraft_names[selected_index_]);
        draw_string(header_card, spec_x0 + 0.025f, spec_top - 0.020f, 0.021f, col_title);
        add_line(spec_x0 + 0.02f, spec_top - 0.040f, spec_x1 - 0.02f, spec_top - 0.040f, col_border_inner);

        // Spec details per aircraft
        struct SpecEntry {
            const char* label;
            const char* val;
        };

        std::vector<SpecEntry> specs;
        switch (selected_index_) {
            case 0: // F-16C
                specs = {
                    {"ROLE & MISSION",    "TACTICAL MULTIROLE FIGHTER"},
                    {"POWERPLANT",        "1X GENERAL ELECTRIC F110-GE-129"},
                    {"THRUST RATINGS",    "DRY: 75.6 KN (17,000 LBF) / AB: 129.0 KN (29,000 LBF)"},
                    {"SUPERCRUISE",       "NO (TRANSONIC AFTERBURNING ENVELOPE)"},
                    {"MAX AIRSPEED",      "MACH 2.05 @ 36,000 FT (1,176 KNOTS)"},
                    {"WEIGHTS",           "EMPTY: 20,500 LB | FUEL: 7,000 LB | MTOW: 42,300 LB"},
                    {"FLCS ARCHITECTURE", "QUAD-REDUNDANT DIGITAL FLY-BY-WIRE (+9.0G / 25.2 AOA)"},
                    {"AERODYNAMICS",      "NASA TP-1538 EMPIRICAL WIND TUNNEL TABLES"},
                    {"AVIONICS / RADAR",  "AN/APG-68 PULSE DOPPLER + CAT I / CAT III FLCS"},
                };
                break;
            case 1: // F-15EX
                specs = {
                    {"ROLE & MISSION",    "HEAVY ALL-WEATHER AIR SUPERIORITY"},
                    {"POWERPLANT",        "2X GENERAL ELECTRIC F110-GE-129 (TWIN ENGINE)"},
                    {"THRUST RATINGS",    "DRY: 151.2 KN (34,000 LBF) / AB: 262.4 KN (59,000 LBF)"},
                    {"SUPERCRUISE",       "NO (EXTREME AFTERBURNING CLIMB / COMBAT PERSISTENCE)"},
                    {"MAX AIRSPEED",      "MACH 2.50 @ 40,000 FT (1,433 KNOTS)"},
                    {"WEIGHTS",           "EMPTY: 31,700 LB | FUEL: 13,550 LB | MTOW: 81,000 LB"},
                    {"FLCS ARCHITECTURE", "DIGITAL FLY-BY-WIRE (DFBW) (+9.0G / 29.5 DEG AOA)"},
                    {"AERODYNAMICS",      "TWIN-TAIL AIR SUPERIORITY POLAR + HIGH COMPRESSIBILITY"},
                    {"AVIONICS / RADAR",  "AN/APG-82(V)1 AESA + ADCP-II MISSION COMPUTER"},
                };
                break;
            case 2: // Eurofighter Typhoon
                specs = {
                    {"ROLE & MISSION",    "HIGH-AGILITY CANARD-DELTA MULTIROLE AIR DEFENSE"},
                    {"POWERPLANT",        "2X EUROJET EJ200 ADVANCED AFTERBURNING TURBOFANS"},
                    {"THRUST RATINGS",    "DRY: 120.0 KN (26,980 LBF) / AB: 180.0 KN (40,460 LBF)"},
                    {"SUPERCRUISE",       "YES : SUSTAINED MACH 1.50 DRY CRUISE (NO REHEAT)"},
                    {"MAX AIRSPEED",      "MACH 2.00+ @ 36,000 FT (1,150 KNOTS)"},
                    {"WEIGHTS",           "EMPTY: 24,250 LB | FUEL: 11,020 LB | MTOW: 51,800 LB"},
                    {"FLCS ARCHITECTURE", "QUAD CAREFREE HANDLING FBW + CLOSE-COUPLED CANARDS"},
                    {"AERODYNAMICS",      "UNSTABLE PITCH SYNTHESIS (+9.0G / 35.0 DEG AOA LIMIT)"},
                    {"AVIONICS / RADAR",  "CAPTOR-M / E-SCAN AESA + PIRATE FLIR IRST"},
                };
                break;
            case 3: // F-22A Raptor
                specs = {
                    {"ROLE & MISSION",    "5TH-GEN VERY LOW OBSERVABLE AIR DOMINANCE"},
                    {"POWERPLANT",        "2X PRATT & WHITNEY F119-PW-100 (2D PITCH TVC)"},
                    {"THRUST RATINGS",    "DRY: 232.0 KN (52,150 LBF) / AB: 312.0 KN (70,140 LBF)"},
                    {"SUPERCRUISE",       "YES : SUSTAINED MACH 1.82 DRY CRUISE (MIL POWER)"},
                    {"MAX AIRSPEED",      "MACH 2.25+ @ 45,000 FT (1,290 KNOTS)"},
                    {"WEIGHTS",           "EMPTY: 43,340 LB | FUEL: 18,000 LB | MTOW: 83,500 LB"},
                    {"FLCS ARCHITECTURE", "FBW INTEGRATED WITH +/-20 DEG 2D THRUST VECTOR NOZZLES"},
                    {"AERODYNAMICS",      "SUPERMANEUVERABILITY ENVELOPE (+9.5G / 60+ DEG POST-STALL)"},
                    {"AVIONICS / RADAR",  "AN/APG-77 AESA + INTEGRATED ELECTRONIC WARFARE"},
                };
                break;
            case 4: // A-10C Warthog
                specs = {
                    {"ROLE & MISSION",    "CLOSE AIR SUPPORT (CAS) & FORWARD AIR CONTROL"},
                    {"POWERPLANT",        "2X GENERAL ELECTRIC TF34-GE-100A HIGH-BYPASS TURBOFANS"},
                    {"THRUST RATINGS",    "DRY ONLY: 80.6 KN (18,130 LBF) TOTAL (NON-AFTERBURNING)"},
                    {"SUPERCRUISE",       "NO : SUBSONIC LOW-ALTITUDE CRUISE TANK BUSTER"},
                    {"MAX AIRSPEED",      "MACH 0.56 (380 KIAS DECK LEVEL / NEVER EXCEED 450 KIAS)"},
                    {"WEIGHTS",           "EMPTY: 24,959 LB | FUEL: 10,700 LB | MTOW: 50,000 LB"},
                    {"FLCS ARCHITECTURE", "DUAL HYDRAULIC REVERSIBLE LINKAGE + PITCH/YAW SAS"},
                    {"AERODYNAMICS",      "HIGH-LIFT UNSWEPT WING (AR 6.54) + NACELLE PITCH COUPLING"},
                    {"AVIONICS / RADAR",  "SADL TACTICAL DATALINK + SPLIT-DECELERON AIRBRAKES"},
                };
                break;
        }

        // Text must stay inside the card: nothing here clips, so an overlong
        // value would otherwise run straight over the border.
        const float spec_text_x = spec_x0 + pad_m;
        const float spec_avail  = (spec_x1 - pad_m) - spec_text_x;

        float curr_y = spec_top - 0.080f;
        const float line_step = 0.058f;
        for (const auto& sp : specs) {
            draw_string(sp.label, spec_text_x, curr_y, type_spec_lbl, col_spec_label);

            // Shrink an overlong value to fit rather than letting it overflow.
            float val_scale = type_spec_val;
            const float val_w = measure_string(sp.val, val_scale);
            if (val_w > spec_avail && val_w > 0.0f) {
                val_scale *= spec_avail / val_w;
            }
            draw_string(sp.val, spec_text_x, curr_y - 0.024f, val_scale, col_spec_val);
            curr_y -= line_step;
        }

        // Upload Triangles
        glBindBuffer(GL_ARRAY_BUFFER, vbo_triangles_);
        glBufferData(GL_ARRAY_BUFFER, triangle_verts_.size() * sizeof(MenuVertex), triangle_verts_.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Upload Lines
        glBindBuffer(GL_ARRAY_BUFFER, vbo_lines_);
        glBufferData(GL_ARRAY_BUFFER, line_verts_.size() * sizeof(MenuVertex), line_verts_.data(), GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Setup OpenGL pipeline
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        const Mat4 proj = Mat4::ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);

        shader_.use();
        shader_.set_mat4("uProjection", proj);

        // Draw Translucent Glass Panels
        if (!triangle_verts_.empty()) {
            glBindVertexArray(vao_triangles_);
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(triangle_verts_.size()));
        }

        // Draw Glowing Lines and Text
        if (!line_verts_.empty()) {
            glLineWidth(2.0f);
            glBindVertexArray(vao_lines_);
            glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(line_verts_.size()));
            glLineWidth(1.0f);
        }

        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
    }
};

} // namespace fastjet::graphics
