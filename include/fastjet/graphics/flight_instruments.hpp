#pragma once

#include "fastjet/graphics/instrument_canvas.hpp"
#include "fastjet/graphics/cockpit_telemetry.hpp"
#include "fastjet/aircraft/aircraft_type.hpp"
#include "fastjet/fdm/flight_state.hpp"
#include "fastjet/environment/atmosphere1976.hpp"
#include <vector>
#include <cmath>
#include <cstdio>
#include <algorithm>

namespace fastjet::graphics {

/// @brief Cockpit dashboard flight instruments rendered to an off-screen FBO texture
class FlightInstruments : public InstrumentCanvas {
public:
    static constexpr int TEX_WIDTH = 2048;
    static constexpr int TEX_HEIGHT = 2048;  ///< With the centre display row

    FlightInstruments() = default;

    ~FlightInstruments() {
        destroy();
    }

    /// @param centre_page Also draw the centre display. Without it the atlas
    /// holds only the MFD row, at half the height: the same pixels per
    /// display, with half the clearing and mipmapping each frame.
    bool init(bool centre_page = true) {
        centre_page_ = centre_page;
        return init_canvas(TEX_WIDTH, centre_page ? TEX_HEIGHT : TEX_HEIGHT / 2);
    }

    /// @brief Atlas V range of the MFD row (the two MFD pages).
    [[nodiscard]] float mfd_row_v0() const noexcept { return centre_page_ ? 0.5f : 0.0f; }

