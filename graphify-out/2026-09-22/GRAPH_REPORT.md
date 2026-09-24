# Graph Report - Fast Jet Simulator New Version  (2026-09-22)

## Corpus Check
- 123 files · ~289,330 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 3909 nodes · 8310 edges · 154 communities (145 shown, 9 thin omitted)
- Extraction: 95% EXTRACTED · 5% INFERRED · 0% AMBIGUOUS · INFERRED: 443 edges (avg confidence: 0.81)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `c96dfd53`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- Quaternion
- .step
- ControllerProfile
- TP1538Tables
- Flight
- .process_bipolar
- F110Engine
- F16AeroModel
- OnBoardFlightComputer
- VirtualHOTASDriver
- Actuator
- cgltf.h
- PitchController
- stb_image.h
- CameraRig
- InputManager
- ThrottleController
- AircraftMenu
- SpeedbrakeController
- SkyGroundRenderer
- SimRunner
- SettingsScreen
- test_ground_collision.cpp
- FastJetGLFunctions
- CockpitGeometry
- FlightInstruments
- HUDCollimator
- ModelGLB
- LandingGear
- Key Technical Specifications
- DrawCmd
- RenderEngine
- LevelRun
- TerrainMesh
- FlightState
- rules/graphify.md
- workflows/graphify.md
- string
- HdrPipeline
- .render
- MenuSystem
- AxisCalibration
- stbi__context
- Agent Guidelines & Rules
- test_hud_symbology_generation_gl
- .evaluate_force_curve
- cgltf_data
- cgltf_buffer_view
- AircraftAeroModel
- cgltf_size
- cgltf_material
- cgltf_primitive
- cgltf_node
- IMUData
- stbi__err
- cgltf_extras
- AvionicsTelemetry
- AudioEngine
- Color4
- array
- DrawList
- stbi__load_main
- cgltf_texture
- MultiEngine
- MenuGlyph
- CloudLayer
- PropulsionConfig
- stbi__zbuf
- MassProperties
- WindTurbulenceModel
- MenuServices
- Sim
- AtmosphereModel
- stbi__parse_png_file
- vector
- test_throttle_control.cpp
- Widget
- LayoutContext
- FlightSimHarness
- aircraft_type.hpp
- AeroConfig
- test_render_fidelity.cpp
- DropdownState
- SimHarness
- PerfResult
- Vector3
- Metrics
- GraphicsSettings
- InputEvent
- Palette
- UiCanvas
- test_settings_edit_apply_revert
- test_integrated_flight.cpp
- .get
- DigitalTrimHat
- Parser
- ShadowMap
- ControlSettings
- test_multi_aircraft_performance.cpp
- TerrainTextures
- Fast Jet Simulator - Authentic Audio Asset Directory
- ExhaustPlume
- ShaderProgram
- Value
- LateralDirectionalController
- Vertex3D
- .find
- .update
- ColorFilterPass
- .compute_coefficients
- main
- MenuVertex
- ModalSpec
- Writer
- SDL3InputDriver
- Motion
- cgltf_skin
- UserSettings
- theme.hpp
- string_view
- AircraftFLCS
- LoadResult
- Theme
- ButtonStyle
- RenderQuality
- AircraftConfig
- AeroCoefficients
- .destroy
- FlightStateDeriv
- test_user_settings.cpp
- Spacing
- Typography
- ScrollRegion
- Vertex
- cgltf_parse_json
- AccessibilitySettings
- WeatherState
- Radii
- test_ofc_closed_loop.cpp
- TerrainField
- .on_nav
- ClosedLoopSim
- Airframe
- CloudSettings
- AirData
- .height
- .compute
- .render
- Geometry
- Atmosphere1976
- test_atmosphere.cpp
- Geometry
- test_gcas_throttle_authority
- Ring

## God Nodes (most connected - your core abstractions)
1. `MenuSystem` - 145 edges
2. `cgltf_data` - 111 edges
3. `FastJetGLFunctions` - 98 edges
4. `AircraftMenu` - 89 edges
5. `OnBoardFlightComputer` - 79 edges
6. `cgltf_options` - 74 edges
7. `RenderEngine` - 74 edges
8. `SettingsScreen` - 72 edges
9. `FlightState` - 68 edges
10. `cgltf_material` - 64 edges

## Surprising Connections (you probably didn't know these)
- `find()` --calls--> `widgets_`  [INFERRED]
  tests/test_menu_navigation.cpp → include/fastjet/ui/menu_system.hpp
- `expect_saved()` --calls--> `to_short_string()`  [INFERRED]
  tests/test_ofc_closed_loop.cpp → include/fastjet/aircraft/aircraft_type.hpp
- `test_tumble_law_respects_airframe_alpha_envelope()` --calls--> `to_short_string()`  [INFERRED]
  tests/test_ofc_closed_loop.cpp → include/fastjet/aircraft/aircraft_type.hpp
- `main()` --calls--> `cgltf_free()`  [INFERRED]
  tests/test_model_glb.cpp → include/fastjet/graphics/cgltf.h
- `main()` --references--> `RenderEngine`  [INFERRED]
  src/f16_sim_viewer.cpp → include/fastjet/graphics/render_engine.hpp

