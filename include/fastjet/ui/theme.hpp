#pragma once

#include "fastjet/ui/draw_list.hpp"
#include "fastjet/ui/user_settings.hpp"
#include <algorithm>
#include <cmath>

/// @file
/// @brief Centralised design tokens for every menu surface.
///
/// Visual language: "modern aerospace avionics" - obsidian slate surfaces,
/// cobalt selection, sky-blue accent, amber focus. The default palette uses
/// the exact values the tactical AircraftMenu shipped with, so the new menus
/// and the in-flight overlay read as one product.
///
/// All sizes are logical pixels at a 1920x1080 reference and a 100% UI scale;
/// views multiply them by the frame's scale factor. Nothing outside this file
/// should contain a literal colour, font size, radius or duration.

namespace fastjet::ui {

/// @brief Semantic colour roles.
struct Palette {
    // Surfaces (back to front)
    Rgba backdrop;       ///< Full-screen dimming behind menus
    Rgba surface_void;   ///< Deepest wells: slider tracks, input fields
    Rgba surface_panel;  ///< Cards and main panels
    Rgba surface_header; ///< Header / footer strips
    Rgba surface_raised; ///< Idle button fill
    Rgba surface_sunken; ///< Inactive tabs
    Rgba surface_row_alt;///< Zebra rows, hovered list rows
    Rgba selection;      ///< Selected list row / hovered button fill
    // Lines
    Rgba border_subtle;
    Rgba border_strong;
    Rgba border_bright;
    // Text
    Rgba text_primary;
    Rgba text_secondary;
    Rgba text_muted;
    Rgba text_label;     ///< Captions, section headers
    Rgba text_on_accent;
    // Accents and status
    Rgba accent;         ///< Sky-blue highlight, slider fill, active indicators
    Rgba accent_strong;  ///< Primary button / active tab fill
    Rgba accent_strong_hover;
    Rgba highlight;      ///< Amber: modified markers, key capture
    Rgba focus_ring;     ///< Keyboard / gamepad focus outline
    Rgba success;
    Rgba success_fill;
    Rgba success_fill_hover;
    Rgba danger;
    Rgba danger_fill;
    Rgba danger_fill_hover;
    Rgba shadow;
    Rgba grid_line;      ///< Animated background grid
};

/// @brief Font sizes (em, px) and tracking for each text role.
struct Typography {
    float display = 60.0f;       ///< Main menu wordmark
    float display_sub = 22.0f;   ///< Wordmark subtitle
    float title = 32.0f;         ///< Screen titles
    float heading = 20.0f;       ///< Modal titles, credit headings
    float body = 18.0f;          ///< Row labels, buttons
    float label = 14.0f;         ///< Section headers, tabs, chips
    float caption = 14.0f;       ///< Hints and secondary lines
    float tracking_wide = 2.4f;  ///< Uppercase section labels
    float tracking_button = 1.2f;
};

/// @brief 4-px spacing scale.
struct Spacing {
    float xxs = 2.0f;
    float xs = 4.0f;
    float sm = 8.0f;
    float md = 12.0f;
    float lg = 16.0f;
    float xl = 24.0f;
    float xxl = 32.0f;
    float xxxl = 48.0f;
    float page = 96.0f; ///< Outer page margin on the main menu
};

struct Radii {
    float sm = 4.0f;
    float md = 6.0f;
    float lg = 10.0f;
    float pill = 999.0f; ///< Clamped to half the height by the shader
};

struct Borders {
    float hairline = 1.0f;
    float regular = 1.5f;
    float focus = 2.0f;
    float focus_offset = 3.0f; ///< Gap between a control and its focus ring
};

struct Shadow {
    float blur;
    float offset_y;
    float opacity;
};

struct Elevation {
    Shadow panel{28.0f, 10.0f, 0.55f};
    Shadow raised{10.0f, 3.0f, 0.35f};
    Shadow modal{40.0f, 16.0f, 0.70f};
};

/// @brief Durations (s) and distances for transitions.
struct Motion {
    float view_out = 0.14f;      ///< Old screen fade/slide out
    float view_in = 0.24f;       ///< New screen fade/slide in
    float tab_switch = 0.22f;
    float menu_open = 0.22f;
    float modal = 0.18f;
    float state = 0.10f;         ///< Hover / press smoothing time constant
    float focus = 0.12f;
    float toggle = 0.12f;
    float scroll = 0.08f;
    float toast_hold = 2.6f;
    float toast_fade = 0.35f;
    float slide_distance = 28.0f;///< px at 100% scale
    float modal_scale_from = 0.96f;
    float background_drift = 9.0f; ///< Grid drift, px/s
    float parallax_near = 22.0f; ///< Max parallax offset of the near layer, px
    float parallax_far = 9.0f;
    float sweep_period = 7.0f;   ///< Radar sweep revolution, s
};

/// @brief Control dimensions.
struct Metrics {
    float reference_height = 1080.0f;
    float reference_width = 1920.0f;

