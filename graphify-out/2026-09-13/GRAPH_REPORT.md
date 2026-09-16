# Graph Report - Fast-Jet-Simulator-main  (2026-09-13)

## Corpus Check
- 88 files · ~136,073 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 2268 nodes · 4988 edges · 106 communities (100 shown, 6 thin omitted)
- Extraction: 97% EXTRACTED · 3% INFERRED · 0% AMBIGUOUS · INFERRED: 163 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Community Hubs (Navigation)
- Quaternion
- MassProperties
- ControllerProfile
- TP1538Tables
- Flight
- .process_bipolar
- F110Engine
- F16AeroModel
- f16_aero_model.hpp
- VirtualHOTASDriver
- Actuator
- interpolator.hpp
- PitchController
- WheelBrakes
- CameraRig
- InputManager
- ThrottleController
- IMUData
- DigitalTrimHat
- SkyGroundRenderer
- SimRunner
- F16FLCS
- LateralDirectionalController
- PilotCommands
- CockpitGeometry
- FlightInstruments
- HUDCollimator
- ShaderProgram
- LandingGear
- Key Technical Specifications
- Sim
- RenderEngine
- LevelRun
- TerrainMesh
- Mat4
- rules/graphify.md
- workflows/graphify.md
- string
- FlightState
- Vector3
- IJoystickDriver
- AxisCalibration
- render
- Agent Guidelines & Rules
- .compute_infinity_view_matrix
- .evaluate_force_curve
- FlightStateDeriv
- Matrix3x3
- EulerAngles
- SpeedbrakeController
- cgltf_material
- cgltf_primitive
- cgltf_node
- cgltf_extras
- stbi__jpeg
- cgltf_extension
- AvionicsTelemetry
- AudioEngine
- Color4
- cgltf_buffer_view
- cgltf_buffer
- stbi__bmp_load
- cgltf_texture
- MultiEngine
- f16_sim_viewer.cpp
- flight_state.hpp
- PropulsionConfig
- stbi__err
- MassProperties
- WindTurbulenceModel
- SDL3InputDriver
- LevelRun
- cgltf_texture_view
- stbi__parse_png_file
- cgltf_sampler
- AudioSample
- AirData
- cgltf_mesh
- FlightSimHarness
- Flight
- .compute
- cgltf_skin
- test_integrated_flight.cpp
- cgltf_camera
- .build_symbology
- test_multi_aircraft_performance.cpp
- test_aircraft_specs.cpp
- Atmosphere1976
- cgltf_scene
- test_atmosphere.cpp
- cgltf_iridescence
- cgltf_clearcoat
- cgltf_pbr_metallic_roughness
- cgltf_pbr_specular_glossiness
- main
- cgltf_sheen
- cgltf_specular
- cgltf_volume
- Vertex3D
- test_full_render_pipeline
- Fast Jet Simulator - Authentic Audio Asset Directory
- cgltf_anisotropy
- InstVertex
- string
- FlightStateDeriv
- MassConfig

## God Nodes (most connected - your core abstractions)
1. `cgltf_data` - 111 edges
2. `FastJetGLFunctions` - 90 edges
3. `cgltf_options` - 74 edges
4. `cgltf_material` - 64 edges
5. `FlightState` - 49 edges
6. `CameraRig` - 49 edges
7. `cgltf_skip_json()` - 45 edges
8. `FlightInstruments` - 44 edges
9. `cgltf_json_strcmp()` - 43 edges
10. `cgltf_node` - 41 edges

## Surprising Connections (you probably didn't know these)
- `main()` --calls--> `cgltf_parse_file()`  [INFERRED]
  tests/test_model_glb.cpp → include/fastjet/graphics/cgltf.h
- `main()` --calls--> `cgltf_load_buffers()`  [INFERRED]
  tests/test_model_glb.cpp → include/fastjet/graphics/cgltf.h
- `main()` --calls--> `cgltf_free()`  [INFERRED]
  tests/test_model_glb.cpp → include/fastjet/graphics/cgltf.h
- `main()` --calls--> `camera_rig_`  [INFERRED]
  src/f16_sim_viewer.cpp → include/fastjet/graphics/render_engine.hpp
- `FlightSimHarness` --references--> `AircraftAeroModel`  [EXTRACTED]
  tests/test_multi_aircraft_performance.cpp → include/fastjet/aero/aircraft_aero_model.hpp

## Import Cycles
- None detected.

