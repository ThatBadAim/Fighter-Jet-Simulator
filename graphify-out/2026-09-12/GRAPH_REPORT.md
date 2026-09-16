# Graph Report - Fast Jet Simulator  (2026-09-07)

## Corpus Check
- 71 files · ~64,016 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 1022 nodes · 1901 edges · 50 communities (46 shown, 4 thin omitted)
- Extraction: 92% EXTRACTED · 8% INFERRED · 0% AMBIGUOUS · INFERRED: 147 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `187e5fa3`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

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

## God Nodes (most connected - your core abstractions)
1. `FlightInstruments` - 44 edges
2. `FlightState` - 42 edges
3. `SkyGroundRenderer` - 35 edges
4. `ControllerProfile` - 34 edges
5. `Vector3` - 33 edges
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

### Community 0 - "Quaternion"
Cohesion: 0.17
Nodes (5): Quaternion, w, x, y, z

### Community 1 - "MassProperties"
Cohesion: 0.07
Nodes (38): ControlSurfaces, delta_a, delta_e, delta_lef, delta_r, speedbrake, AircraftForces, force_b (+30 more)

### Community 2 - "ControllerProfile"
Cohesion: 0.07
Nodes (27): ControllerProfile, btn_parking_brake, btn_speedbrake_extend, btn_speedbrake_retract, btn_trim_down, btn_trim_left, btn_trim_right, btn_trim_up (+19 more)

### Community 3 - "TP1538Tables"
Cohesion: 0.09
Nodes (31): array, size_t, idx3d(), init_CD_table(), init_CL_roll_table(), init_CL_table(), init_CM_table(), init_CN_yaw_table() (+23 more)

### Community 4 - "Flight"
Cohesion: 0.14
Nodes (19): Flight, aero, engine, flcs, forces, integrator, mass, speedbrake (+11 more)

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
Cohesion: 0.13
Nodes (6): Vertex3D, color, normal, pos, uv, vector

### Community 9 - "VirtualHOTASDriver"
Cohesion: 0.06
Nodes (17): array, size_t, string, vector, SDL3InputDriver, initialized_, joysticks_, VirtualHOTASDriver (+9 more)

### Community 10 - "Actuator"
Cohesion: 0.16
Nodes (13): Actuator, is_pos_limited, is_rate_limited, pos_max, pos_min, position, rate, rate_limit (+5 more)

### Community 11 - "interpolator.hpp"
Cohesion: 0.24
Nodes (15): find_index_and_weight(), array, size_t, interp1d(), interp2d(), interp3d(), Interpolator, N (+7 more)

### Community 12 - "PitchController"
Cohesion: 0.13
Nodes (13): PitchController, ALPHA_MAX_DEG, integrator, K_alpha, K_alpha_rate, Kd, Ki, Kp (+5 more)

### Community 13 - "WheelBrakes"
Cohesion: 0.29
Nodes (5): WheelBrakes, left_brake, parking_brake, right_brake, test_speedbrake_and_wheel_brakes()

### Community 14 - "CameraRig"
Cohesion: 0.10
Nodes (15): CameraRig, DEP_X, DEP_Y, DEP_Z, disp_b_, k_x_, k_y_, k_z_ (+7 more)

### Community 15 - "InputManager"
Cohesion: 0.17
Nodes (12): InputManager, brakes, config, driver, pitch_in, roll_in, speedbrake, throttle (+4 more)

### Community 16 - "ThrottleController"
Cohesion: 0.17
Nodes (12): DetentState, ThrottleController, ab_power, DETENT_CUTOFF, DETENT_IDLE, DETENT_MIL, dry_power, net_thrust_n (+4 more)

### Community 17 - "IMUData"
Cohesion: 0.17
Nodes (12): IMUData, airspeed, alpha_deg, altitude, beta_deg, Nx, Ny, Nz (+4 more)

### Community 18 - "DigitalTrimHat"
Cohesion: 0.24
Nodes (7): DigitalTrimHat, slew_rate, trim_pitch, trim_roll, main(), test_digital_trim_hat(), test_throttle_detents()

### Community 19 - "SkyGroundRenderer"
Cohesion: 0.10
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

### Community 24 - "CockpitGeometry"
Cohesion: 0.10
Nodes (13): CockpitGeometry, glass_vao_, glass_vbo_, glass_vertex_count_, initialized_, mfd_vao_, mfd_vbo_, mfd_vertex_count_ (+5 more)

### Community 25 - "FlightInstruments"
Cohesion: 0.07
Nodes (27): FlightInstruments, fbo_, initialized_, line_verts_, rbo_, shader_, TEX_HEIGHT, TEX_WIDTH (+19 more)