    float main_button_w = 400.0f;
    float main_button_h = 54.0f;
    float main_button_gap = 10.0f;
    float accent_bar_w = 4.0f;

    float panel_max_w = 1180.0f;
    float panel_margin = 56.0f;
    float header_h = 92.0f;
    float tab_h = 46.0f;
    float footer_h = 80.0f;
    float footer_button_h = 44.0f;
    float footer_button_min_w = 132.0f;

    float row_h = 66.0f;          ///< Setting row with label + hint
    float row_h_compact = 50.0f;  ///< Key binding row
    float section_h = 46.0f;
    float control_w = 380.0f;     ///< Right-hand control column

    float slider_track_h = 6.0f;
    float slider_knob = 18.0f;
    float slider_value_w = 64.0f;
    float toggle_w = 48.0f;
    float toggle_h = 26.0f;
    float selector_h = 38.0f;
    float chevron = 7.0f;
    float keycap_w = 168.0f;
    float keycap_h = 36.0f;
    float modified_dot = 6.0f;

    float dropdown_item_h = 40.0f;
    int dropdown_max_visible = 8;

    float modal_w = 560.0f;
    float scrollbar_w = 4.0f;
    float scroll_step = 72.0f;    ///< Wheel notch / nav scroll distance
};

enum class ButtonVariant : uint8_t { PRIMARY, SECONDARY, GHOST, DANGER, MENU };

/// @brief Fully resolved colours for one visual state.
struct StateColors {
    Rgba fill;
    Rgba border;
    Rgba text;
};

/// @brief Per-variant colours for the idle/hover/active/disabled states.
/// Focus is drawn as a separate ring so it composes with any state.
struct ButtonStyle {
    StateColors idle;
    StateColors hover;
    StateColors active;
    StateColors disabled;

    /// @brief Blends states from animated hover/press amounts in [0, 1].
    [[nodiscard]] StateColors resolve(float hover_amount, float press_amount, bool is_disabled) const noexcept {
        if (is_disabled) return disabled;
        auto mix = [](const StateColors& a, const StateColors& b, float t) {
            return StateColors{Rgba::lerp(a.fill, b.fill, t), Rgba::lerp(a.border, b.border, t), Rgba::lerp(a.text, b.text, t)};
        };
        return mix(mix(idle, hover, std::clamp(hover_amount, 0.0f, 1.0f)), active, std::clamp(press_amount, 0.0f, 1.0f));
    }
};

struct Theme {
    Palette palette{};
    Typography type{};
    Spacing space{};
    Radii radius{};
    Borders border{};
    Elevation elevation{};
    Motion motion{};
    Metrics metrics{};
    bool reduce_motion = false;

    [[nodiscard]] ButtonStyle button(ButtonVariant v) const noexcept {
        const Palette& p = palette;
        const StateColors disabled{p.surface_sunken.faded(0.6f), p.border_subtle.faded(0.5f), p.text_muted.faded(0.55f)};
        switch (v) {
            case ButtonVariant::PRIMARY:
                return {{p.accent_strong, p.border_bright, p.text_on_accent},
                        {p.accent_strong_hover, p.accent, p.text_on_accent},
                        {p.accent_strong.faded(0.8f), p.accent, p.text_on_accent},
                        disabled};
            case ButtonVariant::DANGER:
                return {{p.danger_fill, p.danger.faded(0.6f), p.text_primary},
                        {p.danger_fill_hover, p.danger, p.text_on_accent},
                        {p.danger_fill, p.danger, p.text_on_accent},
                        disabled};
            case ButtonVariant::GHOST:
                return {{p.surface_raised.with_alpha(0.0f), p.border_subtle.with_alpha(0.0f), p.text_secondary},
                        {p.surface_row_alt, p.border_subtle, p.text_primary},
                        {p.selection, p.border_strong, p.text_primary},
                        {p.surface_raised.with_alpha(0.0f), p.border_subtle.with_alpha(0.0f), p.text_muted.faded(0.55f)}};
            case ButtonVariant::MENU:
                return {{p.surface_panel.faded(0.55f), p.border_subtle.faded(0.6f), p.text_secondary},
                        {p.selection, p.border_bright, p.text_primary},
                        {p.accent_strong, p.accent, p.text_on_accent},
                        disabled};
            case ButtonVariant::SECONDARY:
            default:
                return {{p.surface_raised, p.border_strong, p.text_secondary},
                        {p.selection, p.border_bright, p.text_primary},
                        {p.accent_strong, p.accent, p.text_on_accent},
                        disabled};
        }
    }

