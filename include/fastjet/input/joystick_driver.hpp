#pragma once

#include <SDL3/SDL.h>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace fastjet {
namespace input {

/**
 * @brief Abstract interface for low-latency controller hardware polling.
 */
class IJoystickDriver {
public:
    virtual ~IJoystickDriver() = default;

    virtual bool initialize() = 0;
    virtual void poll() = 0;
    virtual int16_t get_axis(int device_index, int axis_index) const = 0;
    virtual bool get_button(int device_index, int button_index) const = 0;
    virtual uint8_t get_hat(int device_index, int hat_index) const = 0;
    virtual int get_device_count() const = 0;
    virtual std::string get_device_name(int device_index) const = 0;
};

/**
 * @brief SDL3 hardware driver for direct low-latency HOTAS polling.
 */
class SDL3InputDriver : public IJoystickDriver {
private:
    std::vector<SDL_Joystick*> joysticks_{};
    bool initialized_{false};

public:
    SDL3InputDriver() = default;

    ~SDL3InputDriver() override {
        shutdown();
    }

    bool initialize() override {
        if (initialized_) return true;

        if (!SDL_Init(SDL_INIT_JOYSTICK)) {
            return false;
        }

        int count = 0;
        SDL_JoystickID* ids = SDL_GetJoysticks(&count);
        if (ids) {
            for (int i = 0; i < count; ++i) {
                SDL_Joystick* joy = SDL_OpenJoystick(ids[i]);
                if (joy) {
                    joysticks_.push_back(joy);
                }
            }
            SDL_free(ids);
        }

        initialized_ = true;
        return true;
    }

    void poll() override {
        if (!initialized_) return;
        SDL_UpdateJoysticks();
    }

    [[nodiscard]] int16_t get_axis(int device_index, int axis_index) const override {
        if (device_index < 0 || device_index >= static_cast<int>(joysticks_.size())) {
            return 0;
        }
        return SDL_GetJoystickAxis(joysticks_[device_index], axis_index);
    }

    [[nodiscard]] bool get_button(int device_index, int button_index) const override {
        if (device_index < 0 || device_index >= static_cast<int>(joysticks_.size())) {
            return false;
        }
        return SDL_GetJoystickButton(joysticks_[device_index], button_index);
    }

    [[nodiscard]] uint8_t get_hat(int device_index, int hat_index) const override {
        if (device_index < 0 || device_index >= static_cast<int>(joysticks_.size())) {
            return SDL_HAT_CENTERED;
        }
        return SDL_GetJoystickHat(joysticks_[device_index], hat_index);
    }

    [[nodiscard]] int get_device_count() const override {
        return static_cast<int>(joysticks_.size());
    }

    [[nodiscard]] std::string get_device_name(int device_index) const override {
        if (device_index < 0 || device_index >= static_cast<int>(joysticks_.size())) {
            return "Unknown Device";
        }
        const char* name = SDL_GetJoystickName(joysticks_[device_index]);
        return name ? std::string(name) : "Generic Joystick";
    }

    void shutdown() {
        for (SDL_Joystick* joy : joysticks_) {
            if (joy) {
                SDL_CloseJoystick(joy);
            }
        }
        joysticks_.clear();
        if (initialized_) {
            SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
            initialized_ = false;
        }
    }
};

/**
 * @brief Programmable Virtual HOTAS device driver for deterministic unit testing.
 */
class VirtualHOTASDriver : public IJoystickDriver {
public:
    static constexpr std::size_t MAX_AXES = 8;
    static constexpr std::size_t MAX_BUTTONS = 32;
    static constexpr std::size_t MAX_HATS = 4;

    std::array<int16_t, MAX_AXES> axes_{0};
    std::array<bool, MAX_BUTTONS> buttons_{false};
    std::array<uint8_t, MAX_HATS> hats_{SDL_HAT_CENTERED};

    VirtualHOTASDriver() = default;

    bool initialize() override {
        return true;
    }

    void poll() override {
        // Mock driver, instant state access
    }

    void set_axis(std::size_t axis_idx, int16_t val) noexcept {
        if (axis_idx < MAX_AXES) axes_[axis_idx] = val;
    }

    void set_button(std::size_t btn_idx, bool pressed) noexcept {
        if (btn_idx < MAX_BUTTONS) buttons_[btn_idx] = pressed;
    }

    void set_hat(std::size_t hat_idx, uint8_t hat_val) noexcept {
        if (hat_idx < MAX_HATS) hats_[hat_idx] = hat_val;
    }

    [[nodiscard]] int16_t get_axis(int, int axis_index) const override {
        if (axis_index >= 0 && axis_index < static_cast<int>(MAX_AXES)) {
            return axes_[axis_index];
        }
        return 0;
    }

    [[nodiscard]] bool get_button(int, int button_index) const override {
        if (button_index >= 0 && button_index < static_cast<int>(MAX_BUTTONS)) {
            return buttons_[button_index];
        }
        return false;
    }

    [[nodiscard]] uint8_t get_hat(int, int hat_index) const override {
        if (hat_index >= 0 && hat_index < static_cast<int>(MAX_HATS)) {
            return hats_[hat_index];
        }
        return SDL_HAT_CENTERED;
    }

    [[nodiscard]] int get_device_count() const override {
        return 1;
    }

    [[nodiscard]] std::string get_device_name(int) const override {
        return "Virtual HOTAS Device (Mock)";
    }
};

} // namespace input
} // namespace fastjet
