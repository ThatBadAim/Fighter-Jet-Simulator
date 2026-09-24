#pragma once

#include "fastjet/ui/draw_list.hpp"
#include "fastjet/ui/theme.hpp"
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <string>

/// @file
/// @brief Widget view-models and the single painter that draws them.
///
/// Screens (controllers) describe each interactive element as a `Widget`
/// snapshot every frame. `MenuSystem` owns focus, hover and animation state,
/// and `WidgetPainter` turns (Widget, WidgetAnim, Theme) into draw commands.
/// Because every control goes through one painter, all screens share the
/// same states, spacing and motion.

namespace fastjet::ui {

using WidgetId = uint32_t;
inline constexpr WidgetId kNoWidget = 0;

enum class WidgetKind : uint8_t {
    BUTTON,   ///< Push button (footer, main menu, modal)
    TAB,      ///< Settings category tab
    TOGGLE,   ///< Setting row with an on/off switch
    SLIDER,   ///< Setting row with a 0-100% style slider
    SELECTOR, ///< Setting row cycling through options with < >
    DROPDOWN, ///< Setting row opening a popup list
    KEYBIND,  ///< One key-binding cell (primary or secondary)
};

/// @brief True for controls whose value LEFT/RIGHT changes directly.
[[nodiscard]] constexpr bool is_adjustable(WidgetKind k) noexcept {
    return k == WidgetKind::TOGGLE || k == WidgetKind::SLIDER || k == WidgetKind::SELECTOR || k == WidgetKind::DROPDOWN;
}

/// @brief Logical position used for keyboard/gamepad traversal.
///
/// UP/DOWN move between rows of a group, then into the neighbouring group;
/// LEFT/RIGHT move between columns of a row. Logical coordinates keep
/// traversal predictable inside scrolled content, where on-screen geometry
/// alone would jump to off-screen rows.
struct NavCoord {
    int group = 0;
    int row = 0;
    int col = 0;
};

/// @brief Per-frame description of one interactive element.
struct Widget {
    WidgetId id = kNoWidget;
    WidgetKind kind = WidgetKind::BUTTON;
    Rect rect{};    ///< Hit and highlight area
    Rect control{}; ///< Value control inside a setting row (slider track area, selector box, ...)
    NavCoord nav{};

    std::string label;
    std::string hint;
    std::string value_text;

    float value01 = 0.0f;       ///< SLIDER position
    bool checked = false;       ///< TOGGLE state
    bool disabled = false;
    bool selected = false;      ///< Active TAB
    bool modified = false;      ///< Pending value differs from the saved one
    bool capturing = false;     ///< KEYBIND waiting for a key
    bool expanded = false;      ///< DROPDOWN popup open
    bool can_decrement = true;  ///< SELECTOR "<" available
    bool can_increment = true;  ///< SELECTOR ">" available
    bool in_scroll = false;     ///< Lives inside the screen's scroll region
    ButtonVariant variant = ButtonVariant::SECONDARY;
};

/// @brief Smoothed interaction state for one widget, in [0, 1].
struct WidgetAnim {
    float hover = 0.0f;
    float focus = 0.0f;
    float press = 0.0f;
    float toggle = 0.0f;
    bool seen = false;
};

class WidgetPainter {
public:
    WidgetPainter(const Theme& theme, float scale) noexcept : t_(theme), s_(scale) {}

    void paint(const Widget& w, const WidgetAnim& a, DrawList& dl, bool focus_visible) const {
        switch (w.kind) {
            case WidgetKind::BUTTON: paint_button(w, a, dl); break;
            case WidgetKind::TAB: paint_tab(w, a, dl); break;
            case WidgetKind::KEYBIND: paint_keycap(w, a, dl); break;
            case WidgetKind::TOGGLE:
            case WidgetKind::SLIDER:
            case WidgetKind::SELECTOR:
            case WidgetKind::DROPDOWN: paint_row(w, a, dl); break;
        }
        if (focus_visible && a.focus > 0.01f && !w.disabled) paint_focus_ring(w, a, dl);
    }