## Import Cycles
- None detected.

## Communities (154 total, 9 thin omitted)

### Community 0 - "Quaternion"
Cohesion: 0.08
Nodes (11): Matrix3x3, m, EulerAngles, pitch, roll, yaw, Quaternion, w (+3 more)

### Community 1 - ".step"
Cohesion: 0.15
Nodes (18): main(), test_horizontal_ballistic_flight(), test_inclined_ballistic_flight_arbitrary_attitude(), test_steady_level_trim_flight(), main(), test_hands_off_level_flight(), main(), test_high_speed_9g_pull() (+10 more)

### Community 2 - "ControllerProfile"
Cohesion: 0.07
Nodes (27): ControllerProfile, btn_parking_brake, btn_speedbrake_extend, btn_speedbrake_retract, btn_trim_down, btn_trim_left, btn_trim_right, btn_trim_up (+19 more)

### Community 3 - "TP1538Tables"
Cohesion: 0.09
Nodes (31): array, size_t, idx3d(), init_CD_table(), init_CL_roll_table(), init_CL_table(), init_CM_table(), init_CN_yaw_table() (+23 more)

### Community 4 - "Flight"
Cohesion: 0.11
Nodes (16): F16FLCS, flaperon, lat_dir_ctrl, pitch_ctrl, rudder, stabilator, Flight, aero (+8 more)

### Community 5 - ".process_bipolar"
Cohesion: 0.36
Nodes (5): SignalConditioner, main(), test_axis_calibration_and_inversion(), test_deadband_filtering(), test_unipolar_axis_conditioning()

### Community 6 - "F110Engine"
Cohesion: 0.09
Nodes (18): F110Engine, commanded_thrust_n, fuel_flow_kgs, fuel_kg, INTERNAL_FUEL_CAPACITY_KG, JP8_DENSITY, K_RAM_AB, K_RAM_DRY (+10 more)

### Community 7 - "F16AeroModel"
Cohesion: 0.09
Nodes (22): F16AeroModel, B_SPAN, B_SPAN_FT, C_BAR, C_BAR_FT, CD_Q_RISE, CD_SPEEDBRAKE_FULL, CD_SUPERSONIC_RISE (+14 more)

### Community 8 - "OnBoardFlightComputer"
Cohesion: 0.03
Nodes (58): GcasState, GlocState, OnBoardFlightComputer, airframe, blackout_fraction, departure_rotation, g_exposure, GCAS_ALTITUDE_FLOOR_M (+50 more)

### Community 9 - "VirtualHOTASDriver"
Cohesion: 0.12
Nodes (9): array, size_t, VirtualHOTASDriver, axes_, buttons_, hats_, MAX_AXES, MAX_BUTTONS (+1 more)

### Community 10 - "Actuator"
Cohesion: 0.16
Nodes (13): Actuator, is_pos_limited, is_rate_limited, pos_max, pos_min, position, rate, rate_limit (+5 more)

### Community 11 - "cgltf.h"
Cohesion: 0.15
Nodes (81): cgltf_calloc(), cgltf_fill_float_array(), cgltf_json_strcmp(), cgltf_json_to_bool(), cgltf_json_to_component_type(), cgltf_json_to_float(), cgltf_json_to_int(), cgltf_json_to_primitive_type() (+73 more)

### Community 12 - "PitchController"
Cohesion: 0.12
Nodes (14): PitchController, ALPHA_LEAD_S, ALPHA_MAX_DEG, integrator, K_alpha, K_alpha_rate, Kd, Ki (+6 more)

### Community 13 - "stb_image.h"
Cohesion: 0.06
Nodes (88): FILE, load_jpeg_image(), resample_row_1(), stbi__bitcount(), stbi__blinn_8x8(), stbi__clamp(), stbi__compute_y(), stbi__compute_y_16() (+80 more)

### Community 14 - "CameraRig"
Cohesion: 0.06
Nodes (27): CameraMode, CameraRig, chase_distance_, chase_height_, chase_pitch_, chase_yaw_, DEP_X, DEP_Y (+19 more)

### Community 15 - "InputManager"
Cohesion: 0.10
Nodes (20): InputManager, brakes, config, driver, pitch_in, roll_in, speedbrake, throttle (+12 more)

### Community 16 - "ThrottleController"
Cohesion: 0.17
Nodes (12): DetentState, ThrottleController, ab_power, DETENT_CUTOFF, DETENT_IDLE, DETENT_MIL, dry_power, net_thrust_n (+4 more)

### Community 17 - "AircraftMenu"
Cohesion: 0.04
Nodes (47): AircraftMenu, active_tab_, col_accent_gold, col_active_badge, col_alert_red, col_bg_void, col_border_bright, col_border_dim (+39 more)

### Community 18 - "SpeedbrakeController"
Cohesion: 0.16
Nodes (9): SpeedbrakeController, position, slew_rate, WheelBrakes, left_brake, parking_brake, right_brake, SwitchPosition (+1 more)

### Community 19 - "SkyGroundRenderer"
Cohesion: 0.07
Nodes (34): array, GLsizei, GLuint, SkyGroundRenderer, ATMOSPHERE_GLSL, cirrus_tex_, ground_shader_, ground_textures_ (+26 more)

