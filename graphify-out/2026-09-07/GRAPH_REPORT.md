# Graph Report - Fast Jet Simulator  (2026-09-06)

## Corpus Check
- 71 files · ~60,178 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 1012 nodes · 1871 edges · 50 communities (46 shown, 4 thin omitted)
- Extraction: 92% EXTRACTED · 8% INFERRED · 0% AMBIGUOUS · INFERRED: 147 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `2af8c856`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- Vector3
- FlightState
- ControllerProfile
- TP1538Tables
- test_throttle_control.cpp
- .process_bipolar
- F110Engine
- F16AeroModel
- f16_aero_model.hpp
- VirtualHOTASDriver
- Actuator
- interpolator.hpp
- PitchController
- SpeedbrakeController
- CameraRig
- InputManager
- ThrottleController
- IMUData
- avionics_controls.hpp
- SkyGroundRenderer
- SimRunner
- F16FLCS
- LateralDirectionalController
- PilotCommands
- RenderEngine
- FlightInstruments
- HUDCollimator
- ShaderProgram
- LandingGear
- Key Technical Specifications
- Sim
- Color4
- test_performance_envelope.cpp
- TerrainMesh
- Mat4
- rules/graphify.md
- workflows/graphify.md
- string
- SDL3InputDriver
- .build_symbology
- IJoystickDriver
- AxisCalibration
- render
- Agent Guidelines & Rules
- test_hud_collimation.cpp
- .evaluate_force_curve
- InstVertex
- Flight
- LevelRun
- string

## God Nodes (most connected - your core abstractions)
1. `FlightState` - 39 edges
2. `SkyGroundRenderer` - 36 edges
3. `ControllerProfile` - 34 edges
4. `Vector3` - 34 edges
5. `FlightInstruments` - 33 edges
6. `F16AeroModel` - 31 edges
7. `CameraRig` - 31 edges
8. `F110Engine` - 28 edges
9. `LandingGear` - 26 edges
10. `Color4` - 26 edges

## Surprising Connections (you probably didn't know these)
- `main()` --calls--> `camera_rig_`  [INFERRED]
  src/f16_sim_viewer.cpp → include/fastjet/graphics/render_engine.hpp
- `SimRunner` --references--> `F16AeroModel`  [EXTRACTED]
  tests/test_flcs_review.cpp → include/fastjet/aero/f16_aero_model.hpp
- `Sim` --references--> `F16AeroModel`  [EXTRACTED]
  tests/test_integrated_flight.cpp → include/fastjet/aero/f16_aero_model.hpp
- `LevelRun` --references--> `F16AeroModel`  [EXTRACTED]
  tests/test_performance_envelope.cpp → include/fastjet/aero/f16_aero_model.hpp
- `Flight` --references--> `F16AeroModel`  [EXTRACTED]
  tests/test_throttle_control.cpp → include/fastjet/aero/f16_aero_model.hpp

## Import Cycles
- None detected.

## Communities (50 total, 4 thin omitted)

### Community 0 - "Vector3"
Cohesion: 0.05
Nodes (22): FlightStateDeriv, d_omega_b, d_pos_ned, d_q_att, d_vel_b, operator*(), Matrix3x3, m (+14 more)

### Community 1 - "FlightState"
Cohesion: 0.06
Nodes (41): ForceFn, AircraftForces, force_b, moment_b, FlightState, omega_b, pos_ned, q_att (+33 more)

### Community 2 - "ControllerProfile"
Cohesion: 0.07
Nodes (27): ControllerProfile, btn_parking_brake, btn_speedbrake_extend, btn_speedbrake_retract, btn_trim_down, btn_trim_left, btn_trim_right, btn_trim_up (+19 more)

### Community 3 - "TP1538Tables"
Cohesion: 0.09
Nodes (31): array, size_t, idx3d(), init_CD_table(), init_CL_roll_table(), init_CL_table(), init_CM_table(), init_CN_yaw_table() (+23 more)

### Community 4 - "test_throttle_control.cpp"
Cohesion: 0.35
Nodes (9): main(), test_afterburner_accelerates(), test_detent_boundaries(), test_resting_hardware_lever_does_not_capture_throttle(), test_speedbrake_adds_deceleration(), test_speedbrake_slews_not_snaps(), test_status_cadence_is_time_based(), test_throttle_back_decelerates() (+1 more)

### Community 5 - ".process_bipolar"
Cohesion: 0.36
Nodes (5): SignalConditioner, main(), test_axis_calibration_and_inversion(), test_deadband_filtering(), test_unipolar_axis_conditioning()

### Community 6 - "F110Engine"
Cohesion: 0.05
Nodes (47): AirData, density, dynamic_pressure, geometric_altitude, geopotential_altitude, mach_number, pressure, speed_of_sound (+39 more)

### Community 7 - "F16AeroModel"
Cohesion: 0.06
Nodes (45): AeroCoefficients, alpha_deg, beta_deg, CD, CL, Cm, Cn, CY (+37 more)

