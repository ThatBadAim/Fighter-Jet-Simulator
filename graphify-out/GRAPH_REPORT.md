# Graph Report - Fast Jet Simulator New Version  (2026-09-24)

## Corpus Check
- 130 files · ~302,825 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 4183 nodes · 8860 edges · 166 communities (152 shown, 11 thin omitted)
- Extraction: 94% EXTRACTED · 6% INFERRED · 0% AMBIGUOUS · INFERRED: 512 edges (avg confidence: 0.81)
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
- InstrumentCanvas
- .process_bipolar
- F110Engine
- F16AeroModel
- OnBoardFlightComputer
- VirtualHOTASDriver
- Actuator
- cgltf.h
- F16FLCS
- stb_image.h
- CameraRig
- InputManager
- ThrottleController
- AircraftMenu
- avionics_controls.hpp
- SkyGroundRenderer
- SimRunner
- SettingsScreen
- test_ground_collision.cpp
- FastJetGLFunctions
- CockpitGeometry
- CockpitGauges
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
- MenuSystem
- AxisCalibration
- stbi__context
- Agent Guidelines & Rules
- .update_and_render
- .evaluate_force_curve
- cgltf_data
- cgltf_buffer_view
- AircraftAeroModel
- cgltf_size
- cgltf_material
- cgltf_primitive
- cgltf_node
- IMUData
- stbi__jpeg
- cgltf_extension
- AvionicsTelemetry
- AudioEngine
- Color4
- interpolator.hpp
- DrawList
- stbi__bmp_load
- cgltf_texture
- MultiEngine
- MenuGlyph
- CloudLayer
- PropulsionConfig
- stbi__err
- MassProperties
- Vector3
- MenuServices
- Sim
- AtmosphereModel
- stbi__parse_png_file
- vector
- Flight
- Widget
- ThreadPool
- FlightSimHarness
- f16_sim_viewer.cpp
- AeroConfig
- test_render_fidelity.cpp
- DropdownState
- SimHarness
- PerfResult
- CreditsScreen
- Metrics
- GraphicsSettings
- InputEvent
- Palette
- UiCanvas
- test_settings_edit_apply_revert
- test_integrated_flight.cpp
- .get
- GpuProfiler
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
- main
- .update
- ColorFilterPass
- .compute_coefficients
- SettingsManager
- MenuVertex
- ModalSpec
- string
- Readings
- Motion
- cgltf_extras
- UserSettings
- theme.hpp
- 5. Phases
- AircraftFLCS
- SettingsStorage
- Theme
- ButtonStyle
- RenderQuality
- LandingGearConfig
- AeroCoefficients
- DialSpec
- test_hud_collimation.cpp
- .render
- Spacing
- Typography
- FlightInstruments
- .compute
- Vertex
- cgltf_parse_json
- AccessibilitySettings
- AircraftConfig
- WeatherState
- Radii
- FrameContext
- MassConfig
- test_ofc_closed_loop.cpp
- .init
- widgets.hpp
- .screen
- ClosedLoopSim
- Airframe
- CloudSettings
- .compute
- .relayout
- Strut
- AircraftType
- decode_menu_font_atlas
- test_user_settings.cpp
- string_view
- Atmosphere1976
- SceneLighting
- LayoutContext
- test_gcas_throttle_authority
- FLCSConfig
- MenuTab
- Geometry
- Image

## God Nodes (most connected - your core abstractions)
1. `MenuSystem` - 145 edges
2. `cgltf_data` - 112 edges
3. `FastJetGLFunctions` - 98 edges
4. `AircraftMenu` - 89 edges
5. `OnBoardFlightComputer` - 79 edges
6. `RenderEngine` - 79 edges
7. `cgltf_options` - 74 edges
8. `SettingsScreen` - 72 edges
9. `FlightState` - 71 edges
10. `CockpitGauges` - 70 edges

## Surprising Connections (you probably didn't know these)
- `find()` --calls--> `widgets_`  [INFERRED]
  tests/test_menu_navigation.cpp → include/fastjet/ui/menu_system.hpp
- `expect_saved()` --calls--> `to_short_string()`  [INFERRED]
  tests/test_ofc_closed_loop.cpp → include/fastjet/aircraft/aircraft_type.hpp
- `test_tumble_law_respects_airframe_alpha_envelope()` --calls--> `to_short_string()`  [INFERRED]
  tests/test_ofc_closed_loop.cpp → include/fastjet/aircraft/aircraft_type.hpp