    /// @brief Chevron built from two strokes; `dir` is -1 left, +1 right, +2 down, -2 up.
    void chevron(DrawList& dl, float cx, float cy, int dir, const Rgba& c) const {
        const float k = t_.metrics.chevron * s_ * 0.5f;
        const float th = t_.border.regular * s_ * 1.2f;
        switch (dir) {
            case -1: dl.line(cx + k * 0.5f, cy - k, cx - k * 0.5f, cy, th, c); dl.line(cx - k * 0.5f, cy, cx + k * 0.5f, cy + k, th, c); break;
            case +1: dl.line(cx - k * 0.5f, cy - k, cx + k * 0.5f, cy, th, c); dl.line(cx + k * 0.5f, cy, cx - k * 0.5f, cy + k, th, c); break;
            case +2: dl.line(cx - k, cy - k * 0.5f, cx, cy + k * 0.5f, th, c); dl.line(cx, cy + k * 0.5f, cx + k, cy - k * 0.5f, th, c); break;
            default: dl.line(cx - k, cy + k * 0.5f, cx, cy - k * 0.5f, th, c); dl.line(cx, cy - k * 0.5f, cx + k, cy + k * 0.5f, th, c); break;
        }
    }

private:
    const Theme& t_;
    float s_;

    [[nodiscard]] float px(float v) const noexcept { return v * s_; }
    [[nodiscard]] const Palette& p() const noexcept { return t_.palette; }

    void paint_focus_ring(const Widget& w, const WidgetAnim& a, DrawList& dl) const {
        const float off = px(t_.border.focus_offset);
        const float radius = (w.kind == WidgetKind::KEYBIND ? px(t_.radius.sm) : px(t_.radius.md)) + off;
        dl.stroke(w.rect.inset(-off), p().focus_ring.faded(a.focus), px(t_.border.focus), radius);
    }

    void paint_button(const Widget& w, const WidgetAnim& a, DrawList& dl) const {
        const StateColors c = t_.button(w.variant).resolve(a.hover, a.press, w.disabled);
        const float radius = px(t_.radius.md);
        if (w.variant != ButtonVariant::GHOST && !w.disabled) {
            const Shadow& sh = t_.elevation.raised;
            dl.shadow(w.rect, radius, px(sh.blur), p().shadow.faded(sh.opacity * (0.5f + 0.5f * a.hover)), px(sh.offset_y));
        }
        dl.rect(w.rect, RectStyle::outlined(c.fill, c.border, px(t_.border.hairline), radius));

        const float size = px(t_.type.body);
        const float tracking = px(t_.type.tracking_button);
        if (w.variant == ButtonVariant::MENU) {
            // Left-aligned label with an accent bar that grows in on hover/focus.
            const float bar_h = w.rect.h * (0.30f + 0.40f * a.hover);
            const float bar_x = w.rect.x + px(t_.space.md);
            dl.fill({bar_x, w.rect.cy() - bar_h * 0.5f, px(t_.metrics.accent_bar_w), bar_h},
                    p().accent.faded(0.35f + 0.65f * a.hover), px(t_.radius.sm));
            const float text_x = w.rect.x + px(t_.space.xxl) + px(t_.space.sm) * a.hover;
            dl.text_v_centered(w.label, text_x, w.rect.cy(), size, c.text, tracking);
            if (!w.value_text.empty()) {
                dl.text_right(w.value_text, w.rect.right() - px(t_.space.xl), w.rect.cy(), px(t_.type.caption),
                              p().text_muted.faded(0.6f + 0.4f * a.hover), px(t_.type.tracking_button));
            }
        } else {
            dl.text_centered(w.label, w.rect.cx(), w.rect.cy(), size * 0.9f, c.text, tracking);
        }
    }

