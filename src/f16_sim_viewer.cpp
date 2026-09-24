#include <SDL3/SDL.h>
#include <epoxy/gl.h>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <vector>

#include "fastjet/fdm/flight_state.hpp"
#include "fastjet/fdm/mass_properties.hpp"
#include "fastjet/fdm/rk4_integrator.hpp"
#include "fastjet/fdm/six_dof_fdm.hpp"
#include "fastjet/environment/atmosphere1976.hpp"
#include "fastjet/environment/wind_turbulence.hpp"
#include "fastjet/aircraft/aircraft_type.hpp"
#include "fastjet/aircraft/aircraft_config.hpp"
#include "fastjet/aero/aircraft_aero_model.hpp"
#include "fastjet/flcs/aircraft_flcs.hpp"
#include "fastjet/flcs/onboard_flight_computer.hpp"
#include "fastjet/input/input_manager.hpp"
#include "fastjet/propulsion/multi_engine.hpp"
#include "fastjet/fdm/fuel_system.hpp"
#include "fastjet/gear/landing_gear.hpp"
#include "fastjet/environment/ground_collision.hpp"

#define CGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "fastjet/graphics/render_engine.hpp"
#include "fastjet/graphics/frame_pacer.hpp"
#include "fastjet/graphics/cockpit_telemetry.hpp"
#include "fastjet/audio/audio_engine.hpp"
#include "fastjet/ui/menu_system.hpp"
#include "fastjet/ui/sdl_menu_bridge.hpp"
#include "fastjet/version.hpp"

using namespace fastjet;

namespace {

/// @brief --profile: frame-time distribution and CPU time per loop stage,
/// reported (and reset) every couple of seconds.
struct FrameStats {
    std::vector<double> frame_ms;
    double physics_ms = 0.0;
    double render_ms = 0.0;
    double present_ms = 0.0;
    double elapsed_s = 0.0;

    void add(double frame, double physics, double render, double present) {
        frame_ms.push_back(frame);
        physics_ms += physics;
        render_ms += render;
        present_ms += present;
        elapsed_s += frame / 1000.0;
    }

    [[nodiscard]] std::string report() {
        if (frame_ms.empty()) return {};
        const double n = static_cast<double>(frame_ms.size());
        std::vector<double> sorted = frame_ms;
        std::sort(sorted.begin(), sorted.end());
        auto pct = [&](double p) { return sorted[static_cast<size_t>(p * (n - 1.0))]; };
        char line[256];
        std::snprintf(line, sizeof(line),
                      "[PROFILE] %.1f fps | frame p50 %.2f p99 %.2f max %.2f ms | cpu physics %.2f  render %.2f"
                      "  present %.2f ms\n",
                      n / elapsed_s, pct(0.5), pct(0.99), sorted.back(), physics_ms / n, render_ms / n,
                      present_ms / n);
        *this = FrameStats{};
        return line;
    }
};

} // namespace