- `main()` --calls--> `cgltf_parse_file()`  [INFERRED]
  tests/test_model_glb.cpp → include/fastjet/graphics/cgltf.h
- `main()` --calls--> `cgltf_load_buffers()`  [INFERRED]
  tests/test_model_glb.cpp → include/fastjet/graphics/cgltf.h

## Import Cycles
- None detected.

## Communities (166 total, 11 thin omitted)

### Community 0 - "Quaternion"
Cohesion: 0.06
Nodes (17): FlightStateDeriv, d_omega_b, d_pos_ned, d_q_att, d_vel_b, operator*(), Matrix3x3, m (+9 more)

### Community 1 - ".step"
Cohesion: 0.15
Nodes (18): main(), test_horizontal_ballistic_flight(), test_inclined_ballistic_flight_arbitrary_attitude(), test_steady_level_trim_flight(), main(), test_hands_off_level_flight(), main(), test_high_speed_9g_pull() (+10 more)

### Community 2 - "ControllerProfile"
Cohesion: 0.07
Nodes (27): ControllerProfile, btn_parking_brake, btn_speedbrake_extend, btn_speedbrake_retract, btn_trim_down, btn_trim_left, btn_trim_right, btn_trim_up (+19 more)

### Community 3 - "TP1538Tables"
Cohesion: 0.09
Nodes (31): array, size_t, idx3d(), init_CD_table(), init_CL_roll_table(), init_CL_table(), init_CM_table(), init_CN_yaw_table() (+23 more)

### Community 4 - "InstrumentCanvas"
Cohesion: 0.06
Nodes (24): GLuint, InstrumentCanvas, fbo_, font_tex_, initialized_, kNoTex, kPi, line_verts_ (+16 more)

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
Cohesion: 0.06
Nodes (17): array, size_t, string, vector, SDL3InputDriver, initialized_, joysticks_, VirtualHOTASDriver (+9 more)

### Community 10 - "Actuator"
Cohesion: 0.16
Nodes (13): Actuator, is_pos_limited, is_rate_limited, pos_max, pos_min, position, rate, rate_limit (+5 more)

### Community 11 - "cgltf.h"
Cohesion: 0.16
Nodes (80): cgltf_calloc(), cgltf_fill_float_array(), cgltf_json_strcmp(), cgltf_json_to_bool(), cgltf_json_to_component_type(), cgltf_json_to_float(), cgltf_json_to_int(), cgltf_json_to_primitive_type() (+72 more)

### Community 12 - "F16FLCS"
Cohesion: 0.08
Nodes (20): F16FLCS, flaperon, lat_dir_ctrl, pitch_ctrl, rudder, stabilator, PitchController, ALPHA_LEAD_S (+12 more)

### Community 13 - "stb_image.h"
Cohesion: 0.06
Nodes (88): FILE, load_jpeg_image(), resample_row_1(), stbi__blinn_8x8(), stbi__clamp(), stbi__cleanup_jpeg(), stbi__compute_y(), stbi__convert_16_to_8() (+80 more)

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
Nodes (45): AircraftMenu, col_accent_gold, col_active_badge, col_alert_red, col_bg_void, col_border_bright, col_border_dim, col_border_outer (+37 more)

### Community 18 - "avionics_controls.hpp"
Cohesion: 0.10
Nodes (16): DigitalTrimHat, slew_rate, trim_pitch, trim_roll, SpeedbrakeController, position, slew_rate, WheelBrakes (+8 more)

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
Nodes (25): CockpitGeometry, gauge_vao_, gauge_vbo_, gauge_vertex_count_, glass_vao_, glass_vbo_, glass_vertex_count_, initialized_ (+17 more)

### Community 25 - "CockpitGauges"
Cohesion: 0.07
Nodes (28): Cell, emissive, h, w, x, y, CockpitGauges, kAmberBand (+20 more)

### Community 26 - "HUDCollimator"
Cohesion: 0.10
Nodes (20): GLuint, vector, HUDCollimator, initialized_, lines_, masked_vertex_count_, peak_g_, shader_ (+12 more)

### Community 27 - "ModelGLB"
Cohesion: 0.04
Nodes (48): F16PartType, GLBSubmesh, base_color_factor, base_texture, bounds_max, bounds_min, ebo, emissive (+40 more)