### Community 20 - "SimRunner"
Cohesion: 0.23
Nodes (13): main(), SimRunner, aero_model, current_forces, flcs, integrator, mass, sim_time (+5 more)

### Community 21 - "SettingsScreen"
Cohesion: 0.04
Nodes (59): Accessor, FieldSpec, dynamic_hint, enabled, get, hint, kind, label (+51 more)

### Community 22 - "test_ground_collision.cpp"
Cohesion: 0.15
Nodes (18): GroundContactPoint, CollisionResult, contact_point, contact_pos_ned, has_collided, penetration_depth, terrain_elev_m, GroundCollision (+10 more)

### Community 23 - "FastJetGLFunctions"
Cohesion: 0.02
Nodes (97): FastJetGLFunctions, activeTexture, attachShader, bindBuffer, bindFramebuffer, bindRenderbuffer, bindVertexArray, blendFuncSeparate (+89 more)

### Community 24 - "CockpitGeometry"
Cohesion: 0.07
Nodes (21): CockpitGeometry, glass_vao_, glass_vbo_, glass_vertex_count_, initialized_, mfd_vao_, mfd_vbo_, mfd_vertex_count_ (+13 more)

### Community 25 - "FlightInstruments"
Cohesion: 0.08
Nodes (23): FlightInstruments, fbo_, initialized_, line_verts_, rbo_, shader_, TEX_HEIGHT, TEX_WIDTH (+15 more)

### Community 26 - "HUDCollimator"
Cohesion: 0.11
Nodes (19): GLuint, vector, HUDCollimator, initialized_, lines_, masked_vertex_count_, peak_g_, shader_ (+11 more)

### Community 27 - "ModelGLB"
Cohesion: 0.05
Nodes (41): F16PartType, GLBSubmesh, base_color_factor, base_texture, bounds_max, bounds_min, ebo, emissive (+33 more)

### Community 28 - "LandingGear"
Cohesion: 0.07
Nodes (38): AircraftType, array, LandingGear, brake_left, brake_right, collapsed, CORNERING_STIFFNESS, CREEP_SPEED (+30 more)

### Community 29 - "Key Technical Specifications"
Cohesion: 0.09
Nodes (21): 10. First-Person Graphics, Collimated HUD & Cockpit Avionics, 1. Zero-Allocation Architecture, 2. State Vector & Coordinate Frames, 3. Core Equations of Motion, 4. F-16C Mass & Inertia Properties, 5. 1976 US Standard Atmosphere Model, 6. NASA TP-1538 Aerodynamics & Multidimensional Interpolation, 7. Digital Fly-By-Wire Flight Control System (FLCS) & Actuators (+13 more)

### Community 30 - "DrawCmd"
Cohesion: 0.11
Nodes (18): DrawCmdType, DrawCmd, blur, border, border_width, fill_bottom, fill_top, horizontal (+10 more)

### Community 31 - "RenderEngine"
Cohesion: 0.06
Nodes (33): ColorblindMode, RenderEngine, aircraft_menu_, caster_shader_, clouds_, cockpit_, cockpit_shader_, color_filter_ (+25 more)

### Community 32 - "LevelRun"
Cohesion: 0.16
Nodes (17): LevelRun, aero, alt_target, engine, flcs, forces, integrator, mass (+9 more)

### Community 33 - "TerrainMesh"
Cohesion: 0.11
Nodes (14): GLsizei, GLuint, vector, TerrainMesh, APRON_X0, APRON_X1, APRON_Y0, APRON_Y1 (+6 more)

### Community 34 - "FlightState"
Cohesion: 0.10
Nodes (10): FlightState, omega_b, pos_ned, q_att, vel_b, array, Mat4, m (+2 more)

### Community 37 - "string"
Cohesion: 0.43
Nodes (3): string, main(), test_json_serialization_and_file_persistence()

### Community 38 - "HdrPipeline"
Cohesion: 0.05
Nodes (31): HdrPipeline, composite_shader_, down_shader_, empty_vao_, initialized_, kMaxBloomLevels, kMinBloomSize, mips_ (+23 more)

### Community 40 - "MenuSystem"
Cohesion: 0.04
Nodes (44): array, optional, QualityPreset, SubscriptionId, MenuSystem, actions_, anims_, capture_ (+36 more)

### Community 41 - "AxisCalibration"
Cohesion: 0.22
Nodes (8): AxisCalibration, curvature, inner_deadband, inverted, outer_deadband, raw_center, raw_max, raw_min

### Community 42 - "stbi__context"
Cohesion: 0.11
Nodes (53): stbi__at_eof(), stbi__bmp_info(), stbi__bmp_parse_header(), stbi__bmp_set_mask_defaults(), stbi__bmp_test(), stbi__bmp_test_raw(), stbi__check_png_header(), stbi__copyval() (+45 more)

### Community 44 - "test_hud_symbology_generation_gl"
Cohesion: 0.20
Nodes (8): main(), test_dep_and_equilibrium(), test_g_load_dynamics(), main(), test_fpm_velocity_vector_tracking(), test_hud_conformal_roll_and_boresight_alignment(), test_hud_symbology_generation_gl(), test_optical_infinity_projection()

