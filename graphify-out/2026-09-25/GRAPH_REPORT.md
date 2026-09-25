# Graph Report - Fast Jet Simulator New Version  (2026-09-25)

## Corpus Check
- 149 files · ~253,401 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 4967 nodes · 10300 edges · 198 communities (190 shown, 7 thin omitted)
- Extraction: 94% EXTRACTED · 6% INFERRED · 0% AMBIGUOUS · INFERRED: 622 edges (avg confidence: 0.81)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `915b605d`
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
- jsmntok_t
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
- test_hud_symbology_generation_gl
- ModelGLB
- LandingGear
- Key Technical Specifications
- DrawCmd
- RenderEngine
- test_performance_envelope.cpp
- TerrainMesh
- FlightState
- rules/graphify.md
- workflows/graphify.md
- string
- HdrPipeline
- LayoutContext
- MenuSystem
- AxisCalibration
- stbi__context
- Agent Guidelines & Rules
- BfmPilot
- .evaluate_force_curve
- cgltf_data
- cgltf_buffer_view
- AircraftAeroModel
- cgltf.h
- cgltf_material
- cgltf_primitive
- cgltf_node
- IMUData
- stbi__jpeg
- cgltf_extras
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
- GunSpec
- MenuServices
- Sim
- AtmosphereModel
- World
- vector
- test_throttle_control.cpp
- Widget
- ThreadPool
- FlightSimHarness
- Vector3
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
- .handle
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
- test_missiles.cpp
- ModalSpec
- Writer
- Readings
- Motion
- cgltf_skin
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
- MissileSpec
- SDL3InputDriver
- GunSolution
- WeatherState
- Radii
- EvasionPilot
- draw_list.hpp
- test_ofc_closed_loop.cpp
- .init
- test_evade_mode.cpp
- .screen
- ReferenceLoop
- Airframe
- GunSystem
- AirData
- .relayout
- EngagementSetup
- test_bfm_ai.cpp
- GLBSubmesh
- .from_json
- Atmosphere1976
- WindTurbulenceModel
- MainMenuScreen
- TracerRenderer
- MissileShooter
- PilotSkill
- DamageRng
- FireControlRadar
- Geometry
- Image
- .compute
- RangeTargetPilot
- test_air_combat_geometry.cpp
- FlightStateDeriv
- Matrix3x3
- Flight
- RwrStatus
- EngagementStats
- Projectile
- ClosedLoopSim
- test_atmosphere.cpp
- EvadeProfile
- test_aircraft_entity.cpp
- EulerAngles
- ScreenQuad
- CloudSettings
- stbi__parse_png_file
- FramePacer
- .compute
- AudioSample
- Strut
- RadarSpec
- enum_field
- LevelRun
- .feed
- IJoystickDriver
- FrameStats
- AccessibilitySettings
- LevelFlight
- decode_menu_font_atlas
- HUDVertex
- Traffic

## God Nodes (most connected - your core abstractions)
1. `MenuSystem` - 145 edges
2. `Vector3` - 122 edges
3. `cgltf_data` - 112 edges
4. `FastJetGLFunctions` - 98 edges
5. `FlightState` - 93 edges
6. `RenderEngine` - 93 edges
7. `AircraftMenu` - 89 edges
8. `OnBoardFlightComputer` - 81 edges
9. `World` - 78 edges
10. `cgltf_options` - 74 edges

## Surprising Connections (you probably didn't know these)
- `expect_saved()` --calls--> `to_short_string()`  [INFERRED]
  tests/test_ofc_closed_loop.cpp → include/fastjet/aircraft/aircraft_type.hpp
- `test_tumble_law_respects_airframe_alpha_envelope()` --calls--> `to_short_string()`  [INFERRED]
  tests/test_ofc_closed_loop.cpp → include/fastjet/aircraft/aircraft_type.hpp
- `main()` --calls--> `cgltf_parse_file()`  [INFERRED]
  tests/test_model_glb.cpp → include/fastjet/graphics/cgltf.h
- `main()` --calls--> `cgltf_load_buffers()`  [INFERRED]
  tests/test_model_glb.cpp → include/fastjet/graphics/cgltf.h
- `main()` --calls--> `cgltf_free()`  [INFERRED]
  tests/test_model_glb.cpp → include/fastjet/graphics/cgltf.h

## Import Cycles
- None detected.

## Communities (198 total, 7 thin omitted)

### Community 0 - "Quaternion"
Cohesion: 0.15
Nodes (5): Quaternion, w, x, y, z

### Community 1 - ".step"
Cohesion: 0.16
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
Cohesion: 0.12
Nodes (9): array, size_t, VirtualHOTASDriver, axes_, buttons_, hats_, MAX_AXES, MAX_BUTTONS (+1 more)

### Community 10 - "Actuator"
Cohesion: 0.16
Nodes (13): Actuator, is_pos_limited, is_rate_limited, pos_max, pos_min, position, rate, rate_limit (+5 more)

