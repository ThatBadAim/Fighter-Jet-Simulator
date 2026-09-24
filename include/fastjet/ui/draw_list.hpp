#pragma once

#include "fastjet/graphics/menu_font.hpp"
#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>

namespace fastjet::ui {

/// @brief Straight-alpha RGBA colour used by the UI layer.
///
/// Kept separate from graphics::Color4 so the UI core never pulls in an
/// OpenGL header and can be exercised by headless tests.
struct Rgba {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;

    [[nodiscard]] constexpr Rgba with_alpha(float alpha) const noexcept { return {r, g, b, alpha}; }
    [[nodiscard]] constexpr Rgba faded(float factor) const noexcept { return {r, g, b, a * factor}; }

    [[nodiscard]] static constexpr Rgba lerp(const Rgba& x, const Rgba& y, float t) noexcept {
        return {x.r + (y.r - x.r) * t, x.g + (y.g - x.g) * t,
                x.b + (y.b - x.b) * t, x.a + (y.a - x.a) * t};
    }

    bool operator==(const Rgba&) const = default;
};

/// @brief Axis-aligned rectangle in framebuffer pixels (origin top-left, y down).
struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    [[nodiscard]] constexpr float right() const noexcept { return x + w; }
    [[nodiscard]] constexpr float bottom() const noexcept { return y + h; }
    [[nodiscard]] constexpr float cx() const noexcept { return x + w * 0.5f; }
    [[nodiscard]] constexpr float cy() const noexcept { return y + h * 0.5f; }

    [[nodiscard]] constexpr bool contains(float px, float py) const noexcept {
        return px >= x && px < x + w && py >= y && py < y + h;
    }
    [[nodiscard]] constexpr Rect inset(float d) const noexcept { return {x + d, y + d, w - 2.0f * d, h - 2.0f * d}; }
    [[nodiscard]] constexpr Rect inset(float dx, float dy) const noexcept { return {x + dx, y + dy, w - 2.0f * dx, h - 2.0f * dy}; }
    [[nodiscard]] constexpr Rect translated(float dx, float dy) const noexcept { return {x + dx, y + dy, w, h}; }

    [[nodiscard]] static Rect intersect(const Rect& a, const Rect& b) noexcept {
        const float x0 = std::max(a.x, b.x);
        const float y0 = std::max(a.y, b.y);
        const float x1 = std::min(a.right(), b.right());
        const float y1 = std::min(a.bottom(), b.bottom());
        return {x0, y0, std::max(0.0f, x1 - x0), std::max(0.0f, y1 - y0)};
    }

    bool operator==(const Rect&) const = default;
};

// ---------------------------------------------------------------------------
// Text metrics (em-normalised glyph data from the embedded menu font)
// ---------------------------------------------------------------------------

/// Cap height of the embedded face as a fraction of the em size.
inline constexpr float kFontCapHeight = 0.722f;

/// @brief Horizontal advance of a string at the given em size, in pixels.
[[nodiscard]] inline float text_width(std::string_view text, float size, float tracking = 0.0f) noexcept {
    float w = 0.0f;
    for (const char c : text) {
        const graphics::MenuGlyph* g = graphics::find_menu_glyph(static_cast<unsigned char>(c));
        w += (g ? g->advance : 0.5f) * size + tracking;
    }
    if (!text.empty()) w -= tracking; // no trailing tracking after the last glyph
    return w;
}

/// @brief Baseline that vertically centres capital letters on `center_y`.
[[nodiscard]] constexpr float baseline_for_center(float center_y, float size) noexcept {
    return center_y + kFontCapHeight * 0.5f * size;
}

/// @brief Truncates `text` with an ellipsis so it fits in `max_width`.
[[nodiscard]] inline std::string fit_text(std::string_view text, float max_width, float size, float tracking = 0.0f) {
    if (text_width(text, size, tracking) <= max_width) return std::string(text);
    constexpr std::string_view kEllipsis = "...";
    const float ellipsis_w = text_width(kEllipsis, size, tracking);
    std::string out;
    for (const char c : text) {
        out.push_back(c);
        if (text_width(out, size, tracking) + ellipsis_w > max_width) {
            out.pop_back();
            break;
        }
    }
    return out + std::string(kEllipsis);
}

/// @brief Greedy word wrap into lines no wider than `max_width`.
[[nodiscard]] inline std::vector<std::string> wrap_text(std::string_view text, float max_width, float size) {
    std::vector<std::string> lines;
    std::string line;
    size_t i = 0;
    while (i < text.size()) {
        const size_t next = text.find(' ', i);
        const std::string_view word = text.substr(i, next == std::string_view::npos ? std::string_view::npos : next - i);
        const std::string candidate = line.empty() ? std::string(word) : line + " " + std::string(word);
        if (!line.empty() && text_width(candidate, size) > max_width) {
            lines.push_back(line);
            line = std::string(word);
        } else {
            line = candidate;
        }
        if (next == std::string_view::npos) break;
        i = next + 1;
    }
    if (!line.empty()) lines.push_back(line);
    return lines;
}