### Community 45 - ".evaluate_force_curve"
Cohesion: 0.67
Nodes (4): main(), test_curvature_tuning(), test_force_curve_endpoints_and_symmetry(), test_strict_monotonicity()

### Community 46 - "cgltf_data"
Cohesion: 0.03
Nodes (74): cgltf_camera_type, cgltf_file_type, cgltf_camera, data, extensions, extensions_count, extras, cgltf_camera_index() (+66 more)

### Community 47 - "cgltf_buffer_view"
Cohesion: 0.06
Nodes (37): cgltf_buffer_view_type, cgltf_data_free_method, cgltf_meshopt_compression_filter, cgltf_meshopt_compression_mode, cgltf_buffer, data, data_free_method, extensions (+29 more)

### Community 48 - "AircraftAeroModel"
Cohesion: 0.15
Nodes (10): AircraftAeroModel, aircraft_type, config, AircraftType, ControlSurfaces, delta_a, delta_e, delta_lef (+2 more)

### Community 49 - "cgltf_size"
Cohesion: 0.06
Nodes (65): cgltf_bool, cgltf_component_type, cgltf_result, cgltf_size, cgltf_ssize, cgltf_type, cgltf_uint, cgltf_accessor (+57 more)

### Community 50 - "cgltf_material"
Cohesion: 0.02
Nodes (108): cgltf_alpha_mode, cgltf_float, cgltf_anisotropy, anisotropy_rotation, anisotropy_strength, anisotropy_texture, cgltf_clearcoat, clearcoat_factor (+100 more)

### Community 51 - "cgltf_primitive"
Cohesion: 0.05
Nodes (41): cgltf_attribute_type, cgltf_int, cgltf_primitive_type, cgltf_attribute, data, index, name, type (+33 more)

### Community 52 - "cgltf_node"
Cohesion: 0.05
Nodes (39): cgltf_light_type, cgltf_light, color, extras, cgltf_light_index(), intensity, name, range (+31 more)

### Community 53 - "IMUData"
Cohesion: 0.09
Nodes (17): IMUData, airspeed, alpha_deg, altitude, beta_deg, Nx, Ny, Nz (+9 more)

### Community 54 - "stbi__err"
Cohesion: 0.16
Nodes (33): stbi__addints_valid(), stbi__build_fast_ac(), stbi__build_huffman(), stbi__cleanup_jpeg(), stbi__cpuid3(), stbi__decode_jpeg_header(), stbi__decode_jpeg_image(), stbi__err() (+25 more)

### Community 55 - "cgltf_extras"
Cohesion: 0.03
Nodes (72): cgltf_animation_path_type, cgltf_interpolation_type, cgltf_animation, cgltf_animation_channel, extensions, extensions_count, extras, cgltf_animation_channel_index() (+64 more)

### Community 56 - "AvionicsTelemetry"
Cohesion: 0.05
Nodes (38): AvionicsTelemetry, aircraft_switch_timer, aircraft_type, bingo_fuel_alert, brake_left, brake_right, detent_str, engine_ftit_deg_c (+30 more)

### Community 57 - "AudioEngine"
Cohesion: 0.06
Nodes (31): AudioEngine, alerts_bus_gain_, CHANNELS, device_id_, enabled_, engine_bus_gain_, initialized_, prev_gear_deployed_ (+23 more)

### Community 58 - "Color4"
Cohesion: 0.16
Nodes (5): Color4, a, b, g, r

### Community 59 - "array"
Cohesion: 0.15
Nodes (17): deque, find_index_and_weight(), array, N, size_t, interp1d(), interp2d(), interp3d() (+9 more)

### Community 60 - "DrawList"
Cohesion: 0.07
Nodes (33): baseline_for_center(), size, DrawList, cmds_, layers_, string, string_view, vector (+25 more)

### Community 61 - "stbi__load_main"
Cohesion: 0.22
Nodes (25): stbi__addsizes_valid(), stbi__bmp_load(), stbi__convert_format(), stbi__convert_format16(), stbi__do_png(), stbi__getn(), stbi__gif_load(), stbi__hdr_load() (+17 more)

### Community 62 - "cgltf_texture"
Cohesion: 0.06
Nodes (33): cgltf_filter_type, cgltf_wrap_mode, cgltf_image, buffer_view, extensions, extensions_count, extras, cgltf_image_index() (+25 more)

### Community 63 - "MultiEngine"
Cohesion: 0.12
Nodes (14): AircraftType, DetentState, MultiEngine, ab_power, aircraft_type, commanded_thrust_n, config, detent_state (+6 more)

### Community 64 - "MenuGlyph"
Cohesion: 0.15
Nodes (12): find_menu_glyph(), MenuGlyph, advance, bearing_x, bearing_y, code, h, u0 (+4 more)

### Community 65 - "CloudLayer"
Cohesion: 0.08
Nodes (23): CloudLayer, build_strip_, building_, front_, front_valid_, kExtent, kMapOctaves, kMapRecenter (+15 more)

### Community 66 - "PropulsionConfig"
Cohesion: 0.12
Nodes (17): PropulsionConfig, engine_count, has_afterburner, has_supercruise, has_thrust_vectoring, static_ab_thrust_n, static_dry_thrust_n, static_idle_thrust_n (+9 more)