## Communities (106 total, 6 thin omitted)

### Community 0 - "Quaternion"
Cohesion: 0.09
Nodes (8): Matrix3x3, m, EulerAngles, Quaternion, w, x, y, z

### Community 1 - "MassProperties"
Cohesion: 0.17
Nodes (17): EulerAngles, main(), test_horizontal_ballistic_flight(), test_inclined_ballistic_flight_arbitrary_attitude(), test_steady_level_trim_flight(), test_hands_off_level_flight(), main(), test_high_speed_9g_pull() (+9 more)

### Community 2 - "ControllerProfile"
Cohesion: 0.07
Nodes (27): ControllerProfile, btn_parking_brake, btn_speedbrake_extend, btn_speedbrake_retract, btn_trim_down, btn_trim_left, btn_trim_right, btn_trim_up (+19 more)

### Community 3 - "TP1538Tables"
Cohesion: 0.09
Nodes (31): array, size_t, idx3d(), init_CD_table(), init_CL_roll_table(), init_CL_table(), init_CM_table(), init_CN_yaw_table() (+23 more)

### Community 4 - "Flight"
Cohesion: 0.35
Nodes (9): main(), test_afterburner_accelerates(), test_detent_boundaries(), test_resting_hardware_lever_does_not_capture_throttle(), test_speedbrake_adds_deceleration(), test_speedbrake_slews_not_snaps(), test_status_cadence_is_time_based(), test_throttle_back_decelerates() (+1 more)

### Community 5 - ".process_bipolar"
Cohesion: 0.70
Nodes (4): main(), test_axis_calibration_and_inversion(), test_deadband_filtering(), test_unipolar_axis_conditioning()

### Community 6 - "F110Engine"
Cohesion: 0.09
Nodes (18): F110Engine, commanded_thrust_n, fuel_flow_kgs, fuel_kg, INTERNAL_FUEL_CAPACITY_KG, JP8_DENSITY, K_RAM_AB, K_RAM_DRY (+10 more)

### Community 7 - "F16AeroModel"
Cohesion: 0.07
Nodes (40): F16AeroModel, B_SPAN, B_SPAN_FT, C_BAR, C_BAR_FT, CD_Q_RISE, CD_SPEEDBRAKE_FULL, CD_SUPERSONIC_RISE (+32 more)

### Community 9 - "VirtualHOTASDriver"
Cohesion: 0.12
Nodes (9): array, size_t, VirtualHOTASDriver, axes_, buttons_, hats_, MAX_AXES, MAX_BUTTONS (+1 more)

### Community 10 - "Actuator"
Cohesion: 0.07
Nodes (31): FLCSConfig, alpha_limit_deg, law_type, max_g_negative, max_g_positive, max_roll_rate_dps, Actuator, is_pos_limited (+23 more)

### Community 11 - "interpolator.hpp"
Cohesion: 0.09
Nodes (113): cgltf_result, cgltf_calloc(), cgltf_combine_paths(), cgltf_copy_extras_json(), cgltf_decode_string(), cgltf_decode_uri(), cgltf_default_file_read(), cgltf_dispersion (+105 more)

### Community 12 - "PitchController"
Cohesion: 0.13
Nodes (13): PitchController, ALPHA_MAX_DEG, integrator, K_alpha, K_alpha_rate, Kd, Ki, Kp (+5 more)

### Community 13 - "WheelBrakes"
Cohesion: 0.06
Nodes (88): FILE, load_jpeg_image(), resample_row_1(), stbi__blinn_8x8(), stbi__clamp(), stbi__cleanup_jpeg(), stbi__compute_y(), stbi__convert_16_to_8() (+80 more)

### Community 14 - "CameraRig"
Cohesion: 0.06
Nodes (26): CameraMode, CameraRig, chase_distance_, chase_height_, chase_pitch_, chase_yaw_, DEP_X, DEP_Y (+18 more)

### Community 15 - "InputManager"
Cohesion: 0.17
Nodes (12): InputManager, brakes, config, driver, pitch_in, roll_in, speedbrake, throttle (+4 more)

### Community 16 - "ThrottleController"
Cohesion: 0.17
Nodes (12): DetentState, ThrottleController, ab_power, DETENT_CUTOFF, DETENT_IDLE, DETENT_MIL, dry_power, net_thrust_n (+4 more)

