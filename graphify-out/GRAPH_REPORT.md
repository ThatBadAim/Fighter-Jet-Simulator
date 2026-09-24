# Graph Report - Fast Jet Simulator New Version  (2026-09-24)

## Corpus Check
- 143 files · ~238,980 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 4644 nodes · 9693 edges · 183 communities (177 shown, 6 thin omitted)
- Extraction: 94% EXTRACTED · 6% INFERRED · 0% AMBIGUOUS · INFERRED: 558 edges (avg confidence: 0.81)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `3509fb56`
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
- PitchController
- stb_image.h
- CameraRig
- InputManager
- ThrottleController
- AircraftMenu
- DigitalTrimHat
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
- FieldSpec
- MenuSystem
- AxisCalibration
- stbi__context
- Agent Guidelines & Rules
- BfmPilot
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
- cgltf_extension
- AvionicsTelemetry
- AudioEngine
- Color4
- interpolator.hpp
- DrawList
- stbi__malloc
- cgltf_texture
- MultiEngine
- MenuGlyph
- CloudLayer
- PropulsionConfig
- stbi__zbuf
- MassProperties
- Vector3
- MenuServices
- Sim
- AtmosphereModel
- World
- vector
- test_throttle_control.cpp
- Widget
- ThreadPool
- FlightSimHarness
- flight_state.hpp
- AeroConfig
- test_render_fidelity.cpp
- DropdownState
- SimHarness
- PerfResult
- ScrollRegion
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
- AircraftConfig
- Aircraft
- DialSpec
- DamageState
- CombatTelemetry
- Spacing
- Typography
- FlightInstruments
- Engagement
- Vertex
- cgltf_parse_json
- SDL3InputDriver
- AirCombatGeometry
- WeatherState
- Radii
- FrameContext
- MassConfig
- ClosedLoopSim
- .destroy
- widgets.hpp
- .screen
- ReferenceLoop
- Airframe
- GunSystem
- AirData
- Transition
- EngagementSetup
- test_bfm_ai.cpp
- TerrainField
- test_user_settings.cpp
- aircraft_type.hpp
- Atmosphere1976
- WindTurbulenceModel
- MainMenuScreen
- TracerRenderer
- test_guns.cpp
- PilotSkill
- AirframeCombatSpec
- .height
- Geometry
- Image
- .compute
- RangeTargetPilot
- test_air_combat_geometry.cpp
- FlightStateDeriv
- Matrix3x3
- AircraftControls
- stbi__gif_load_next
- EngagementStats
- Projectile
- CombatEvent
- test_atmosphere.cpp
- SpeedbrakeController
- test_aircraft_entity.cpp
- EulerAngles
- Cell
- Borders
- Ring

## God Nodes (most connected - your core abstractions)
1. `MenuSystem` - 145 edges
2. `cgltf_data` - 112 edges
3. `Vector3` - 101 edges
4. `FastJetGLFunctions` - 98 edges
5. `RenderEngine` - 93 edges
6. `FlightState` - 90 edges
7. `AircraftMenu` - 89 edges
8. `OnBoardFlightComputer` - 81 edges
9. `cgltf_options` - 74 edges
10. `SettingsScreen` - 72 edges

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

## Communities (183 total, 6 thin omitted)

### Community 0 - "Quaternion"
Cohesion: 0.16
Nodes (5): Quaternion, w, x, y, z

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
Nodes (23): GLuint, InstrumentCanvas, fbo_, font_tex_, initialized_, kNoTex, kPi, line_verts_ (+15 more)

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
Cohesion: 0.16
Nodes (80): cgltf_calloc(), cgltf_fill_float_array(), cgltf_json_strcmp(), cgltf_json_to_bool(), cgltf_json_to_component_type(), cgltf_json_to_float(), cgltf_json_to_int(), cgltf_json_to_primitive_type() (+72 more)

### Community 12 - "PitchController"
Cohesion: 0.12
Nodes (14): PitchController, ALPHA_LEAD_S, ALPHA_MAX_DEG, integrator, K_alpha, K_alpha_rate, Kd, Ki (+6 more)