### Community 67 - "stbi__zbuf"
Cohesion: 0.30
Nodes (18): stbi__bit_reverse(), stbi__bitreverse16(), stbi__compute_huffman_codes(), stbi__fill_bits(), stbi__parse_huffman_block(), stbi__parse_uncompressed_block(), stbi__parse_zlib(), stbi__parse_zlib_header() (+10 more)

### Community 68 - "MassProperties"
Cohesion: 0.15
Nodes (12): FuelSystem, EMPTY_MASS_KG, AircraftType, MassProperties, I, I_inv, inv_mass, Ixx (+4 more)

### Community 69 - "WindTurbulenceModel"
Cohesion: 0.18
Nodes (8): WindTurbulenceModel, base_wind_speed_mps, gust_u_, gust_v_, gust_w_, rng_state_, turbulence_enabled, wind_heading_rad

### Community 70 - "MenuServices"
Cohesion: 0.05
Nodes (33): WidgetId, MenuServices, begin_key_capture, emit, focused, in_flight, is_capturing, key_name (+25 more)

### Community 71 - "Sim"
Cohesion: 0.12
Nodes (15): AircraftForces, force_b, moment_b, Sim, aero, engine, flcs, forces (+7 more)

### Community 72 - "AtmosphereModel"
Cohesion: 0.09
Nodes (24): AtmosphereModel, EXPOSURE, MIE, MIE_G, MIN_MU, PI, RAYLEIGH, SCALE_H (+16 more)

### Community 73 - "stbi__parse_png_file"
Cohesion: 0.22
Nodes (16): stbi__compute_transparency(), stbi__compute_transparency16(), stbi__create_png_alpha_expand8(), stbi__create_png_image(), stbi__create_png_image_raw(), stbi__de_iphone(), stbi__expand_png_palette(), stbi__get32be() (+8 more)

### Community 74 - "vector"
Cohesion: 0.18
Nodes (4): string, vector, apply_texture_anisotropy(), max_texture_anisotropy()

### Community 75 - "test_throttle_control.cpp"
Cohesion: 0.29
Nodes (9): main(), test_afterburner_accelerates(), test_detent_boundaries(), test_resting_hardware_lever_does_not_capture_throttle(), test_speedbrake_adds_deceleration(), test_speedbrake_slews_not_snaps(), test_status_cadence_is_time_based(), test_throttle_back_decelerates() (+1 more)

### Community 76 - "Widget"
Cohesion: 0.07
Nodes (44): fit_text(), ButtonVariant, string, WidgetId, WidgetKind, is_adjustable(), LegendItem, action (+36 more)

### Community 77 - "LayoutContext"
Cohesion: 0.08
Nodes (13): LayoutContext, pointer_nx, pointer_ny, s, scroll_y, time, viewport, vector (+5 more)

### Community 78 - "FlightSimHarness"
Cohesion: 0.13
Nodes (14): RK4Integrator, DEFAULT_DT, dt, AircraftType, FlightSimHarness, aero, engine, flcs (+6 more)

### Community 80 - "AeroConfig"
Cohesion: 0.11
Nodes (19): AeroConfig, aspect_ratio, b_span, c_bar, cd0, cd_speedbrake, cd_wave_peak, cd_wave_super (+11 more)

### Community 81 - "test_render_fidelity.cpp"
Cohesion: 0.17
Nodes (20): camera_rig_, hdr_, shadow_map_, QualityPreset, apply(), array, vector, Frame (+12 more)

### Community 82 - "DropdownState"
Cohesion: 0.06
Nodes (33): CaptureState, anchor, error, on_clear, on_key, title, DropdownState, anchor (+25 more)

### Community 83 - "SimHarness"
Cohesion: 0.17
Nodes (12): AircraftType, SimHarness, aero, cfg, engine, flcs, forces, integrator (+4 more)

### Community 84 - "PerfResult"
Cohesion: 0.12
Nodes (17): string, PerfResult, ab_thrust_kn, alt_m, aoa_limit_deg, combat_mass_kg, dry_speed_alt_mach, dry_thrust_kn (+9 more)

### Community 85 - "Vector3"
Cohesion: 0.14
Nodes (9): BodyBounds, max, min, valid, array, Vector3, x, y (+1 more)

### Community 86 - "Metrics"
Cohesion: 0.06
Nodes (33): Metrics, accent_bar_w, chevron, control_w, dropdown_item_h, dropdown_max_visible, footer_button_h, footer_button_min_w (+25 more)

### Community 87 - "GraphicsSettings"
Cohesion: 0.10
Nodes (19): DisplayMode, FrameRateCap, apply_video(), string, vector, key_name(), query_resolutions(), video_differs() (+11 more)

### Community 88 - "InputEvent"
Cohesion: 0.11
Nodes (22): NavCommand, Type, InputEvent, nav, scancode, scroll, source, type (+14 more)

### Community 89 - "Palette"
Cohesion: 0.07
Nodes (30): Palette, accent, accent_strong, accent_strong_hover, backdrop, border_bright, border_strong, border_subtle (+22 more)