### Community 28 - "LandingGear"
Cohesion: 0.10
Nodes (20): AircraftType, array, LandingGear, brake_left, brake_right, collapsed, CORNERING_STIFFNESS, CREEP_SPEED (+12 more)

### Community 29 - "Key Technical Specifications"
Cohesion: 0.09
Nodes (22): 10. First-Person Graphics, Collimated HUD & Cockpit Avionics, 11. Threading and Frame Pacing, 1. Zero-Allocation Architecture, 2. State Vector & Coordinate Frames, 3. Core Equations of Motion, 4. F-16C Mass & Inertia Properties, 5. 1976 US Standard Atmosphere Model, 6. NASA TP-1538 Aerodynamics & Multidimensional Interpolation (+14 more)

### Community 30 - "DrawCmd"
Cohesion: 0.11
Nodes (18): DrawCmdType, DrawCmd, blur, border, border_width, fill_bottom, fill_top, horizontal (+10 more)

### Community 31 - "RenderEngine"
Cohesion: 0.05
Nodes (35): ColorblindMode, RenderEngine, aircraft_menu_, caster_shader_, clouds_, cockpit_, cockpit_shader_, color_filter_ (+27 more)

### Community 32 - "LevelRun"
Cohesion: 0.16
Nodes (17): LevelRun, aero, alt_target, engine, flcs, forces, integrator, mass (+9 more)

### Community 33 - "TerrainMesh"
Cohesion: 0.06
Nodes (35): TerrainField, BASE_AMP, BASE_FREQ, BASIN_FADE, BASIN_RADIUS, FIELD_CENTER_X, FIELD_CENTER_Y, GAIN (+27 more)

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
Nodes (46): array, deque, optional, QualityPreset, SubscriptionId, unique_ptr, MenuSystem, actions_ (+38 more)

### Community 41 - "AxisCalibration"
Cohesion: 0.22
Nodes (8): AxisCalibration, curvature, inner_deadband, inverted, outer_deadband, raw_center, raw_max, raw_min

### Community 42 - "stbi__context"
Cohesion: 0.11
Nodes (64): stbi__at_eof(), stbi__bmp_info(), stbi__bmp_test(), stbi__bmp_test_raw(), stbi__check_png_header(), stbi__convert_format(), stbi__get16be(), stbi__get16le() (+56 more)

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
Cohesion: 0.07
Nodes (62): cgltf_bool, cgltf_component_type, cgltf_result, cgltf_size, cgltf_ssize, cgltf_type, cgltf_uint, cgltf_accessor (+54 more)

### Community 50 - "cgltf_material"
Cohesion: 0.02
Nodes (108): cgltf_alpha_mode, cgltf_float, cgltf_anisotropy, anisotropy_rotation, anisotropy_strength, anisotropy_texture, cgltf_clearcoat, clearcoat_factor (+100 more)

### Community 51 - "cgltf_primitive"
Cohesion: 0.05
Nodes (41): cgltf_attribute_type, cgltf_int, cgltf_primitive_type, cgltf_attribute, data, index, name, type (+33 more)

### Community 52 - "cgltf_node"
Cohesion: 0.04
Nodes (48): cgltf_light_type, cgltf_light, color, extras, cgltf_light_index(), intensity, name, range (+40 more)

### Community 53 - "IMUData"
Cohesion: 0.09
Nodes (17): IMUData, airspeed, alpha_deg, altitude, beta_deg, Nx, Ny, Nz (+9 more)

### Community 54 - "stbi__jpeg"
Cohesion: 0.15
Nodes (32): stbi__addints_valid(), stbi__bitreverse16(), stbi__build_fast_ac(), stbi__build_huffman(), stbi__cpuid3(), stbi__decode_jpeg_header(), stbi__decode_jpeg_image(), stbi__extend_receive() (+24 more)

### Community 55 - "cgltf_extension"
Cohesion: 0.06
Nodes (39): cgltf_animation_path_type, cgltf_interpolation_type, cgltf_animation, cgltf_animation_channel, extensions, extensions_count, extras, cgltf_animation_channel_index() (+31 more)

### Community 56 - "AvionicsTelemetry"
Cohesion: 0.05
Nodes (38): AvionicsTelemetry, aircraft_switch_timer, aircraft_type, bingo_fuel_alert, brake_left, brake_right, detent_str, engine_ftit_deg_c (+30 more)