### Community 8 - "f16_aero_model.hpp"
Cohesion: 0.10
Nodes (14): ControlSurfaces, delta_a, delta_e, delta_lef, delta_r, speedbrake, Vertex3D, color (+6 more)

### Community 9 - "VirtualHOTASDriver"
Cohesion: 0.12
Nodes (9): array, size_t, VirtualHOTASDriver, axes_, buttons_, hats_, MAX_AXES, MAX_BUTTONS (+1 more)

### Community 10 - "Actuator"
Cohesion: 0.16
Nodes (13): Actuator, is_pos_limited, is_rate_limited, pos_max, pos_min, position, rate, rate_limit (+5 more)

### Community 11 - "interpolator.hpp"
Cohesion: 0.24
Nodes (15): find_index_and_weight(), array, size_t, interp1d(), interp2d(), interp3d(), Interpolator, N (+7 more)

### Community 12 - "PitchController"
Cohesion: 0.13
Nodes (13): PitchController, ALPHA_MAX_DEG, integrator, K_alpha, K_alpha_rate, Kd, Ki, Kp (+5 more)

### Community 13 - "SpeedbrakeController"
Cohesion: 0.16
Nodes (9): SpeedbrakeController, position, slew_rate, WheelBrakes, left_brake, parking_brake, right_brake, SwitchPosition (+1 more)

### Community 14 - "CameraRig"
Cohesion: 0.11
Nodes (16): CameraRig, DEP_X, DEP_Y, DEP_Z, disp_b_, k_x_, k_y_, k_z_ (+8 more)

### Community 15 - "InputManager"
Cohesion: 0.17
Nodes (12): InputManager, brakes, config, driver, pitch_in, roll_in, speedbrake, throttle (+4 more)

### Community 16 - "ThrottleController"
Cohesion: 0.17
Nodes (12): DetentState, ThrottleController, ab_power, DETENT_CUTOFF, DETENT_IDLE, DETENT_MIL, dry_power, net_thrust_n (+4 more)

### Community 17 - "IMUData"
Cohesion: 0.17
Nodes (12): IMUData, airspeed, alpha_deg, altitude, beta_deg, Nx, Ny, Nz (+4 more)

### Community 18 - "avionics_controls.hpp"
Cohesion: 0.21
Nodes (7): DigitalTrimHat, slew_rate, trim_pitch, trim_roll, main(), test_digital_trim_hat(), test_throttle_detents()

### Community 19 - "SkyGroundRenderer"
Cohesion: 0.09
Nodes (17): GLsizei, GLuint, SkyGroundRenderer, ATMOSPHERE_GLSL, ground_shader_, ground_vao_, ground_vbo_, ground_vertex_count_ (+9 more)

### Community 20 - "SimRunner"
Cohesion: 0.16
Nodes (16): RK4Integrator, DEFAULT_DT, dt, main(), SimRunner, aero_model, current_forces, flcs (+8 more)

### Community 21 - "F16FLCS"
Cohesion: 0.25
Nodes (6): F16FLCS, flaperon, lat_dir_ctrl, pitch_ctrl, rudder, stabilator

### Community 22 - "LateralDirectionalController"
Cohesion: 0.25
Nodes (7): LateralDirectionalController, ALPHA_ATTEN_MAX, ALPHA_ATTEN_MIN, K_beta, K_roll, K_yaw_damper, P_CMD_MAX_DPS

### Community 23 - "PilotCommands"
Cohesion: 0.25
Nodes (4): PilotCommands, pitch_stick, roll_stick, rudder_pedal

### Community 24 - "RenderEngine"
Cohesion: 0.06
Nodes (26): CockpitGeometry, glass_vao_, glass_vbo_, glass_vertex_count_, initialized_, mfd_vao_, mfd_vbo_, mfd_vertex_count_ (+18 more)

### Community 25 - "FlightInstruments"
Cohesion: 0.12
Nodes (15): FlightInstruments, fbo_, initialized_, line_verts_, rbo_, shader_, TEX_HEIGHT, TEX_WIDTH (+7 more)

### Community 26 - "HUDCollimator"
Cohesion: 0.12
Nodes (13): GLuint, vector, HUDCollimator, initialized_, lines_, peak_g_, shader_, vao_ (+5 more)

### Community 27 - "ShaderProgram"
Cohesion: 0.16
Nodes (6): GLenum, GLint, GLuint, ShaderProgram, program_id_, valid_

### Community 28 - "LandingGear"
Cohesion: 0.07
Nodes (37): array, LandingGear, brake_left, brake_right, collapsed, CORNERING_STIFFNESS, CREEP_SPEED, deployed (+29 more)

### Community 29 - "Key Technical Specifications"
Cohesion: 0.09
Nodes (21): 10. First-Person Graphics, Collimated HUD & Cockpit Avionics, 1. Zero-Allocation Architecture, 2. State Vector & Coordinate Frames, 3. Core Equations of Motion, 4. F-16C Mass & Inertia Properties, 5. 1976 US Standard Atmosphere Model, 6. NASA TP-1538 Aerodynamics & Multidimensional Interpolation, 7. Digital Fly-By-Wire Flight Control System (FLCS) & Actuators (+13 more)