### Community 90 - "UiCanvas"
Cohesion: 0.11
Nodes (15): decode_menu_font_atlas(), vector, UiCanvas, clip_stack_, fb_h_, fb_w_, font_tex_, initialized_ (+7 more)

### Community 91 - "test_settings_edit_apply_revert"
Cohesion: 0.28
Nodes (18): toast_text_, pending_, click(), NavCommand, WidgetId, find(), focus_down_to(), frames() (+10 more)

### Community 92 - "test_integrated_flight.cpp"
Cohesion: 0.51
Nodes (7): main(), test_altitude_thrust_lapse_in_flight(), test_energy_sanity_in_cruise(), test_fuel_burn_changes_mass(), test_gear_survives_gentle_landing(), test_speedbrake_decelerates_in_flight(), test_takeoff_roll()

### Community 93 - ".get"
Cohesion: 0.32
Nodes (8): main(), test_a10_specifications(), test_f15ex_specifications(), test_f16c_specifications(), test_f22_specifications(), test_typhoon_specifications(), evaluate_aircraft(), main()

### Community 94 - "DigitalTrimHat"
Cohesion: 0.24
Nodes (7): DigitalTrimHat, slew_rate, trim_pitch, trim_roll, main(), test_digital_trim_hat(), test_throttle_detents()

### Community 95 - "Parser"
Cohesion: 0.24
Nodes (9): optional, string, string_view, parse(), Parser, error_, kMaxDepth, pos_ (+1 more)

### Community 96 - "ShadowMap"
Cohesion: 0.07
Nodes (19): FrameContext, clouds, eye_ned, ground_shadow, hdr_output, kShadowUnit, lighting, shadow (+11 more)

### Community 97 - "ControlSettings"
Cohesion: 0.10
Nodes (17): is_down(), ActionInfo, defaults, json_key, ControlSettings, bindings, invert_mouse_y, invert_pitch (+9 more)

### Community 98 - "test_multi_aircraft_performance.cpp"
Cohesion: 0.36
Nodes (12): to_short_string(), main(), test_a10_full_stick_is_lift_and_feel_limited(), test_a10_nacelle_pitch_coupling(), test_a10_subsonic_drag_barrier(), test_a10_thrust_falls_with_mach(), test_f15ex_high_speed_dash(), test_f22_thrust_vectoring() (+4 more)

### Community 99 - "TerrainTextures"
Cohesion: 0.13
Nodes (12): array, GLuint, TerrainTextures, albedo_, bake_shader_, initialized_, kAlbedoResolution, kAnisotropy (+4 more)

### Community 100 - "Fast Jet Simulator - Authentic Audio Asset Directory"
Cohesion: 0.50
Nodes (3): Fast Jet Simulator - Authentic Audio Asset Directory, Supported Sound Files, Where to get authentic sounds

### Community 101 - "ExhaustPlume"
Cohesion: 0.14
Nodes (11): ExhaustPlume, kNozzleRadius, kPlumeLength, kRings, kSegments, shader_, vao_, vbo_ (+3 more)

### Community 102 - "ShaderProgram"
Cohesion: 0.15
Nodes (6): GLint, GLenum, GLuint, ShaderProgram, program_id_, valid_

### Community 103 - "Value"
Cohesion: 0.11
Nodes (13): Array, Type, Value, data_, enum_field(), E, N, string (+5 more)

### Community 104 - "LateralDirectionalController"
Cohesion: 0.17
Nodes (9): LateralDirectionalController, ALPHA_ATTEN_MAX, ALPHA_ATTEN_MIN, K_beta, K_roll, K_yaw_damper, Ki_roll, P_CMD_MAX_DPS (+1 more)

### Community 105 - "Vertex3D"
Cohesion: 0.40
Nodes (5): Vertex3D, color, normal, pos, uv

### Community 106 - ".find"
Cohesion: 0.13
Nodes (9): SettingsSection, Transition, active, direction, phase_out, t, to, step (+1 more)

### Community 107 - ".update"
Cohesion: 0.19
Nodes (30): main(), test_gcas_avoids_mountain_ahead_in_level_flight(), test_gcas_commands_nominal_g_not_max_g(), test_gcas_does_not_disturb_normal_landing(), test_gcas_does_not_intervene_on_shallow_dive(), test_gcas_does_not_latch_when_airspeed_collapses(), test_gcas_elevated_terrain_dive(), test_gcas_engages_earlier_when_slow() (+22 more)

### Community 108 - "ColorFilterPass"
Cohesion: 0.13
Nodes (11): ColorFilterPass, fbo_, h_, initialized_, kFragmentSrc, kVertexSrc, shader_, tex_ (+3 more)

### Community 109 - ".compute_coefficients"
Cohesion: 0.25
Nodes (13): main(), test_baseline_lookups(), test_dimensional_force_moment_synthesis(), test_elevator_control_power(), test_relaxed_static_stability_derivative(), main(), make_state(), test_dynamic_pressure_drag() (+5 more)

### Community 110 - "main"
Cohesion: 0.08
Nodes (18): Fn, FrameLimiter, next_ns_, edit(), pair, SettingsSection, SubscriptionId, vector (+10 more)