    void paint_tab(const Widget& w, const WidgetAnim& a, DrawList& dl) const {
        if (!w.selected && a.hover > 0.01f) {
            dl.fill(w.rect.inset(0.0f, px(t_.space.xs)), p().surface_row_alt.faded(a.hover), px(t_.radius.md));
        }
        const Rgba text = w.selected ? p().text_primary : Rgba::lerp(p().text_muted, p().text_primary, a.hover);
        dl.text_centered(w.label, w.rect.cx(), w.rect.cy(), px(t_.type.label), text, px(t_.type.tracking_wide));
        if (w.modified) {
            const float d = px(t_.metrics.modified_dot);
            const float lx = w.rect.cx() + text_width(w.label, px(t_.type.label), px(t_.type.tracking_wide)) * 0.5f + px(t_.space.sm);
            dl.fill({lx, w.rect.cy() - d * 0.5f - px(t_.space.xs), d, d}, p().highlight, d);
        }
    }

    void paint_row(const Widget& w, const WidgetAnim& a, DrawList& dl) const {
        const float radius = px(t_.radius.md);
        const float emphasis = std::max(a.hover, a.focus);
        if (emphasis > 0.01f) {
            dl.rect(w.rect, RectStyle::outlined(p().surface_row_alt.faded(emphasis),
                                                p().border_subtle.faded(emphasis * 0.8f), px(t_.border.hairline), radius));
        }

        const float label_x = w.rect.x + px(t_.space.xl);
        const bool has_hint = !w.hint.empty();
        const float label_cy = has_hint ? w.rect.cy() - px(t_.space.md) * 0.85f : w.rect.cy();
        const Rgba label_col = w.disabled ? p().text_muted.faded(0.7f) : p().text_primary;
        dl.text_v_centered(w.label, label_x, label_cy, px(t_.type.body * 0.9f), label_col, px(t_.type.tracking_button * 0.5f));
        if (has_hint) {
            const float hint_w = w.control.x - label_x - px(t_.space.xl);
            dl.text_v_centered(fit_text(w.hint, hint_w, px(t_.type.caption)), label_x, w.rect.cy() + px(t_.space.md),
                               px(t_.type.caption), p().text_muted.faded(w.disabled ? 0.6f : 1.0f));
        }
        if (w.modified) {
            const float d = px(t_.metrics.modified_dot);
            dl.fill({w.rect.x + px(t_.space.sm), label_cy - d * 0.5f, d, d}, p().highlight, d);
        }

        switch (w.kind) {
            case WidgetKind::TOGGLE: paint_toggle(w, a, dl); break;
            case WidgetKind::SLIDER: paint_slider(w, a, dl); break;
            case WidgetKind::SELECTOR: paint_selector(w, a, dl); break;
            case WidgetKind::DROPDOWN: paint_dropdown(w, a, dl); break;
            default: break;
        }
    }

    void paint_toggle(const Widget& w, const WidgetAnim& a, DrawList& dl) const {
        const float tw = px(t_.metrics.toggle_w);
        const float th = px(t_.metrics.toggle_h);
        const Rect track{w.control.right() - tw, w.control.cy() - th * 0.5f, tw, th};
        const float on = a.toggle;
        const float dim = w.disabled ? 0.5f : 1.0f;
        dl.rect(track, RectStyle::outlined(Rgba::lerp(p().surface_void, p().accent_strong, on).faded(dim),
                                           Rgba::lerp(p().border_strong, p().accent, on).faded(dim),
                                           px(t_.border.hairline), th * 0.5f));
        const float pad = px(3.0f);
        const float knob = th - 2.0f * pad;
        const float kx = track.x + pad + (tw - 2.0f * pad - knob) * on;
        dl.shadow({kx, track.y + pad, knob, knob}, knob * 0.5f, px(4.0f), p().shadow.faded(0.4f * dim), px(1.0f));
        dl.fill({kx, track.y + pad, knob, knob}, Rgba::lerp(p().text_muted, p().text_primary, on).faded(dim), knob * 0.5f);
        dl.text_right(w.value_text, track.x - px(t_.space.md), track.cy(), px(t_.type.label),
                      Rgba::lerp(p().text_muted, p().accent, on).faded(dim), px(t_.type.tracking_button));
    }