### Community 11 - "jsmntok_t"
Cohesion: 0.14
Nodes (80): cgltf_calloc(), cgltf_fill_float_array(), cgltf_json_strcmp(), cgltf_json_to_bool(), cgltf_json_to_component_type(), cgltf_json_to_float(), cgltf_json_to_int(), cgltf_json_to_primitive_type() (+72 more)

### Community 12 - "PitchController"
Cohesion: 0.12
Nodes (14): PitchController, ALPHA_LEAD_S, ALPHA_MAX_DEG, integrator, K_alpha, K_alpha_rate, Kd, Ki (+6 more)

### Community 13 - "stb_image.h"
Cohesion: 0.06
Nodes (88): FILE, load_jpeg_image(), resample_row_1(), stbi__blinn_8x8(), stbi__clamp(), stbi__cleanup_jpeg(), stbi__compute_y(), stbi__convert_16_to_8() (+80 more)

### Community 14 - "CameraRig"
Cohesion: 0.06
Nodes (27): CameraMode, CameraRig, chase_distance_, chase_height_, chase_pitch_, chase_yaw_, DEP_X, DEP_Y (+19 more)

### Community 15 - "InputManager"
Cohesion: 0.17
Nodes (12): InputManager, brakes, config, driver, pitch_in, roll_in, speedbrake, throttle (+4 more)

### Community 16 - "ThrottleController"
Cohesion: 0.17
Nodes (12): DetentState, ThrottleController, ab_power, DETENT_CUTOFF, DETENT_IDLE, DETENT_MIL, dry_power, net_thrust_n (+4 more)

### Community 17 - "AircraftMenu"
Cohesion: 0.04
Nodes (52): AircraftMenu, active_tab_, col_accent_gold, col_active_badge, col_alert_red, col_bg_void, col_border_bright, col_border_dim (+44 more)

### Community 18 - "DigitalTrimHat"
Cohesion: 0.13
Nodes (12): DigitalTrimHat, slew_rate, trim_pitch, trim_roll, WheelBrakes, left_brake, parking_brake, right_brake (+4 more)

### Community 19 - "SkyGroundRenderer"
Cohesion: 0.07
Nodes (34): array, GLsizei, GLuint, SkyGroundRenderer, ATMOSPHERE_GLSL, cirrus_tex_, ground_shader_, ground_textures_ (+26 more)

### Community 20 - "SimRunner"
Cohesion: 0.23
Nodes (13): main(), SimRunner, aero_model, current_forces, flcs, integrator, mass, sim_time (+5 more)

### Community 21 - "SettingsScreen"
Cohesion: 0.05
Nodes (60): Accessor, acc, FieldSpec, dynamic_hint, enabled, get, hint, kind (+52 more)

### Community 22 - "test_ground_collision.cpp"
Cohesion: 0.15
Nodes (18): GroundContactPoint, CollisionResult, contact_point, contact_pos_ned, has_collided, penetration_depth, terrain_elev_m, GroundCollision (+10 more)

### Community 23 - "FastJetGLFunctions"
Cohesion: 0.02
Nodes (97): FastJetGLFunctions, activeTexture, attachShader, bindBuffer, bindFramebuffer, bindRenderbuffer, bindVertexArray, blendFuncSeparate (+89 more)

### Community 24 - "CockpitGeometry"
Cohesion: 0.09
Nodes (15): CockpitGeometry, gauge_vao_, gauge_vbo_, gauge_vertex_count_, glass_vao_, glass_vbo_, glass_vertex_count_, initialized_ (+7 more)

### Community 25 - "CockpitGauges"
Cohesion: 0.09
Nodes (22): CockpitGauges, kAmberBand, kDialBrightness, kFace, kGreenBand, kHub, kInk, kInkDim (+14 more)

### Community 26 - "test_hud_symbology_generation_gl"
Cohesion: 0.20
Nodes (8): main(), test_dep_and_equilibrium(), test_g_load_dynamics(), main(), test_fpm_velocity_vector_tracking(), test_hud_conformal_roll_and_boresight_alignment(), test_hud_symbology_generation_gl(), test_optical_infinity_projection()

### Community 27 - "ModelGLB"
Cohesion: 0.06
Nodes (30): F16PartType, BodyBounds, max, min, valid, string, unique_ptr, vector (+22 more)

### Community 28 - "LandingGear"
Cohesion: 0.10
Nodes (20): AircraftType, array, LandingGear, brake_left, brake_right, collapsed, CORNERING_STIFFNESS, CREEP_SPEED (+12 more)

### Community 29 - "Key Technical Specifications"
Cohesion: 0.08
Nodes (23): 10. First-Person Graphics, Collimated HUD & Cockpit Avionics, 11. Threading and Frame Pacing, 12. Evade Mode: Surviving a Missile Shot, 1. Zero-Allocation Architecture, 2. State Vector & Coordinate Frames, 3. Core Equations of Motion, 4. F-16C Mass & Inertia Properties, 5. 1976 US Standard Atmosphere Model (+15 more)

### Community 30 - "DrawCmd"
Cohesion: 0.11
Nodes (18): DrawCmdType, DrawCmd, blur, border, border_width, fill_bottom, fill_top, horizontal (+10 more)

