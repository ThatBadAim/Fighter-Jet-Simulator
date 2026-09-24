#pragma once

#include <SDL3/SDL.h>
#include "fastjet/ui/input_events.hpp"
#include "fastjet/ui/user_settings.hpp"
#include <algorithm>
#include <optional>
#include <string>
#include <vector>

/// @file
/// @brief SDL3 platform adapter for the menu system and user settings.
///
/// This is the only place where menu input and settings touch SDL: it turns
/// device events into ui::InputEvent and turns GraphicsSettings into window
/// state. The menu core stays platform-independent.

namespace fastjet::ui::sdl {

/// @brief Converts SDL events into menu commands.
class MenuInputTranslator {
public:
    /// Stick deflection that counts as a D-pad press, and the level it must
    /// fall back below before the next press (hysteresis avoids repeats).
    static constexpr float kStickPress = 0.6f;
    static constexpr float kStickRelease = 0.3f;

    /// @param pixel_density Framebuffer pixels per window coordinate.
    /// @param capturing_key True while a key-rebind prompt is waiting.
    [[nodiscard]] std::optional<InputEvent> translate(const SDL_Event& ev, float pixel_density, bool capturing_key) {
        switch (ev.type) {
            case SDL_EVENT_MOUSE_MOTION:
                return InputEvent::pointer(InputEvent::Type::POINTER_MOVE, ev.motion.x * pixel_density, ev.motion.y * pixel_density);
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (ev.button.button != SDL_BUTTON_LEFT) return std::nullopt;
                return InputEvent::pointer(ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN ? InputEvent::Type::POINTER_DOWN
                                                                                  : InputEvent::Type::POINTER_UP,
                                           ev.button.x * pixel_density, ev.button.y * pixel_density);
            case SDL_EVENT_MOUSE_WHEEL: {
                const float dir = ev.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1.0f : 1.0f;
                return InputEvent::wheel(ev.wheel.y * dir);
            }
            case SDL_EVENT_KEY_DOWN: return translate_key(ev.key, capturing_key);
            case SDL_EVENT_GAMEPAD_BUTTON_DOWN: return translate_button(ev.gbutton.button);
            case SDL_EVENT_GAMEPAD_AXIS_MOTION: return translate_axis(ev.gaxis);
            default: return std::nullopt;
        }
    }

private:
    int stick_x_ = 0; ///< Latched stick direction per axis: -1, 0, +1
    int stick_y_ = 0;

    static std::optional<InputEvent> nav(NavCommand c, InputSource s = InputSource::KEYBOARD) {
        return InputEvent::navigate(c, s);
    }

    static std::optional<InputEvent> translate_key(const SDL_KeyboardEvent& k, bool capturing) {
        if (capturing) {
            if (k.repeat) return std::nullopt;
            if (k.scancode == SDL_SCANCODE_ESCAPE) return nav(NavCommand::BACK);
            if (k.scancode == SDL_SCANCODE_DELETE || k.scancode == SDL_SCANCODE_BACKSPACE) return nav(NavCommand::CLEAR);
            return InputEvent::key_capture(static_cast<int32_t>(k.scancode));
        }
        switch (k.scancode) {
            // Direction keys auto-repeat so holding LEFT sweeps a slider.
            case SDL_SCANCODE_UP: return nav(NavCommand::UP);
            case SDL_SCANCODE_DOWN: return nav(NavCommand::DOWN);
            case SDL_SCANCODE_LEFT: return nav(NavCommand::LEFT);
            case SDL_SCANCODE_RIGHT: return nav(NavCommand::RIGHT);
            default: break;
        }
        if (k.repeat) return std::nullopt;
        switch (k.scancode) {
            case SDL_SCANCODE_RETURN:
            case SDL_SCANCODE_KP_ENTER:
            case SDL_SCANCODE_SPACE: return nav(NavCommand::ACCEPT);
            case SDL_SCANCODE_ESCAPE:
            case SDL_SCANCODE_BACKSPACE: return nav(NavCommand::BACK);
            case SDL_SCANCODE_TAB: return nav((k.mod & SDL_KMOD_SHIFT) ? NavCommand::PREV_TAB : NavCommand::NEXT_TAB);
            case SDL_SCANCODE_Q: return nav(NavCommand::PREV_TAB);
            case SDL_SCANCODE_E: return nav(NavCommand::NEXT_TAB);
            case SDL_SCANCODE_DELETE: return nav(NavCommand::CLEAR);
            default: return std::nullopt;
        }
    }

    static std::optional<InputEvent> translate_button(Uint8 button) {
        constexpr InputSource g = InputSource::GAMEPAD;
        switch (button) {
            case SDL_GAMEPAD_BUTTON_DPAD_UP: return nav(NavCommand::UP, g);
            case SDL_GAMEPAD_BUTTON_DPAD_DOWN: return nav(NavCommand::DOWN, g);
            case SDL_GAMEPAD_BUTTON_DPAD_LEFT: return nav(NavCommand::LEFT, g);
            case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: return nav(NavCommand::RIGHT, g);
            case SDL_GAMEPAD_BUTTON_SOUTH: return nav(NavCommand::ACCEPT, g);
            case SDL_GAMEPAD_BUTTON_EAST: return nav(NavCommand::BACK, g);
            case SDL_GAMEPAD_BUTTON_WEST: return nav(NavCommand::CLEAR, g);
            case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: return nav(NavCommand::PREV_TAB, g);
            case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return nav(NavCommand::NEXT_TAB, g);
            default: return std::nullopt;
        }
    }