### Community 57 - "AudioEngine"
Cohesion: 0.05
Nodes (35): AudioEngine, alerts_bus_gain_, CHANNELS, device_id_, enabled_, engine_bus_gain_, initialized_, kMixChunkFrames (+27 more)

### Community 58 - "Color4"
Cohesion: 0.16
Nodes (5): Color4, a, b, g, r

### Community 59 - "interpolator.hpp"
Cohesion: 0.23
Nodes (14): find_index_and_weight(), N, size_t, interp1d(), interp2d(), interp3d(), Interpolator, NX (+6 more)

### Community 60 - "DrawList"
Cohesion: 0.07
Nodes (34): baseline_for_center(), size, DrawList, cmds_, layers_, fit_text(), string, string_view (+26 more)

### Community 61 - "stbi__bmp_load"
Cohesion: 0.15
Nodes (22): stbi__addsizes_valid(), stbi__bitcount(), stbi__bmp_load(), stbi__bmp_parse_header(), stbi__bmp_set_mask_defaults(), stbi__create_png_alpha_expand8(), stbi__create_png_image(), stbi__create_png_image_raw() (+14 more)

### Community 62 - "cgltf_texture"
Cohesion: 0.06
Nodes (33): cgltf_filter_type, cgltf_wrap_mode, cgltf_image, buffer_view, extensions, extensions_count, extras, cgltf_image_index() (+25 more)

### Community 63 - "MultiEngine"
Cohesion: 0.11
Nodes (14): AircraftType, DetentState, MultiEngine, ab_power, aircraft_type, commanded_thrust_n, config, detent_state (+6 more)

### Community 64 - "MenuGlyph"
Cohesion: 0.18
Nodes (11): MenuGlyph, advance, bearing_x, bearing_y, code, h, u0, u1 (+3 more)

### Community 65 - "CloudLayer"
Cohesion: 0.08
Nodes (23): CloudLayer, build_strip_, building_, front_, front_valid_, kExtent, kMapOctaves, kMapRecenter (+15 more)

### Community 66 - "PropulsionConfig"
Cohesion: 0.12
Nodes (17): PropulsionConfig, engine_count, has_afterburner, has_supercruise, has_thrust_vectoring, static_ab_thrust_n, static_dry_thrust_n, static_idle_thrust_n (+9 more)

### Community 67 - "stbi__err"
Cohesion: 0.34
Nodes (17): stbi__bit_reverse(), stbi__compute_huffman_codes(), stbi__err(), stbi__fill_bits(), stbi__parse_huffman_block(), stbi__parse_uncompressed_block(), stbi__parse_zlib(), stbi__parse_zlib_header() (+9 more)

### Community 68 - "MassProperties"
Cohesion: 0.09
Nodes (19): ForceFn, AircraftForces, force_b, moment_b, FuelSystem, EMPTY_MASS_KG, AircraftType, MassProperties (+11 more)

### Community 69 - "Vector3"
Cohesion: 0.08
Nodes (17): WindTurbulenceModel, base_wind_speed_mps, gust_u_, gust_v_, gust_w_, rng_state_, turbulence_enabled, wind_heading_rad (+9 more)

### Community 70 - "MenuServices"
Cohesion: 0.07
Nodes (19): WidgetId, MenuServices, begin_key_capture, emit, focused, in_flight, is_capturing, key_name (+11 more)

### Community 71 - "Sim"
Cohesion: 0.15
Nodes (12): Sim, aero, engine, flcs, forces, gear, integrator, mass (+4 more)

### Community 72 - "AtmosphereModel"
Cohesion: 0.13
Nodes (15): AtmosphereModel, EXPOSURE, MIE, MIE_G, MIN_MU, PI, RAYLEIGH, SCALE_H (+7 more)

### Community 73 - "stbi__parse_png_file"
Cohesion: 0.26
Nodes (13): stbi__compute_transparency(), stbi__compute_transparency16(), stbi__compute_y_16(), stbi__convert_format16(), stbi__de_iphone(), stbi__do_png(), stbi__expand_png_palette(), stbi__get_chunk_header() (+5 more)

### Community 74 - "vector"
Cohesion: 0.15
Nodes (5): array, string, vector, apply_texture_anisotropy(), max_texture_anisotropy()

### Community 75 - "Flight"
Cohesion: 0.14
Nodes (19): Flight, aero, engine, flcs, forces, integrator, mass, speedbrake (+11 more)