### Community 17 - "IMUData"
Cohesion: 0.08
Nodes (17): AircraftMenu, initialized_, is_open_, line_verts_, selected_index_, shader_, triangle_verts_, vao_lines_ (+9 more)

### Community 18 - "DigitalTrimHat"
Cohesion: 0.16
Nodes (9): SpeedbrakeController, position, slew_rate, WheelBrakes, left_brake, parking_brake, right_brake, SwitchPosition (+1 more)

### Community 19 - "SkyGroundRenderer"
Cohesion: 0.10
Nodes (27): GLsizei, GLuint, SkyGroundRenderer, ATMOSPHERE_GLSL, ground_shader_, ground_vao_, ground_vbo_, ground_vertex_count_ (+19 more)

### Community 20 - "SimRunner"
Cohesion: 0.23
Nodes (13): main(), SimRunner, aero_model, current_forces, flcs, integrator, mass, sim_time (+5 more)

### Community 21 - "F16FLCS"
Cohesion: 0.25
Nodes (6): F16FLCS, flaperon, lat_dir_ctrl, pitch_ctrl, rudder, stabilator

### Community 22 - "LateralDirectionalController"
Cohesion: 0.08
Nodes (23): IMUData, airspeed, alpha_deg, altitude, beta_deg, Nx, Ny, Nz (+15 more)

### Community 23 - "PilotCommands"
Cohesion: 0.02
Nodes (89): FastJetGLFunctions, activeTexture, attachShader, bindBuffer, bindFramebuffer, bindRenderbuffer, bindVertexArray, bufferData (+81 more)

### Community 24 - "CockpitGeometry"
Cohesion: 0.10
Nodes (13): CockpitGeometry, glass_vao_, glass_vbo_, glass_vertex_count_, initialized_, mfd_vao_, mfd_vbo_, mfd_vertex_count_ (+5 more)

### Community 25 - "FlightInstruments"
Cohesion: 0.08
Nodes (23): FlightInstruments, fbo_, initialized_, line_verts_, rbo_, shader_, TEX_HEIGHT, TEX_WIDTH (+15 more)

### Community 26 - "HUDCollimator"
Cohesion: 0.11
Nodes (19): GLuint, vector, HUDCollimator, initialized_, lines_, masked_vertex_count_, peak_g_, shader_ (+11 more)

### Community 27 - "ShaderProgram"
Cohesion: 0.05
Nodes (31): F16PartType, GLint, GLBSubmesh, base_color_factor, ebo, index_count, index_type, is_transparent (+23 more)

### Community 28 - "LandingGear"
Cohesion: 0.06
Nodes (39): AircraftType, array, LandingGear, brake_left, brake_right, collapsed, CORNERING_STIFFNESS, CREEP_SPEED (+31 more)

### Community 29 - "Key Technical Specifications"
Cohesion: 0.09
Nodes (21): 10. First-Person Graphics, Collimated HUD & Cockpit Avionics, 1. Zero-Allocation Architecture, 2. State Vector & Coordinate Frames, 3. Core Equations of Motion, 4. F-16C Mass & Inertia Properties, 5. 1976 US Standard Atmosphere Model, 6. NASA TP-1538 Aerodynamics & Multidimensional Interpolation, 7. Digital Fly-By-Wire Flight Control System (FLCS) & Actuators (+13 more)

### Community 31 - "RenderEngine"
Cohesion: 0.10
Nodes (18): RenderEngine, aircraft_menu_, camera_rig_, cockpit_, cockpit_shader_, environment_, f16_model_, fov_deg_ (+10 more)

### Community 32 - "LevelRun"
Cohesion: 0.36
Nodes (7): main(), test_max_level_speed_at_altitude(), test_max_level_speed_at_sea_level(), test_military_power_is_subsonic_at_altitude(), test_sea_level_static_thrust_matches_f110(), test_thrust_lapse_with_altitude(), test_thrust_to_weight_ratio()

### Community 33 - "TerrainMesh"
Cohesion: 0.06
Nodes (33): TerrainField, BASE_AMP, BASE_FREQ, BASIN_FADE, BASIN_RADIUS, FIELD_CENTER_X, FIELD_CENTER_Y, GAIN (+25 more)

### Community 34 - "Mat4"
Cohesion: 0.17
Nodes (3): array, Mat4, m

### Community 37 - "string"
Cohesion: 0.43
Nodes (3): string, main(), test_json_serialization_and_file_persistence()