### Community 31 - "RenderEngine"
Cohesion: 0.05
Nodes (42): array, ColorblindMode, vector, RenderEngine, aircraft_menu_, caster_shader_, clouds_, cockpit_ (+34 more)

### Community 32 - "test_performance_envelope.cpp"
Cohesion: 0.36
Nodes (7): main(), test_max_level_speed_at_altitude(), test_max_level_speed_at_sea_level(), test_military_power_is_subsonic_at_altitude(), test_sea_level_static_thrust_matches_f110(), test_thrust_lapse_with_altitude(), test_thrust_to_weight_ratio()

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

### Community 39 - "LayoutContext"
Cohesion: 0.07
Nodes (22): LayoutContext, pointer_nx, pointer_ny, s, scroll_y, time, viewport, CreditsScreen (+14 more)

### Community 40 - "MenuSystem"
Cohesion: 0.04
Nodes (45): array, deque, optional, QualityPreset, SubscriptionId, unique_ptr, MenuSystem, actions_ (+37 more)

### Community 41 - "AxisCalibration"
Cohesion: 0.22
Nodes (8): AxisCalibration, curvature, inner_deadband, inverted, outer_deadband, raw_center, raw_max, raw_min

### Community 42 - "stbi__context"
Cohesion: 0.11
Nodes (64): stbi__at_eof(), stbi__bmp_info(), stbi__bmp_test(), stbi__bmp_test_raw(), stbi__check_png_header(), stbi__convert_format(), stbi__get16be(), stbi__get16le() (+56 more)

### Community 44 - "BfmPilot"
Cohesion: 0.03
Nodes (59): AirCombatGeometry, aspect, ata, closure, hca, los_b, los_ned, range (+51 more)

### Community 45 - ".evaluate_force_curve"
Cohesion: 0.67
Nodes (4): main(), test_curvature_tuning(), test_force_curve_endpoints_and_symmetry(), test_strict_monotonicity()

### Community 46 - "cgltf_data"
Cohesion: 0.03
Nodes (74): cgltf_camera_type, cgltf_file_type, cgltf_camera, data, extensions, extensions_count, extras, cgltf_camera_index() (+66 more)

### Community 47 - "cgltf_buffer_view"
Cohesion: 0.05
Nodes (44): cgltf_buffer_view_type, cgltf_data_free_method, cgltf_meshopt_compression_filter, cgltf_meshopt_compression_mode, cgltf_accessor_sparse, count, indices_buffer_view, indices_byte_offset (+36 more)

### Community 48 - "AircraftAeroModel"
Cohesion: 0.09
Nodes (20): AircraftAeroModel, aircraft_type, config, AircraftType, ControlSurfaces, delta_a, delta_e, delta_lef (+12 more)

### Community 49 - "cgltf.h"
Cohesion: 0.08
Nodes (65): cgltf_bool, cgltf_component_type, cgltf_result, cgltf_size, cgltf_ssize, cgltf_type, cgltf_uint, cgltf_accessor (+57 more)

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

### Community 55 - "cgltf_extras"
Cohesion: 0.04
Nodes (64): cgltf_animation_path_type, cgltf_interpolation_type, cgltf_animation, cgltf_animation_channel, extensions, extensions_count, extras, cgltf_animation_channel_index() (+56 more)

### Community 56 - "AvionicsTelemetry"
Cohesion: 0.05
Nodes (39): AvionicsTelemetry, aircraft_switch_timer, aircraft_type, bingo_fuel_alert, brake_left, brake_right, combat, detent_str (+31 more)

### Community 57 - "AudioEngine"
Cohesion: 0.06
Nodes (31): AudioEngine, alerts_bus_gain_, CHANNELS, device_id_, enabled_, engine_bus_gain_, initialized_, kMixChunkFrames (+23 more)

### Community 58 - "Color4"
Cohesion: 0.08
Nodes (21): Color4, a, b, g, r, GLuint, vector, HUDCollimator (+13 more)

### Community 59 - "interpolator.hpp"
Cohesion: 0.23
Nodes (14): find_index_and_weight(), N, size_t, interp1d(), interp2d(), interp3d(), Interpolator, NX (+6 more)

### Community 60 - "DrawList"
Cohesion: 0.08
Nodes (26): DrawList, cmds_, layers_, Layer, base_x, base_y, opacity, pivot_x (+18 more)

### Community 61 - "stbi__bmp_load"
Cohesion: 0.15
Nodes (22): stbi__addsizes_valid(), stbi__bitcount(), stbi__bmp_load(), stbi__bmp_parse_header(), stbi__bmp_set_mask_defaults(), stbi__create_png_alpha_expand8(), stbi__create_png_image(), stbi__create_png_image_raw() (+14 more)

### Community 62 - "cgltf_texture"
Cohesion: 0.06
Nodes (33): cgltf_filter_type, cgltf_wrap_mode, cgltf_image, buffer_view, extensions, extensions_count, extras, cgltf_image_index() (+25 more)