    /// @brief Scale factor from reference pixels to framebuffer pixels.
    [[nodiscard]] float scale_for(float fb_w, float fb_h, int ui_scale_percent) const noexcept {
        const float fit = std::min(fb_w / metrics.reference_width, fb_h / metrics.reference_height);
        return std::max(0.35f, fit * static_cast<float>(ui_scale_percent) / 100.0f);
    }

    /// @brief Exponential smoothing factor for a time constant, honouring reduced motion.
    [[nodiscard]] float approach(float dt, float tau) const noexcept {
        if (reduce_motion || tau <= 0.0f) return 1.0f;
        return 1.0f - std::exp(-dt / tau);
    }
};

// ---------------------------------------------------------------------------
// Easing
// ---------------------------------------------------------------------------

[[nodiscard]] inline float ease_out_cubic(float t) noexcept {
    t = std::clamp(t, 0.0f, 1.0f);
    const float u = 1.0f - t;
    return 1.0f - u * u * u;
}

[[nodiscard]] inline float ease_in_cubic(float t) noexcept {
    t = std::clamp(t, 0.0f, 1.0f);
    return t * t * t;
}

// ---------------------------------------------------------------------------
// Palettes
// ---------------------------------------------------------------------------

[[nodiscard]] inline Palette make_default_palette() noexcept {
    Palette p;
    p.backdrop            = {0.015f, 0.025f, 0.040f, 0.78f};
    p.surface_void        = {0.04f, 0.06f, 0.09f, 0.96f};
    p.surface_panel       = {0.07f, 0.10f, 0.14f, 0.94f};
    p.surface_header      = {0.10f, 0.14f, 0.20f, 0.98f};
    p.surface_raised      = {0.11f, 0.155f, 0.215f, 1.00f};
    p.surface_sunken      = {0.06f, 0.09f, 0.13f, 0.85f};
    p.surface_row_alt     = {0.09f, 0.13f, 0.18f, 0.60f};
    p.selection           = {0.14f, 0.26f, 0.44f, 0.90f};
    p.border_subtle       = {0.18f, 0.25f, 0.34f, 0.80f};
    p.border_strong       = {0.28f, 0.40f, 0.55f, 0.95f};
    p.border_bright       = {0.40f, 0.68f, 0.95f, 1.00f};
    p.text_primary        = {0.96f, 0.98f, 1.00f, 1.00f};
    p.text_secondary      = {0.84f, 0.89f, 0.95f, 1.00f};
    p.text_muted          = {0.58f, 0.66f, 0.75f, 1.00f};
    p.text_label          = {0.48f, 0.68f, 0.88f, 1.00f};
    p.text_on_accent      = {1.00f, 1.00f, 1.00f, 1.00f};
    p.accent              = {0.35f, 0.80f, 1.00f, 1.00f};
    p.accent_strong       = {0.15f, 0.38f, 0.68f, 1.00f};
    p.accent_strong_hover = {0.20f, 0.46f, 0.80f, 1.00f};
    p.highlight           = {1.00f, 0.78f, 0.20f, 1.00f};
    p.focus_ring          = {1.00f, 0.78f, 0.20f, 1.00f};
    p.success             = {0.20f, 0.88f, 0.55f, 1.00f};
    p.success_fill        = {0.12f, 0.50f, 0.35f, 1.00f};
    p.success_fill_hover  = {0.18f, 0.65f, 0.45f, 1.00f};
    p.danger              = {1.00f, 0.35f, 0.35f, 1.00f};
    p.danger_fill         = {0.36f, 0.10f, 0.12f, 1.00f};
    p.danger_fill_hover   = {0.55f, 0.15f, 0.17f, 1.00f};
    p.shadow              = {0.00f, 0.00f, 0.00f, 1.00f};
    p.grid_line           = {0.40f, 0.68f, 0.95f, 0.055f};
    return p;
}

/// @brief Maximum-legibility variant: opaque black surfaces, white text,
/// yellow focus, brighter borders (targets WCAG AAA text contrast).
[[nodiscard]] inline Palette make_high_contrast_palette() noexcept {
    Palette p = make_default_palette();
    p.backdrop            = {0.00f, 0.00f, 0.00f, 0.90f};
    p.surface_void        = {0.00f, 0.00f, 0.00f, 1.00f};
    p.surface_panel       = {0.00f, 0.00f, 0.00f, 0.98f};
    p.surface_header      = {0.06f, 0.06f, 0.06f, 1.00f};
    p.surface_raised      = {0.10f, 0.10f, 0.10f, 1.00f};
    p.surface_sunken      = {0.04f, 0.04f, 0.04f, 1.00f};
    p.surface_row_alt     = {0.14f, 0.14f, 0.14f, 1.00f};
    p.selection           = {0.00f, 0.22f, 0.50f, 1.00f};
    p.border_subtle       = {0.60f, 0.60f, 0.60f, 1.00f};
    p.border_strong       = {0.85f, 0.85f, 0.85f, 1.00f};
    p.border_bright       = {1.00f, 1.00f, 1.00f, 1.00f};
    p.text_primary        = {1.00f, 1.00f, 1.00f, 1.00f};
    p.text_secondary      = {1.00f, 1.00f, 1.00f, 1.00f};
    p.text_muted          = {0.86f, 0.86f, 0.86f, 1.00f};
    p.text_label          = {0.55f, 0.85f, 1.00f, 1.00f};
    p.accent              = {0.40f, 0.88f, 1.00f, 1.00f};
    p.accent_strong       = {0.00f, 0.32f, 0.70f, 1.00f};
    p.accent_strong_hover = {0.00f, 0.42f, 0.88f, 1.00f};
    p.highlight           = {1.00f, 0.92f, 0.00f, 1.00f};
    p.focus_ring          = {1.00f, 0.92f, 0.00f, 1.00f};
    p.grid_line           = {1.00f, 1.00f, 1.00f, 0.05f};
    return p;
}

/// @brief Swaps red/green and blue/yellow status pairs for hues each
/// deficiency can separate (after the Okabe-Ito colour-blind-safe set).
inline void apply_colorblind_palette(Palette& p, ColorblindMode mode) noexcept {
    switch (mode) {
        case ColorblindMode::PROTANOPIA:
        case ColorblindMode::DEUTERANOPIA:
            // Red-green confusion: status becomes blue (good) vs orange (bad).
            p.success            = {0.34f, 0.71f, 0.91f, 1.00f};
            p.success_fill       = {0.05f, 0.33f, 0.58f, 1.00f};
            p.success_fill_hover = {0.08f, 0.42f, 0.72f, 1.00f};
            p.danger             = {0.90f, 0.62f, 0.00f, 1.00f};
            p.danger_fill        = {0.45f, 0.26f, 0.00f, 1.00f};
            p.danger_fill_hover  = {0.60f, 0.36f, 0.00f, 1.00f};
            p.highlight          = {0.94f, 0.89f, 0.26f, 1.00f};
            p.focus_ring         = {1.00f, 1.00f, 1.00f, 1.00f};
            break;
        case ColorblindMode::TRITANOPIA:
            // Blue-yellow confusion: amber accents become vermillion/pink.
            p.highlight          = {0.97f, 0.45f, 0.62f, 1.00f};
            p.focus_ring         = {1.00f, 1.00f, 1.00f, 1.00f};
            p.success            = {0.00f, 0.80f, 0.62f, 1.00f};
            p.danger             = {0.84f, 0.37f, 0.00f, 1.00f};
            break;
        case ColorblindMode::OFF:
        case ColorblindMode::COUNT:
            break;
    }
}

/// @brief Builds the active theme from the accessibility preferences.
[[nodiscard]] inline Theme build_theme(const AccessibilitySettings& a) noexcept {
    Theme t;
    t.palette = a.high_contrast ? make_high_contrast_palette() : make_default_palette();
    apply_colorblind_palette(t.palette, a.colorblind);
    if (a.high_contrast) {
        t.border.hairline = 1.5f;
        t.border.regular = 2.0f;
        t.border.focus = 3.0f;
    }
    t.reduce_motion = a.reduce_motion;
    return t;
}

} // namespace fastjet::ui