// ---------------------------------------------------------------------------
// Draw commands
// ---------------------------------------------------------------------------

enum class DrawCmdType : uint8_t {
    RECT,      ///< Rounded rectangle with optional vertical gradient and border
    SHADOW,    ///< Soft drop shadow (Gaussian-like falloff around a rounded rect)
    LINE,      ///< Anti-aliased capsule segment
    TEXT,      ///< Single-line string on a baseline
    CLIP_PUSH, ///< Intersect the scissor with `rect`
    CLIP_POP,  ///< Restore the previous scissor
};

struct DrawCmd {
    DrawCmdType type = DrawCmdType::RECT;
    Rect rect{};
    Rgba fill_top{};
    Rgba fill_bottom{};
    Rgba border{0, 0, 0, 0};
    float radius = 0.0f;
    float border_width = 0.0f;
    float blur = 0.0f;
    bool horizontal = false; ///< RECT gradient runs left->right (fill_top = left)
    // LINE endpoints / TEXT origin
    float x0 = 0.0f, y0 = 0.0f, x1 = 0.0f, y1 = 0.0f;
    float thickness = 1.0f;
    // TEXT
    std::string text;
    float size = 16.0f;
    float tracking = 0.0f;
};

/// @brief Style for DrawList::rect().
struct RectStyle {
    Rgba fill_top{0, 0, 0, 0};
    Rgba fill_bottom{0, 0, 0, 0};
    Rgba border{0, 0, 0, 0};
    float radius = 0.0f;
    float border_width = 0.0f;

    [[nodiscard]] static RectStyle solid(const Rgba& fill, float radius = 0.0f) noexcept {
        return {fill, fill, {0, 0, 0, 0}, radius, 0.0f};
    }
    [[nodiscard]] static RectStyle outlined(const Rgba& fill, const Rgba& border, float border_width, float radius) noexcept {
        return {fill, fill, border, radius, border_width};
    }
};

/// @brief Backend-agnostic, ordered list of 2D draw commands.
///
/// Views emit into a DrawList; graphics::UiCanvas turns it into GPU work.
/// A layer stack applies opacity, translation and uniform scale to every
/// command so whole views can be animated (fade / slide / scale) without the
/// views knowing about it.
class DrawList {
public:
    /// Affine map `out = base + (in - pivot) * scale`, plus an opacity factor.
    struct Layer {
        float opacity = 1.0f;
        float scale = 1.0f;
        float pivot_x = 0.0f; ///< Scale pivot in the layer's input space
        float pivot_y = 0.0f;
        float base_x = 0.0f;  ///< Where the pivot lands in framebuffer space
        float base_y = 0.0f;
    };

    void clear() noexcept {
        cmds_.clear();
        layers_.clear();
    }

    [[nodiscard]] const std::vector<DrawCmd>& commands() const noexcept { return cmds_; }
    [[nodiscard]] bool empty() const noexcept { return cmds_.empty(); }

    /// @brief Pushes a transform layer composed with the current one: the new
    /// content is scaled about (origin_x, origin_y), offset by (dx, dy), then
    /// passed through every enclosing layer.
    void push_layer(float opacity, float dx = 0.0f, float dy = 0.0f,
                    float scale = 1.0f, float origin_x = 0.0f, float origin_y = 0.0f) {
        const Layer parent = current();
        Layer l;
        l.opacity = parent.opacity * opacity;
        l.scale = parent.scale * scale;
        l.pivot_x = origin_x;
        l.pivot_y = origin_y;
        l.base_x = apply_x(parent, origin_x) + dx * parent.scale;
        l.base_y = apply_y(parent, origin_y) + dy * parent.scale;
        layers_.push_back(l);
    }
    void pop_layer() {
        if (!layers_.empty()) layers_.pop_back();
    }

    void rect(const Rect& r, const RectStyle& style) {
        if (r.w <= 0.0f || r.h <= 0.0f) return;
        DrawCmd c;
        c.type = DrawCmdType::RECT;
        c.rect = transform(r);
        c.fill_top = fade(style.fill_top);
        c.fill_bottom = fade(style.fill_bottom);
        c.border = fade(style.border);
        c.radius = style.radius * current().scale;
        c.border_width = style.border_width * current().scale;
        if (c.fill_top.a <= 0.0f && c.fill_bottom.a <= 0.0f && (c.border.a <= 0.0f || c.border_width <= 0.0f)) return;
        cmds_.push_back(std::move(c));
    }

