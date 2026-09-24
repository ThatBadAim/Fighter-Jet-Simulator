#pragma once

#include <cstdint>

/// @file
/// @brief Device-independent menu input.
///
/// The platform layer (see sdl_menu_bridge.hpp) translates mouse, keyboard
/// and gamepad events into these commands, so menu controllers never see a
/// device API and behave identically for every input method.

namespace fastjet::ui {

enum class NavCommand : uint8_t {
    UP,
    DOWN,
    LEFT,     ///< Move focus left, or decrease the focused control
    RIGHT,    ///< Move focus right, or increase the focused control
    ACCEPT,   ///< Enter / Space / gamepad A
    BACK,     ///< Esc / gamepad B
    NEXT_TAB, ///< Tab / E / gamepad RB
    PREV_TAB, ///< Shift+Tab / Q / gamepad LB
    CLEAR,    ///< Delete / Backspace / gamepad X: clear a key binding slot
};

enum class InputSource : uint8_t { POINTER, KEYBOARD, GAMEPAD };

struct InputEvent {
    enum class Type : uint8_t {
        NAVIGATE,
        POINTER_MOVE,
        POINTER_DOWN, ///< Primary button
        POINTER_UP,
        SCROLL,       ///< Positive = content moves down (wheel up)
        KEY_CAPTURE,  ///< Raw key while a rebind prompt is open
    };

    Type type = Type::NAVIGATE;
    InputSource source = InputSource::KEYBOARD;
    NavCommand nav = NavCommand::ACCEPT;
    float x = 0.0f; ///< Framebuffer pixels
    float y = 0.0f;
    float scroll = 0.0f;
    int32_t scancode = 0;

    [[nodiscard]] static InputEvent navigate(NavCommand c, InputSource s = InputSource::KEYBOARD) noexcept {
        InputEvent e;
        e.type = Type::NAVIGATE;
        e.nav = c;
        e.source = s;
        return e;
    }
    [[nodiscard]] static InputEvent pointer(Type t, float px, float py) noexcept {
        InputEvent e;
        e.type = t;
        e.source = InputSource::POINTER;
        e.x = px;
        e.y = py;
        return e;
    }
    [[nodiscard]] static InputEvent wheel(float amount) noexcept {
        InputEvent e;
        e.type = Type::SCROLL;
        e.source = InputSource::POINTER;
        e.scroll = amount;
        return e;
    }
    [[nodiscard]] static InputEvent key_capture(int32_t sc) noexcept {
        InputEvent e;
        e.type = Type::KEY_CAPTURE;
        e.scancode = sc;
        return e;
    }
};

} // namespace fastjet::ui