### Community 63 - "MultiEngine"
Cohesion: 0.12
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

### Community 69 - "GunSpec"
Cohesion: 0.12
Nodes (17): GunSpec, boresight_elev_deg, calibre_m, dispersion_mrad, drag_coeff, lethality, muzzle_b, muzzle_velocity_mps (+9 more)

### Community 70 - "MenuServices"
Cohesion: 0.08
Nodes (19): WidgetId, MenuServices, begin_key_capture, emit, focused, in_flight, is_capturing, key_name (+11 more)

### Community 71 - "Sim"
Cohesion: 0.10
Nodes (18): F16FLCS, flaperon, lat_dir_ctrl, pitch_ctrl, rudder, stabilator, Sim, aero (+10 more)

### Community 72 - "AtmosphereModel"
Cohesion: 0.09
Nodes (24): AtmosphereModel, EXPOSURE, MIE, MIE_G, MIN_MU, PI, RAYLEIGH, SCALE_H (+16 more)

### Community 73 - "World"
Cohesion: 0.04
Nodes (60): segment_segment_distance(), AircraftControls, brake_left, brake_right, dispense, speedbrake_out, stick, throttle (+52 more)

### Community 74 - "vector"
Cohesion: 0.16
Nodes (5): array, string, vector, apply_texture_anisotropy(), max_texture_anisotropy()

### Community 75 - "test_throttle_control.cpp"
Cohesion: 0.15
Nodes (11): SpeedbrakeController, position, slew_rate, SwitchPosition, main(), test_afterburner_accelerates(), test_speedbrake_adds_deceleration(), test_speedbrake_slews_not_snaps() (+3 more)

### Community 76 - "Widget"
Cohesion: 0.07
Nodes (42): ButtonVariant, string, WidgetId, WidgetKind, is_adjustable(), LegendItem, action, keys (+34 more)

### Community 77 - "ThreadPool"
Cohesion: 0.13
Nodes (21): condition_variable, F, future, deque, function, vector, parallel_for(), submit() (+13 more)

### Community 78 - "FlightSimHarness"
Cohesion: 0.18
Nodes (11): AircraftType, FlightSimHarness, aero, engine, flcs, forces, integrator, mass (+3 more)

### Community 79 - "Vector3"
Cohesion: 0.05
Nodes (46): FlyoutTarget, array, operator*(), Vector3, x, y, z, EnergyState (+38 more)

### Community 80 - "AeroConfig"
Cohesion: 0.11
Nodes (19): AeroConfig, aspect_ratio, b_span, c_bar, cd0, cd_speedbrake, cd_wave_peak, cd_wave_super (+11 more)

### Community 81 - "test_render_fidelity.cpp"
Cohesion: 0.16
Nodes (21): camera_rig_, hdr_, shadow_map_, QualityPreset, apply(), array, vector, Frame (+13 more)

### Community 82 - "DropdownState"
Cohesion: 0.06
Nodes (30): CaptureState, anchor, error, on_clear, on_key, title, DropdownState, anchor (+22 more)

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
Cohesion: 0.09
Nodes (22): DisplayMode, FrameRateCap, apply_video(), string, vector, key_name(), query_resolutions(), video_differs() (+14 more)

### Community 88 - "InputEvent"
Cohesion: 0.11
Nodes (22): NavCommand, Type, InputEvent, nav, scancode, scroll, source, type (+14 more)

### Community 89 - "Palette"
Cohesion: 0.07
Nodes (30): Palette, accent, accent_strong, accent_strong_hover, backdrop, border_bright, border_strong, border_subtle (+22 more)

### Community 90 - "UiCanvas"
Cohesion: 0.13
Nodes (14): vector, UiCanvas, clip_stack_, fb_h_, fb_w_, font_tex_, initialized_, kAaPad (+6 more)

### Community 91 - ".handle"
Cohesion: 0.28
Nodes (16): widgets_, click(), NavCommand, WidgetId, find(), focus_down_to(), frames(), main() (+8 more)

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
Cohesion: 0.24
Nodes (9): optional, string, string_view, parse(), Parser, error_, kMaxDepth, pos_ (+1 more)

### Community 96 - "ShadowMap"
Cohesion: 0.07
Nodes (19): FrameContext, clouds, eye_ned, ground_shadow, hdr_output, kShadowUnit, lighting, shadow (+11 more)

### Community 97 - "ControlSettings"
Cohesion: 0.15
Nodes (12): is_down(), ControlSettings, bindings, invert_mouse_y, invert_pitch, mouse_sensitivity, stick_sensitivity, array (+4 more)

### Community 98 - "test_multi_aircraft_performance.cpp"
Cohesion: 0.23
Nodes (18): engine_designation(), AircraftType, string_view, parse_aircraft_type(), signature_spec(), to_short_string(), to_string(), main() (+10 more)

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
Cohesion: 0.13
Nodes (7): Array, Type, Value, data_, nullptr_t, Object, variant