    void paint_slider(const Widget& w, const WidgetAnim& a, DrawList& dl) const {
        const float dim = w.disabled ? 0.5f : 1.0f;
        const Rect track = slider_track(w);
        const float th = track.h;
        dl.fill(track, p().surface_void.faded(dim), th * 0.5f);
        dl.stroke(track, p().border_subtle.faded(dim), px(t_.border.hairline), th * 0.5f);
        const float fill_w = std::max(th, track.w * std::clamp(w.value01, 0.0f, 1.0f));
        dl.gradient_h({track.x, track.y, fill_w, th}, p().accent_strong.faded(dim), p().accent.faded(dim));
        const float knob = px(t_.metrics.slider_knob) * (1.0f + 0.18f * std::max(a.press, a.focus * 0.5f));
        const float kx = track.x + track.w * std::clamp(w.value01, 0.0f, 1.0f) - knob * 0.5f;
        const Rect knob_r{kx, track.cy() - knob * 0.5f, knob, knob};
        dl.shadow(knob_r, knob * 0.5f, px(5.0f), p().shadow.faded(0.45f * dim), px(1.5f));
        dl.rect(knob_r, RectStyle::outlined(p().text_primary.faded(dim), p().accent.faded(dim), px(t_.border.regular), knob * 0.5f));
        dl.text_right(w.value_text, w.control.right(), w.control.cy(), px(t_.type.body * 0.9f),
                      w.disabled ? p().text_muted : p().text_primary);
    }

    void paint_box(const Widget& w, const WidgetAnim& a, DrawList& dl, const Rect& box) const {
        const float emphasis = std::max(a.hover, a.focus);
        const Rgba border = w.disabled ? p().border_subtle.faded(0.5f)
                                       : Rgba::lerp(p().border_strong, p().border_bright, emphasis);
        dl.rect(box, RectStyle::outlined(p().surface_void.faded(w.disabled ? 0.5f : 1.0f), border,
                                         px(t_.border.hairline), px(t_.radius.sm)));
    }

    void paint_selector(const Widget& w, const WidgetAnim& a, DrawList& dl) const {
        const Rect box = selector_box(w);
        paint_box(w, a, dl, box);
        const float inset = px(t_.space.lg);
        const Rgba on = p().accent;
        const Rgba off = p().text_muted.faded(0.35f);
        chevron(dl, box.x + inset, box.cy(), -1, (w.can_decrement && !w.disabled) ? on : off);
        chevron(dl, box.right() - inset, box.cy(), +1, (w.can_increment && !w.disabled) ? on : off);
        dl.text_centered(w.value_text, box.cx(), box.cy(), px(t_.type.label),
                         w.disabled ? p().text_muted : p().text_primary, px(t_.type.tracking_button));
    }

    void paint_dropdown(const Widget& w, const WidgetAnim& a, DrawList& dl) const {
        const Rect box = selector_box(w);
        paint_box(w, a, dl, box);
        const float inset = px(t_.space.lg);
        const float text_w = box.w - 3.0f * inset;
        dl.text_v_centered(fit_text(w.value_text, text_w, px(t_.type.label)), box.x + inset, box.cy(), px(t_.type.label),
                           w.disabled ? p().text_muted : p().text_primary, px(t_.type.tracking_button));
        chevron(dl, box.right() - inset, box.cy(), w.expanded ? -2 : +2, w.disabled ? p().text_muted.faded(0.4f) : p().accent);
    }

