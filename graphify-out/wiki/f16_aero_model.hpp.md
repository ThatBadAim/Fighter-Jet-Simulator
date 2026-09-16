# f16_aero_model.hpp

> 23 nodes · cohesion 0.19

## Key Concepts

- **f16_aero_model.hpp** (13 connections) — `include/fastjet/aero/f16_aero_model.hpp`
- **f16_flcs.hpp** (13 connections) — `include/fastjet/flcs/f16_flcs.hpp`
- **rk4_integrator.hpp** (12 connections) — `include/fastjet/fdm/rk4_integrator.hpp`
- **flight_state.hpp** (10 connections) — `include/fastjet/fdm/flight_state.hpp`
- **six_dof_fdm.hpp** (9 connections) — `include/fastjet/fdm/six_dof_fdm.hpp`
- **atmosphere1976.hpp** (8 connections) — `include/fastjet/environment/atmosphere1976.hpp`
- **imu_sensor.hpp** (8 connections) — `include/fastjet/flcs/imu_sensor.hpp`
- **vector3.hpp** (8 connections) — `include/fastjet/math/vector3.hpp`
- **aircraft_forces.hpp** (7 connections) — `include/fastjet/fdm/aircraft_forces.hpp`
- **mass_properties.hpp** (7 connections) — `include/fastjet/fdm/mass_properties.hpp`
- **matrix3x3.hpp** (6 connections) — `include/fastjet/math/matrix3x3.hpp`
- **test_flcs_level_flight.cpp** (6 connections) — `tests/test_flcs_level_flight.cpp`
- **test_pitch_divergence.cpp** (6 connections) — `tests/test_pitch_divergence.cpp`
- **RK4Integrator** (5 connections) — `include/fastjet/fdm/rk4_integrator.hpp`
- **pilot_commands.hpp** (5 connections) — `include/fastjet/flcs/pilot_commands.hpp`
- **quaternion.hpp** (5 connections) — `include/fastjet/math/quaternion.hpp`
- **lateral_directional_controller.hpp** (4 connections) — `include/fastjet/flcs/lateral_directional_controller.hpp`
- **pitch_controller.hpp** (4 connections) — `include/fastjet/flcs/pitch_controller.hpp`
- **main()** (2 connections) — `tests/test_flcs_level_flight.cpp`
- **main()** (2 connections) — `tests/test_pitch_divergence.cpp`
- **DEFAULT_DT** (1 connections) — `include/fastjet/fdm/rk4_integrator.hpp`
- **dt** (1 connections) — `include/fastjet/fdm/rk4_integrator.hpp`
- **.RK4Integrator()** (1 connections) — `include/fastjet/fdm/rk4_integrator.hpp`

## Relationships

- [FlightState](FlightState.md) (13 shared connections)
- [Quaternion](Quaternion.md) (6 shared connections)
- [ControlSurfaces](ControlSurfaces.md) (4 shared connections)
- [MassProperties](MassProperties.md) (4 shared connections)
- [SDL3InputDriver](SDL3InputDriver.md) (3 shared connections)
- [AirData](AirData.md) (3 shared connections)
- [interpolator.hpp](interpolator.hpp.md) (2 shared connections)
- [ControllerProfile](ControllerProfile.md) (2 shared connections)
- [TP1538Tables](TP1538Tables.md) (1 shared connections)
- [F16AeroModel](F16AeroModel.md) (1 shared connections)
- [Actuator](Actuator.md) (1 shared connections)
- [F16FLCS](F16FLCS.md) (1 shared connections)

## Source Files

- `include/fastjet/aero/f16_aero_model.hpp`
- `include/fastjet/environment/atmosphere1976.hpp`
- `include/fastjet/fdm/aircraft_forces.hpp`
- `include/fastjet/fdm/flight_state.hpp`
- `include/fastjet/fdm/mass_properties.hpp`
- `include/fastjet/fdm/rk4_integrator.hpp`
- `include/fastjet/fdm/six_dof_fdm.hpp`
- `include/fastjet/flcs/f16_flcs.hpp`
- `include/fastjet/flcs/imu_sensor.hpp`
- `include/fastjet/flcs/lateral_directional_controller.hpp`
- `include/fastjet/flcs/pilot_commands.hpp`
- `include/fastjet/flcs/pitch_controller.hpp`
- `include/fastjet/math/matrix3x3.hpp`
- `include/fastjet/math/quaternion.hpp`
- `include/fastjet/math/vector3.hpp`
- `tests/test_flcs_level_flight.cpp`
- `tests/test_pitch_divergence.cpp`

## Audit Trail

- EXTRACTED: 94 (100%)
- INFERRED: 0 (0%)
- AMBIGUOUS: 0 (0%)

---

*Part of the graphify knowledge wiki. See [index](index.md) to navigate.*