### Community 104 - "LateralDirectionalController"
Cohesion: 0.17
Nodes (9): LateralDirectionalController, ALPHA_ATTEN_MAX, ALPHA_ATTEN_MIN, K_beta, K_roll, K_yaw_damper, Ki_roll, P_CMD_MAX_DPS (+1 more)

### Community 105 - "Vertex3D"
Cohesion: 0.40
Nodes (5): Vertex3D, color, normal, pos, uv

### Community 106 - "main"
Cohesion: 0.11
Nodes (6): gpu_profiler_, FrameLimiter, next_ns_, string, main(), Uint64

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
Cohesion: 0.14
Nodes (17): Fn, toast_text_, edit(), pair, SettingsSection, SubscriptionId, vector, SettingsManager (+9 more)

### Community 111 - "test_missiles.cpp"
Cohesion: 0.14
Nodes (23): MissileBody, a_lat, pos, tof, vel, array, unique_ptr, cruise() (+15 more)

### Community 112 - "ModalSpec"
Cohesion: 0.11
Nodes (17): AppInfo, title, version, ButtonVariant, function, string, vector, ModalButton (+9 more)

### Community 113 - "Writer"
Cohesion: 0.24
Nodes (4): serialize(), Writer, indent_, out_

### Community 114 - "Readings"
Cohesion: 0.06
Nodes (33): Readings, alt_ft, aoa_deg, blink, cabin_alt_ft, cdi_dots, config, dme_nm (+25 more)

### Community 115 - "Motion"
Cohesion: 0.11
Nodes (18): Motion, background_drift, focus, menu_open, modal, modal_scale_from, parallax_far, parallax_near (+10 more)

### Community 116 - "cgltf_skin"
Cohesion: 0.20
Nodes (10): cgltf_skin, extensions, extensions_count, extras, cgltf_skin_index(), inverse_bind_matrices, joints, joints_count (+2 more)

### Community 117 - "UserSettings"
Cohesion: 0.11
Nodes (12): AudioSettings, alerts, engine, master, muted, SettingsSection, UserSettings, accessibility (+4 more)

### Community 118 - "theme.hpp"
Cohesion: 0.18
Nodes (14): apply_colorblind_palette(), build_theme(), ease_in_cubic(), Elevation, modal, panel, raised, ColorblindMode (+6 more)

### Community 119 - "5. Phases"
Cohesion: 0.08
Nodes (25): 0. Status and revisions (2026-09-24), 1. Design pillars, 2. What already exists to build on, 3. Gaps, 4. Architecture, 5. Phases, 6. Milestones, 7. Testing strategy (+17 more)

### Community 120 - "AircraftFLCS"
Cohesion: 0.08
Nodes (23): FLCSConfig, alpha_limit_deg, law_type, max_g_negative, max_g_positive, max_roll_rate_dps, AircraftFLCS, aileron (+15 more)

### Community 121 - "SettingsStorage"
Cohesion: 0.18
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

### Community 125 - "AircraftConfig"
Cohesion: 0.07
Nodes (28): AircraftConfig, aero, display_name, flcs, gear, mass, propulsion, type (+20 more)

### Community 126 - "Aircraft"
Cohesion: 0.09
Nodes (22): Aircraft, aero_model, crashed, damage, DT, engine, flcs, forces (+14 more)

### Community 127 - "DialSpec"
Cohesion: 0.11
Nodes (18): Band, col, v0, v1, DialSpec, bands, label_size, labels (+10 more)

### Community 128 - "DamageState"
Cohesion: 0.08
Nodes (24): DamageState, destroyed, engine_count, ENGINE_DAMAGE_PER_HIT, engine_fire, ENGINE_FIRE_CHANCE, engine_health, FIRE_BURN_THROUGH_S (+16 more)

### Community 129 - "CombatTelemetry"
Cohesion: 0.05
Nodes (38): CombatTelemetry, active, ammo, aspect_deg, ata_deg, banner, blink, chaff (+30 more)

### Community 130 - "Spacing"
Cohesion: 0.20
Nodes (10): Spacing, lg, md, page, sm, xl, xs, xxl (+2 more)

### Community 131 - "Typography"
Cohesion: 0.20
Nodes (10): Typography, body, caption, display, display_sub, heading, label, title (+2 more)

### Community 132 - "FlightInstruments"
Cohesion: 0.09
Nodes (17): Cell, emissive, h, w, x, y, Gauge, FlightInstruments (+9 more)

### Community 133 - "Engagement"
Cohesion: 0.09
Nodes (23): Engagement, ai, ai_controlled, BANDIT, chaff_at_start_, end_time, ESCAPE_HOLD_S, ESCAPE_RANGE_M (+15 more)

### Community 134 - "Vertex"
Cohesion: 0.25
Nodes (8): Vertex, border, fill, half, local, params, pos, uv

### Community 135 - "MissileSpec"
Cohesion: 0.07
Nodes (27): MissileSpec, arm_time_s, autopilot_tau_s, burn_time_s, burnout_mass_kg, chaff_seduction, cl_max, diameter_m (+19 more)