### Community 76 - "Widget"
Cohesion: 0.10
Nodes (31): ButtonVariant, string, WidgetId, Widget, can_decrement, can_increment, capturing, checked (+23 more)

### Community 77 - "ThreadPool"
Cohesion: 0.12
Nodes (22): condition_variable, F, future, deque, function, vector, parallel_for(), submit() (+14 more)

### Community 78 - "FlightSimHarness"
Cohesion: 0.13
Nodes (14): RK4Integrator, DEFAULT_DT, dt, AircraftType, FlightSimHarness, aero, engine, flcs (+6 more)

### Community 80 - "AeroConfig"
Cohesion: 0.11
Nodes (19): AeroConfig, aspect_ratio, b_span, c_bar, cd0, cd_speedbrake, cd_wave_peak, cd_wave_super (+11 more)

### Community 81 - "test_render_fidelity.cpp"
Cohesion: 0.14
Nodes (21): camera_rig_, hdr_, shadow_map_, QualityPreset, apply(), array, vector, Frame (+13 more)

### Community 82 - "DropdownState"
Cohesion: 0.06
Nodes (33): CaptureState, anchor, error, on_clear, on_key, title, DropdownState, anchor (+25 more)

### Community 83 - "SimHarness"
Cohesion: 0.14
Nodes (14): AircraftType, evaluate_aircraft(), main(), SimHarness, aero, cfg, engine, flcs (+6 more)

### Community 84 - "PerfResult"
Cohesion: 0.12
Nodes (17): string, PerfResult, ab_thrust_kn, alt_m, aoa_limit_deg, combat_mass_kg, dry_speed_alt_mach, dry_thrust_kn (+9 more)

### Community 85 - "CreditsScreen"
Cohesion: 0.08
Nodes (20): optional, ScrollRegion, content_h, view, CreditsScreen, content_h_, footer_, kBack (+12 more)

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
Cohesion: 0.13
Nodes (14): vector, UiCanvas, clip_stack_, fb_h_, fb_w_, font_tex_, initialized_, kAaPad (+6 more)

### Community 91 - "test_settings_edit_apply_revert"
Cohesion: 0.28
Nodes (18): toast_text_, pending_, click(), NavCommand, WidgetId, find(), focus_down_to(), frames() (+10 more)

### Community 92 - "test_integrated_flight.cpp"
Cohesion: 0.51
Nodes (7): main(), test_altitude_thrust_lapse_in_flight(), test_energy_sanity_in_cruise(), test_fuel_burn_changes_mass(), test_gear_survives_gentle_landing(), test_speedbrake_decelerates_in_flight(), test_takeoff_roll()

### Community 93 - ".get"
Cohesion: 0.57
Nodes (6): main(), test_a10_specifications(), test_f15ex_specifications(), test_f16c_specifications(), test_f22_specifications(), test_typhoon_specifications()

### Community 94 - "GpuProfiler"
Cohesion: 0.09
Nodes (19): Frame, count, names, queries, GpuProfiler, cur_, frames_, kFramesInFlight (+11 more)

### Community 95 - "Parser"
Cohesion: 0.25
Nodes (8): optional, string_view, parse(), Parser, error_, kMaxDepth, pos_, s_

### Community 96 - "ShadowMap"
Cohesion: 0.12
Nodes (10): GLuint, ShadowMap, depth_tex_, fbo_, kDepthRange, kFocusRadius, kReceiverBias, light_view_proj_ (+2 more)

### Community 97 - "ControlSettings"
Cohesion: 0.10
Nodes (16): is_down(), ActionInfo, defaults, json_key, ControlSettings, bindings, invert_mouse_y, invert_pitch (+8 more)

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
Nodes (14): Array, Type, Value, data_, enum_field(), array, E, N (+6 more)

### Community 104 - "LateralDirectionalController"
Cohesion: 0.17
Nodes (9): LateralDirectionalController, ALPHA_ATTEN_MAX, ALPHA_ATTEN_MIN, K_beta, K_roll, K_yaw_damper, Ki_roll, P_CMD_MAX_DPS (+1 more)

### Community 105 - "Vertex3D"
Cohesion: 0.40
Nodes (5): Vertex3D, color, normal, pos, uv

### Community 106 - "main"
Cohesion: 0.07
Nodes (21): GLsync, FramePacer, fences_, kMaxFramesInFlight, kWaitStepNs, slot_, array, gpu_profiler_ (+13 more)