### Community 13 - "stb_image.h"
Cohesion: 0.06
Nodes (84): FILE, resample_row_1(), stbi__clamp(), stbi__convert_16_to_8(), stbi__convert_8_to_16(), stbi_convert_iphone_png_to_rgb(), stbi_convert_iphone_png_to_rgb_thread(), stbi_convert_wchar_to_utf8() (+76 more)

### Community 14 - "CameraRig"
Cohesion: 0.06
Nodes (31): CameraMode, CameraRig, chase_distance_, chase_height_, chase_pitch_, chase_yaw_, DEP_X, DEP_Y (+23 more)

### Community 15 - "InputManager"
Cohesion: 0.10
Nodes (20): InputManager, brakes, config, driver, pitch_in, roll_in, speedbrake, throttle (+12 more)

### Community 16 - "ThrottleController"
Cohesion: 0.17
Nodes (12): DetentState, ThrottleController, ab_power, DETENT_CUTOFF, DETENT_IDLE, DETENT_MIL, dry_power, net_thrust_n (+4 more)

### Community 17 - "AircraftMenu"
Cohesion: 0.04
Nodes (48): AircraftMenu, active_tab_, col_accent_gold, col_active_badge, col_alert_red, col_bg_void, col_border_bright, col_border_dim (+40 more)

### Community 18 - "DigitalTrimHat"
Cohesion: 0.14
Nodes (12): DigitalTrimHat, slew_rate, trim_pitch, trim_roll, WheelBrakes, left_brake, parking_brake, right_brake (+4 more)

### Community 19 - "SkyGroundRenderer"
Cohesion: 0.07
Nodes (34): array, GLsizei, GLuint, SkyGroundRenderer, ATMOSPHERE_GLSL, cirrus_tex_, ground_shader_, ground_textures_ (+26 more)

### Community 20 - "SimRunner"
Cohesion: 0.23
Nodes (13): main(), SimRunner, aero_model, current_forces, flcs, integrator, mass, sim_time (+5 more)

### Community 21 - "SettingsScreen"
Cohesion: 0.07
Nodes (29): array, kInputActionCount, pair, SettingsSection, string_view, WidgetId, SettingsScreen, content_h_ (+21 more)

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
Cohesion: 0.08
Nodes (23): CockpitGauges, kAmberBand, kDialBrightness, kFace, kGreenBand, kHub, kInk, kInkDim (+15 more)

### Community 26 - "HUDCollimator"
Cohesion: 0.08
Nodes (23): GLuint, vector, HUDCollimator, initialized_, lines_, masked_vertex_count_, peak_g_, shader_ (+15 more)

### Community 27 - "ModelGLB"
Cohesion: 0.04
Nodes (51): F16PartType, BodyBounds, max, min, valid, GLBSubmesh, base_color_factor, base_texture (+43 more)

### Community 28 - "LandingGear"
Cohesion: 0.07
Nodes (38): AircraftType, array, LandingGear, brake_left, brake_right, collapsed, CORNERING_STIFFNESS, CREEP_SPEED (+30 more)

### Community 29 - "Key Technical Specifications"
Cohesion: 0.09
Nodes (22): 10. First-Person Graphics, Collimated HUD & Cockpit Avionics, 11. Threading and Frame Pacing, 1. Zero-Allocation Architecture, 2. State Vector & Coordinate Frames, 3. Core Equations of Motion, 4. F-16C Mass & Inertia Properties, 5. 1976 US Standard Atmosphere Model, 6. NASA TP-1538 Aerodynamics & Multidimensional Interpolation (+14 more)

### Community 30 - "DrawCmd"
Cohesion: 0.11
Nodes (18): DrawCmdType, DrawCmd, blur, border, border_width, fill_bottom, fill_top, horizontal (+10 more)

### Community 31 - "RenderEngine"
Cohesion: 0.04
Nodes (45): array, ColorblindMode, vector, RenderEngine, aircraft_menu_, caster_shader_, clouds_, cockpit_ (+37 more)

### Community 32 - "LevelRun"
Cohesion: 0.16
Nodes (17): LevelRun, aero, alt_target, engine, flcs, forces, integrator, mass (+9 more)