### Community 26 - "HUDCollimator"
Cohesion: 0.09
Nodes (22): GLuint, vector, HUDCollimator, initialized_, lines_, peak_g_, shader_, vao_ (+14 more)

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

### Community 31 - "RenderEngine"
Cohesion: 0.09
Nodes (15): RenderEngine, camera_rig_, cockpit_, cockpit_shader_, environment_, fov_deg_, hud_, initialized_ (+7 more)

### Community 32 - "LevelRun"
Cohesion: 0.16
Nodes (17): LevelRun, aero, alt_target, engine, flcs, forces, integrator, mass (+9 more)

### Community 33 - "TerrainMesh"
Cohesion: 0.06
Nodes (35): TerrainField, BASE_AMP, BASE_FREQ, BASIN_FADE, BASIN_RADIUS, FIELD_CENTER_X, FIELD_CENTER_Y, GAIN (+27 more)

### Community 34 - "Mat4"
Cohesion: 0.19
Nodes (3): array, Mat4, m

### Community 37 - "string"
Cohesion: 0.29
Nodes (3): string, main(), test_json_serialization_and_file_persistence()

### Community 38 - "FlightState"
Cohesion: 0.18
Nodes (9): ForceFn, FlightState, omega_b, pos_ned, q_att, vel_b, step(), SixDoFFDM (+1 more)

### Community 39 - "Vector3"
Cohesion: 0.18
Nodes (5): operator*(), Vector3, x, y, z

### Community 40 - "IJoystickDriver"
Cohesion: 0.22
Nodes (8): IJoystickDriver, get_axis, get_button, get_device_count, get_device_name, get_hat, initialize, poll

### Community 41 - "AxisCalibration"
Cohesion: 0.22
Nodes (8): AxisCalibration, curvature, inner_deadband, inverted, outer_deadband, raw_center, raw_max, raw_min

### Community 42 - "render"
Cohesion: 0.42
Nodes (10): count_magenta(), vector, Frame, rgb, main(), render(), test_horizon_has_no_seam(), test_runway_is_visible_over_terrain() (+2 more)

### Community 44 - ".compute_infinity_view_matrix"
Cohesion: 0.42
Nodes (5): main(), test_dep_and_equilibrium(), test_g_load_dynamics(), test_view_matrices(), test_optical_infinity_projection()

### Community 45 - ".evaluate_force_curve"
Cohesion: 0.67
Nodes (4): main(), test_curvature_tuning(), test_force_curve_endpoints_and_symmetry(), test_strict_monotonicity()

### Community 46 - "FlightStateDeriv"
Cohesion: 0.22
Nodes (6): FlightStateDeriv, d_omega_b, d_pos_ned, d_q_att, d_vel_b, operator*()

### Community 48 - "EulerAngles"
Cohesion: 0.33
Nodes (4): EulerAngles, pitch, roll, yaw

### Community 49 - "SpeedbrakeController"
Cohesion: 0.33
Nodes (4): SpeedbrakeController, position, slew_rate, SwitchPosition

## Knowledge Gaps
- **427 isolated node(s):** `delta_e`, `delta_a`, `delta_r`, `delta_lef`, `speedbrake` (+422 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 506 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **4 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `FlightState` connect `FlightState` to `Quaternion`, `MassProperties`, `Mat4`, `LevelRun`, `Flight`, `F16AeroModel`, `f16_aero_model.hpp`, `Vector3`, `.compute_infinity_view_matrix`, `FlightStateDeriv`, `CameraRig`, `EulerAngles`, `SimRunner`, `FlightInstruments`, `HUDCollimator`, `LandingGear`, `Sim`, `RenderEngine`?**
  _High betweenness centrality (0.190) - this node is a cross-community bridge._
- **Why does `FlightInstruments` connect `FlightInstruments` to `f16_aero_model.hpp`, `ShaderProgram`, `RenderEngine`?**
  _High betweenness centrality (0.088) - this node is a cross-community bridge._
- **Why does `F110Engine` connect `F110Engine` to `LevelRun`, `Flight`, `f16_aero_model.hpp`, `ThrottleController`, `Sim`, `RenderEngine`?**
  _High betweenness centrality (0.081) - this node is a cross-community bridge._
- **What connects `delta_e`, `delta_a`, `delta_r` to the rest of the system?**
  _427 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `MassProperties` be split into smaller, more focused modules?**
  _Cohesion score 0.07191961924907457 - nodes in this community are weakly interconnected._
- **Should `ControllerProfile` be split into smaller, more focused modules?**
  _Cohesion score 0.07407407407407407 - nodes in this community are weakly interconnected._
- **Should `TP1538Tables` be split into smaller, more focused modules?**
  _Cohesion score 0.08669354838709678 - nodes in this community are weakly interconnected._