    std::optional<InputEvent> translate_axis(const SDL_GamepadAxisEvent& a) {
        const bool is_x = a.axis == SDL_GAMEPAD_AXIS_LEFTX;
        if (!is_x && a.axis != SDL_GAMEPAD_AXIS_LEFTY) return std::nullopt;
        const float v = static_cast<float>(a.value) / 32767.0f;
        int& latched = is_x ? stick_x_ : stick_y_;
        if (latched != 0) {
            if (std::abs(v) < kStickRelease) latched = 0;
            return std::nullopt;
        }
        if (std::abs(v) < kStickPress) return std::nullopt;
        latched = v > 0.0f ? 1 : -1;
        const NavCommand c = is_x ? (latched > 0 ? NavCommand::RIGHT : NavCommand::LEFT)
                                  : (latched > 0 ? NavCommand::DOWN : NavCommand::UP);
        return nav(c, InputSource::GAMEPAD);
    }
};

/// @brief Human-readable name of a physical key in the active keyboard layout.
[[nodiscard]] inline std::string key_name(int32_t scancode) {
    const auto sc = static_cast<SDL_Scancode>(scancode);
    const SDL_Keycode key = SDL_GetKeyFromScancode(sc, SDL_KMOD_NONE, false);
    const char* name = SDL_GetKeyName(key);
    if (!name || !*name) name = SDL_GetScancodeName(sc);
    return name ? std::string(name) : std::string();
}

/// @brief True if either key bound to an action is held.
[[nodiscard]] inline bool is_down(const bool* keys, const KeyBinding& b) noexcept {
    if (!keys) return false;
    auto held = [keys](int32_t sc) { return sc > scancode::NONE && sc < SDL_SCANCODE_COUNT && keys[sc]; };
    return held(b.primary) || held(b.secondary);
}

/// @brief Distinct fullscreen resolutions of the window's display, largest first.
[[nodiscard]] inline std::vector<Resolution> query_resolutions(SDL_Window* window) {
    std::vector<Resolution> out;
    const SDL_DisplayID display = window ? SDL_GetDisplayForWindow(window) : SDL_GetPrimaryDisplay();
    int count = 0;
    SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(display, &count);
    if (!modes) return out;
    for (int i = 0; i < count; ++i) {
        const Resolution r{modes[i]->w, modes[i]->h};
        if (r.width < limits::kMinResolution) continue;
        if (std::find(out.begin(), out.end(), r) == out.end()) out.push_back(r);
    }
    SDL_free(modes);
    return out;
}

/// @brief True if two graphics configurations need different window state.
[[nodiscard]] inline bool video_differs(const GraphicsSettings& a, const GraphicsSettings& b) noexcept {
    return a.resolution != b.resolution || a.display_mode != b.display_mode || a.vsync != b.vsync;
}

/// @brief Applies display mode, resolution and vsync to a window.
/// @param resize_window False keeps the current window size (e.g. a size
///        given on the command line).
inline void apply_video(SDL_Window* window, const GraphicsSettings& g, bool resize_window = true) {
    if (!window) return;
    // Adaptive vsync (-1) would be preferable, but not every driver has it.
    SDL_GL_SetSwapInterval(g.vsync ? 1 : 0);

    switch (g.display_mode) {
        case DisplayMode::WINDOWED:
            SDL_SetWindowFullscreen(window, false);
            SDL_SetWindowBordered(window, true);
            if (resize_window) {
                SDL_SetWindowSize(window, g.resolution.width, g.resolution.height);
                SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
            }
            break;
        case DisplayMode::BORDERLESS:
            SDL_SetWindowFullscreenMode(window, nullptr); // Desktop-sized, no mode switch
            SDL_SetWindowFullscreen(window, true);
            break;
        case DisplayMode::FULLSCREEN: {
            SDL_DisplayMode mode{};
            const SDL_DisplayID display = SDL_GetDisplayForWindow(window);
            if (SDL_GetClosestFullscreenDisplayMode(display, g.resolution.width, g.resolution.height, 0.0f, true, &mode)) {
                SDL_SetWindowFullscreenMode(window, &mode);
            } else {
                SDL_SetWindowFullscreenMode(window, nullptr);
            }
            SDL_SetWindowFullscreen(window, true);
            break;
        }
        case DisplayMode::COUNT: break;
    }
    SDL_SyncWindow(window);
}

/// @brief Sleeps to hold a frame-rate cap, with drift correction.
class FrameLimiter {
public:
    /// @param hz Target rate; 0 disables limiting.
    void wait(int hz) {
        if (hz <= 0) {
            next_ns_ = 0;
            return;
        }
        const Uint64 period = SDL_NS_PER_SECOND / static_cast<Uint64>(hz);
        const Uint64 now = SDL_GetTicksNS();
        // Resynchronise after a stall (loading, dragging the window) instead
        // of racing to catch up.
        if (next_ns_ == 0 || now > next_ns_ + period) next_ns_ = now;
        else if (now < next_ns_) SDL_DelayPrecise(next_ns_ - now);
        next_ns_ += period;
    }

private:
    Uint64 next_ns_ = 0;
};

} // namespace fastjet::ui::sdl