### Community 33 - "TerrainMesh"
Cohesion: 0.11
Nodes (14): GLsizei, GLuint, vector, TerrainMesh, APRON_X0, APRON_X1, APRON_Y0, APRON_Y1 (+6 more)

### Community 34 - "FlightState"
Cohesion: 0.09
Nodes (10): FlightState, omega_b, pos_ned, q_att, vel_b, array, Mat4, m (+2 more)

### Community 37 - "string"
Cohesion: 0.43
Nodes (3): string, main(), test_json_serialization_and_file_persistence()

### Community 38 - "HdrPipeline"
Cohesion: 0.05
Nodes (31): HdrPipeline, composite_shader_, down_shader_, empty_vao_, initialized_, kMaxBloomLevels, kMinBloomSize, mips_ (+23 more)

### Community 39 - "FieldSpec"
Cohesion: 0.07
Nodes (34): Accessor, acc, FieldSpec, dynamic_hint, enabled, get, hint, kind (+26 more)

### Community 40 - "MenuSystem"
Cohesion: 0.04
Nodes (47): array, deque, optional, QualityPreset, SettingsSection, SubscriptionId, unique_ptr, MenuSystem (+39 more)

### Community 41 - "AxisCalibration"
Cohesion: 0.22
Nodes (8): AxisCalibration, curvature, inner_deadband, inverted, outer_deadband, raw_center, raw_max, raw_min

### Community 42 - "stbi__context"
Cohesion: 0.11
Nodes (57): stbi__at_eof(), stbi__bitcount(), stbi__bmp_info(), stbi__bmp_load(), stbi__bmp_parse_header(), stbi__bmp_set_mask_defaults(), stbi__bmp_test(), stbi__bmp_test_raw() (+49 more)

### Community 44 - "BfmPilot"
Cohesion: 0.05
Nodes (42): BfmPilot, aim_error_rad, assess_timer_, burst_timer_, cooldown_timer_, CPA_HORIZON_S, CPA_MIN_MISS_M, DEG (+34 more)

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
Cohesion: 0.09
Nodes (20): AircraftAeroModel, aircraft_type, config, AircraftType, ControlSurfaces, delta_a, delta_e, delta_lef (+12 more)

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

### Community 54 - "stbi__err"
Cohesion: 0.19
Nodes (29): stbi__addints_valid(), stbi__build_fast_ac(), stbi__build_huffman(), stbi__decode_jpeg_header(), stbi__decode_jpeg_image(), stbi__err(), stbi__extend_receive(), stbi__get_marker() (+21 more)

### Community 55 - "cgltf_extension"
Cohesion: 0.06
Nodes (39): cgltf_animation_path_type, cgltf_interpolation_type, cgltf_animation, cgltf_animation_channel, extensions, extensions_count, extras, cgltf_animation_channel_index() (+31 more)

### Community 56 - "AvionicsTelemetry"
Cohesion: 0.05
Nodes (39): AvionicsTelemetry, aircraft_switch_timer, aircraft_type, bingo_fuel_alert, brake_left, brake_right, combat, detent_str (+31 more)

### Community 57 - "AudioEngine"
Cohesion: 0.05
Nodes (35): AudioEngine, alerts_bus_gain_, CHANNELS, device_id_, enabled_, engine_bus_gain_, initialized_, kMixChunkFrames (+27 more)

### Community 58 - "Color4"
Cohesion: 0.18
Nodes (5): Color4, a, b, g, r

### Community 59 - "interpolator.hpp"
Cohesion: 0.23
Nodes (14): find_index_and_weight(), N, size_t, interp1d(), interp2d(), interp3d(), Interpolator, NX (+6 more)

### Community 60 - "DrawList"
Cohesion: 0.07
Nodes (36): baseline_for_center(), size, DrawList, cmds_, layers_, fit_text(), string, string_view (+28 more)