int main(int argc, char* argv[]) {
    std::cout << "=========================================================\n";
    std::cout << "  Fast Jet Flight Simulator - Realistic Multi-Aircraft Sim\n";
    std::cout << "=========================================================\n";
    std::cout << "Aircraft Available:\n";
    std::cout << "  [F3] F-16C Fighting Falcon (Single F110, Analog/Digital FBW)\n";
    std::cout << "  [F4] F-15EX Eagle II (Twin F110, Digital FBW, Mach 2.50)\n";
    std::cout << "  [F5] Eurofighter Typhoon (Twin EJ200, Canard Carefree FBW, Supercruise)\n";
    std::cout << "  [F6] F-22A Raptor (Twin F119 2D TVC, 5th-Gen FBW, Supercruise)\n";
    std::cout << "  [F7] A-10C Thunderbolt II (Twin TF34 Non-AB, Dual SAS, CAS)\n";
    std::cout << "  [F8] Cycle Next Aircraft\n";
    std::cout << "Controls:\n";
    std::cout << "  Right Mouse Drag / Tab: Look around cockpit (scan MFDs, canopy, runway)\n";
    std::cout << "  Middle Mouse:           Recenter pilot view forward\n";
    std::cout << "  Arrow Keys / Stick:     Pitch and roll stick inceptor commands\n";
    std::cout << "  A / D:                  Rudder pedals (left / right)\n";
    std::cout << "  Shift / Ctrl:           Throttle increase / decrease (detents 1/2/3/4)\n";
    std::cout << "  B:                      Speedbrakes extend / retract\n";
    std::cout << "  Space:                  Wheel brakes (on ground) / Recenter view (air)\n";
    std::cout << "  G:                      Landing gear handle up / down\n";
    std::cout << "  F1 / F2 / V:            Cockpit View (F1) / Chase View (F2) / Toggle View (V)\n";
    std::cout << "  Mouse Wheel:            Zoom chase camera in/out (Chase mode)\n";
    std::cout << "  H:                      Toggle Cockpit G-Head Motion (Default: Fixed DEP like MSFS)\n";
    std::cout << "  T / R:                  Trim reset / Reset simulation\n";
    std::cout << "  Esc / Gamepad Start:    Pause menu (settings, credits, quit)\n";
    std::cout << "  (Flight keys can be rebound in Settings > Controls.)\n";
    std::cout << "  --profile:              Print frame timing and per-pass GPU cost every 2 s\n";
    std::cout << "---------------------------------------------------------\n";

    bool headless = false;
    int max_frames = -1;
    int win_w = 1920;
    int win_h = 1080;
    bool fullscreen = false;
    aircraft::AircraftType selected_aircraft = aircraft::AircraftType::F16_FIGHTING_FALCON;
    std::string screenshot_path;
    bool no_menu = false;
    bool start_chase = false;
    bool start_airborne = false;
    bool profile = false;            // --profile: frame timing report
    bool cli_size = false;           // --width/--height override the saved resolution for this run
    std::string settings_arg;        // --settings PATH
    ui::ScreenId boot_screen = ui::ScreenId::MAIN;
    std::optional<ui::SettingsSection> boot_tab;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--headless") headless = true;
        if (arg == "--frames" && i + 1 < argc) max_frames = std::atoi(argv[++i]);
        if (arg == "--width" && i + 1 < argc) { win_w = std::atoi(argv[++i]); cli_size = true; }
        if (arg == "--height" && i + 1 < argc) { win_h = std::atoi(argv[++i]); cli_size = true; }
        if (arg == "--settings" && i + 1 < argc) settings_arg = argv[++i];
        // Tooling: open the menu on a given screen / settings tab (screenshots).
        if (arg == "--ui-screen" && i + 1 < argc) {
            const std::string v = argv[++i];
            if (v == "settings") boot_screen = ui::ScreenId::SETTINGS;
            else if (v == "credits") boot_screen = ui::ScreenId::CREDITS;
        }
        if (arg == "--ui-tab" && i + 1 < argc) {
            const std::string v = argv[++i];
            if (v == "graphics") boot_tab = ui::SettingsSection::GRAPHICS;
            else if (v == "audio") boot_tab = ui::SettingsSection::AUDIO;
            else if (v == "controls") boot_tab = ui::SettingsSection::CONTROLS;
            else if (v == "accessibility") boot_tab = ui::SettingsSection::ACCESSIBILITY;
        }
        if (arg == "--fullscreen") fullscreen = true;
        if (arg == "--screenshot" && i + 1 < argc) screenshot_path = argv[++i];
        if (arg == "--no-menu") no_menu = true;
        if (arg == "--chase") start_chase = true;
        if (arg == "--airborne") start_airborne = true;
        if (arg == "--profile") profile = true;
        if (arg == "--aircraft" && i + 1 < argc) {
            selected_aircraft = aircraft::parse_aircraft_type(argv[++i]);
        }
    }

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    // User preferences. Loaded before the window exists because resolution,
    // display mode and MSAA are fixed at creation. Headless runs keep
    // settings in memory so automated captures are reproducible and never
    // touch the user's file, unless --settings names one explicitly.
    std::filesystem::path settings_path;
    if (!settings_arg.empty()) {
        settings_path = settings_arg;
    } else if (!headless) {
        if (char* pref = SDL_GetPrefPath("FastJet", "FastJetSimulator")) {
            settings_path = std::filesystem::path(pref) / "settings.json";
            SDL_free(pref);
        }
    }
    ui::SettingsManager settings{ui::SettingsStorage(settings_path)};
    {
        const auto loaded = settings.load();
        if (!loaded.message.empty()) std::cout << "[SETTINGS] " << loaded.message << "\n";
    }
    const ui::GraphicsSettings boot_gfx = settings.committed().graphics;
    if (!cli_size) {
        win_w = boot_gfx.resolution.width;
        win_h = boot_gfx.resolution.height;
    }
    if (boot_gfx.display_mode != ui::DisplayMode::WINDOWED && !headless) fullscreen = true;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    // Window multisampling (cockpit, HUD, menus) from the quality preset.
    const int msaa = ui::msaa_samples(boot_gfx.quality);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, msaa > 0 ? 1 : 0);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, msaa);

    Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (headless) window_flags |= SDL_WINDOW_HIDDEN;
    if (fullscreen) window_flags |= SDL_WINDOW_FULLSCREEN;

    SDL_Window* window = SDL_CreateWindow("F-16 Flight Sim - Cockpit View", win_w, win_h, window_flags);
    if (!window) {
        std::cerr << "Failed to create SDL window: " << SDL_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    SDL_GLContext gl_ctx = SDL_GL_CreateContext(window);
    if (!gl_ctx) {
        std::cerr << "Failed to create GL context: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

#if defined(_WIN32) || !defined(__has_include) || !__has_include(<epoxy/gl.h>)
    FastJetGLFunctions::instance().load();
#endif

    if (headless) {
        SDL_GL_SetSwapInterval(0); // Never block an offscreen capture on the display
    } else {
        ui::sdl::apply_video(window, boot_gfx, /*resize_window=*/!cli_size);
    }

    // Query actual physical pixel dimensions (for High-DPI / Retina / fractional scale displays)
    int pixel_w = win_w;
    int pixel_h = win_h;
    SDL_GetWindowSizeInPixels(window, &pixel_w, &pixel_h);

    // 1. Initialize Graphics Engine
    graphics::RenderEngine renderer;
    if (!renderer.init(pixel_w, pixel_h, 60.0f)) {
        std::cerr << "Failed to initialize graphics render engine!\n";
        SDL_GL_DestroyContext(gl_ctx);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // 2. Initialize Physics & Flight Dynamics Model
    // Configurable across F-16C, F-15EX, Typhoon, F-22A, and A-10C
    propulsion::MultiEngine engine(selected_aircraft);
    gear::LandingGear landing_gear(selected_aircraft);
    auto mass = fdm::FuelSystem::compute(engine.fuel_kg, selected_aircraft);
    const fdm::RK4Integrator integrator(0.005);
    aero::AircraftAeroModel aero_model(selected_aircraft);
    flcs::AircraftFLCS flight_control_system(selected_aircraft);
    flcs::OnBoardFlightComputer ofc(selected_aircraft); ///< On-Board Flight Computer (GLOC + Auto-GCAS)

    auto joy_driver = std::make_shared<input::SDL3InputDriver>();
    if (joy_driver->initialize() && joy_driver->get_device_count() > 0) {
        std::cout << "[HOTAS] Controller detected: " << joy_driver->get_device_name(0) << "\n";
    } else {
        std::cout << "[HOTAS] No physical joystick detected. Keyboard control active.\n";
    }
    input::InputManager input_mgr(joy_driver);

    // 3. Audio & Environmental Simulation Subsystems
    audio::AudioEngine audio_engine;
    if (!headless) {
        if (audio_engine.init()) {
            std::cout << "[AUDIO] Procedural cockpit sound engine online (SDL3).\n";
        }
    }
    environment::WindTurbulenceModel wind_model;
    wind_model.base_wind_speed_mps = 5.0; // 10 knot crosswind

    // 4. Menu system and settings observers. The menu edits the settings
    //    model only; these observers push changes out to each subsystem.
    ui::MenuSystem main_menu(settings, ui::AppInfo{"FAST JET SIMULATOR", kVersionString});
    main_menu.set_key_name_provider(ui::sdl::key_name);
    main_menu.set_resolutions(ui::sdl::query_resolutions(window));
    main_menu.set_launch_quality(boot_gfx.quality);
    ui::sdl::MenuInputTranslator menu_input;
    ui::sdl::FrameLimiter frame_limiter;
    ui::DrawList ui_draw_list;

    // Preview-safe settings follow the pending copy, so sliders and colour
    // options respond while the user is still editing.
    auto apply_live_settings = [&](const ui::UserSettings& s) {
        audio_engine.set_bus_gains(s.audio.bus_gain(s.audio.engine), s.audio.bus_gain(s.audio.alerts));
        renderer.set_color_filter(s.accessibility.colorblind);
        renderer.apply_theme(ui::build_theme(s.accessibility));
    };
    ui::GraphicsSettings applied_gfx = boot_gfx;
    auto apply_committed_settings = [&](const ui::UserSettings& s) {
        if (!headless && ui::sdl::video_differs(s.graphics, applied_gfx)) {
            ui::sdl::apply_video(window, s.graphics);
        }
        applied_gfx = s.graphics;
        renderer.camera_rig().set_g_head_motion(s.graphics.screen_shake);
        // Scene quality (world MSAA, shadows, bloom, clouds) applies live.
        renderer.set_quality(graphics::RenderQuality::from_preset(s.graphics.quality));
    };
    settings.subscribe([&](ui::SettingsEvent event, const ui::SettingsManager& m) {
        apply_live_settings(m.pending());
        if (event == ui::SettingsEvent::APPLIED) apply_committed_settings(m.committed());
    });
    apply_live_settings(settings.committed());
    apply_committed_settings(settings.committed());

    // Pilot head tracking and mouse-look state
    bool right_mouse_down = false;
    bool mouse_look_toggle = false;

    // Compute static stance altitude for the selected airframe resting on ground
    auto compute_ground_stance_z = [](aircraft::AircraftType type) noexcept -> double {
        const auto cfg = aircraft::AircraftConfig::get(type);
        const double uncompressed_h = cfg.gear.main_l_pos_b.z + cfg.gear.rest_length;
        const double static_weight = (cfg.mass.empty_mass_kg + cfg.mass.internal_fuel_capacity_kg) * 9.80665;
        const double main_load_per_strut = (static_weight * 0.85) * 0.5;
        const double static_compression = main_load_per_strut / cfg.gear.main_spring_k;
        const double stance_h = uncompressed_h - std::clamp(static_compression, 0.05, cfg.gear.max_stroke * 0.8);
        return -stance_h; // In NED, down is positive, so altitude above ground is negative Z
    };

    // Initial state: Runway 09 threshold lineup, gear resting on asphalt, ready for takeoff roll
    auto reset_flight_state = [&]() -> fdm::FlightState {
        fdm::FlightState s{};
        if (start_airborne) {
            s.pos_ned = math::Vector3(0.0, 0.0, -1500.0); // 1,500 m altitude
            s.vel_b   = math::Vector3(220.0, 0.0, 0.0);
            s.omega_b = math::Vector3::zero();
            s.q_att   = math::Quaternion::identity();
            return s;
        }
        const double stance_z = compute_ground_stance_z(selected_aircraft);
        s.pos_ned = math::Vector3(150.0, 0.0, stance_z); // Runway 09 lineup
        s.vel_b   = math::Vector3(0.0, 0.0, 0.0);         // Stationary on runway
        s.omega_b = math::Vector3::zero();
        s.q_att   = math::Quaternion::identity();        // Heading 000 deg down the runway
        return s;
    };

    // How long the HUD holds the airframe change confirmation banner.
    constexpr double AIRCRAFT_SWITCH_BANNER_SEC = 2.5;
    double aircraft_switch_timer = 0.0;

    fdm::FlightState state = reset_flight_state();
    fdm::FlightState prev_state = state;
    bool is_crashed = false;
    bool gear_down = true;

    double sim_time = 0.0;
    fdm::AircraftForces current_forces{};
    flcs::IMUData current_imu{};
    current_imu.Nz = 1.0;

    auto switch_aircraft = [&](aircraft::AircraftType new_type) {
        // Re-selecting the active airframe would otherwise reset every actuator
        // and re-seat the gear for no visible reason, UNLESS currently crashed.
        if (new_type == selected_aircraft && !is_crashed) {
            std::cout << "[AIRCRAFT] " << aircraft::to_string(new_type)
                      << " already selected.\n";
            return;
        }

        selected_aircraft = new_type;
        // Carry the fuel state across: an in-flight airframe change must not
        // hand the pilot a full tank.
        engine.configure(new_type, /*preserve_fuel=*/true);
        aero_model.configure(new_type);
        flight_control_system.configure(new_type);
        // The OFC plans recoveries on the airframe's own G, alpha and roll limits.
        ofc.configure(new_type);
        landing_gear.configure(new_type);
        mass = fdm::FuelSystem::compute(engine.fuel_kg, new_type);

        // If switching while crashed or stationary on the runway, reset stance cleanly
        if (is_crashed || (!start_airborne && state.airspeed() < 5.0 && landing_gear.weight_on_wheels())) {
            state = reset_flight_state();
            prev_state = state;
            is_crashed = false;
            engine.reset();
            landing_gear.reset(new_type);
            ofc.reset();
            flight_control_system.reset();
            input_mgr.trim_hat.reset();
            current_forces = fdm::AircraftForces{};
            current_imu = flcs::IMUData{};
            current_imu.Nz = 1.0;
        }

        // Raise the on-screen confirmation. The console banner below is
        // invisible in fullscreen, so the HUD is the signal that actually
        // reaches the pilot.
        aircraft_switch_timer = AIRCRAFT_SWITCH_BANNER_SEC;

        std::cout << "\n=========================================================\n";
        std::cout << "  [AIRCRAFT SWITCHED] -> " << aircraft::to_string(new_type) << "\n";
        std::cout << "  " << aircraft::signature_spec(new_type) << "\n";
        std::cout << "=========================================================\n";
        if (window) {
            std::string title = std::string(aircraft::to_string(new_type)) + " - Cockpit View";
            SDL_SetWindowTitle(window, title.c_str());
        }
    };

    bool flight_started = no_menu;          // False until the pilot leaves the boot menu
    bool mission_from_main_menu = false;    // Tactical menu opened from the main menu returns there
    if (!no_menu) {
        main_menu.open(boot_screen, /*in_flight=*/false);
        main_menu.update(0.0f, static_cast<float>(pixel_w), static_cast<float>(pixel_h));
        if (boot_tab) main_menu.show_settings_tab(*boot_tab);
    }
    SDL_Gamepad* gamepad = nullptr;
    auto upper = [](std::string text) {
        for (char& c : text) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return text;
    };
    if (start_chase) {
        renderer.camera_rig().set_mode(graphics::CameraMode::CHASE);
    }

    // Keyboard stick emulation state
    double target_pitch = 0.0;
    double target_roll  = 0.0;
    double target_yaw   = 0.0;
    double stick_pitch  = 0.0;
    double stick_roll   = 0.0;
    double pedal_yaw    = 0.0;
    double throttle     = start_airborne ? 0.65 : 0.08; // Ground idle for runway lineup
    double throttle_dir = 0.0;  // Commanded lever direction this frame [-1, +1]

    // Physical throttle lever slew: a pilot takes roughly 1.5 s to run the
    // lever from idle to full reheat, so the full travel is covered in that
    // time regardless of frame rate.
    constexpr double THROTTLE_SLEW_PER_SEC = 0.67;

    // A hardware throttle takes over only once the pilot actually moves it,
    // so an idle or uncalibrated axis never silently overrides the keyboard.
    bool hardware_throttle_active = false;
    bool hardware_throttle_disabled = false;
    double hardware_throttle_ref = -1.0;

    // A hardware lever must move by this much before it takes over, so that
    // axis noise on an idle stick cannot capture the throttle.
    constexpr double HARDWARE_THROTTLE_DEADBAND = 0.25;

    bool speedbrake_out = false; // Commanded speedbrake switch position
    bool wheel_braking = false;  // Toe brakes (keyboard)

    bool running = true;
    int frame_count = 0;
    FrameStats frame_stats;
    if (profile) renderer.gpu_profiler().set_enabled(true);
    auto prev_time = std::chrono::high_resolution_clock::now();
    double accumulator = 0.0;
    // Keeps the loop at most two frames ahead of the GPU (see FramePacer).
    graphics::FramePacer frame_pacer;

    while (running) {
        // Before input is read, so what is sampled here is what gets drawn.
        frame_pacer.wait();

        // Event handling
        const ui::ControlSettings& controls = settings.committed().controls;
        auto bound = [&controls](ui::InputAction a, SDL_Scancode sc) { return controls.binding(a).matches(static_cast<int32_t>(sc)); };

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_EVENT_QUIT) {
                running = false;
                continue;
            }
            if (ev.type == SDL_EVENT_WINDOW_RESIZED || ev.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
                int pw = 0, ph = 0;
                SDL_GetWindowSizeInPixels(window, &pw, &ph);
                if (pw > 0 && ph > 0) {
                    renderer.set_viewport(pw, ph);
                    pixel_w = pw;
                    pixel_h = ph;
                }
                continue;
            }
            if (ev.type == SDL_EVENT_GAMEPAD_ADDED && !gamepad) {
                gamepad = SDL_OpenGamepad(ev.gdevice.which);
                continue;
            }
            if (ev.type == SDL_EVENT_GAMEPAD_REMOVED && gamepad && SDL_GetGamepadID(gamepad) == ev.gdevice.which) {
                SDL_CloseGamepad(gamepad);
                gamepad = nullptr;
                continue;
            }

            // The main/pause menu owns all input while it is open.
            if (main_menu.is_open()) {
                if (auto menu_event = menu_input.translate(ev, SDL_GetWindowPixelDensity(window), main_menu.is_capturing_key())) {
                    main_menu.handle(*menu_event);
                }
                continue;
            }
            const bool pause_pressed =
                (ev.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN && ev.gbutton.button == SDL_GAMEPAD_BUTTON_START) ||
                (ev.type == SDL_EVENT_KEY_DOWN && ev.key.key == SDLK_ESCAPE && !ev.key.repeat && !renderer.menu().is_open());
            if (pause_pressed && !renderer.menu().is_open()) {
                main_menu.open(ui::ScreenId::MAIN, flight_started);
                continue;
            }

            if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                if (renderer.menu().is_open() && ev.button.button == SDL_BUTTON_LEFT) {
                    bool launched = renderer.menu().handle_click();
                    if (launched || renderer.menu().selected_index() != static_cast<int>(selected_aircraft)) {
                        switch_aircraft(renderer.menu().get_selected_type());
                    }
                    if (launched) {
                        state = reset_flight_state();
                        prev_state = state;
                        is_crashed = false;
                        throttle = start_airborne ? 0.65 : 0.08;
                        flight_started = true;
                        mission_from_main_menu = false;
                    }
                } else if (ev.button.button == SDL_BUTTON_RIGHT) {
                    right_mouse_down = true;
                } else if (ev.button.button == SDL_BUTTON_MIDDLE) {
                    renderer.camera_rig().reset_head_look();
                }
            } else if (ev.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                if (ev.button.button == SDL_BUTTON_RIGHT) {
                    right_mouse_down = false;
                }
            } else if (ev.type == SDL_EVENT_MOUSE_MOTION) {
                if (renderer.menu().is_open()) {
                    int win_w = 0, win_h = 0;
                    SDL_GetWindowSize(window, &win_w, &win_h);
                    const float aspect = (win_h > 0) ? static_cast<float>(win_w) / static_cast<float>(win_h) : 16.0f / 9.0f;
                    renderer.menu().update_mouse(static_cast<int>(ev.motion.x), static_cast<int>(ev.motion.y), win_w, win_h, aspect);
                } else if (right_mouse_down || mouse_look_toggle) {
                    // 0.0032 rad/px at 100% sensitivity: smooth pilot cervical rotation
                    constexpr float MOUSE_LOOK_RAD_PER_PX = 0.0032f;
                    const float gain = MOUSE_LOOK_RAD_PER_PX * static_cast<float>(controls.mouse_sensitivity) / 100.0f;
                    const float y_sign = controls.invert_mouse_y ? 1.0f : -1.0f;
                    renderer.camera_rig().add_head_look(
                        static_cast<float>(ev.motion.xrel) * gain,
                        static_cast<float>(ev.motion.yrel) * gain * y_sign
                    );
                }
            } else if (ev.type == SDL_EVENT_MOUSE_WHEEL) {
                renderer.camera_rig().zoom_chase(static_cast<float>(ev.wheel.y) * 1.5f);
            } else if (ev.type == SDL_EVENT_KEY_DOWN) {
                if (ev.key.key == SDLK_M) {
                    renderer.menu().toggle(selected_aircraft);
                    std::cout << "[MENU] Aircraft Selection Menu " << (renderer.menu().is_open() ? "OPEN" : "CLOSED") << "\n";
                }

                // If menu is open, handle menu navigation and airframe selection
                if (renderer.menu().is_open()) {
                    if (ev.key.key == SDLK_ESCAPE) {
                        renderer.menu().close();
                        if (mission_from_main_menu) {
                            mission_from_main_menu = false;
                            main_menu.open(ui::ScreenId::MAIN, flight_started);
                        }
                    } else if (ev.key.key == SDLK_TAB) {
                        renderer.menu().next_tab();
                    } else if (ev.key.key == SDLK_F1) {
                        renderer.menu().set_tab(fastjet::ui::MenuTab::AIRFRAME_SELECT);
                    } else if (ev.key.key == SDLK_F2) {
                        renderer.menu().set_tab(fastjet::ui::MenuTab::FLIGHT_ENVELOPE);
                    } else if (ev.key.key == SDLK_F3) {
                        renderer.menu().set_tab(fastjet::ui::MenuTab::HARDWARE_CALIBRATION);
                    } else if (ev.key.key == SDLK_F4) {
                        renderer.menu().set_tab(fastjet::ui::MenuTab::SORTIE_DISPATCH);
                    } else if (ev.key.key == SDLK_UP || ev.key.key == SDLK_W) {
                        renderer.menu().move_up();
                    } else if (ev.key.key == SDLK_DOWN || ev.key.key == SDLK_S) {
                        renderer.menu().move_down();
                    } else if (ev.key.key == SDLK_RETURN || ev.key.key == SDLK_KP_ENTER || ev.key.key == SDLK_SPACE) {
                        switch_aircraft(renderer.menu().get_selected_type());
                        state = reset_flight_state();
                        prev_state = state;
                        is_crashed = false;
                        throttle = start_airborne ? 0.65 : 0.08;
                        renderer.menu().close();
                        flight_started = true;
                        mission_from_main_menu = false;
                    } else if (ev.key.key == SDLK_1) {
                        switch_aircraft(aircraft::AircraftType::F16_FIGHTING_FALCON);
                        renderer.menu().select_index(0);
                    } else if (ev.key.key == SDLK_2) {
                        switch_aircraft(aircraft::AircraftType::F15EX_EAGLE_II);
                        renderer.menu().select_index(1);
                    } else if (ev.key.key == SDLK_3) {
                        switch_aircraft(aircraft::AircraftType::EUROFIGHTER_TYPHOON);
                        renderer.menu().select_index(2);
                    } else if (ev.key.key == SDLK_4) {
                        switch_aircraft(aircraft::AircraftType::F22_RAPTOR);
                        renderer.menu().select_index(3);
                    } else if (ev.key.key == SDLK_5) {
                        switch_aircraft(aircraft::AircraftType::A10_THUNDERBOLT);
                        renderer.menu().select_index(4);
                    }
                    continue;
                }

                if (ev.key.key == SDLK_F1) {
                    renderer.camera_rig().set_mode(fastjet::graphics::CameraMode::COCKPIT);
                    std::cout << "[CAMERA] Cockpit View (First-Person)\n";
                }
                if (ev.key.key == SDLK_F2) {
                    renderer.camera_rig().set_mode(fastjet::graphics::CameraMode::CHASE);
                    std::cout << "[CAMERA] Chase View (3D External Airframe)\n";
                }
                if (bound(ui::InputAction::TOGGLE_VIEW, ev.key.scancode) && !ev.key.repeat) {
                    renderer.camera_rig().toggle_mode();
                    std::cout << "[CAMERA] Toggled: "
                              << (renderer.camera_rig().mode() == fastjet::graphics::CameraMode::CHASE ? "Chase View" : "Cockpit View") << "\n";
                }
                if (bound(ui::InputAction::MOUSE_LOOK, ev.key.scancode) && !ev.key.repeat) {
                    mouse_look_toggle = !mouse_look_toggle;
                    SDL_SetWindowRelativeMouseMode(window, mouse_look_toggle);
                }
                if (bound(ui::InputAction::WHEEL_BRAKES, ev.key.scancode) && environment::GroundCollision::get_agl(state) > 20.0) {
                    renderer.camera_rig().reset_head_look();
                }
                if (bound(ui::InputAction::TRIM_RESET, ev.key.scancode)) input_mgr.trim_hat.reset();
                if (ev.key.key == SDLK_H && !ev.key.repeat) {
                    // Routed through settings so the hotkey and the Screen
                    // Shake option stay in sync and the choice persists.
                    settings.edit([](ui::UserSettings& u) { u.graphics.screen_shake = !u.graphics.screen_shake; });
                    settings.apply();
                    std::cout << "[CAMERA] Cockpit G-Head Motion: "
                              << (renderer.camera_rig().g_head_motion() ? "ENABLED" : "DISABLED (Fixed DEP Screen-Lock mode like MSFS/Real Jets)") << "\n";
                }
                if (bound(ui::InputAction::RESET_FLIGHT, ev.key.scancode) && !ev.key.repeat) {
                    // Reset from crash
                    state = reset_flight_state();
                    prev_state = state;
                    is_crashed = false;
                    engine.reset();
                    landing_gear.reset(selected_aircraft);
                    ofc.reset();
                    flight_control_system.reset();
                    input_mgr.trim_hat.reset();
                    speedbrake_out = false;
                    wheel_braking = false;
                    input_mgr.speedbrake.position = 0.0;
                    gear_down = true;
                    mass = fdm::FuelSystem::compute(engine.fuel_kg, selected_aircraft);
                    target_pitch = 0.0;
                    target_roll  = 0.0;
                    target_yaw   = 0.0;
                    stick_pitch  = 0.0;
                    stick_roll   = 0.0;
                    pedal_yaw    = 0.0;
                    throttle     = start_airborne ? 0.65 : 0.08;
                    current_forces = fdm::AircraftForces{};
                    current_imu = flcs::IMUData{};
                    current_imu.Nz = 1.0;
                    renderer.camera_rig().reset();
                    std::cout << "[SIM] Flight state reset ("
                              << (start_airborne ? "level flight" : "runway lineup") << ") ("
                              << aircraft::to_string(selected_aircraft) << ").\n";
                }
                if (bound(ui::InputAction::SPEEDBRAKE, ev.key.scancode) && !ev.key.repeat) {
                    // Toggle the switch; the surface itself slews at the
                    // modelled hydraulic rate rather than snapping open.
                    speedbrake_out = !speedbrake_out;
                    std::cout << "[SPEEDBRAKE] " << (speedbrake_out ? "EXTEND" : "RETRACT") << "\n";
                }
                if (bound(ui::InputAction::LANDING_GEAR, ev.key.scancode) && !ev.key.repeat) {
                    gear_down = !gear_down;
                    landing_gear.deployed = gear_down;
                    std::cout << "[GEAR] " << (gear_down ? "DOWN and locked" : "UP")
                              << "\n";
                }

                // Aircraft selection hotkeys
                if (ev.key.key == SDLK_F3) switch_aircraft(aircraft::AircraftType::F16_FIGHTING_FALCON);
                if (ev.key.key == SDLK_F4) switch_aircraft(aircraft::AircraftType::F15EX_EAGLE_II);
                if (ev.key.key == SDLK_F5) switch_aircraft(aircraft::AircraftType::EUROFIGHTER_TYPHOON);
                if (ev.key.key == SDLK_F6) switch_aircraft(aircraft::AircraftType::F22_RAPTOR);
                if (ev.key.key == SDLK_F7) switch_aircraft(aircraft::AircraftType::A10_THUNDERBOLT);
                if (ev.key.key == SDLK_F8) {
                    const int next_idx = (static_cast<int>(selected_aircraft) + 1) % 5;
                    switch_aircraft(static_cast<aircraft::AircraftType>(next_idx));
                }
                // Quick throttle positions, so the pilot can slam to a detent
                // without holding a key. These override a hardware lever only
                // until it is next moved.
                if (ev.key.key == SDLK_1) { throttle = 0.0;  hardware_throttle_active = false; }
                if (ev.key.key == SDLK_2) { throttle = 0.08; hardware_throttle_active = false; }
                if (ev.key.key == SDLK_3) { throttle = 0.85; hardware_throttle_active = false; }
                if (ev.key.key == SDLK_4) { throttle = 1.0;  hardware_throttle_active = false; }

                // Invert throttle axis hotkey (for joysticks with inverted sliders)
                if (ev.key.key == SDLK_I) {
                    input_mgr.config.throttle_cal.inverted = !input_mgr.config.throttle_cal.inverted;
                    std::cout << "[THROTTLE] Axis Inversion: "
                              << (input_mgr.config.throttle_cal.inverted ? "INVERTED (forward = max power)"
                                                                         : "NORMAL (forward = cutoff)")
                              << "\n";
                    // Reset latching reference so change takes effect smoothly
                    hardware_throttle_ref = -1.0;
                }

                // Hard lockout for a noisy or uncalibrated hardware lever, so
                // the keyboard throttle can always be recovered in flight.
                if (ev.key.key == SDLK_J) {
                    hardware_throttle_disabled = !hardware_throttle_disabled;
                    if (hardware_throttle_disabled) {
                        hardware_throttle_active = false;
                    }
                    hardware_throttle_ref = -1.0;
                    std::cout << "[THROTTLE] Hardware lever "
                              << (hardware_throttle_disabled ? "IGNORED - keyboard only"
                                                             : "enabled")
                              << "\n";
                }
            }
        }

        // Requests raised by the main/pause menu
        while (const auto action = main_menu.poll_action()) {
            switch (*action) {
                case ui::MenuAction::START_FLIGHT:
                    main_menu.close();
                    flight_started = true;
                    break;
                case ui::MenuAction::RESUME_FLIGHT:
                    main_menu.close();
                    break;
                case ui::MenuAction::OPEN_MISSION_SELECT:
                    main_menu.close();
                    if (flight_started) renderer.menu().open(selected_aircraft);
                    else renderer.menu().open_as_dispatch(selected_aircraft);
                    mission_from_main_menu = true;
                    break;
                case ui::MenuAction::QUIT:
                    running = false;
                    break;
            }
        }

        // Poll keyboard state for inceptors
        const bool* keys = SDL_GetKeyboardState(nullptr);
        const bool any_menu_open = renderer.menu().is_open() || main_menu.is_open();
        auto held = [&](ui::InputAction a) { return ui::sdl::is_down(keys, controls.binding(a)); };
        if (any_menu_open) {
            throttle_dir = 0.0; // A lever held while opening a menu must not keep moving
        }
        if (keys && !is_crashed && !any_menu_open) {
            target_pitch = 0.0;
            target_roll  = 0.0;
            target_yaw   = 0.0;

            if (held(ui::InputAction::PITCH_DOWN))   target_pitch -= 0.75; // Nose down
            if (held(ui::InputAction::PITCH_UP))     target_pitch += 0.90; // Nose up (pull Gs)
            if (held(ui::InputAction::ROLL_LEFT))    target_roll  -= 0.85; // Roll left
            if (held(ui::InputAction::ROLL_RIGHT))   target_roll  += 0.85; // Roll right
            if (held(ui::InputAction::RUDDER_LEFT))  target_yaw   -= 0.70; // Rudder left
            if (held(ui::InputAction::RUDDER_RIGHT)) target_yaw   += 0.70; // Rudder right

            // Throttle lever direction. The lever itself is moved below, once
            // frame_dt is known, so the slew rate is in units per second rather
            // than per rendered frame.
            throttle_dir = 0.0;
            if (held(ui::InputAction::THROTTLE_UP))   throttle_dir += 1.0;
            if (held(ui::InputAction::THROTTLE_DOWN)) throttle_dir -= 1.0;

            // Wheel brakes for the rollout.
            wheel_braking = held(ui::InputAction::WHEEL_BRAKES);
        }

        // Compute delta time
        auto cur_time = std::chrono::high_resolution_clock::now();
        double frame_dt = std::chrono::duration<double>(cur_time - prev_time).count();
        prev_time = cur_time;
        if (frame_dt > 0.1) frame_dt = 0.1; // Clamp max frame time

        // Pause simulation physics while tactical menu is open
        const bool menu_open = renderer.menu().is_open() || main_menu.is_open();
        if (!menu_open) {
            accumulator += frame_dt;
        } else {
            accumulator = 0.0;
        }

        // Move the throttle lever at a fixed rate per second. Doing this here
        // (rather than in the key poll above) keeps the response identical at
        // 60 FPS and at 8,000 FPS.
        if (throttle_dir != 0.0) {
            throttle = std::clamp(
                throttle + throttle_dir * THROTTLE_SLEW_PER_SEC * frame_dt, 0.0, 1.0);
        }

        // 200 Hz fixed-step physics integration
        using ProfileClock = std::chrono::steady_clock;
        const auto t_physics = ProfileClock::now();
        constexpr double SIM_DT = 0.005;
        while (accumulator >= SIM_DT) {
            prev_state = state;
            if (!is_crashed) {
                flcs::PilotCommands pilot_cmd = input_mgr.update(SIM_DT);

                // Latch onto the hardware throttle the first time it moves.
                // Latch onto a hardware throttle only on a large, deliberate
                // movement. A resting axis dithers by a few percent (and an
                // uncalibrated one can sit anywhere), so a small threshold
                // would hand control to a lever the pilot is not touching and
                // make the keyboard throttle appear dead.
                if (joy_driver->get_device_count() > 0 && !hardware_throttle_active &&
                    !hardware_throttle_disabled) {
                    if (hardware_throttle_ref < 0.0) {
                        hardware_throttle_ref = input_mgr.throttle_in;
                    } else if (std::abs(input_mgr.throttle_in - hardware_throttle_ref) >
                               HARDWARE_THROTTLE_DEADBAND) {
                        hardware_throttle_active = true;
                        std::cout << "[THROTTLE] Hardware lever moved - now driving the engine"
                                     " (press J to return to keyboard).\n";
                    }
                }

                // Smooth keyboard inceptor transitions (tau ~ 0.08s for responsive yet smooth feel)
                const double filter_alpha = 1.0 - std::exp(-SIM_DT / 0.08);
                stick_pitch += (target_pitch - stick_pitch) * filter_alpha;
                stick_roll  += (target_roll  - stick_roll)  * filter_alpha;
                pedal_yaw   += (target_yaw   - pedal_yaw)   * filter_alpha;

                // Blend hardware joystick with keyboard controls
                if (std::abs(stick_pitch) > 0.01) pilot_cmd.pitch_stick = stick_pitch;
                if (std::abs(stick_roll)  > 0.01) pilot_cmd.roll_stick  = stick_roll;
                if (std::abs(pedal_yaw)   > 0.01) pilot_cmd.rudder_pedal = pedal_yaw;

                // Pilot preferences: stick gain and pitch inversion apply to
                // the blended command, so keyboard and hardware behave alike.
                const double stick_gain = static_cast<double>(controls.stick_sensitivity) / 100.0;
                pilot_cmd.pitch_stick = std::clamp(pilot_cmd.pitch_stick * stick_gain, -1.0, 1.0);
                pilot_cmd.roll_stick  = std::clamp(pilot_cmd.roll_stick * stick_gain, -1.0, 1.0);
                if (controls.invert_pitch) pilot_cmd.pitch_stick = -pilot_cmd.pitch_stick;

                // Air data
                const auto air = environment::Atmosphere1976::compute(state.altitude(), state.airspeed());

                // IMU measurement
                current_imu = flcs::IMUData::read(state, current_forces, mass, air.dynamic_pressure);

                // OFC intercept: overrides pilot_cmd if GLOC or Auto-GCAS is active
                const flcs::PilotCommands effective_cmd =
                    ofc.update(SIM_DT, state, current_imu, pilot_cmd, landing_gear.deployed);

                // FLCS update (uses OFC-augmented commands, not raw pilot input)
                auto surfaces = flight_control_system.update(
                    SIM_DT, state, effective_cmd, air.dynamic_pressure, current_forces, mass
                );

                // Drive the speedbrake actuator toward the commanded switch
                // position at its hydraulic slew rate (2.0 s full travel),
                // then feed the resulting position to the aerodynamic model.
                input_mgr.speedbrake.update(
                    speedbrake_out ? input::SpeedbrakeController::SwitchPosition::EXTEND
                                   : input::SpeedbrakeController::SwitchPosition::RETRACT,
                    SIM_DT);
                surfaces.speedbrake = input_mgr.speedbrake.position;

                // Engine: detent schedule + altitude/Mach lapse + spool lag + fuel burn.
                // Prefer a physical throttle lever, but only once it has actually
                // been moved off its resting position. An unmoved or uncalibrated
                // axis would otherwise pin the engine at idle and make the
                // keyboard throttle appear dead.
                double active_throttle = throttle;
                if (hardware_throttle_active) {
                    active_throttle = input_mgr.throttle_in;
                }
                // Auto-GCAS throttle authority: idle while still banking into
                // wings-level, full power once actively pulling out. Overrides
                // whatever lever position the pilot (or hardware throttle) commanded.
                if (ofc.has_throttle_override()) {
                    active_throttle = ofc.throttle_override_value();
                }
                engine.tvc_pitch_cmd = flight_control_system.tvc_pitch_cmd;
                const double thrust_n = engine.update(active_throttle, air, SIM_DT);

                // Burning fuel changes gross mass and inertia.
                mass = fdm::FuelSystem::compute(engine.fuel_kg, selected_aircraft);

                // Ground steering and braking from the rudder pedals / toe brakes.
                landing_gear.steer_cmd = pilot_cmd.rudder_pedal;
                // Toe brakes: hardware pedals if present, otherwise the
                // keyboard brake key applies both mains evenly.
                const double kbd_brake = wheel_braking ? 1.0 : 0.0;
                landing_gear.brake_left =
                    (std::max)(input_mgr.brakes.effective_left(), kbd_brake);
                landing_gear.brake_right =
                    (std::max)(input_mgr.brakes.effective_right(), kbd_brake);

                // Update environmental wind & continuous Dryden turbulence with true AGL
                const double current_agl = environment::GroundCollision::get_agl(state);
                wind_model.update(SIM_DT, current_agl, state.airspeed());

                // RK4 step. Forces are re-evaluated at every stage from that
                // stage's state with dynamic atmospheric wind and turbulence.
                integrator.step(state, sim_time, mass,
                    [&](double, const fdm::FlightState& s) noexcept -> fdm::AircraftForces {
                        const double s_agl = environment::GroundCollision::get_agl(s);
                        const math::Vector3 v_rel = wind_model.compute_relative_velocity(s.vel_b, s_agl, s.q_att);
                        const double v_rel_norm = (std::max)(1.0, v_rel.norm());

                        const auto stage_air = environment::Atmosphere1976::compute(
                            s.altitude(), v_rel_norm);

                        fdm::FlightState s_aero = s;
                        s_aero.vel_b = v_rel;

                        fdm::AircraftForces f = aero_model.compute_forces_and_moments(
                            s_aero, surfaces, stage_air.dynamic_pressure, stage_air.mach_number);

                        // Thrust acts along the body X axis, and engine pitch moment (A-10 nacelle / F-22 TVC)
                        f.force_b.x += thrust_n;
                        f.moment_b.y += engine.pitch_moment();

                        // Ground reaction from the landing gear struts.
                        const auto gear_f = landing_gear.compute(s, mass);
                        f.force_b = f.force_b + gear_f.force_b;
                        f.moment_b = f.moment_b + gear_f.moment_b;

                        return f;
                    });

                // Record the forces acting at the new state for the IMU/HUD.
                const double state_agl = environment::GroundCollision::get_agl(state);
                const math::Vector3 v_rel_curr = wind_model.compute_relative_velocity(state.vel_b, state_agl, state.q_att);
                fdm::FlightState state_aero = state;
                state_aero.vel_b = v_rel_curr;
                current_forces = aero_model.compute_forces_and_moments(
                    state_aero, surfaces, air.dynamic_pressure, air.mach_number);
                current_forces.force_b.x += thrust_n;
                current_forces.moment_b.y += engine.pitch_moment();

                // =========================================================================
                // Ground Contact & Structural Integrity (Full-Terrain Collision)
                // =========================================================================
                if (!is_crashed) {
                    const auto col_res = environment::GroundCollision::check_collision(
                        state, selected_aircraft, landing_gear.deployed, landing_gear.collapsed);
                    if (col_res.has_collided) {
                        is_crashed = true;
                    }
                }

                if (is_crashed) {
                    environment::GroundCollision::clamp_to_surface(state);
                    current_imu.Nz = 0.0;
                }
            }

            sim_time += SIM_DT;
            accumulator -= SIM_DT;
        }

        const auto t_physics_end = ProfileClock::now();

        // =========================================================================
        // Build Real-Time Cockpit Avionics & Propulsion Telemetry
        // All flight data is displayed inside the cockpit on the MFDs & HUD.
        // =========================================================================
        // Retire the airframe change banner in wall-clock time, so it holds for
        // the same duration regardless of frame rate.
        if (aircraft_switch_timer > 0.0) {
            aircraft_switch_timer = (std::max)(0.0, aircraft_switch_timer - frame_dt);
        }

        graphics::AvionicsTelemetry tel{};
        tel.aircraft_type  = selected_aircraft;
        tel.aircraft_switch_timer = aircraft_switch_timer;
        const double shown_throttle = hardware_throttle_active ? input_mgr.throttle_in : throttle;
        tel.throttle_input = shown_throttle;
        tel.net_thrust_n   = engine.net_thrust_n;

        // Propulsion spool rotor speed & core temperatures
        tel.engine_rpm_pct = 65.0 + 35.0 * std::clamp(shown_throttle, 0.0, 1.0)
                           + (shown_throttle > 0.85 ? 4.0 * (shown_throttle - 0.85) / 0.15 : 0.0);
        tel.engine_ftit_deg_c = 420.0 + 380.0 * std::clamp(shown_throttle, 0.0, 0.85) / 0.85
                              + (shown_throttle > 0.85 ? 180.0 * (shown_throttle - 0.85) / 0.15 : 0.0);
        tel.nozzle_pos_pct = (shown_throttle > 0.80) ? 90.0 * (shown_throttle - 0.80) / 0.20 : 12.0;
        tel.oil_pressure_psi = 45.0 + 8.0 * (tel.engine_rpm_pct / 100.0);
        tel.hyd_press_a_psi  = 3000.0;
        tel.hyd_press_b_psi  = 3000.0;

        switch (engine.detent_state) {
            case input::ThrottleController::DetentState::CUTOFF:      tel.detent_str = "CUTOFF"; break;
            case input::ThrottleController::DetentState::IDLE:        tel.detent_str = "IDLE"; break;
            case input::ThrottleController::DetentState::MIL_POWER:   tel.detent_str = "MIL POWER"; break;
            case input::ThrottleController::DetentState::AFTERBURNER: tel.detent_str = "AFTERBURNER"; break;
        }

        tel.fuel_remaining_kg = engine.fuel_kg;
        tel.fuel_fraction     = engine.fuel_fraction();
        tel.fuel_flow_kg_hr   = (shown_throttle > 0.85)
            ? (3200.0 + 8500.0 * (shown_throttle - 0.85) / 0.15)
            : (800.0 + 1600.0 * shown_throttle);

        tel.gear_deployed     = landing_gear.deployed;
        tel.gear_collapsed    = landing_gear.collapsed;
        tel.gear_transit_pos  = landing_gear.deployed ? 1.0 : 0.0;
        tel.brake_left        = landing_gear.brake_left;
        tel.brake_right       = landing_gear.brake_right;
        tel.on_ground         = landing_gear.weight_on_wheels() || (environment::GroundCollision::get_agl(state) <= 2.2);

        tel.speedbrake_pos    = input_mgr.speedbrake.position;
        tel.is_crashed        = is_crashed;
        tel.over_g_alert      = (std::abs(current_imu.Nz) > 8.5);
        tel.high_aoa_alert    = (state.alpha() * (180.0 / M_PI) > 20.0);
        tel.bingo_fuel_alert  = (engine.fuel_kg < 800.0);
        tel.master_caution    = is_crashed || landing_gear.collapsed || tel.bingo_fuel_alert || tel.over_g_alert;

        tel.is_hardware_hotas = hardware_throttle_active || (joy_driver->get_device_count() > 0);
        tel.input_name        = tel.is_hardware_hotas ? "HOTAS / JOYSTICK" : "KEYBOARD [SLEW]";

        // On-Board Flight Computer telemetry
        tel.ofc_gloc_active        = ofc.is_gloc_active();
        tel.ofc_gcas_active        = ofc.is_gcas_active();
        tel.ofc_tumble_active      = ofc.is_tumble_active();
        tel.ofc_gloc_blackout_frac = ofc.blackout_fraction_value();
        tel.ofc_gcas_tti_sec       = ofc.gcas_tti_seconds();
        tel.ofc_g_exposure         = ofc.g_exposure_value();

        // Update procedural audio engine with real-time acoustics
        const auto air_now = environment::Atmosphere1976::compute(state.altitude(), state.airspeed());
        audio_engine.update(tel, air_now.dynamic_pressure, state.airspeed(), frame_dt);

        // Render Frame with in-cockpit telemetry and live hardware calibration state
        // Interpolate state across sub-steps to eliminate temporal aliasing and micro-stutter at any display refresh rate
        const double render_alpha = std::clamp(accumulator / SIM_DT, 0.0, 1.0);
        const fdm::FlightState render_state = is_crashed ? state : fdm::FlightState::interpolate(prev_state, state, render_alpha);
        const auto t_render = ProfileClock::now();
        renderer.render_frame(frame_dt, render_state, current_imu, is_crashed, tel, &input_mgr.config, &input_mgr);
        const auto t_render_end = ProfileClock::now();

        // Main / pause menu composited over the (paused) scene
        if (main_menu.is_visible()) {
            main_menu.set_status_text(upper(std::string(aircraft::to_string(selected_aircraft))) +
                                      (flight_started ? "  /  FLIGHT PAUSED" : "  /  RUNWAY 09 LINE-UP"));
        }
        main_menu.update(static_cast<float>(frame_dt), static_cast<float>(pixel_w), static_cast<float>(pixel_h));
        if (main_menu.is_visible()) {
            ui_draw_list.clear();
            main_menu.render(ui_draw_list);
            renderer.render_ui(ui_draw_list);
        }

        if (!screenshot_path.empty() && max_frames > 0 && frame_count + 1 >= max_frames) {
            std::vector<uint8_t> pixels(pixel_w * pixel_h * 4);
            glReadPixels(0, 0, pixel_w, pixel_h, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
            std::ofstream out(screenshot_path, std::ios::binary);
            if (out) {
                out << "P6\n" << pixel_w << " " << pixel_h << "\n255\n";
                for (int y = pixel_h - 1; y >= 0; --y) {
                    for (int x = 0; x < pixel_w; ++x) {
                        int idx = (y * pixel_w + x) * 4;
                        out.put(pixels[idx + 0]);
                        out.put(pixels[idx + 1]);
                        out.put(pixels[idx + 2]);
                    }
                }
            }
        }

        const auto t_present = ProfileClock::now();
        SDL_GL_SwapWindow(window);
        frame_pacer.frame_submitted();
        if (!headless) frame_limiter.wait(ui::frame_rate_cap_hz(settings.committed().graphics.frame_rate_cap));

        if (profile) {
            auto ms = [](ProfileClock::time_point a, ProfileClock::time_point b) {
                return std::chrono::duration<double, std::milli>(b - a).count();
            };
            frame_stats.add(frame_dt * 1000.0, ms(t_physics, t_physics_end), ms(t_render, t_render_end),
                            ms(t_present, ProfileClock::now()));
            if (frame_stats.elapsed_s >= 2.0) {
                std::cout << frame_stats.report() << renderer.gpu_profiler().report() << std::flush;
            }
        }

        ++frame_count;
        if (max_frames > 0 && frame_count >= max_frames) {
            running = false;
        }
    }

    std::cout << "Simulation finished. Total frames rendered: " << frame_count << "\n";

    if (gamepad) SDL_CloseGamepad(gamepad);
    frame_pacer.destroy();
    renderer.destroy();
    audio_engine.destroy();
    SDL_GL_DestroyContext(gl_ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