### Community 30 - "Sim"
Cohesion: 0.15
Nodes (19): main(), Sim, aero, engine, flcs, forces, gear, integrator (+11 more)

### Community 31 - "Color4"
Cohesion: 0.17
Nodes (5): Color4, a, b, g, r

### Community 32 - "test_performance_envelope.cpp"
Cohesion: 0.36
Nodes (7): main(), test_max_level_speed_at_altitude(), test_max_level_speed_at_sea_level(), test_military_power_is_subsonic_at_altitude(), test_sea_level_static_thrust_matches_f110(), test_thrust_lapse_with_altitude(), test_thrust_to_weight_ratio()

### Community 33 - "TerrainMesh"
Cohesion: 0.06
Nodes (35): TerrainField, BASE_AMP, BASE_FREQ, BASIN_FADE, BASIN_RADIUS, FIELD_CENTER_X, FIELD_CENTER_Y, GAIN (+27 more)

### Community 34 - "Mat4"
Cohesion: 0.20
Nodes (3): array, Mat4, m

### Community 37 - "string"
Cohesion: 0.29
Nodes (3): string, main(), test_json_serialization_and_file_persistence()

### Community 38 - "SDL3InputDriver"
Cohesion: 0.16
Nodes (7): vector, SDL3InputDriver, initialized_, joysticks_, SDL_Joystick, main(), test_sdl3_driver_initialization()

### Community 39 - ".build_symbology"
Cohesion: 0.53
Nodes (6): main(), test_fpm_ghost_clamping(), test_hud_supersonic_and_centering(), test_hud_typography_and_resolution(), test_pitch_ladder_full_range(), test_stencil_clear_persistence()

### Community 40 - "IJoystickDriver"
Cohesion: 0.22
Nodes (8): IJoystickDriver, get_axis, get_button, get_device_count, get_device_name, get_hat, initialize, poll

### Community 41 - "AxisCalibration"
Cohesion: 0.22
Nodes (8): AxisCalibration, curvature, inner_deadband, inverted, outer_deadband, raw_center, raw_max, raw_min

### Community 42 - "render"
Cohesion: 0.42
Nodes (10): count_magenta(), vector, Frame, rgb, main(), render(), test_horizon_has_no_seam(), test_runway_is_visible_over_terrain() (+2 more)

### Community 44 - "test_hud_collimation.cpp"
Cohesion: 0.33
Nodes (7): main(), test_dep_and_equilibrium(), test_g_load_dynamics(), main(), test_fpm_velocity_vector_tracking(), test_hud_conformal_roll_and_boresight_alignment(), test_optical_infinity_projection()

### Community 45 - ".evaluate_force_curve"
Cohesion: 0.67
Nodes (4): main(), test_curvature_tuning(), test_force_curve_endpoints_and_symmetry(), test_strict_monotonicity()

### Community 46 - "InstVertex"
Cohesion: 0.67
Nodes (3): InstVertex, color, pos

### Community 47 - "Flight"
Cohesion: 0.18
Nodes (10): Flight, aero, engine, flcs, forces, integrator, mass, speedbrake (+2 more)

### Community 48 - "LevelRun"
Cohesion: 0.20
Nodes (10): LevelRun, aero, alt_target, engine, flcs, forces, integrator, mass (+2 more)

## Knowledge Gaps
- **423 isolated node(s):** `delta_e`, `delta_a`, `delta_r`, `delta_lef`, `speedbrake` (+418 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 502 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **4 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `FlightState` connect `FlightState` to `Vector3`, `Mat4`, `F16AeroModel`, `f16_aero_model.hpp`, `.build_symbology`, `CameraRig`, `Flight`, `LevelRun`, `SimRunner`, `LandingGear`, `Sim`, `Color4`?**
  _High betweenness centrality (0.156) - this node is a cross-community bridge._
- **Why does `FlightInstruments` connect `FlightInstruments` to `f16_aero_model.hpp`, `InstVertex`, `RenderEngine`, `ShaderProgram`, `Color4`?**
  _High betweenness centrality (0.078) - this node is a cross-community bridge._
- **Why does `F16AeroModel` connect `F16AeroModel` to `FlightState`, `f16_aero_model.hpp`, `Flight`, `LevelRun`, `SimRunner`, `Sim`?**
  _High betweenness centrality (0.072) - this node is a cross-community bridge._
- **What connects `delta_e`, `delta_a`, `delta_r` to the rest of the system?**
  _423 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Vector3` be split into smaller, more focused modules?**
  _Cohesion score 0.05152394775036284 - nodes in this community are weakly interconnected._
- **Should `FlightState` be split into smaller, more focused modules?**
  _Cohesion score 0.06293965198074787 - nodes in this community are weakly interconnected._
- **Should `ControllerProfile` be split into smaller, more focused modules?**
  _Cohesion score 0.07407407407407407 - nodes in this community are weakly interconnected._