### Community 61 - "stbi__malloc"
Cohesion: 0.10
Nodes (41): load_jpeg_image(), stbi__addsizes_valid(), stbi__blinn_8x8(), stbi__cleanup_jpeg(), stbi__compute_transparency(), stbi__compute_transparency16(), stbi__compute_y(), stbi__compute_y_16() (+33 more)

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
Cohesion: 0.05
Nodes (35): CloudLayer, build_strip_, building_, front_, front_valid_, kExtent, kMapOctaves, kMapRecenter (+27 more)

### Community 66 - "PropulsionConfig"
Cohesion: 0.12
Nodes (17): PropulsionConfig, engine_count, has_afterburner, has_supercruise, has_thrust_vectoring, static_ab_thrust_n, static_dry_thrust_n, static_idle_thrust_n (+9 more)

### Community 67 - "stbi__zbuf"
Cohesion: 0.30
Nodes (17): stbi__bit_reverse(), stbi__bitreverse16(), stbi__compute_huffman_codes(), stbi__fill_bits(), stbi__parse_huffman_block(), stbi__parse_uncompressed_block(), stbi__parse_zlib(), stbi__parse_zlib_header() (+9 more)

### Community 68 - "MassProperties"
Cohesion: 0.09
Nodes (19): ForceFn, AircraftForces, force_b, moment_b, FuelSystem, EMPTY_MASS_KG, AircraftType, MassProperties (+11 more)

### Community 69 - "Vector3"
Cohesion: 0.07
Nodes (27): array, Vector3, x, y, z, segment_segment_distance(), TrackSample, pos (+19 more)

### Community 70 - "MenuServices"
Cohesion: 0.04
Nodes (41): WidgetId, LayoutContext, pointer_nx, pointer_ny, s, scroll_y, time, viewport (+33 more)

### Community 71 - "Sim"
Cohesion: 0.06
Nodes (31): RK4Integrator, DEFAULT_DT, dt, F16FLCS, flaperon, lat_dir_ctrl, pitch_ctrl, rudder (+23 more)

### Community 72 - "AtmosphereModel"
Cohesion: 0.09
Nodes (24): AtmosphereModel, EXPOSURE, MIE, MIE_G, MIN_MU, PI, RAYLEIGH, SCALE_H (+16 more)

### Community 73 - "World"
Cohesion: 0.09
Nodes (18): array, World, aircraft, count, dispersion, events_, KILL_CREDIT_WINDOW_S, kMaxAircraft (+10 more)

### Community 74 - "vector"
Cohesion: 0.14
Nodes (6): array, string, vector, apply_texture_anisotropy(), max_texture_anisotropy(), unordered_map

### Community 75 - "test_throttle_control.cpp"
Cohesion: 0.35
Nodes (9): main(), test_afterburner_accelerates(), test_detent_boundaries(), test_resting_hardware_lever_does_not_capture_throttle(), test_speedbrake_adds_deceleration(), test_speedbrake_slews_not_snaps(), test_status_cadence_is_time_based(), test_throttle_back_decelerates() (+1 more)

### Community 76 - "Widget"
Cohesion: 0.10
Nodes (31): ButtonVariant, string, WidgetId, Widget, can_decrement, can_increment, capturing, checked (+23 more)

### Community 77 - "ThreadPool"
Cohesion: 0.12
Nodes (22): condition_variable, F, future, deque, function, vector, parallel_for(), submit() (+14 more)

### Community 78 - "FlightSimHarness"
Cohesion: 0.18
Nodes (11): AircraftType, FlightSimHarness, aero, engine, flcs, forces, integrator, mass (+3 more)

### Community 79 - "flight_state.hpp"
Cohesion: 0.21
Nodes (5): operator*(), EnergyState, energy_height, ps, turn_rate

### Community 80 - "AeroConfig"
Cohesion: 0.11
Nodes (19): AeroConfig, aspect_ratio, b_span, c_bar, cd0, cd_speedbrake, cd_wave_peak, cd_wave_super (+11 more)

### Community 81 - "test_render_fidelity.cpp"
Cohesion: 0.15
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

### Community 85 - "ScrollRegion"
Cohesion: 0.22
Nodes (6): optional, ScrollRegion, content_h, view, optional, optional

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
Cohesion: 0.27
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