### Community 107 - ".update"
Cohesion: 0.19
Nodes (30): main(), test_gcas_avoids_mountain_ahead_in_level_flight(), test_gcas_commands_nominal_g_not_max_g(), test_gcas_does_not_disturb_normal_landing(), test_gcas_does_not_intervene_on_shallow_dive(), test_gcas_does_not_latch_when_airspeed_collapses(), test_gcas_elevated_terrain_dive(), test_gcas_engages_earlier_when_slow() (+22 more)

### Community 108 - "ColorFilterPass"
Cohesion: 0.13
Nodes (11): ColorFilterPass, fbo_, h_, initialized_, kFragmentSrc, kVertexSrc, shader_, tex_ (+3 more)

### Community 109 - ".compute_coefficients"
Cohesion: 0.25
Nodes (13): main(), test_baseline_lookups(), test_dimensional_force_moment_synthesis(), test_elevator_control_power(), test_relaxed_static_stability_derivative(), main(), make_state(), test_dynamic_pressure_drag() (+5 more)

### Community 110 - "SettingsManager"
Cohesion: 0.15
Nodes (14): Fn, edit(), pair, SettingsSection, SubscriptionId, vector, SettingsManager, committed_ (+6 more)

### Community 111 - "MenuVertex"
Cohesion: 0.50
Nodes (4): MenuVertex, color, pos, uv

### Community 112 - "ModalSpec"
Cohesion: 0.12
Nodes (17): AppInfo, title, version, ButtonVariant, function, string, vector, ModalButton (+9 more)

### Community 113 - "string"
Cohesion: 0.26
Nodes (5): string, serialize(), Writer, indent_, out_

### Community 114 - "Readings"
Cohesion: 0.08
Nodes (25): Readings, alt_ft, aoa_deg, cabin_alt_ft, cdi_dots, config, dme_nm, eng_fire (+17 more)

### Community 115 - "Motion"
Cohesion: 0.11
Nodes (18): Motion, background_drift, focus, menu_open, modal, modal_scale_from, parallax_far, parallax_near (+10 more)

### Community 116 - "cgltf_extras"
Cohesion: 0.06
Nodes (35): cgltf_camera_orthographic, extras, xmag, ymag, zfar, znear, cgltf_camera_perspective, aspect_ratio (+27 more)

### Community 117 - "UserSettings"
Cohesion: 0.12
Nodes (12): AudioSettings, alerts, engine, master, muted, SettingsSection, UserSettings, accessibility (+4 more)

### Community 118 - "theme.hpp"
Cohesion: 0.18
Nodes (14): apply_colorblind_palette(), build_theme(), ease_in_cubic(), Elevation, modal, panel, raised, ColorblindMode (+6 more)

### Community 119 - "5. Phases"
Cohesion: 0.11
Nodes (18): 1. Design pillars, 2. What already exists to build on, 3. Gaps, 4. Architecture, 5. Phases, 6. Milestones, 7. Testing strategy, 8. Decisions needed from you (+10 more)

### Community 120 - "AircraftFLCS"
Cohesion: 0.11
Nodes (16): AircraftFLCS, aileron, aircraft_type, ALPHA_LEAD_S, config, f16_core, pitch_integrator, roll_trim (+8 more)

### Community 121 - "SettingsStorage"
Cohesion: 0.17
Nodes (12): path, string, LoadResult, message, settings, status, SaveResult, message (+4 more)

### Community 122 - "Theme"
Cohesion: 0.11
Nodes (15): Borders, focus, focus_offset, hairline, regular, Theme, border, elevation (+7 more)

### Community 123 - "ButtonStyle"
Cohesion: 0.18
Nodes (10): ButtonStyle, active, disabled, hover, idle, ButtonVariant, StateColors, border (+2 more)

### Community 124 - "RenderQuality"
Cohesion: 0.25
Nodes (7): RenderQuality, bloom, cirrus, clouds, exhaust, scene_samples, shadow_map_size

### Community 125 - "LandingGearConfig"
Cohesion: 0.18
Nodes (11): LandingGearConfig, main_damping_c, main_l_pos_b, main_r_pos_b, main_spring_k, max_steer_deg, max_stroke, nose_damping_c (+3 more)

### Community 126 - "AeroCoefficients"
Cohesion: 0.20
Nodes (10): AeroCoefficients, alpha_deg, beta_deg, CD, CL, Cm, Cn, CY (+2 more)