### Community 38 - "FlightState"
Cohesion: 0.10
Nodes (16): ForceFn, ControlSurfaces, delta_a, delta_e, delta_lef, delta_r, speedbrake, AircraftForces (+8 more)

### Community 39 - "Vector3"
Cohesion: 0.18
Nodes (5): operator*(), Vector3, x, y, z

### Community 40 - "IJoystickDriver"
Cohesion: 0.22
Nodes (8): IJoystickDriver, get_axis, get_button, get_device_count, get_device_name, get_hat, initialize, poll

### Community 41 - "AxisCalibration"
Cohesion: 0.17
Nodes (9): AxisCalibration, curvature, inner_deadband, inverted, outer_deadband, raw_center, raw_max, raw_min (+1 more)

### Community 42 - "render"
Cohesion: 0.11
Nodes (64): stbi__at_eof(), stbi__bmp_info(), stbi__bmp_test(), stbi__bmp_test_raw(), stbi__check_png_header(), stbi__convert_format(), stbi__get16be(), stbi__get16le() (+56 more)

### Community 44 - ".compute_infinity_view_matrix"
Cohesion: 0.22
Nodes (8): main(), test_dep_and_equilibrium(), test_g_load_dynamics(), test_view_matrices(), main(), test_fpm_velocity_vector_tracking(), test_hud_conformal_roll_and_boresight_alignment(), test_optical_infinity_projection()

### Community 45 - ".evaluate_force_curve"
Cohesion: 0.67
Nodes (4): main(), test_curvature_tuning(), test_force_curve_endpoints_and_symmetry(), test_strict_monotonicity()

### Community 46 - "FlightStateDeriv"
Cohesion: 0.04
Nodes (53): cgltf_file_type, cgltf_data, accessors, accessors_count, animations, animations_count, asset, bin (+45 more)

### Community 47 - "Matrix3x3"
Cohesion: 0.36
Nodes (10): find_index_and_weight(), array, size_t, interp1d(), interp2d(), interp3d(), N, NX (+2 more)

### Community 48 - "EulerAngles"
Cohesion: 0.11
Nodes (19): AeroConfig, aspect_ratio, b_span, c_bar, cd0, cd_speedbrake, cd_wave_peak, cd_wave_super (+11 more)

### Community 49 - "SpeedbrakeController"
Cohesion: 0.10
Nodes (43): cgltf_bool, cgltf_component_type, cgltf_size, cgltf_ssize, cgltf_type, cgltf_uint, cgltf_accessor, buffer_view (+35 more)

### Community 50 - "cgltf_material"
Cohesion: 0.05
Nodes (41): cgltf_alpha_mode, cgltf_material, alpha_cutoff, alpha_mode, anisotropy, clearcoat, diffuse_transmission, dispersion (+33 more)

### Community 51 - "cgltf_primitive"
Cohesion: 0.06
Nodes (40): cgltf_attribute_type, cgltf_int, cgltf_primitive_type, cgltf_attribute, data, index, name, type (+32 more)

### Community 52 - "cgltf_node"
Cohesion: 0.06
Nodes (39): cgltf_float, cgltf_light_type, cgltf_light, color, extras, cgltf_light_index(), intensity, name (+31 more)

### Community 53 - "cgltf_extras"
Cohesion: 0.06
Nodes (33): cgltf_asset, copyright, extensions, extensions_count, extras, generator, min_version, version (+25 more)

### Community 54 - "stbi__jpeg"
Cohesion: 0.15
Nodes (32): stbi__addints_valid(), stbi__bitreverse16(), stbi__build_fast_ac(), stbi__build_huffman(), stbi__cpuid3(), stbi__decode_jpeg_header(), stbi__decode_jpeg_image(), stbi__extend_receive() (+24 more)

### Community 55 - "cgltf_extension"
Cohesion: 0.08
Nodes (31): cgltf_animation_path_type, cgltf_interpolation_type, cgltf_animation, cgltf_animation_channel, extensions, extensions_count, extras, cgltf_animation_channel_index() (+23 more)

### Community 56 - "AvionicsTelemetry"
Cohesion: 0.06
Nodes (31): AvionicsTelemetry, aircraft_type, bingo_fuel_alert, brake_left, brake_right, detent_str, engine_ftit_deg_c, engine_rpm_pct (+23 more)

### Community 57 - "AudioEngine"
Cohesion: 0.06
Nodes (29): AudioEngine, CHANNELS, device_id_, enabled_, initialized_, prev_gear_deployed_, prev_on_ground_, prev_over_g_ (+21 more)