### Community 106 - "main"
Cohesion: 0.06
Nodes (21): GLsync, FramePacer, fences_, kMaxFramesInFlight, kWaitStepNs, slot_, array, gpu_profiler_ (+13 more)

### Community 107 - ".update"
Cohesion: 0.18
Nodes (31): main(), test_gcas_avoids_mountain_ahead_in_level_flight(), test_gcas_commands_nominal_g_not_max_g(), test_gcas_does_not_disturb_normal_landing(), test_gcas_does_not_intervene_on_shallow_dive(), test_gcas_does_not_latch_when_airspeed_collapses(), test_gcas_elevated_terrain_dive(), test_gcas_engages_earlier_when_slow() (+23 more)

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
Cohesion: 0.29
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
Cohesion: 0.08
Nodes (18): AccessibilitySettings, colorblind, high_contrast, reduce_motion, ui_scale, AudioSettings, alerts, engine (+10 more)

### Community 118 - "theme.hpp"
Cohesion: 0.18
Nodes (14): apply_colorblind_palette(), build_theme(), ease_in_cubic(), Elevation, modal, panel, raised, ColorblindMode (+6 more)

### Community 119 - "5. Phases"
Cohesion: 0.08
Nodes (24): 0. Status and revisions (2026-09-24), 1. Design pillars, 2. What already exists to build on, 3. Gaps, 4. Architecture, 5. Phases, 6. Milestones, 7. Testing strategy (+16 more)

### Community 120 - "AircraftFLCS"
Cohesion: 0.08
Nodes (23): FLCSConfig, alpha_limit_deg, law_type, max_g_negative, max_g_positive, max_roll_rate_dps, AircraftFLCS, aileron (+15 more)

### Community 121 - "SettingsStorage"
Cohesion: 0.17
Nodes (12): path, string, LoadResult, message, settings, status, SaveResult, message (+4 more)

### Community 122 - "Theme"
Cohesion: 0.15
Nodes (10): Theme, border, elevation, metrics, motion, palette, radius, reduce_motion (+2 more)

### Community 123 - "ButtonStyle"
Cohesion: 0.18
Nodes (10): ButtonStyle, active, disabled, hover, idle, ButtonVariant, StateColors, border (+2 more)

### Community 124 - "RenderQuality"
Cohesion: 0.15
Nodes (8): vector, RenderQuality, bloom, cirrus, clouds, exhaust, scene_samples, shadow_map_size

### Community 125 - "AircraftConfig"
Cohesion: 0.09
Nodes (20): AircraftConfig, aero, display_name, flcs, gear, mass, propulsion, type (+12 more)

### Community 126 - "Aircraft"
Cohesion: 0.08
Nodes (22): Aircraft, aero_model, crashed, damage, DT, engine, flcs, forces (+14 more)

### Community 127 - "DialSpec"
Cohesion: 0.11
Nodes (18): Band, col, v0, v1, DialSpec, bands, label_size, labels (+10 more)

### Community 128 - "DamageState"
Cohesion: 0.08
Nodes (24): DamageState, destroyed, engine_count, ENGINE_DAMAGE_PER_HIT, engine_fire, ENGINE_FIRE_CHANCE, engine_health, FIRE_BURN_THROUGH_S (+16 more)

### Community 129 - "CombatTelemetry"
Cohesion: 0.07
Nodes (27): CombatTelemetry, active, ammo, aspect_deg, ata_deg, banner, closure_mps, control_damage (+19 more)

### Community 130 - "Spacing"
Cohesion: 0.20
Nodes (10): Spacing, lg, md, page, sm, xl, xs, xxl (+2 more)

### Community 131 - "Typography"
Cohesion: 0.22
Nodes (9): Typography, caption, display, display_sub, heading, label, title, tracking_button (+1 more)

### Community 132 - "FlightInstruments"
Cohesion: 0.13
Nodes (10): FlightInstruments, centre_page_, TEX_HEIGHT, TEX_WIDTH, array, Gauge, gauge_pixel(), main() (+2 more)

### Community 133 - "Engagement"
Cohesion: 0.13
Nodes (13): Engagement, ai, ai_controlled, BANDIT, end_time, events_read_, outcome, OWN (+5 more)