### Community 127 - "DialSpec"
Cohesion: 0.11
Nodes (18): Band, col, v0, v1, DialSpec, bands, label_size, labels (+10 more)

### Community 128 - "test_hud_collimation.cpp"
Cohesion: 0.26
Nodes (7): main(), test_dep_and_equilibrium(), test_g_load_dynamics(), main(), test_fpm_velocity_vector_tracking(), test_hud_conformal_roll_and_boresight_alignment(), test_optical_infinity_projection()

### Community 129 - ".render"
Cohesion: 0.26
Nodes (3): text, ease_out_cubic(), paint_panel()

### Community 130 - "Spacing"
Cohesion: 0.20
Nodes (10): Spacing, lg, md, page, sm, xl, xs, xxl (+2 more)

### Community 131 - "Typography"
Cohesion: 0.22
Nodes (9): Typography, caption, display, display_sub, heading, label, title, tracking_button (+1 more)

### Community 132 - "FlightInstruments"
Cohesion: 0.13
Nodes (10): FlightInstruments, centre_page_, TEX_HEIGHT, TEX_WIDTH, array, Gauge, gauge_pixel(), main() (+2 more)

### Community 133 - ".compute"
Cohesion: 0.44
Nodes (8): main(), make_ground_state(), test_braking_decelerates(), test_gear_collapse_on_hard_landing(), test_no_force_when_airborne(), test_nosewheel_steering(), test_retracted_gear_inert(), test_static_equilibrium()

### Community 134 - "Vertex"
Cohesion: 0.25
Nodes (8): Vertex, border, fill, half, local, params, pos, uv

### Community 135 - "cgltf_parse_json"
Cohesion: 0.36
Nodes (10): cgltf_fixup_pointers(), cgltf_parse_json(), jsmn_alloc_token(), jsmn_fill_token(), jsmn_init(), jsmn_parse(), jsmn_parse_primitive(), jsmn_parse_string() (+2 more)

### Community 136 - "AccessibilitySettings"
Cohesion: 0.29
Nodes (6): AccessibilitySettings, colorblind, high_contrast, reduce_motion, ui_scale, ColorblindMode

### Community 137 - "AircraftConfig"
Cohesion: 0.22
Nodes (9): AircraftConfig, aero, display_name, flcs, gear, mass, propulsion, type (+1 more)

### Community 138 - "WeatherState"
Cohesion: 0.25
Nodes (7): WeatherState, cirrus_cover, cumulus_base_m, cumulus_coverage, cumulus_top_m, wind_east_mps, wind_north_mps

### Community 139 - "Radii"
Cohesion: 0.40
Nodes (5): Radii, lg, md, pill, sm

### Community 140 - "FrameContext"
Cohesion: 0.17
Nodes (9): FrameContext, clouds, eye_ned, ground_shadow, hdr_output, kShadowUnit, lighting, shadow (+1 more)

### Community 141 - "MassConfig"
Cohesion: 0.25
Nodes (8): MassConfig, empty_mass_kg, internal_fuel_capacity_kg, Ixx, Ixz, Iyy, Izz, mtow_kg

### Community 142 - "test_ofc_closed_loop.cpp"
Cohesion: 0.24
Nodes (14): function, expect_saved(), main(), Outcome, crashed, gcas_s, max_nz, min_agl (+6 more)

### Community 144 - "widgets.hpp"
Cohesion: 0.20
Nodes (11): WidgetKind, is_adjustable(), LegendItem, action, keys, NavCoord, col, group (+3 more)

### Community 146 - "ClosedLoopSim"
Cohesion: 0.14
Nodes (14): ClosedLoopSim, aero, cfg, engine, flcs, forces, gear_down, integrator (+6 more)

### Community 147 - "Airframe"
Cohesion: 0.17
Nodes (9): Airframe, alpha_limit_deg, cl_max, g_command_law, max_g, ref_mass_kg, roll_rate_dps, wing_area_m2 (+1 more)

### Community 148 - "CloudSettings"
Cohesion: 0.15
Nodes (12): CloudSettings, base_m, coverage, kMapUnit, map_inv_size, map_origin_x, map_origin_y, map_texture (+4 more)

### Community 149 - ".compute"
Cohesion: 0.14
Nodes (21): AirData, density, dynamic_pressure, geometric_altitude, geopotential_altitude, mach_number, pressure, speed_of_sound (+13 more)