### Community 136 - "SDL3InputDriver"
Cohesion: 0.13
Nodes (8): string, vector, SDL3InputDriver, initialized_, joysticks_, SDL_Joystick, main(), test_sdl3_driver_initialization()

### Community 137 - "GunSolution"
Cohesion: 0.22
Nodes (9): GunSolution, lead_dir_b, MAX_RANGE_M, miss_distance, miss_ned, pipper_ned, range, time_of_flight (+1 more)

### Community 138 - "WeatherState"
Cohesion: 0.25
Nodes (7): WeatherState, cirrus_cover, cumulus_base_m, cumulus_coverage, cumulus_top_m, wind_east_mps, wind_north_mps

### Community 139 - "Radii"
Cohesion: 0.40
Nodes (5): Radii, lg, md, pill, sm

### Community 140 - "EvasionPilot"
Cohesion: 0.11
Nodes (17): EvasionPilot, beam_side_, break_time_to_go_s, chaff_interval_s, chaff_timer_, cruise_alt_, cruise_heading_, floor_agl_m (+9 more)

### Community 141 - "draw_list.hpp"
Cohesion: 0.16
Nodes (11): baseline_for_center(), size, fit_text(), string, string_view, vector, text_width(), wrap_text() (+3 more)

### Community 142 - "test_ofc_closed_loop.cpp"
Cohesion: 0.24
Nodes (14): function, expect_saved(), main(), Outcome, crashed, gcas_s, max_nz, min_agl (+6 more)

### Community 144 - "test_evade_mode.cpp"
Cohesion: 0.21
Nodes (21): EngagementOutcome, EvadeDifficulty, evade(), fly_defended(), fly_level(), lost(), main(), Result (+13 more)

### Community 146 - "ReferenceLoop"
Cohesion: 0.12
Nodes (17): AircraftType, ReferenceLoop, aero_model, current_forces, current_imu, engine, flight_control_system, integrator (+9 more)

### Community 147 - "Airframe"
Cohesion: 0.17
Nodes (9): Airframe, alpha_limit_deg, cl_max, g_command_law, max_g, ref_mass_kg, roll_rate_dps, wing_area_m2 (+1 more)

### Community 148 - "GunSystem"
Cohesion: 0.13
Nodes (19): gun_spec(), AircraftType, GunSystem, accumulator, ammo, rounds_fired, spec, spin (+11 more)

### Community 149 - "AirData"
Cohesion: 0.20
Nodes (9): AirData, density, dynamic_pressure, geometric_altitude, geopotential_altitude, mach_number, pressure, speed_of_sound (+1 more)

### Community 150 - ".relayout"
Cohesion: 0.15
Nodes (9): SettingsSection, Transition, active, direction, phase_out, t, to, step (+1 more)

### Community 151 - "EngagementSetup"
Cohesion: 0.10
Nodes (20): EngagementMode, EngagementSetup, altitude_m, bandit_type, evade_difficulty, geometry, hard_deck_agl_m, mode (+12 more)

### Community 152 - "test_bfm_ai.cpp"
Cohesion: 0.18
Nodes (16): AiSkill, EngagementOutcome, FightResult, hits_scored, midairs, outcome, peak_nz, time (+8 more)

### Community 153 - "GLBSubmesh"
Cohesion: 0.09
Nodes (22): GLBSubmesh, base_color_factor, base_texture, bounds_max, bounds_min, ebo, emissive, index_count (+14 more)

### Community 154 - ".from_json"
Cohesion: 0.19
Nodes (12): InputAction, optional, path, string, main(), scratch_dir(), test_binding_conflicts(), test_defaults_round_trip() (+4 more)

### Community 156 - "Atmosphere1976"
Cohesion: 0.25
Nodes (8): Atmosphere1976, G0, GAMMA, P0, R_EARTH, R_GAS, RHO0, T0

### Community 157 - "WindTurbulenceModel"
Cohesion: 0.16
Nodes (8): WindTurbulenceModel, base_wind_speed_mps, gust_u_, gust_v_, gust_w_, rng_state_, turbulence_enabled, wind_heading_rad

### Community 158 - "MainMenuScreen"
Cohesion: 0.10
Nodes (14): Geometry, buttons_top, chip_h, chip_y, left, overline_cy, rule_y, subtitle_baseline (+6 more)

### Community 159 - "TracerRenderer"
Cohesion: 0.16
Nodes (7): GLuint, vector, TracerRenderer, shader_, vao_, vbo_, verts_

### Community 160 - "MissileShooter"
Cohesion: 0.11
Nodes (18): FlyoutResult, hit, miss_m, terminal_speed_mps, time_s, LaunchDoctrine, MissileShooter, confirm_lock_s (+10 more)

### Community 161 - "PilotSkill"
Cohesion: 0.15
Nodes (11): AiSkill, PilotSkill, aim_noise_mrad, assess_period_s, fire_miss_m, g_fraction, greyout_limit, max_fire_range_m (+3 more)

### Community 162 - "DamageRng"
Cohesion: 0.40
Nodes (4): DamageRng, state, HitZone, to_string()