### Community 58 - "Color4"
Cohesion: 0.18
Nodes (5): Color4, a, b, g, r

### Community 59 - "cgltf_buffer_view"
Cohesion: 0.09
Nodes (22): cgltf_buffer_view_type, cgltf_accessor_sparse, count, indices_buffer_view, indices_byte_offset, indices_component_type, values_buffer_view, values_byte_offset (+14 more)

### Community 60 - "cgltf_buffer"
Cohesion: 0.09
Nodes (22): cgltf_data_free_method, cgltf_meshopt_compression_filter, cgltf_meshopt_compression_mode, cgltf_buffer, data, data_free_method, extensions, extensions_count (+14 more)

### Community 61 - "stbi__bmp_load"
Cohesion: 0.15
Nodes (22): stbi__addsizes_valid(), stbi__bitcount(), stbi__bmp_load(), stbi__bmp_parse_header(), stbi__bmp_set_mask_defaults(), stbi__create_png_alpha_expand8(), stbi__create_png_image(), stbi__create_png_image_raw() (+14 more)

### Community 62 - "cgltf_texture"
Cohesion: 0.10
Nodes (21): cgltf_image, buffer_view, extensions, extensions_count, extras, cgltf_image_index(), mime_type, name (+13 more)

### Community 63 - "MultiEngine"
Cohesion: 0.12
Nodes (14): AircraftType, DetentState, MultiEngine, ab_power, aircraft_type, commanded_thrust_n, config, detent_state (+6 more)

### Community 64 - "f16_sim_viewer.cpp"
Cohesion: 0.36
Nodes (4): AircraftType, parse_aircraft_type(), to_string(), string_view

### Community 66 - "PropulsionConfig"
Cohesion: 0.12
Nodes (17): PropulsionConfig, engine_count, has_afterburner, has_supercruise, has_thrust_vectoring, static_ab_thrust_n, static_dry_thrust_n, static_idle_thrust_n (+9 more)

### Community 67 - "stbi__err"
Cohesion: 0.34
Nodes (17): stbi__bit_reverse(), stbi__compute_huffman_codes(), stbi__err(), stbi__fill_bits(), stbi__parse_huffman_block(), stbi__parse_uncompressed_block(), stbi__parse_zlib(), stbi__parse_zlib_header() (+9 more)

### Community 68 - "MassProperties"
Cohesion: 0.15
Nodes (12): FuelSystem, EMPTY_MASS_KG, AircraftType, MassProperties, I, I_inv, inv_mass, Ixx (+4 more)

### Community 69 - "WindTurbulenceModel"
Cohesion: 0.16
Nodes (8): WindTurbulenceModel, base_wind_speed_mps, gust_u_, gust_v_, gust_w_, rng_state_, turbulence_enabled, wind_heading_rad

### Community 70 - "SDL3InputDriver"
Cohesion: 0.13
Nodes (8): string, vector, SDL3InputDriver, initialized_, joysticks_, SDL_Joystick, main(), test_sdl3_driver_initialization()

### Community 71 - "LevelRun"
Cohesion: 0.14
Nodes (13): RK4Integrator, DEFAULT_DT, dt, LevelRun, aero, alt_target, engine, flcs (+5 more)

### Community 72 - "cgltf_texture_view"
Cohesion: 0.14
Nodes (14): cgltf_diffuse_transmission, diffuse_transmission_color_factor, diffuse_transmission_color_texture, diffuse_transmission_factor, diffuse_transmission_texture, cgltf_texture_view, has_transform, scale (+6 more)

### Community 73 - "stbi__parse_png_file"
Cohesion: 0.26
Nodes (13): stbi__compute_transparency(), stbi__compute_transparency16(), stbi__compute_y_16(), stbi__convert_format16(), stbi__de_iphone(), stbi__do_png(), stbi__expand_png_palette(), stbi__get_chunk_header() (+5 more)

### Community 74 - "cgltf_sampler"
Cohesion: 0.17
Nodes (12): cgltf_filter_type, cgltf_wrap_mode, cgltf_sampler, extensions, extensions_count, extras, cgltf_sampler_index(), mag_filter (+4 more)

### Community 75 - "AudioSample"
Cohesion: 0.18
Nodes (11): LandingGearConfig, main_damping_c, main_l_pos_b, main_r_pos_b, main_spring_k, max_steer_deg, max_stroke, nose_damping_c (+3 more)