    void fill(const Rect& r, const Rgba& color, float radius = 0.0f) { rect(r, RectStyle::solid(color, radius)); }

    void gradient(const Rect& r, const Rgba& top, const Rgba& bottom, float radius = 0.0f) {
        rect(r, {top, bottom, {0, 0, 0, 0}, radius, 0.0f});
    }

    /// @brief Left-to-right gradient fill.
    void gradient_h(const Rect& r, const Rgba& left, const Rgba& right) {
        const size_t before = cmds_.size();
        rect(r, {left, right, {0, 0, 0, 0}, 0.0f, 0.0f});
        if (cmds_.size() > before) cmds_.back().horizontal = true;
    }

    void stroke(const Rect& r, const Rgba& color, float width, float radius = 0.0f) {
        rect(r, {{0, 0, 0, 0}, {0, 0, 0, 0}, color, radius, width});
    }

    void shadow(const Rect& r, float radius, float blur, const Rgba& color, float offset_y = 0.0f) {
        if (color.a <= 0.0f) return;
        DrawCmd c;
        c.type = DrawCmdType::SHADOW;
        c.rect = transform(r.translated(0.0f, offset_y));
        c.fill_top = c.fill_bottom = fade(color);
        c.radius = radius * current().scale;
        c.blur = blur * current().scale;
        cmds_.push_back(std::move(c));
    }

    void line(float x0, float y0, float x1, float y1, float thickness, const Rgba& color) {
        if (color.a <= 0.0f) return;
        const Layer& l = current();
        DrawCmd c;
        c.type = DrawCmdType::LINE;
        c.x0 = apply_x(l, x0);
        c.y0 = apply_y(l, y0);
        c.x1 = apply_x(l, x1);
        c.y1 = apply_y(l, y1);
        c.thickness = thickness * l.scale;
        c.fill_top = c.fill_bottom = fade(color);
        cmds_.push_back(std::move(c));
    }

    /// @brief Draws text with its baseline at `baseline_y`, starting at `x`.
    void text(std::string_view str, float x, float baseline_y, float size, const Rgba& color, float tracking = 0.0f) {
        if (str.empty() || color.a <= 0.0f) return;
        const Layer& l = current();
        DrawCmd c;
        c.type = DrawCmdType::TEXT;
        c.x0 = apply_x(l, x);
        c.y0 = apply_y(l, baseline_y);
        c.size = size * l.scale;
        c.tracking = tracking * l.scale;
        c.text = std::string(str);
        c.fill_top = c.fill_bottom = fade(color);
        cmds_.push_back(std::move(c));
    }

    /// @brief Text whose capitals are vertically centred on `center_y`.
    void text_v_centered(std::string_view str, float x, float center_y, float size, const Rgba& color, float tracking = 0.0f) {
        text(str, x, baseline_for_center(center_y, size), size, color, tracking);
    }

    void text_centered(std::string_view str, float center_x, float center_y, float size, const Rgba& color, float tracking = 0.0f) {
        text_v_centered(str, center_x - text_width(str, size, tracking) * 0.5f, center_y, size, color, tracking);
    }

    void text_right(std::string_view str, float right_x, float center_y, float size, const Rgba& color, float tracking = 0.0f) {
        text_v_centered(str, right_x - text_width(str, size, tracking), center_y, size, color, tracking);
    }

    void push_clip(const Rect& r) {
        DrawCmd c;
        c.type = DrawCmdType::CLIP_PUSH;
        c.rect = transform(r);
        cmds_.push_back(std::move(c));
    }
    void pop_clip() {
        DrawCmd c;
        c.type = DrawCmdType::CLIP_POP;
        cmds_.push_back(std::move(c));
    }

private:
    std::vector<DrawCmd> cmds_;
    std::vector<Layer> layers_;

    [[nodiscard]] Layer current() const noexcept { return layers_.empty() ? Layer{} : layers_.back(); }

    [[nodiscard]] static float apply_x(const Layer& l, float x) noexcept { return l.base_x + (x - l.pivot_x) * l.scale; }
    [[nodiscard]] static float apply_y(const Layer& l, float y) noexcept { return l.base_y + (y - l.pivot_y) * l.scale; }

    [[nodiscard]] Rect transform(const Rect& r) const noexcept {
        const Layer& l = current();
        return {apply_x(l, r.x), apply_y(l, r.y), r.w * l.scale, r.h * l.scale};
    }
    [[nodiscard]] Rgba fade(const Rgba& c) const noexcept { return c.faded(current().opacity); }
};

} // namespace fastjet::ui