### Community 163 - "FireControlRadar"
Cohesion: 0.11
Nodes (14): FireControlRadar, acq_timer, acquire_s, coast_timer, coasting, last_check, lock_time, mode (+6 more)

### Community 164 - "Geometry"
Cohesion: 0.33
Nodes (6): Geometry, footer, header, panel, tabs, view

### Community 165 - "Image"
Cohesion: 0.50
Nodes (4): Image, height, pixels, width

### Community 166 - ".compute"
Cohesion: 0.35
Nodes (8): main(), test_altitude_lapse(), test_fuel_burn_and_flameout(), test_ram_recovery(), test_sea_level_static_ratings(), test_spool_dynamics(), test_variable_mass(), test_detent_boundaries()

### Community 167 - "RangeTargetPilot"
Cohesion: 0.20
Nodes (10): RangeTargetPilot, bank_deg, profile, target_altitude_m, target_speed_mps, throttle_, time_, trim_ (+2 more)

### Community 168 - "test_air_combat_geometry.cpp"
Cohesion: 0.58
Nodes (9): jet(), main(), near(), test_beam(), test_dead_six(), test_head_on(), test_quarter_aspect(), test_segment_distance() (+1 more)

### Community 169 - "FlightStateDeriv"
Cohesion: 0.22
Nodes (6): FlightStateDeriv, d_omega_b, d_pos_ned, d_q_att, d_vel_b, operator*()

### Community 171 - "Flight"
Cohesion: 0.13
Nodes (13): RK4Integrator, DEFAULT_DT, dt, Flight, aero, engine, flcs, forces (+5 more)

### Community 172 - "RwrStatus"
Cohesion: 0.14
Nodes (11): RwrStatus, emitter_bearing, emitter_symbol, emitter_valid, level, missile_bearing, missile_range_m, missile_seeker_active (+3 more)

### Community 173 - "EngagementStats"
Cohesion: 0.14
Nodes (14): EngagementStats, chaff_used, duration_s, hits_scored, hits_taken, locks_broken_by_chaff, min_agl_m, missile_detonations (+6 more)

### Community 174 - "Projectile"
Cohesion: 0.25
Nodes (8): Projectile, active, age, k_drag, lethality, owner, pos, vel

### Community 175 - "ClosedLoopSim"
Cohesion: 0.14
Nodes (14): ClosedLoopSim, aero, cfg, engine, flcs, forces, gear_down, integrator (+6 more)

### Community 176 - "test_atmosphere.cpp"
Cohesion: 0.52
Nodes (5): main(), test_sea_level(), test_stratosphere_25000m(), test_tropopause_11000m(), test_troposphere_5000m()

### Community 177 - "EvadeProfile"
Cohesion: 0.14
Nodes (14): EvadeProfile, altitude_advantage_m, doctrine, missiles, off_tail_deg, pilot, radar_acquire_s, shoot_shoot (+6 more)

### Community 178 - "test_aircraft_entity.cpp"
Cohesion: 0.39
Nodes (7): Script, compare(), main(), same(), test_airborne_manoeuvring_is_bit_identical(), test_takeoff_roll_is_bit_identical(), test_undamaged_jet_ignores_damage_model()

### Community 179 - "EulerAngles"
Cohesion: 0.33
Nodes (4): EulerAngles, pitch, roll, yaw

### Community 180 - "ScreenQuad"
Cohesion: 0.21
Nodes (10): array, GLsizei, vector, ScreenQuad, brightness, corners, u0, u1 (+2 more)

### Community 181 - "CloudSettings"
Cohesion: 0.15
Nodes (12): CloudSettings, base_m, coverage, kMapUnit, map_inv_size, map_origin_x, map_origin_y, map_texture (+4 more)

### Community 182 - "stbi__parse_png_file"
Cohesion: 0.26
Nodes (13): stbi__compute_transparency(), stbi__compute_transparency16(), stbi__compute_y_16(), stbi__convert_format16(), stbi__de_iphone(), stbi__do_png(), stbi__expand_png_palette(), stbi__get_chunk_header() (+5 more)

### Community 183 - "FramePacer"
Cohesion: 0.18
Nodes (8): GLsync, FramePacer, fences_, kMaxFramesInFlight, kWaitStepNs, slot_, array, uint64_t

### Community 184 - ".compute"
Cohesion: 0.44
Nodes (8): main(), make_ground_state(), test_braking_decelerates(), test_gear_collapse_on_hard_landing(), test_no_force_when_airborne(), test_nosewheel_steering(), test_retracted_gear_inert(), test_static_equilibrium()

### Community 185 - "AudioSample"
Cohesion: 0.18
Nodes (8): AudioSample, current_gain, current_rate, is_playing, loaded, loop, pcm, playhead

### Community 186 - "Strut"
Cohesion: 0.18
Nodes (10): Strut, braked, compression, damping_c, in_contact, max_stroke, pos_b, rest_length (+2 more)

### Community 187 - "RadarSpec"
Cohesion: 0.20
Nodes (10): AircraftType, radar_spec(), RadarSpec, chaff_seduction, fitted, gimbal_deg, memory_s, notch_mps (+2 more)