### Community 134 - "Vertex"
Cohesion: 0.25
Nodes (8): Vertex, border, fill, half, local, params, pos, uv

### Community 135 - "cgltf_parse_json"
Cohesion: 0.36
Nodes (10): cgltf_fixup_pointers(), cgltf_parse_json(), jsmn_alloc_token(), jsmn_fill_token(), jsmn_init(), jsmn_parse(), jsmn_parse_primitive(), jsmn_parse_string() (+2 more)

### Community 136 - "SDL3InputDriver"
Cohesion: 0.13
Nodes (8): string, vector, SDL3InputDriver, initialized_, joysticks_, SDL_Joystick, main(), test_sdl3_driver_initialization()

### Community 137 - "AirCombatGeometry"
Cohesion: 0.11
Nodes (17): AirCombatGeometry, aspect, ata, closure, hca, los_b, los_ned, range (+9 more)

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

### Community 142 - "ClosedLoopSim"
Cohesion: 0.10
Nodes (28): ClosedLoopSim, aero, cfg, engine, flcs, forces, gear_down, integrator (+20 more)

### Community 144 - "widgets.hpp"
Cohesion: 0.20
Nodes (11): WidgetKind, is_adjustable(), LegendItem, action, keys, NavCoord, col, group (+3 more)

### Community 146 - "ReferenceLoop"
Cohesion: 0.12
Nodes (17): AircraftType, ReferenceLoop, aero_model, current_forces, current_imu, engine, flight_control_system, integrator (+9 more)

### Community 147 - "Airframe"
Cohesion: 0.17
Nodes (9): Airframe, alpha_limit_deg, cl_max, g_command_law, max_g, ref_mass_kg, roll_rate_dps, wing_area_m2 (+1 more)

### Community 148 - "GunSystem"
Cohesion: 0.12
Nodes (12): airframe_combat_spec(), gun_spec(), AircraftType, AircraftType, GunSystem, accumulator, ammo, rounds_fired (+4 more)

### Community 149 - "AirData"
Cohesion: 0.18
Nodes (9): AirData, density, dynamic_pressure, geometric_altitude, geopotential_altitude, mach_number, pressure, speed_of_sound (+1 more)

### Community 150 - "Transition"
Cohesion: 0.33
Nodes (6): Transition, active, direction, phase_out, t, to

### Community 151 - "EngagementSetup"
Cohesion: 0.12
Nodes (17): EngagementSetup, altitude_m, bandit_type, geometry, hard_deck_agl_m, own_is_ai, own_type, range_profile (+9 more)

### Community 152 - "test_bfm_ai.cpp"
Cohesion: 0.18
Nodes (16): AiSkill, EngagementOutcome, FightResult, hits_scored, midairs, outcome, peak_nz, time (+8 more)

### Community 153 - "TerrainField"
Cohesion: 0.16
Nodes (11): TerrainField, BASE_AMP, BASE_FREQ, BASIN_FADE, BASIN_RADIUS, FIELD_CENTER_X, FIELD_CENTER_Y, GAIN (+3 more)

### Community 154 - "test_user_settings.cpp"
Cohesion: 0.35
Nodes (10): path, string, main(), scratch_dir(), test_binding_conflicts(), test_defaults_round_trip(), test_json_parser(), test_partial_and_invalid_values() (+2 more)

### Community 155 - "aircraft_type.hpp"
Cohesion: 0.21
Nodes (6): engine_designation(), AircraftType, string_view, parse_aircraft_type(), signature_spec(), to_string()

### Community 156 - "Atmosphere1976"
Cohesion: 0.25
Nodes (8): Atmosphere1976, G0, GAMMA, P0, R_EARTH, R_GAS, RHO0, T0

### Community 157 - "WindTurbulenceModel"
Cohesion: 0.16
Nodes (8): WindTurbulenceModel, base_wind_speed_mps, gust_u_, gust_v_, gust_w_, rng_state_, turbulence_enabled, wind_heading_rad

