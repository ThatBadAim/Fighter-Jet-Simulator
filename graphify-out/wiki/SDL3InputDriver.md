# SDL3InputDriver

> 15 nodes · cohesion 0.17

## Key Concepts

- **SDL3InputDriver** (15 connections) — `include/fastjet/input/joystick_driver.hpp`
- **test_input_pipeline.cpp** (7 connections) — `tests/test_input_pipeline.cpp`
- **test_sdl3_driver_initialization()** (5 connections) — `tests/test_input_pipeline.cpp`
- **main()** (3 connections) — `tests/test_input_pipeline.cpp`
- **.get_device_count()** (2 connections) — `include/fastjet/input/joystick_driver.hpp`
- **.initialize()** (2 connections) — `include/fastjet/input/joystick_driver.hpp`
- **.poll()** (2 connections) — `include/fastjet/input/joystick_driver.hpp`
- **.SDL3InputDriver()** (2 connections) — `include/fastjet/input/joystick_driver.hpp`
- **.shutdown()** (2 connections) — `include/fastjet/input/joystick_driver.hpp`
- **.get_axis()** (1 connections) — `include/fastjet/input/joystick_driver.hpp`
- **.get_button()** (1 connections) — `include/fastjet/input/joystick_driver.hpp`
- **.get_hat()** (1 connections) — `include/fastjet/input/joystick_driver.hpp`
- **initialized_** (1 connections) — `include/fastjet/input/joystick_driver.hpp`
- **joysticks_** (1 connections) — `include/fastjet/input/joystick_driver.hpp`
- **SDL_Joystick** (1 connections)

## Relationships

- [ControllerProfile](ControllerProfile.md) (3 shared connections)
- [f16_aero_model.hpp](f16_aero_model.hpp.md) (3 shared connections)
- [FlightState](FlightState.md) (2 shared connections)
- [IJoystickDriver](IJoystickDriver.md) (1 shared connections)
- [VirtualHOTASDriver](VirtualHOTASDriver.md) (1 shared connections)

## Source Files

- `include/fastjet/input/joystick_driver.hpp`
- `tests/test_input_pipeline.cpp`

## Audit Trail

- EXTRACTED: 25 (89%)
- INFERRED: 3 (11%)
- AMBIGUOUS: 0 (0%)

---

*Part of the graphify knowledge wiki. See [index](index.md) to navigate.*