    void paint_keycap(const Widget& w, const WidgetAnim& a, DrawList& dl) const {
        const float emphasis = std::max(a.hover, a.focus);
        Rgba border = Rgba::lerp(p().border_strong, p().border_bright, emphasis);
        Rgba fill = Rgba::lerp(p().surface_void, p().selection, emphasis * 0.6f);
        if (w.capturing) {
            border = p().highlight;
            fill = p().highlight.with_alpha(0.14f);
        }
        dl.rect(w.rect, RectStyle::outlined(fill, border, px(t_.border.hairline), px(t_.radius.sm)));
        // Key-cap lip: a darker bottom edge that reads as a physical key.
        dl.fill({w.rect.x + px(2.0f), w.rect.bottom() - px(3.0f), w.rect.w - px(4.0f), px(1.5f)}, p().shadow.faded(0.35f));
        const bool unbound = w.value_text.empty();
        const std::string text = w.capturing ? std::string("PRESS A KEY") : (unbound ? std::string("UNBOUND") : w.value_text);
        const Rgba col = w.capturing ? p().highlight : (unbound ? p().text_muted.faded(0.6f) : p().text_primary);
        dl.text_centered(fit_text(text, w.rect.w - px(t_.space.lg), px(t_.type.label)), w.rect.cx(), w.rect.cy(),
                         px(t_.type.label), col, px(t_.type.tracking_button));
        if (w.modified) {
            const float d = px(t_.metrics.modified_dot);
            dl.fill({w.rect.right() - d - px(t_.space.xs), w.rect.y + px(t_.space.xs), d, d}, p().highlight, d);
        }
    }

public:
    /// @brief Track geometry, shared with MenuSystem for pointer dragging.
    [[nodiscard]] Rect slider_track(const Widget& w) const noexcept {
        const float th = px(t_.metrics.slider_track_h);
        const float value_w = px(t_.metrics.slider_value_w);
        const float knob = px(t_.metrics.slider_knob);
        return {w.control.x + knob * 0.5f, w.control.cy() - th * 0.5f,
                w.control.w - value_w - px(t_.space.lg) - knob, th};
    }

    [[nodiscard]] Rect selector_box(const Widget& w) const noexcept {
        const float h = px(t_.metrics.selector_h);
        return {w.control.x, w.control.cy() - h * 0.5f, w.control.w, h};
    }
};

/// @brief One entry of a controls legend, e.g. {"ENTER / A", "SELECT"}.
struct LegendItem {
    const char* keys;
    const char* action;
};

/// @brief Draws (or, with `dl == nullptr`, only measures) a row of key-cap
/// hints. Returns the total width in pixels.
inline float paint_legend(DrawList* dl, const Theme& t, float s, float x, float cy,
                          std::initializer_list<LegendItem> items) {
    const Palette& p = t.palette;
    const float key_size = t.type.caption * 0.85f * s;
    const float act_size = t.type.caption * s;
    const float pad = t.space.sm * s;
    const float cap_h = t.type.caption * 1.6f * s;
    float pen = x;
    bool first = true;
    for (const LegendItem& it : items) {
        if (!first) pen += t.space.xl * s;
        first = false;
        const float kw = text_width(it.keys, key_size, t.type.tracking_button * s) + 2.0f * pad;
        if (dl) {
            dl->rect({pen, cy - cap_h * 0.5f, kw, cap_h},
                     RectStyle::outlined(p.surface_void.faded(0.8f), p.border_strong, t.border.hairline * s, t.radius.sm * s));
            dl->text_v_centered(it.keys, pen + pad, cy, key_size, p.text_secondary, t.type.tracking_button * s);
        }
        pen += kw + pad;
        if (dl) dl->text_v_centered(it.action, pen, cy, act_size, p.text_muted);
        pen += text_width(it.action, act_size);
    }
    return pen - x;
}

/// @brief Elevated card used by the settings, credits and modal surfaces.
inline void paint_panel(DrawList& dl, const Theme& t, float s, const Rect& r, const Shadow& shadow) {
    const float radius = t.radius.lg * s;
    dl.shadow(r, radius, shadow.blur * s, t.palette.shadow.faded(shadow.opacity), shadow.offset_y * s);
    dl.rect(r, RectStyle::outlined(t.palette.surface_panel, t.palette.border_strong, t.border.hairline * s, radius));
}

} // namespace fastjet::ui