### Community 158 - "MainMenuScreen"
Cohesion: 0.11
Nodes (13): Geometry, buttons_top, chip_h, chip_y, left, overline_cy, rule_y, subtitle_baseline (+5 more)

### Community 159 - "TracerRenderer"
Cohesion: 0.16
Nodes (7): GLuint, vector, TracerRenderer, shader_, vao_, vbo_, verts_

### Community 160 - "test_guns.cpp"
Cohesion: 0.32
Nodes (10): level(), main(), place_on_pipper(), test_damaged_jet_flies_damaged(), test_drag_decay_matches_analytic(), test_pipper_on_target_scores_hits(), test_predictor_is_the_round_that_flies(), test_rate_of_fire_and_spin_up() (+2 more)

### Community 161 - "PilotSkill"
Cohesion: 0.15
Nodes (11): AiSkill, PilotSkill, aim_noise_mrad, assess_period_s, fire_miss_m, g_fraction, greyout_limit, max_fire_range_m (+3 more)

### Community 162 - "AirframeCombatSpec"
Cohesion: 0.17
Nodes (11): AirframeCombatSpec, fuselage_radius_m, length_m, span_m, structure, wing_radius_m, classify_hit(), DamageRng (+3 more)

### Community 163 - ".height"
Cohesion: 0.40
Nodes (7): main(), test_airfield_basin_is_flat(), test_field_is_deterministic(), test_height_is_non_negative_and_bounded(), test_normals_are_unit_and_upward(), test_relief_grows_away_from_field(), test_slope_produces_tilted_normal()

### Community 164 - "Geometry"
Cohesion: 0.33
Nodes (6): Geometry, footer, header, panel, tabs, view

### Community 165 - "Image"
Cohesion: 0.50
Nodes (4): Image, height, pixels, width

### Community 166 - ".compute"
Cohesion: 0.47
Nodes (7): main(), test_altitude_lapse(), test_fuel_burn_and_flameout(), test_ram_recovery(), test_sea_level_static_ratings(), test_spool_dynamics(), test_variable_mass()

### Community 167 - "RangeTargetPilot"
Cohesion: 0.20
Nodes (10): RangeTargetPilot, bank_deg, profile, target_altitude_m, target_speed_mps, throttle_, time_, trim_ (+2 more)

### Community 168 - "test_air_combat_geometry.cpp"
Cohesion: 0.58
Nodes (9): jet(), main(), near(), test_beam(), test_dead_six(), test_head_on(), test_quarter_aspect(), test_segment_distance() (+1 more)

### Community 169 - "FlightStateDeriv"
Cohesion: 0.22
Nodes (6): FlightStateDeriv, d_omega_b, d_pos_ned, d_q_att, d_vel_b, operator*()

### Community 171 - "AircraftControls"
Cohesion: 0.22
Nodes (7): AircraftControls, brake_left, brake_right, speedbrake_out, stick, throttle, trigger

### Community 172 - "stbi__gif_load_next"
Cohesion: 0.43
Nodes (8): stbi__gif_header(), stbi__gif_load_next(), stbi__gif_parse_colortable(), stbi__load_gif_main(), stbi__load_gif_main_outofmem(), stbi__out_gif_code(), stbi__process_gif_raster(), stbi__gif

### Community 173 - "EngagementStats"
Cohesion: 0.25
Nodes (8): EngagementStats, duration_s, hits_scored, hits_taken, min_agl_m, peak_g, rounds_fired, time_above_7g_s

### Community 174 - "Projectile"
Cohesion: 0.25
Nodes (8): Projectile, active, age, k_drag, lethality, owner, pos, vel

### Community 175 - "CombatEvent"
Cohesion: 0.25
Nodes (8): CombatEvent, kind, shooter, time, victim, zone, HitZone, Kind

### Community 176 - "test_atmosphere.cpp"
Cohesion: 0.52
Nodes (5): main(), test_sea_level(), test_stratosphere_25000m(), test_tropopause_11000m(), test_troposphere_5000m()

### Community 177 - "SpeedbrakeController"
Cohesion: 0.29
Nodes (4): SpeedbrakeController, position, slew_rate, SwitchPosition