### Community 111 - "MenuVertex"
Cohesion: 0.50
Nodes (4): MenuVertex, color, pos, uv

### Community 112 - "ModalSpec"
Cohesion: 0.11
Nodes (17): AppInfo, title, version, ButtonVariant, function, string, vector, ModalButton (+9 more)

### Community 113 - "Writer"
Cohesion: 0.24
Nodes (4): serialize(), Writer, indent_, out_

### Community 114 - "SDL3InputDriver"
Cohesion: 0.13
Nodes (8): string, vector, SDL3InputDriver, initialized_, joysticks_, SDL_Joystick, main(), test_sdl3_driver_initialization()

### Community 115 - "Motion"
Cohesion: 0.11
Nodes (18): Motion, background_drift, focus, menu_open, modal, modal_scale_from, parallax_far, parallax_near (+10 more)

### Community 116 - "cgltf_skin"
Cohesion: 0.20
Nodes (10): cgltf_skin, extensions, extensions_count, extras, cgltf_skin_index(), inverse_bind_matrices, joints, joints_count (+2 more)

### Community 117 - "UserSettings"
Cohesion: 0.12
Nodes (12): AudioSettings, alerts, engine, master, muted, SettingsSection, UserSettings, accessibility (+4 more)

### Community 118 - "theme.hpp"
Cohesion: 0.18
Nodes (14): apply_colorblind_palette(), build_theme(), ease_in_cubic(), Elevation, modal, panel, raised, ColorblindMode (+6 more)

### Community 119 - "string_view"
Cohesion: 0.53
Nodes (6): engine_designation(), AircraftType, string_view, parse_aircraft_type(), signature_spec(), to_string()

### Community 120 - "AircraftFLCS"
Cohesion: 0.08
Nodes (23): FLCSConfig, alpha_limit_deg, law_type, max_g_negative, max_g_positive, max_roll_rate_dps, AircraftFLCS, aileron (+15 more)

### Community 121 - "LoadResult"
Cohesion: 0.23
Nodes (10): string, LoadResult, message, settings, status, SaveResult, message, ok (+2 more)

### Community 122 - "Theme"
Cohesion: 0.11
Nodes (15): Borders, focus, focus_offset, hairline, regular, Theme, border, elevation (+7 more)

### Community 123 - "ButtonStyle"
Cohesion: 0.18
Nodes (10): ButtonStyle, active, disabled, hover, idle, ButtonVariant, StateColors, border (+2 more)

### Community 124 - "RenderQuality"
Cohesion: 0.25
Nodes (7): RenderQuality, bloom, cirrus, clouds, exhaust, scene_samples, shadow_map_size

### Community 125 - "AircraftConfig"
Cohesion: 0.07
Nodes (28): AircraftConfig, aero, display_name, flcs, gear, mass, propulsion, type (+20 more)

### Community 126 - "AeroCoefficients"
Cohesion: 0.20
Nodes (10): AeroCoefficients, alpha_deg, beta_deg, CD, CL, Cm, Cn, CY (+2 more)

### Community 128 - "FlightStateDeriv"
Cohesion: 0.13
Nodes (10): ForceFn, FlightStateDeriv, d_omega_b, d_pos_ned, d_q_att, d_vel_b, operator*(), step() (+2 more)

### Community 129 - "test_user_settings.cpp"
Cohesion: 0.35
Nodes (10): path_, string, main(), scratch_dir(), test_binding_conflicts(), test_defaults_round_trip(), test_json_parser(), test_partial_and_invalid_values() (+2 more)

### Community 130 - "Spacing"
Cohesion: 0.20
Nodes (10): Spacing, lg, md, page, sm, xl, xs, xxl (+2 more)

### Community 131 - "Typography"
Cohesion: 0.20
Nodes (10): Typography, body, caption, display, display_sub, heading, label, title (+2 more)

### Community 132 - "ScrollRegion"
Cohesion: 0.22
Nodes (6): optional, ScrollRegion, content_h, view, optional, optional

### Community 134 - "Vertex"
Cohesion: 0.25
Nodes (8): Vertex, border, fill, half, local, params, pos, uv

### Community 135 - "cgltf_parse_json"
Cohesion: 0.36
Nodes (10): cgltf_fixup_pointers(), cgltf_parse_json(), jsmn_alloc_token(), jsmn_fill_token(), jsmn_init(), jsmn_parse(), jsmn_parse_primitive(), jsmn_parse_string() (+2 more)

### Community 136 - "AccessibilitySettings"
Cohesion: 0.29
Nodes (6): AccessibilitySettings, colorblind, high_contrast, reduce_motion, ui_scale, ColorblindMode

### Community 138 - "WeatherState"
Cohesion: 0.25
Nodes (7): WeatherState, cirrus_cover, cumulus_base_m, cumulus_coverage, cumulus_top_m, wind_east_mps, wind_north_mps

### Community 139 - "Radii"
Cohesion: 0.40
Nodes (5): Radii, lg, md, pill, sm

### Community 142 - "test_ofc_closed_loop.cpp"
Cohesion: 0.24
Nodes (14): function, expect_saved(), main(), Outcome, crashed, gcas_s, max_nz, min_agl (+6 more)