### Community 150 - ".relayout"
Cohesion: 0.14
Nodes (9): SettingsSection, Transition, active, direction, phase_out, t, to, step (+1 more)

### Community 151 - "Strut"
Cohesion: 0.18
Nodes (10): Strut, braked, compression, damping_c, in_contact, max_stroke, pos_b, rest_length (+2 more)

### Community 154 - "test_user_settings.cpp"
Cohesion: 0.35
Nodes (10): path, string, main(), scratch_dir(), test_binding_conflicts(), test_defaults_round_trip(), test_json_parser(), test_partial_and_invalid_values() (+2 more)

### Community 155 - "string_view"
Cohesion: 0.53
Nodes (6): engine_designation(), AircraftType, string_view, parse_aircraft_type(), signature_spec(), to_string()

### Community 156 - "Atmosphere1976"
Cohesion: 0.25
Nodes (8): Atmosphere1976, G0, GAMMA, P0, R_EARTH, R_GAS, RHO0, T0

### Community 157 - "SceneLighting"
Cohesion: 0.20
Nodes (9): SceneLighting, altitude_m, exposure, GROUND_ALBEDO, ground_ambient, sky_ambient, SKY_AMBIENT_GAIN, sun (+1 more)

### Community 158 - "LayoutContext"
Cohesion: 0.07
Nodes (22): LayoutContext, pointer_nx, pointer_ny, s, scroll_y, time, viewport, Geometry (+14 more)

### Community 160 - "FLCSConfig"
Cohesion: 0.29
Nodes (7): FLCSConfig, alpha_limit_deg, law_type, max_g_negative, max_g_positive, max_roll_rate_dps, LawType

### Community 164 - "Geometry"
Cohesion: 0.33
Nodes (6): Geometry, footer, header, panel, tabs, view

### Community 165 - "Image"
Cohesion: 0.50
Nodes (4): Image, height, pixels, width

## Knowledge Gaps
- **1905 isolated node(s):** `genVertexArrays`, `bindVertexArray`, `deleteVertexArrays`, `genBuffers`, `bindBuffer` (+1900 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 2227 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **11 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `RenderEngine` connect `RenderEngine` to `FlightInstruments`, `WeatherState`, `CameraRig`, `.init`, `AircraftMenu`, `SkyGroundRenderer`, `CockpitGeometry`, `CockpitGauges`, `HUDCollimator`, `ModelGLB`, `FlightState`, `HdrPipeline`, `AvionicsTelemetry`, `CloudLayer`, `vector`, `test_render_fidelity.cpp`, `UiCanvas`, `GpuProfiler`, `ShadowMap`, `ExhaustPlume`, `ShaderProgram`, `main`, `ColorFilterPass`, `Theme`, `RenderQuality`?**
  _High betweenness centrality (0.137) - this node is a cross-community bridge._
- **Why does `Theme` connect `Theme` to `.apply_theme`, `Spacing`, `Typography`, `.render`, `MenuServices`, `MenuSystem`, `vector`, `Radii`, `Widget`, `widgets.hpp`, `Motion`, `theme.hpp`, `Metrics`, `Palette`, `ButtonStyle`, `LayoutContext`?**
  _High betweenness centrality (0.078) - this node is a cross-community bridge._
- **Why does `MenuSystem` connect `MenuSystem` to `.render`, `MenuServices`, `vector`, `main`, `Widget`, `SettingsManager`, `ModalSpec`, `.screen`, `DropdownState`, `widgets.hpp`, `CreditsScreen`, `.relayout`, `Theme`, `test_settings_edit_apply_revert`, `DrawList`?**
  _High betweenness centrality (0.078) - this node is a cross-community bridge._
- **Are the 5 inferred relationships involving `MenuSystem` (e.g. with `test_dropdown()` and `test_key_rebinding()`) actually correct?**
  _`MenuSystem` has 5 INFERRED edges - model-reasoned connections that need verification._
- **What connects `genVertexArrays`, `bindVertexArray`, `deleteVertexArrays` to the rest of the system?**
  _1905 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Quaternion` be split into smaller, more focused modules?**
  _Cohesion score 0.0627177700348432 - nodes in this community are weakly interconnected._
- **Should `ControllerProfile` be split into smaller, more focused modules?**
  _Cohesion score 0.07407407407407407 - nodes in this community are weakly interconnected._