    void destroy() noexcept {
        destroy_canvas();
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
            } else if (tel.ofc_tumble_active) {
                add_rect(bx, py - 0.10f, 0.48f, 0.08f, Color4::amber());
                draw_string("ANTI-SPIN RECOVER", bx - 0.23f, py - 0.10f, 0.048f, Color4::black());
            } else if (tel.ofc_gcas_active) {
                add_rect(bx, py - 0.10f, 0.48f, 0.08f, Color4::amber());
                draw_string("AUTO-GCAS PULL", bx - 0.22f, py - 0.10f, 0.050f, Color4::black());
            } else if (tel.ofc_gloc_active) {
                add_rect(bx, py - 0.10f, 0.48f, 0.08f, Color4::amber());
                draw_string("GLOC AUTO-REC", bx - 0.22f, py - 0.10f, 0.050f, Color4::black());
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

    /// @brief Left MFD, FCR page: the APG-68 B-scope in CRM (RWS / STT).
    ///
    /// Laid out like the F-16's colour MFD: OSB legends round the edge (mode
    /// top left, range scale with its bump arrows and the azimuth/bar settings
    /// down the left, page selectors along the bottom), the scope in the
    /// middle with range on the vertical axis and azimuth across it, the
    /// artificial horizon, antenna azimuth and elevation carets, and the
    /// acquisition cursor with the altitude block the scan covers at its
    /// range. RWS shows bricks only; STT adds the target's velocity vector,
    /// its altitude and the aspect / heading / speed / closure data line.
    void draw_fcr(const RadarTelemetry& r) {
        const Color4 glass{0.008f, 0.012f, 0.010f, 1.0f};
        const Color4 ink{0.30f, 1.0f, 0.38f, 1.0f};
        const Color4 dim{0.10f, 0.44f, 0.15f, 1.0f};
        const Color4 brick_col[3] = {{0.82f, 1.0f, 0.84f, 1.0f}, {0.46f, 0.70f, 0.50f, 1.0f},
                                     {0.26f, 0.42f, 0.29f, 1.0f}};
        constexpr float TS = 0.072f;   // legend text size
        // Scope: azimuth across [-SX, SX], range up from Y0 to Y1.
        constexpr float SX = 0.72f, Y0 = -0.72f, Y1 = 0.70f;
        constexpr float YM = 0.5f * (Y0 + Y1);
        constexpr float OSB[5] = {-0.64f, -0.32f, 0.0f, 0.32f, 0.64f};

        add_rect(0.0f, 0.0f, 2.0f, 2.0f, glass);
        auto seg = [&](float x0, float y0, float x1, float y1, const Color4& c, float w = 0.009f) {
            add_bar(x0, y0, x1, y1, w, c);
        };
        auto az_x = [&](double az_deg) {
            return static_cast<float>(std::clamp(az_deg / r.az_limit_deg, -1.0, 1.0)) * SX;
        };
        auto range_y = [&](double nm) {
            return Y0 + static_cast<float>(std::clamp(nm / r.range_scale_nm, 0.0, 1.0)) * (Y1 - Y0);
        };
        auto in_scope = [&](double nm) { return nm >= 0.0 && nm <= r.range_scale_nm; };
        auto boxed = [&](const char* s, float x, float y, const Color4& c) {
            const float w = text_width(s, TS) + 0.035f;
            draw_text(s, x, y, TS, c, 0);
            const float b = y - 0.022f, t = y + 0.068f;
            seg(x - 0.5f * w, b, x + 0.5f * w, b, c, 0.007f);
            seg(x - 0.5f * w, t, x + 0.5f * w, t, c, 0.007f);
            seg(x - 0.5f * w, b, x - 0.5f * w, t, c, 0.007f);
            seg(x + 0.5f * w, b, x + 0.5f * w, t, c, 0.007f);
        };

        // OSB legends: mode row along the top, page selectors along the bottom.
        const bool stt = r.mode == 2;
        draw_text("CRM", OSB[0], 0.875f, TS, ink, 0);
        draw_text(stt ? "STT" : "RWS", OSB[1], 0.875f, TS, ink, 0);
        draw_text("NORM", OSB[2], 0.875f, TS, ink, 0);
        draw_text("OVRD", OSB[3], 0.875f, TS, ink, 0);
        draw_text("CNTL", OSB[4], 0.875f, TS, ink, 0);
        draw_text("DCLT", OSB[0], -0.935f, TS, ink, 0);
        draw_text("SWAP", OSB[1], -0.935f, TS, ink, 0);
        boxed("FCR", OSB[2], -0.935f, ink);
        draw_text("FLCS", OSB[3], -0.935f, TS, ink, 0);
        draw_text("SMS", OSB[4], -0.935f, TS, ink, 0);

        if (!r.fitted || r.mode == 0) {
            draw_text(r.fitted ? "RDR OFF" : "NO RADAR", 0.0f, -0.02f, 0.10f, ink, 0);
            return;
        }

        // Range scale and its bump arrows (OSB 20/19), azimuth and bars (OSB 18/17).
        {
            const float x = -0.905f;
            add_tri(x, OSB[4] + 0.035f, x - 0.035f, OSB[4] - 0.025f, x + 0.035f, OSB[4] - 0.025f, ink);
            add_tri(x, OSB[3] - 0.035f, x - 0.035f, OSB[3] + 0.025f, x + 0.035f, OSB[3] + 0.025f, ink);
            char buf[8];
            std::snprintf(buf, sizeof(buf), "%d", static_cast<int>(r.range_scale_nm + 0.5));
            draw_text(buf, x, 0.5f * (OSB[3] + OSB[4]) - 0.025f, TS, ink, 0);
            std::snprintf(buf, sizeof(buf), "A%d", static_cast<int>(r.az_limit_deg / 10.0 + 0.5));
            draw_text(buf, -0.965f, OSB[2] - 0.025f, TS, ink);
            std::snprintf(buf, sizeof(buf), "%dB", r.bars);
            draw_text(buf, -0.965f, OSB[1] - 0.025f, TS, ink);
        }

        // Range tics at quarter scale on both edges, azimuth tics every 30 deg.
        for (int q = 1; q <= 3; ++q) {
            const float y = Y0 + (Y1 - Y0) * 0.25f * static_cast<float>(q);
            const float len = q == 2 ? 0.06f : 0.035f;
            seg(-SX, y, -SX + len, y, dim);
            seg(SX - len, y, SX, y, dim);
        }
        for (int a = -60; a <= 60; a += 30) {
            const float x = az_x(a);
            seg(x, Y1 - 0.04f, x, Y1, dim);
            seg(x, Y0, x, Y0 + 0.04f, dim);
        }

        // Artificial horizon, with the usual centre gap, clipped to the scope.
        {
            // Bank right and the horizon rolls the other way: its right end rises.
            const float roll = static_cast<float>(r.roll_deg * kPi / 180.0);
            const float drop = static_cast<float>(std::clamp(r.pitch_deg, -60.0, 60.0)) * 0.0115f;
            const float c = std::cos(roll), s = std::sin(roll);
            const float cx = drop * s, cy = YM - drop * c;
            auto clipped = [&](float a0, float a1) {
                float x0 = cx + a0 * c, y0 = cy + a0 * s, x1 = cx + a1 * c, y1 = cy + a1 * s;
                if (clip_to(x0, y0, x1, y1, -SX, Y0, SX, Y1)) seg(x0, y0, x1, y1, ink, 0.008f);
            };
            clipped(-0.58f, -0.14f);
            clipped(0.14f, 0.58f);
        }

        // Antenna carets: azimuth under the scope, elevation left of it (+/-60 deg scale).
        {
            const float x = az_x(r.ant_az_deg);
            seg(x, Y0 - 0.02f, x, Y0 - 0.075f, ink);
            seg(x - 0.025f, Y0 - 0.075f, x + 0.025f, Y0 - 0.075f, ink);
            const float ex = -SX - 0.03f;
            for (int e = -40; e <= 40; e += 20) {
                const float y = YM + static_cast<float>(e) / 60.0f * 0.5f * (Y1 - Y0);
                seg(ex - (e == 0 ? 0.03f : 0.015f), y, ex, y, dim, 0.007f);
            }
            const float y = YM + static_cast<float>(std::clamp(r.ant_el_deg, -60.0, 60.0) / 60.0) * 0.5f * (Y1 - Y0);
            seg(ex - 0.055f, y, ex - 0.005f, y, ink);
            seg(ex - 0.055f, y - 0.025f, ex - 0.055f, y + 0.025f, ink);
        }

        char buf[16];
        if (!stt) {
            // Search bricks, dimmer with age.
            for (int i = 0; i < r.hit_count; ++i) {
                const auto& h = r.hits[i];
                if (!in_scope(h.range_nm)) continue;
                add_rect(az_x(h.az_deg), range_y(h.range_nm), 0.052f, 0.030f,
                         brick_col[std::clamp(h.age, 0, 2)]);
            }
            // Acquisition cursor and the altitude block covered at its range.
            const float cx = az_x(r.cursor_az_deg), cy = range_y(r.cursor_range_nm);
            seg(cx - 0.042f, cy - 0.04f, cx - 0.042f, cy + 0.04f, ink, 0.008f);
            seg(cx + 0.042f, cy - 0.04f, cx + 0.042f, cy + 0.04f, ink, 0.008f);
            const float tx = cx > 0.45f ? cx - 0.07f : cx + 0.07f;
            const int align = cx > 0.45f ? 1 : -1;
            std::snprintf(buf, sizeof(buf), "%02d", std::clamp(r.cov_top_kft, -99, 99));
            draw_text(buf, tx, cy + 0.008f, 0.058f, ink, align);
            std::snprintf(buf, sizeof(buf), "%02d", std::clamp(r.cov_bottom_kft, -99, 99));
            draw_text(buf, tx, cy - 0.058f, 0.058f, ink, align);
            return;
        }

        // STT data line: aspect, target track, ground speed, closure.
        std::snprintf(buf, sizeof(buf), "%02d%c", r.aspect_tens, r.aspect_side);
        draw_text(buf, -0.60f, 0.755f, TS, ink, 0);
        std::snprintf(buf, sizeof(buf), "%03d", static_cast<int>(std::lround(r.tgt_heading_deg)) % 360);
        draw_text(buf, -0.20f, 0.755f, TS, ink, 0);
        std::snprintf(buf, sizeof(buf), "%d", static_cast<int>(std::lround(r.tgt_gs_kt)));
        draw_text(buf, 0.20f, 0.755f, TS, ink, 0);
        std::snprintf(buf, sizeof(buf), "%+d", static_cast<int>(std::lround(r.closure_kt)));
        draw_text(buf, 0.60f, 0.755f, TS, ink, 0);

        // The tracked target: filled square, velocity vector, altitude below.
        // On memory it flashes.
        if (in_scope(r.tgt_range_nm) && (!r.track_memory || r.blink)) {
            const float x = az_x(r.tgt_az_deg), y = range_y(r.tgt_range_nm);
            add_rect(x, y, 0.050f, 0.050f, ink);
            const float a = static_cast<float>(r.tgt_rel_heading_deg * kPi / 180.0);
            const float len = 0.035f + 0.14f * static_cast<float>(std::clamp(r.tgt_gs_kt / 600.0, 0.0, 1.5));
            seg(x, y, x + std::sin(a) * len, y + std::cos(a) * len, ink);
            // Altitude beside the symbol, on the side the vector does not take.
            std::snprintf(buf, sizeof(buf), "%02d", static_cast<int>(std::lround(std::clamp(r.tgt_alt_kft, 0.0, 99.0))));
            const bool right = std::sin(a) < 0.0f;
            draw_text(buf, right ? x + 0.045f : x - 0.045f, y - 0.085f, 0.060f, ink, right ? -1 : 1);
        }
    }

    /// @brief Liang-Barsky: clip a segment to a rectangle; false if none is left.
    static bool clip_to(float& x0, float& y0, float& x1, float& y1, float xmin, float ymin, float xmax,
                        float ymax) noexcept {
        const float dx = x1 - x0, dy = y1 - y0;
        float t0 = 0.0f, t1 = 1.0f;
        const float p[4] = {-dx, dx, -dy, dy};
        const float q[4] = {x0 - xmin, xmax - x0, y0 - ymin, ymax - y0};
        for (int i = 0; i < 4; ++i) {
            if (p[i] == 0.0f) {
                if (q[i] < 0.0f) return false;
                continue;
            }
            const float t = q[i] / p[i];
            if (p[i] < 0.0f) t0 = std::max(t0, t);
            else t1 = std::min(t1, t);
            if (t0 > t1) return false;
        }
        const float ox = x0, oy = y0;
        x0 = ox + t0 * dx; y0 = oy + t0 * dy;
        x1 = ox + t1 * dx; y1 = oy + t1 * dy;
        return true;
    }

    /// @brief Update every instrument display and render off-screen to the atlas texture
    void update_and_render(const fdm::FlightState& state, const AvionicsTelemetry& telemetry = AvionicsTelemetry{}) {
        if (!initialized_) return;

        begin_canvas();

        // The MFD row fills the top half of the full atlas, or all of the
        // half-height one.
        const float row_cy = centre_page_ ? 0.5f : 0.0f;
        const float row_hh = centre_page_ ? 0.5f : 1.0f;
        set_panel_viewport(-0.5f, row_cy, 0.5f, row_hh);
        if (telemetry.radar.page_fcr) draw_fcr(telemetry.radar);
        else draw_left_mfd(state, telemetry);

        set_panel_viewport(0.5f, row_cy, 0.5f, row_hh);
        draw_right_mfd(state, telemetry);

        if (centre_page_) {
            set_panel_viewport(0.0f, -0.5f, 0.5f, 0.5f);
            draw_center_cpd(state, telemetry);
        }

        render_canvas(Color4{0.04f, 0.05f, 0.06f, 1.0f});
    }

private:
    bool centre_page_ = true;
};

} // namespace fastjet::graphics