### Community 144 - "TerrainField"
Cohesion: 0.16
Nodes (11): TerrainField, BASE_AMP, BASE_FREQ, BASIN_FADE, BASIN_RADIUS, FIELD_CENTER_X, FIELD_CENTER_Y, GAIN (+3 more)

### Community 146 - "ClosedLoopSim"
Cohesion: 0.14
Nodes (14): ClosedLoopSim, aero, cfg, engine, flcs, forces, gear_down, integrator (+6 more)

### Community 147 - "Airframe"
Cohesion: 0.17
Nodes (9): Airframe, alpha_limit_deg, cl_max, g_command_law, max_g, ref_mass_kg, roll_rate_dps, wing_area_m2 (+1 more)

### Community 148 - "CloudSettings"
Cohesion: 0.15
Nodes (12): CloudSettings, base_m, coverage, kMapUnit, map_inv_size, map_origin_x, map_origin_y, map_texture (+4 more)

### Community 149 - "AirData"
Cohesion: 0.18
Nodes (9): AirData, density, dynamic_pressure, geometric_altitude, geopotential_altitude, mach_number, pressure, speed_of_sound (+1 more)

### Community 151 - ".height"
Cohesion: 0.40
Nodes (7): main(), test_airfield_basin_is_flat(), test_field_is_deterministic(), test_height_is_non_negative_and_bounded(), test_normals_are_unit_and_upward(), test_relief_grows_away_from_field(), test_slope_produces_tilted_normal()

### Community 152 - ".compute"
Cohesion: 0.47
Nodes (7): main(), test_altitude_lapse(), test_fuel_burn_and_flameout(), test_ram_recovery(), test_sea_level_static_ratings(), test_spool_dynamics(), test_variable_mass()

### Community 155 - "Geometry"
Cohesion: 0.22
Nodes (9): Geometry, buttons_top, chip_h, chip_y, left, overline_cy, rule_y, subtitle_baseline (+1 more)

### Community 156 - "Atmosphere1976"
Cohesion: 0.25
Nodes (8): Atmosphere1976, G0, GAMMA, P0, R_EARTH, R_GAS, RHO0, T0

### Community 157 - "test_atmosphere.cpp"
Cohesion: 0.52
Nodes (5): main(), test_sea_level(), test_stratosphere_25000m(), test_tropopause_11000m(), test_troposphere_5000m()

### Community 158 - "Geometry"
Cohesion: 0.33
Nodes (6): Geometry, footer, header, panel, tabs, view

### Community 160 - "Ring"
Cohesion: 0.67
Nodes (3): Ring, cell, half_extent

## Knowledge Gaps
- **1783 isolated node(s):** `genVertexArrays`, `bindVertexArray`, `deleteVertexArrays`, `genBuffers`, `bindBuffer` (+1778 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 2088 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **9 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `FlightState` connect `FlightState` to `Quaternion`, `.step`, `FlightStateDeriv`, `Flight`, `ClosedLoopSim`, `SimRunner`, `test_ground_collision.cpp`, `FlightInstruments`, `HUDCollimator`, `LandingGear`, `LevelRun`, `test_hud_symbology_generation_gl`, `AircraftAeroModel`, `IMUData`, `Color4`, `Sim`, `FlightSimHarness`, `aircraft_type.hpp`, `SimHarness`, `Vector3`, `ShadowMap`, `.update`, `.compute_coefficients`, `AircraftFLCS`?**
  _High betweenness centrality (0.093) - this node is a cross-community bridge._
- **Why does `RenderEngine` connect `RenderEngine` to `WeatherState`, `CameraRig`, `AircraftMenu`, `SkyGroundRenderer`, `CockpitGeometry`, `FlightInstruments`, `HUDCollimator`, `ModelGLB`, `FlightState`, `HdrPipeline`, `AvionicsTelemetry`, `CloudLayer`, `vector`, `test_render_fidelity.cpp`, `UiCanvas`, `ShadowMap`, `ExhaustPlume`, `ShaderProgram`, `ColorFilterPass`, `main`, `Theme`, `RenderQuality`, `.destroy`?**
  _High betweenness centrality (0.079) - this node is a cross-community bridge._
- **Why does `MenuSystem` connect `MenuSystem` to `ScrollRegion`, `MenuServices`, `array`, `.find`, `Widget`, `LayoutContext`, `main`, `ModalSpec`, `.on_nav`, `DropdownState`, `.render`, `Theme`, `test_settings_edit_apply_revert`?**
  _High betweenness centrality (0.071) - this node is a cross-community bridge._
- **Are the 5 inferred relationships involving `MenuSystem` (e.g. with `test_dropdown()` and `test_key_rebinding()`) actually correct?**
  _`MenuSystem` has 5 INFERRED edges - model-reasoned connections that need verification._
- **What connects `genVertexArrays`, `bindVertexArray`, `deleteVertexArrays` to the rest of the system?**
  _1783 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Quaternion` be split into smaller, more focused modules?**
  _Cohesion score 0.08021390374331551 - nodes in this community are weakly interconnected._
- **Should `ControllerProfile` be split into smaller, more focused modules?**
  _Cohesion score 0.07407407407407407 - nodes in this community are weakly interconnected._