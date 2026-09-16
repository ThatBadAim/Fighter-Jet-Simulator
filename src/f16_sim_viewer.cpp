#include <SDL3/SDL.h>
#include <epoxy/gl.h>
#include <iostream>
#include <chrono>

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
#include "fastjet/input/input_manager.hpp"
#include "fastjet/propulsion/multi_engine.hpp"
#include "fastjet/fdm/fuel_system.hpp"
#include "fastjet/gear/landing_gear.hpp"

#define CGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "fastjet/graphics/render_engine.hpp"
#include "fastjet/graphics/cockpit_telemetry.hpp"
#include "fastjet/audio/audio_engine.hpp"

using namespace fastjet;

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
    std::cout << "  T / R / Esc:            Trim reset / Reset simulation / Exit\n";
    std::cout << "---------------------------------------------------------\n";

    bool headless = false;
    int max_frames = -1;
    int win_w = 1920;
    int win_h = 1080;
    bool fullscreen = false;
    aircraft::AircraftType selected_aircraft = aircraft::AircraftType::F16_FIGHTING_FALCON;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--headless") headless = true;
        if (arg == "--frames" && i + 1 < argc) max_frames = std::atoi(argv[++i]);
        if (arg == "--width" && i + 1 < argc) win_w = std::atoi(argv[++i]);
        if (arg == "--height" && i + 1 < argc) win_h = std::atoi(argv[++i]);
        if (arg == "--fullscreen") fullscreen = true;
        if (arg == "--aircraft" && i + 1 < argc) {
            selected_aircraft = aircraft::parse_aircraft_type(argv[++i]);
        }
    }

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    // Enable 8x Multisampling (MSAA) for pristine anti-aliased HUD edges and vector geometry
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 8);

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

    // Pilot head tracking and mouse-look state
    bool right_mouse_down = false;
    bool mouse_look_toggle = false;

    // Initial trim state: Level cruise over runway at 1,500 m altitude, 220 m/s
    auto reset_flight_state = [&]() -> fdm::FlightState {
        fdm::FlightState s{};
        s.pos_ned = math::Vector3(0.0, 0.0, -1500.0); // 1,500 m altitude
        s.vel_b   = math::Vector3(220.0, 0.0, 0.0);
        s.omega_b = math::Vector3::zero();
        s.q_att   = math::Quaternion::identity();
        return s;
    };

    // How long the HUD holds the airframe change confirmation banner.
    constexpr double AIRCRAFT_SWITCH_BANNER_SEC = 2.5;
    double aircraft_switch_timer = 0.0;

    auto switch_aircraft = [&](aircraft::AircraftType new_type) {
        // Re-selecting the active airframe would otherwise reset every actuator
        // and re-seat the gear for no visible reason.
        if (new_type == selected_aircraft) {
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
        landing_gear.configure(new_type);
        mass = fdm::FuelSystem::compute(engine.fuel_kg, new_type);

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

    std::cout << "[SIM] Press 'M' to open Aircraft Selection Menu anytime during flight.\n";

    fdm::FlightState state = reset_flight_state();
    bool is_crashed = false;
    bool gear_down = true;

    double sim_time = 0.0;
    fdm::AircraftForces current_forces{};
    flcs::IMUData current_imu{};
    current_imu.Nz = 1.0;

    // Keyboard stick emulation state
    double target_pitch = 0.0;
    double target_roll  = 0.0;
    double target_yaw   = 0.0;
    double stick_pitch  = 0.0;
    double stick_roll   = 0.0;
    double pedal_yaw    = 0.0;
    double throttle     = 0.65; // Cruise dry thrust
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
    auto prev_time = std::chrono::high_resolution_clock::now();
    double accumulator = 0.0;

    while (running) {
        // Event handling
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (ev.type == SDL_EVENT_WINDOW_RESIZED || ev.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
                int pw = 0, ph = 0;
                SDL_GetWindowSizeInPixels(window, &pw, &ph);
                if (pw > 0 && ph > 0) {
                    renderer.set_viewport(pw, ph);
                }
            } else if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                if (ev.button.button == SDL_BUTTON_RIGHT) {
                    right_mouse_down = true;
                } else if (ev.button.button == SDL_BUTTON_MIDDLE) {
                    renderer.camera_rig().reset_head_look();
                }
            } else if (ev.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                if (ev.button.button == SDL_BUTTON_RIGHT) {
                    right_mouse_down = false;
                }
            } else if (ev.type == SDL_EVENT_MOUSE_MOTION) {
                if (right_mouse_down || mouse_look_toggle) {
                    // Sensitivity tuned for smooth pilot cervical rotation
                    renderer.camera_rig().add_head_look(
                        static_cast<float>(ev.motion.xrel) * 0.0032f,
                        static_cast<float>(-ev.motion.yrel) * 0.0032f
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
                    } else if (ev.key.key == SDLK_UP || ev.key.key == SDLK_W) {
                        renderer.menu().move_up();
                    } else if (ev.key.key == SDLK_DOWN || ev.key.key == SDLK_S) {
                        renderer.menu().move_down();
                    } else if (ev.key.key == SDLK_RETURN || ev.key.key == SDLK_KP_ENTER || ev.key.key == SDLK_SPACE) {
                        switch_aircraft(renderer.menu().get_selected_type());
                        renderer.menu().close();
                    } else if (ev.key.key == SDLK_1) {
                        switch_aircraft(aircraft::AircraftType::F16_FIGHTING_FALCON);
                        renderer.menu().close();
                    } else if (ev.key.key == SDLK_2) {
                        switch_aircraft(aircraft::AircraftType::F15EX_EAGLE_II);
                        renderer.menu().close();
                    } else if (ev.key.key == SDLK_3) {
                        switch_aircraft(aircraft::AircraftType::EUROFIGHTER_TYPHOON);
                        renderer.menu().close();
                    } else if (ev.key.key == SDLK_4) {
                        switch_aircraft(aircraft::AircraftType::F22_RAPTOR);
                        renderer.menu().close();
                    } else if (ev.key.key == SDLK_5) {
                        switch_aircraft(aircraft::AircraftType::A10_THUNDERBOLT);
                        renderer.menu().close();
                    }
                    continue;
                }

                if (ev.key.key == SDLK_ESCAPE) running = false;
                if (ev.key.key == SDLK_F1) {
                    renderer.camera_rig().set_mode(fastjet::graphics::CameraMode::COCKPIT);
                    std::cout << "[CAMERA] Cockpit View (First-Person)\n";
                }
                if (ev.key.key == SDLK_F2) {
                    renderer.camera_rig().set_mode(fastjet::graphics::CameraMode::CHASE);
                    std::cout << "[CAMERA] Chase View (3D External Airframe)\n";
                }
                if (ev.key.key == SDLK_V) {
                    renderer.camera_rig().toggle_mode();
                    std::cout << "[CAMERA] Toggled: "
                              << (renderer.camera_rig().mode() == fastjet::graphics::CameraMode::CHASE ? "Chase View" : "Cockpit View") << "\n";
                }
                if (ev.key.key == SDLK_TAB) {
                    mouse_look_toggle = !mouse_look_toggle;
                    SDL_SetWindowRelativeMouseMode(window, mouse_look_toggle);
                }
                if (ev.key.key == SDLK_SPACE && state.altitude() > 20.0) {
                    renderer.camera_rig().reset_head_look();
                }
                if (ev.key.key == SDLK_T) input_mgr.trim_hat.reset();
                if (ev.key.key == SDLK_R) {
                    // Reset from crash
                    state = reset_flight_state();
                    is_crashed = false;
                    engine.reset();
                    landing_gear.reset(selected_aircraft);
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
                    throttle     = 0.65;
                    current_forces = fdm::AircraftForces{};
                    current_imu = flcs::IMUData{};
                    current_imu.Nz = 1.0;
                    renderer.camera_rig().reset();
                    std::cout << "[SIM] Flight state reset to level flight ("
                              << aircraft::to_string(selected_aircraft) << ").\n";
                }
                if (ev.key.key == SDLK_B) {
                    // Toggle the switch; the surface itself slews at the
                    // modelled hydraulic rate rather than snapping open.
                    speedbrake_out = !speedbrake_out;
                    std::cout << "[SPEEDBRAKE] " << (speedbrake_out ? "EXTEND" : "RETRACT") << "\n";
                }
                if (ev.key.key == SDLK_G) {
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

        // Poll keyboard state for inceptors
        const bool* keys = SDL_GetKeyboardState(nullptr);
        if (keys && !is_crashed && !renderer.menu().is_open()) {
            target_pitch = 0.0;
            target_roll  = 0.0;
            target_yaw   = 0.0;

            if (keys[SDL_SCANCODE_UP])    target_pitch -= 0.75; // Nose down
            if (keys[SDL_SCANCODE_DOWN])  target_pitch += 0.90; // Nose up (pull Gs)
            if (keys[SDL_SCANCODE_LEFT])  target_roll  -= 0.85; // Roll left
            if (keys[SDL_SCANCODE_RIGHT]) target_roll  += 0.85; // Roll right
            if (keys[SDL_SCANCODE_A])     target_yaw   -= 0.70; // Rudder left
            if (keys[SDL_SCANCODE_D])     target_yaw   += 0.70; // Rudder right

            // Throttle lever direction. The lever itself is moved below, once
            // frame_dt is known, so the slew rate is in units per second rather
            // than per rendered frame.
            throttle_dir = 0.0;
            if (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT] ||
                keys[SDL_SCANCODE_EQUALS] || keys[SDL_SCANCODE_KP_PLUS] ||
                keys[SDL_SCANCODE_PAGEUP]) {
                throttle_dir += 1.0;
            }
            if (keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL] ||
                keys[SDL_SCANCODE_MINUS] || keys[SDL_SCANCODE_KP_MINUS] ||
                keys[SDL_SCANCODE_PAGEDOWN]) {
                throttle_dir -= 1.0;
            }

            // Wheel brakes for the rollout.
            wheel_braking = keys[SDL_SCANCODE_SPACE];
        }

        // Compute delta time
        auto cur_time = std::chrono::high_resolution_clock::now();
        double frame_dt = std::chrono::duration<double>(cur_time - prev_time).count();
        prev_time = cur_time;
        if (frame_dt > 0.1) frame_dt = 0.1; // Clamp max frame time
        accumulator += frame_dt;

        // Move the throttle lever at a fixed rate per second. Doing this here
        // (rather than in the key poll above) keeps the response identical at
        // 60 FPS and at 8,000 FPS.
        if (throttle_dir != 0.0) {
            throttle = std::clamp(
                throttle + throttle_dir * THROTTLE_SLEW_PER_SEC * frame_dt, 0.0, 1.0);
        }

        // 200 Hz fixed-step physics integration
        constexpr double SIM_DT = 0.005;
        while (accumulator >= SIM_DT) {
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

                // Air data
                const auto air = environment::Atmosphere1976::compute(state.altitude(), state.airspeed());

                // IMU measurement
                current_imu = flcs::IMUData::read(state, current_forces, mass, air.dynamic_pressure);

                // FLCS update
                auto surfaces = flight_control_system.update(
                    SIM_DT, state, pilot_cmd, air.dynamic_pressure, current_forces, mass
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

                // Update environmental wind & continuous Dryden turbulence
                wind_model.update(SIM_DT, state.altitude(), state.airspeed());

                // RK4 step. Forces are re-evaluated at every stage from that
                // stage's state with dynamic atmospheric wind and turbulence.
                integrator.step(state, sim_time, mass,
                    [&](double, const fdm::FlightState& s) noexcept -> fdm::AircraftForces {
                        const math::Vector3 v_rel = wind_model.compute_relative_velocity(s.vel_b, s.altitude(), s.q_att);
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
                const math::Vector3 v_rel_curr = wind_model.compute_relative_velocity(state.vel_b, state.altitude(), state.q_att);
                fdm::FlightState state_aero = state;
                state_aero.vel_b = v_rel_curr;
                current_forces = aero_model.compute_forces_and_moments(
                    state_aero, surfaces, air.dynamic_pressure, air.mach_number);
                current_forces.force_b.x += thrust_n;
                current_forces.moment_b.y += engine.pitch_moment();

                // =========================================================================
                // Ground Contact & Structural Integrity
                // =========================================================================
                if (landing_gear.collapsed && !is_crashed) {
                    is_crashed = true;
                }

                // Belly / wingtip strike: airframe striking ground
                constexpr double BELLY_CLEARANCE = 0.9;
                if (!is_crashed && state.pos_ned.z >= -BELLY_CLEARANCE) {
                    is_crashed = true;
                }

                if (is_crashed) {
                    state.pos_ned.z = -BELLY_CLEARANCE;
                    state.vel_b = math::Vector3::zero();
                    state.omega_b = math::Vector3::zero();
                    current_imu.Nz = 0.0;
                }
            }

            sim_time += SIM_DT;
            accumulator -= SIM_DT;
        }

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
        tel.on_ground         = (state.altitude() <= 2.2);

        tel.speedbrake_pos    = input_mgr.speedbrake.position;
        tel.is_crashed        = is_crashed;
        tel.over_g_alert      = (std::abs(current_imu.Nz) > 8.5);
        tel.high_aoa_alert    = (state.alpha() * (180.0 / M_PI) > 20.0);
        tel.bingo_fuel_alert  = (engine.fuel_kg < 800.0);
        tel.master_caution    = is_crashed || landing_gear.collapsed || tel.bingo_fuel_alert || tel.over_g_alert;

        tel.is_hardware_hotas = hardware_throttle_active || (joy_driver->get_device_count() > 0);
        tel.input_name        = tel.is_hardware_hotas ? "HOTAS / JOYSTICK" : "KEYBOARD [SLEW]";

        // Update procedural audio engine with real-time acoustics
        const auto air_now = environment::Atmosphere1976::compute(state.altitude(), state.airspeed());
        audio_engine.update(tel, air_now.dynamic_pressure, state.airspeed(), frame_dt);

        // Render Frame with in-cockpit telemetry
        renderer.render_frame(frame_dt, state, current_imu, is_crashed, tel);
        SDL_GL_SwapWindow(window);

        ++frame_count;
        if (max_frames > 0 && frame_count >= max_frames) {
            running = false;
        }
    }

    std::cout << "Simulation finished. Total frames rendered: " << frame_count << "\n";

    renderer.destroy();
    SDL_GL_DestroyContext(gl_ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
