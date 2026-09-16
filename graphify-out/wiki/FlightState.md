# FlightState

> 43 nodes · cohesion 0.13

## Key Concepts

- **FlightState** (23 connections) — `include/fastjet/fdm/flight_state.hpp`
- **.step()** (14 connections) — `include/fastjet/fdm/rk4_integrator.hpp`
- **test_hands_off_level_flight()** (14 connections) — `tests/test_flcs_level_flight.cpp`
- **.create_clean_f16()** (13 connections) — `include/fastjet/fdm/mass_properties.hpp`
- **test_high_speed_9g_pull()** (13 connections) — `tests/test_flcs_pullup.cpp`
- **test_low_speed_alpha_limiter_capture()** (13 connections) — `tests/test_flcs_pullup.cpp`
- **test_open_loop_pitch_divergence()** (13 connections) — `tests/test_pitch_divergence.cpp`
- **.update()** (12 connections) — `include/fastjet/flcs/f16_flcs.hpp`
- **.compute_forces_and_moments()** (11 connections) — `include/fastjet/aero/f16_aero_model.hpp`
- **.zero()** (11 connections) — `include/fastjet/math/vector3.hpp`
- **.identity()** (10 connections) — `include/fastjet/math/quaternion.hpp`
- **test_horizontal_ballistic_flight()** (10 connections) — `tests/test_ballistic.cpp`
- **test_virtual_hotas_to_flcs_pipeline()** (10 connections) — `tests/test_input_pipeline.cpp`
- **.airspeed()** (8 connections) — `include/fastjet/fdm/flight_state.hpp`
- **.altitude()** (8 connections) — `include/fastjet/fdm/flight_state.hpp`
- **test_inclined_ballistic_flight_arbitrary_attitude()** (8 connections) — `tests/test_ballistic.cpp`
- **test_steady_level_trim_flight()** (7 connections) — `tests/test_ballistic.cpp`
- **test_flcs_pullup.cpp** (7 connections) — `tests/test_flcs_pullup.cpp`
- **test_pitch_rate_response()** (6 connections) — `tests/test_gyroscopic.cpp`
- **test_performance.cpp** (6 connections) — `tests/test_performance.cpp`
- **test_ballistic.cpp** (5 connections) — `tests/test_ballistic.cpp`
- **test_torque_free_angular_momentum_conservation()** (5 connections) — `tests/test_gyroscopic.cpp`
- **benchmark_throughput()** (5 connections) — `tests/test_performance.cpp`
- **test_zero_garbage_allocation()** (5 connections) — `tests/test_performance.cpp`
- **.zero()** (4 connections) — `include/fastjet/fdm/aircraft_forces.hpp`
- *... and 18 more nodes in this community*

## Relationships

- [Quaternion](Quaternion.md) (13 shared connections)
- [f16_aero_model.hpp](f16_aero_model.hpp.md) (13 shared connections)
- [ControlSurfaces](ControlSurfaces.md) (7 shared connections)
- [MassProperties](MassProperties.md) (7 shared connections)
- [IMUData](IMUData.md) (5 shared connections)
- [AirData](AirData.md) (5 shared connections)
- [F16FLCS](F16FLCS.md) (3 shared connections)
- [SDL3InputDriver](SDL3InputDriver.md) (2 shared connections)
- [F16AeroModel](F16AeroModel.md) (1 shared connections)
- [PilotCommands](PilotCommands.md) (1 shared connections)

## Source Files

- `include/fastjet/aero/f16_aero_model.hpp`
- `include/fastjet/fdm/aircraft_forces.hpp`
- `include/fastjet/fdm/flight_state.hpp`
- `include/fastjet/fdm/mass_properties.hpp`
- `include/fastjet/fdm/rk4_integrator.hpp`
- `include/fastjet/flcs/f16_flcs.hpp`
- `include/fastjet/math/quaternion.hpp`
- `include/fastjet/math/vector3.hpp`
- `tests/test_ballistic.cpp`
- `tests/test_flcs_level_flight.cpp`
- `tests/test_flcs_pullup.cpp`
- `tests/test_gyroscopic.cpp`
- `tests/test_input_pipeline.cpp`
- `tests/test_performance.cpp`
- `tests/test_pitch_divergence.cpp`

## Audit Trail

- EXTRACTED: 136 (79%)
- INFERRED: 36 (21%)
- AMBIGUOUS: 0 (0%)

---

*Part of the graphify knowledge wiki. See [index](index.md) to navigate.*