### Community 76 - "AirData"
Cohesion: 0.18
Nodes (9): AirData, density, dynamic_pressure, geometric_altitude, geopotential_altitude, mach_number, pressure, speed_of_sound (+1 more)

### Community 77 - "cgltf_mesh"
Cohesion: 0.17
Nodes (12): cgltf_mesh, extensions, extensions_count, extras, cgltf_mesh_index(), name, primitives, primitives_count (+4 more)

### Community 78 - "FlightSimHarness"
Cohesion: 0.18
Nodes (11): AircraftType, FlightSimHarness, aero, engine, flcs, forces, integrator, mass (+3 more)

### Community 79 - "Flight"
Cohesion: 0.18
Nodes (10): Flight, aero, engine, flcs, forces, integrator, mass, speedbrake (+2 more)

### Community 80 - ".compute"
Cohesion: 0.47
Nodes (7): main(), test_altitude_lapse(), test_fuel_burn_and_flameout(), test_ram_recovery(), test_sea_level_static_ratings(), test_spool_dynamics(), test_variable_mass()

### Community 81 - "cgltf_skin"
Cohesion: 0.20
Nodes (10): cgltf_skin, extensions, extensions_count, extras, cgltf_skin_index(), inverse_bind_matrices, joints, joints_count (+2 more)

### Community 82 - "test_integrated_flight.cpp"
Cohesion: 0.51
Nodes (8): main(), make_ground_state(), test_braking_decelerates(), test_gear_collapse_on_hard_landing(), test_no_force_when_airborne(), test_nosewheel_steering(), test_retracted_gear_inert(), test_static_equilibrium()

### Community 83 - "cgltf_camera"
Cohesion: 0.22
Nodes (9): cgltf_camera_type, cgltf_camera, data, extensions, extensions_count, extras, cgltf_camera_index(), name (+1 more)

### Community 84 - ".build_symbology"
Cohesion: 0.18
Nodes (10): Strut, braked, compression, damping_c, in_contact, max_stroke, pos_b, rest_length (+2 more)

### Community 85 - "test_multi_aircraft_performance.cpp"
Cohesion: 0.53
Nodes (6): main(), test_a10_nacelle_pitch_coupling(), test_a10_subsonic_drag_barrier(), test_f15ex_high_speed_dash(), test_f22_thrust_vectoring(), test_typhoon_supercruise()

### Community 86 - "test_aircraft_specs.cpp"
Cohesion: 0.57
Nodes (6): main(), test_a10_specifications(), test_f15ex_specifications(), test_f16c_specifications(), test_f22_specifications(), test_typhoon_specifications()

### Community 87 - "Atmosphere1976"
Cohesion: 0.25
Nodes (8): Atmosphere1976, G0, GAMMA, P0, R_EARTH, R_GAS, RHO0, T0

### Community 88 - "cgltf_scene"
Cohesion: 0.25
Nodes (8): cgltf_scene, extensions, extensions_count, extras, cgltf_scene_index(), name, nodes, nodes_count

### Community 89 - "test_atmosphere.cpp"
Cohesion: 0.52
Nodes (5): main(), test_sea_level(), test_stratosphere_25000m(), test_tropopause_11000m(), test_troposphere_5000m()

### Community 90 - "cgltf_iridescence"
Cohesion: 0.29
Nodes (7): cgltf_iridescence, iridescence_factor, iridescence_ior, iridescence_texture, iridescence_thickness_max, iridescence_thickness_min, iridescence_thickness_texture

### Community 91 - "cgltf_clearcoat"
Cohesion: 0.33
Nodes (6): cgltf_clearcoat, clearcoat_factor, clearcoat_normal_texture, clearcoat_roughness_factor, clearcoat_roughness_texture, clearcoat_texture

### Community 92 - "cgltf_pbr_metallic_roughness"
Cohesion: 0.33
Nodes (6): cgltf_pbr_metallic_roughness, base_color_factor, base_color_texture, metallic_factor, metallic_roughness_texture, roughness_factor

### Community 93 - "cgltf_pbr_specular_glossiness"
Cohesion: 0.33
Nodes (6): cgltf_pbr_specular_glossiness, diffuse_factor, diffuse_texture, glossiness_factor, specular_factor, specular_glossiness_texture

### Community 94 - "main"
Cohesion: 0.24
Nodes (7): DigitalTrimHat, slew_rate, trim_pitch, trim_roll, main(), test_digital_trim_hat(), test_throttle_detents()