### Community 188 - "enum_field"
Cohesion: 0.24
Nodes (9): ActionInfo, defaults, json_key, enum_field(), E, N, string, string_view (+1 more)

### Community 189 - "LevelRun"
Cohesion: 0.20
Nodes (10): LevelRun, aero, alt_target, engine, flcs, forces, integrator, mass (+2 more)

### Community 190 - ".feed"
Cohesion: 0.25
Nodes (4): StreamLock, s, SDL_AudioStream, test_resting_hardware_lever_does_not_capture_throttle()

### Community 191 - "IJoystickDriver"
Cohesion: 0.22
Nodes (8): IJoystickDriver, get_axis, get_button, get_device_count, get_device_name, get_hat, initialize, poll

### Community 192 - "FrameStats"
Cohesion: 0.25
Nodes (7): vector, FrameStats, elapsed_s, frame_ms, physics_ms, present_ms, render_ms

### Community 193 - "AccessibilitySettings"
Cohesion: 0.29
Nodes (6): AccessibilitySettings, colorblind, high_contrast, reduce_motion, ui_scale, ColorblindMode

### Community 194 - "LevelFlight"
Cohesion: 0.50
Nodes (4): LevelFlight, alt, throttle, trim

### Community 196 - "HUDVertex"
Cohesion: 0.67
Nodes (3): HUDVertex, color, pos

### Community 197 - "Traffic"
Cohesion: 0.67
Nodes (3): Traffic, gear_down, state

## Knowledge Gaps
- **2365 isolated node(s):** `genVertexArrays`, `bindVertexArray`, `deleteVertexArrays`, `genBuffers`, `bindBuffer` (+2360 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 2732 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **7 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `Vector3` connect `Vector3` to `Quaternion`, `.step`, `GunSolution`, `CameraRig`, `SkyGroundRenderer`, `GunSystem`, `test_ground_collision.cpp`, `EngagementSetup`, `test_hud_symbology_generation_gl`, `ModelGLB`, `aircraft_type.hpp`, `WindTurbulenceModel`, `RenderEngine`, `FlightState`, `FireControlRadar`, `FlightStateDeriv`, `Matrix3x3`, `BfmPilot`, `RwrStatus`, `Projectile`, `test_aircraft_entity.cpp`, `Strut`, `CloudLayer`, `MassProperties`, `GunSpec`, `World`, `vector`, `.get`, `ShadowMap`, `main`, `test_missiles.cpp`, `AircraftConfig`?**
  _High betweenness centrality (0.136) - this node is a cross-community bridge._
- **Why does `RenderEngine` connect `RenderEngine` to `FlightInstruments`, `WeatherState`, `CameraRig`, `.init`, `AircraftMenu`, `SkyGroundRenderer`, `CockpitGeometry`, `CockpitGauges`, `ModelGLB`, `TracerRenderer`, `FlightState`, `HdrPipeline`, `AvionicsTelemetry`, `Color4`, `CloudLayer`, `Traffic`, `vector`, `Vector3`, `test_render_fidelity.cpp`, `UiCanvas`, `GpuProfiler`, `ShadowMap`, `ExhaustPlume`, `ShaderProgram`, `main`, `ColorFilterPass`, `Theme`, `RenderQuality`?**
  _High betweenness centrality (0.127) - this node is a cross-community bridge._
- **Why does `FlightState` connect `FlightState` to `Quaternion`, `.step`, `InstrumentCanvas`, `ReferenceLoop`, `SimRunner`, `GunSystem`, `test_ground_collision.cpp`, `EngagementSetup`, `CockpitGauges`, `test_hud_symbology_generation_gl`, `aircraft_type.hpp`, `test_air_combat_geometry.cpp`, `FlightStateDeriv`, `Flight`, `BfmPilot`, `RwrStatus`, `ClosedLoopSim`, `AircraftAeroModel`, `test_aircraft_entity.cpp`, `EulerAngles`, `IMUData`, `.compute`, `Color4`, `LevelRun`, `MassProperties`, `Traffic`, `GunSpec`, `Sim`, `World`, `FlightSimHarness`, `Vector3`, `SimHarness`, `ShadowMap`, `main`, `.update`, `.compute_coefficients`, `test_missiles.cpp`, `AircraftFLCS`, `Aircraft`?**
  _High betweenness centrality (0.100) - this node is a cross-community bridge._
- **Are the 5 inferred relationships involving `MenuSystem` (e.g. with `test_dropdown()` and `test_key_rebinding()`) actually correct?**
  _`MenuSystem` has 5 INFERRED edges - model-reasoned connections that need verification._
- **What connects `genVertexArrays`, `bindVertexArray`, `deleteVertexArrays` to the rest of the system?**
  _2365 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Quaternion` be split into smaller, more focused modules?**
  _Cohesion score 0.14619883040935672 - nodes in this community are weakly interconnected._
- **Should `ControllerProfile` be split into smaller, more focused modules?**
  _Cohesion score 0.07407407407407407 - nodes in this community are weakly interconnected._