### Community 178 - "test_aircraft_entity.cpp"
Cohesion: 0.52
Nodes (6): Script, compare(), main(), same(), test_airborne_manoeuvring_is_bit_identical(), test_takeoff_roll_is_bit_identical()

### Community 179 - "EulerAngles"
Cohesion: 0.33
Nodes (4): EulerAngles, pitch, roll, yaw

### Community 180 - "Cell"
Cohesion: 0.33
Nodes (6): Cell, emissive, h, w, x, y

### Community 181 - "Borders"
Cohesion: 0.40
Nodes (5): Borders, focus, focus_offset, hairline, regular

### Community 182 - "Ring"
Cohesion: 0.67
Nodes (3): Ring, cell, half_extent

## Knowledge Gaps
- **2171 isolated node(s):** `genVertexArrays`, `bindVertexArray`, `deleteVertexArrays`, `genBuffers`, `bindBuffer` (+2166 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 2524 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **6 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `RenderEngine` connect `RenderEngine` to `FlightInstruments`, `WeatherState`, `CameraRig`, `.destroy`, `AircraftMenu`, `SkyGroundRenderer`, `CockpitGeometry`, `CockpitGauges`, `HUDCollimator`, `ModelGLB`, `TracerRenderer`, `FlightState`, `HdrPipeline`, `AvionicsTelemetry`, `CloudLayer`, `Vector3`, `vector`, `test_render_fidelity.cpp`, `UiCanvas`, `GpuProfiler`, `ShadowMap`, `ExhaustPlume`, `ShaderProgram`, `main`, `ColorFilterPass`, `Theme`, `RenderQuality`?**
  _High betweenness centrality (0.135) - this node is a cross-community bridge._
- **Why does `Vector3` connect `Vector3` to `Quaternion`, `.step`, `Engagement`, `AirCombatGeometry`, `FrameContext`, `CameraRig`, `SkyGroundRenderer`, `test_ground_collision.cpp`, `ModelGLB`, `aircraft_type.hpp`, `WindTurbulenceModel`, `LandingGear`, `RenderEngine`, `test_guns.cpp`, `FlightState`, `AirframeCombatSpec`, `FlightStateDeriv`, `Matrix3x3`, `BfmPilot`, `Projectile`, `test_aircraft_entity.cpp`, `CloudLayer`, `MassProperties`, `World`, `vector`, `flight_state.hpp`, `.get`, `ShadowMap`, `main`, `AircraftConfig`, `Aircraft`?**
  _High betweenness centrality (0.127) - this node is a cross-community bridge._
- **Why does `FlightState` connect `FlightState` to `Quaternion`, `.step`, `InstrumentCanvas`, `Engagement`, `CameraRig`, `ClosedLoopSim`, `ReferenceLoop`, `SkyGroundRenderer`, `SimRunner`, `test_ground_collision.cpp`, `CockpitGauges`, `HUDCollimator`, `LandingGear`, `RenderEngine`, `test_guns.cpp`, `LevelRun`, `test_air_combat_geometry.cpp`, `FlightStateDeriv`, `AircraftAeroModel`, `test_aircraft_entity.cpp`, `EulerAngles`, `IMUData`, `Color4`, `MassProperties`, `Vector3`, `Sim`, `FlightSimHarness`, `flight_state.hpp`, `SimHarness`, `main`, `.update`, `.compute_coefficients`, `AircraftFLCS`, `Aircraft`?**
  _High betweenness centrality (0.086) - this node is a cross-community bridge._
- **Are the 5 inferred relationships involving `MenuSystem` (e.g. with `test_dropdown()` and `test_key_rebinding()`) actually correct?**
  _`MenuSystem` has 5 INFERRED edges - model-reasoned connections that need verification._
- **What connects `genVertexArrays`, `bindVertexArray`, `deleteVertexArrays` to the rest of the system?**
  _2171 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `.step` be split into smaller, more focused modules?**
  _Cohesion score 0.14935988620199148 - nodes in this community are weakly interconnected._
- **Should `ControllerProfile` be split into smaller, more focused modules?**
  _Cohesion score 0.07407407407407407 - nodes in this community are weakly interconnected._