### Community 95 - "cgltf_sheen"
Cohesion: 0.40
Nodes (5): cgltf_sheen, sheen_color_factor, sheen_color_texture, sheen_roughness_factor, sheen_roughness_texture

### Community 96 - "cgltf_specular"
Cohesion: 0.40
Nodes (5): cgltf_specular, specular_color_factor, specular_color_texture, specular_factor, specular_texture

### Community 97 - "cgltf_volume"
Cohesion: 0.40
Nodes (5): cgltf_volume, attenuation_color, attenuation_distance, thickness_factor, thickness_texture

### Community 98 - "Vertex3D"
Cohesion: 0.40
Nodes (5): Vertex3D, color, normal, pos, uv

### Community 99 - "test_full_render_pipeline"
Cohesion: 0.33
Nodes (4): AircraftAeroModel, aircraft_type, config, AircraftType

### Community 100 - "Fast Jet Simulator - Authentic Audio Asset Directory"
Cohesion: 0.50
Nodes (3): Fast Jet Simulator - Authentic Audio Asset Directory, Supported Sound Files, Where to get authentic sounds

### Community 101 - "cgltf_anisotropy"
Cohesion: 0.50
Nodes (4): cgltf_anisotropy, anisotropy_rotation, anisotropy_strength, anisotropy_texture

### Community 102 - "InstVertex"
Cohesion: 0.20
Nodes (10): AeroCoefficients, alpha_deg, beta_deg, CD, CL, Cm, Cn, CY (+2 more)

### Community 103 - "string"
Cohesion: 0.22
Nodes (9): AircraftConfig, aero, display_name, flcs, gear, mass, propulsion, type (+1 more)

### Community 104 - "FlightStateDeriv"
Cohesion: 0.22
Nodes (6): FlightStateDeriv, d_omega_b, d_pos_ned, d_q_att, d_vel_b, operator*()

### Community 105 - "MassConfig"
Cohesion: 0.25
Nodes (8): MassConfig, empty_mass_kg, internal_fuel_capacity_kg, Ixx, Ixz, Iyy, Izz, mtow_kg

## Knowledge Gaps
- **1017 isolated node(s):** `genVertexArrays`, `bindVertexArray`, `deleteVertexArrays`, `genBuffers`, `bindBuffer` (+1012 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **6 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `FlightState` connect `FlightState` to `Quaternion`, `MassProperties`, `F16AeroModel`, `SimRunner`, `F16FLCS`, `LateralDirectionalController`, `FlightInstruments`, `HUDCollimator`, `LandingGear`, `Sim`, `RenderEngine`, `Mat4`, `Vector3`, `Color4`, `LevelRun`, `FlightSimHarness`, `Flight`, `test_integrated_flight.cpp`, `test_full_render_pipeline`, `FlightStateDeriv`?**
  _High betweenness centrality (0.108) - this node is a cross-community bridge._
- **Why does `RenderEngine` connect `RenderEngine` to `flight_state.hpp`, `CameraRig`, `IMUData`, `SkyGroundRenderer`, `CockpitGeometry`, `FlightInstruments`, `HUDCollimator`, `ShaderProgram`?**
  _High betweenness centrality (0.058) - this node is a cross-community bridge._
- **Why does `cgltf_data` connect `FlightStateDeriv` to `cgltf_sampler`, `interpolator.hpp`, `cgltf_mesh`, `SpeedbrakeController`, `cgltf_material`, `cgltf_camera`, `cgltf_node`, `cgltf_extras`, `cgltf_skin`, `cgltf_extension`, `cgltf_scene`, `cgltf_buffer_view`, `cgltf_buffer`, `cgltf_texture`?**
  _High betweenness centrality (0.054) - this node is a cross-community bridge._
- **What connects `genVertexArrays`, `bindVertexArray`, `deleteVertexArrays` to the rest of the system?**
  _1017 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Quaternion` be split into smaller, more focused modules?**
  _Cohesion score 0.09113300492610837 - nodes in this community are weakly interconnected._
- **Should `ControllerProfile` be split into smaller, more focused modules?**
  _Cohesion score 0.07407407407407407 - nodes in this community are weakly interconnected._
- **Should `TP1538Tables` be split into smaller, more focused modules?**
  _Cohesion score 0.08669354838709678 - nodes in this community are weakly interconnected._