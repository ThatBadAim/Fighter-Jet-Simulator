# Fast Jet 6-DoF Flight Dynamics Model & RK4 Integrator

A deterministic, zero-allocation, 6-Degrees-of-Freedom (6-DoF) rigid-body Flight Dynamics Model (FDM) and 4th-Order Runge-Kutta (RK4) numerical integrator for an F-16 flight simulator, implemented in modern C++20.

---

## Key Technical Specifications

### 1. Zero-Allocation Architecture
- **Zero dynamic memory allocation (`new` / `malloc`)** in the hot integration loop.
- All state vectors, kinematic derivatives, intermediate RK4 stage buffers ($k_1, k_2, k_3, k_4$), and matrix operators are stack-allocated value types.
- Execution speed: **~5,000,000 steps/second (~200 ns per step)** on standard hardware, offering a **>24,000x real-time margin** over the 200 Hz ($dt = 0.005$ s) requirement.
- Fully decoupled from any rendering or UI loop.

### 2. State Vector & Coordinate Frames
Defined in [`include/fastjet/fdm/flight_state.hpp`](include/fastjet/fdm/flight_state.hpp):
- **Position ($\mathbf{p}_{NED}$)**: North-East-Down Cartesian frame $[x, y, z]^T$ in meters. Altitude is geometric $-z$.
- **Linear Velocity ($\mathbf{v}_b$)**: Body-fixed frame $[u, v, w]^T$ in m/s ($u$: nose, $v$: right wing, $w$: belly).
- **Angular Velocity ($\boldsymbol{\omega}_b$)**: Body-fixed frame $[p, q, r]^T$ in rad/s ($p$: roll, $q$: pitch, $r$: yaw).
- **Attitude Quaternion ($\mathbf{q}$)**: Unit quaternion $\mathbf{q} = [q_w, q_x, q_y, q_z]^T$ adhering to Hamilton convention. Avoids gimbal lock entirely and provides exact Direction Cosine Matrices $C_{b/n}$ and $C_{n/b}$.

### 3. Core Equations of Motion
Defined in [`include/fastjet/fdm/six_dof_fdm.hpp`](include/fastjet/fdm/six_dof_fdm.hpp):
- **Position Kinematics**:
  $$\dot{\mathbf{p}}_{NED} = C_{n/b} \mathbf{v}_b$$
- **Linear Accelerations (Newton's 2nd Law in rotating body frame)**:
  $$\dot{\mathbf{v}}_b = \frac{\mathbf{F}_{ext}^b}{m} + C_{b/n} \mathbf{g}_{NED} - \boldsymbol{\omega}_b \times \mathbf{v}_b$$
- **Angular Accelerations (Euler's Rotational Equations)**:
  $$\dot{\boldsymbol{\omega}}_b = \mathbf{I}^{-1} \left( \mathbf{M}_{ext}^b - \boldsymbol{\omega}_b \times (\mathbf{I} \boldsymbol{\omega}_b) \right)$$
- **Attitude Kinematics**:
  $$\dot{\mathbf{q}} = \frac{1}{2} \mathbf{q} \otimes [0, p, q, r]^T$$

### 4. F-16C Mass & Inertia Properties
Defined in [`include/fastjet/fdm/mass_properties.hpp`](include/fastjet/fdm/mass_properties.hpp) based on NASA / Stevens & Lewis F-16 Block 50 standards:
- **Empty Weight**: $20,500\text{ lbs} = 9,298.6436\text{ kg}$ ($m = 9,298.64\text{ kg}$)
- **Inertia Tensor**:
  $$I_{xx} = 9,496\text{ slug}\cdot\text{ft}^2 = 12,874.85\text{ kg}\cdot\text{m}^2$$
  $$I_{yy} = 55,814\text{ slug}\cdot\text{ft}^2 = 75,673.62\text{ kg}\cdot\text{m}^2$$
  $$I_{zz} = 63,100\text{ slug}\cdot\text{ft}^2 = 85,552.11\text{ kg}\cdot\text{m}^2$$
  $$I_{xz} = 982\text{ slug}\cdot\text{ft}^2 = 1,331.41\text{ kg}\cdot\text{m}^2$$
  $$I_{xy} = I_{yz} = 0$$
- Analytical inverse $\mathbf{I}^{-1}$ is precomputed at initialization, eliminating runtime matrix inversion.

### 5. 1976 US Standard Atmosphere Model
Defined in [`include/fastjet/environment/atmosphere1976.hpp`](include/fastjet/environment/atmosphere1976.hpp):
- Computes temperature $T$, ambient pressure $P$, density $\rho$, local speed of sound $a$, dynamic pressure $\bar{q} = \frac{1}{2}\rho V^2$, and Mach number $M$ up to 86 km.
- Accommodates geometric-to-geopotential altitude conversion ($H = \frac{R_E h}{R_E + h}$).

### 6. NASA TP-1538 Aerodynamics & Multidimensional Interpolation
Defined in [`include/fastjet/aero/`](include/fastjet/aero/):
- **Multidimensional Interpolation** ([`interpolator.hpp`](include/fastjet/aero/interpolator.hpp)):
  High-performance 1D linear, 2D bilinear, and 3D trilinear interpolation with boundary clamping and zero heap allocation.
- **Lookup Tables** ([`tp1538_tables.hpp`](include/fastjet/aero/tp1538_tables.hpp)):
  NASA TP-1538 wind-tunnel lookup tables in contiguous cache-aligned memory:
  - Longitudinal: $C_L(\alpha, \delta_e, \delta_{lef})$, $C_D(\alpha, \delta_e, \delta_{lef})$, $C_m(\alpha, \delta_e, \delta_{lef})$ ($\alpha \in [-20^\circ, 45^\circ]$, $\delta_e \in [-25^\circ, 25^\circ]$, $\delta_{lef} \in [0^\circ, 25^\circ]$).
  - Lateral-Directional: $C_Y(\beta, \delta_r)$, $C_l(\beta, \delta_a, \delta_r)$, $C_n(\beta, \delta_a, \delta_r)$ ($\beta \in [-30^\circ, 30^\circ]$, $\delta_a \in [-20^\circ, 20^\circ]$, $\delta_r \in [-30^\circ, 30^\circ]$).
  - Dynamic Damping: $C_{m_q}, C_{L_q}, C_{D_q}, C_{l_p}, C_{l_r}, C_{n_p}, C_{n_r}, C_{Y_p}, C_{Y_r}$ as functions of $\alpha$.
- **Force & Moment Synthesizer** ([`f16_aero_model.hpp`](include/fastjet/aero/f16_aero_model.hpp)):
  Synthesizes body-frame forces and moments from dynamic pressure $\bar{q}$, wing area $S = 300\text{ ft}^2$ ($27.87\text{ m}^2$), chord $\bar{c} = 11.32\text{ ft}$ ($3.45\text{ m}$), and wingspan $b = 30.0\text{ ft}$ ($9.14\text{ m}$):
  $$\mathbf{F}_{aero}^b = \bar{q} S \begin{bmatrix} C_L \sin\alpha - C_D \cos\alpha \\ C_Y \\ -C_L \cos\alpha - C_D \sin\alpha \end{bmatrix}, \quad \mathbf{M}_{aero}^b = \bar{q} S \begin{bmatrix} b \, C_l \\ \bar{c} \, C_m \\ b \, C_n \end{bmatrix}$$
- **Relaxed Longitudinal Static Stability (Open-Loop Instability)**:
  Correctly models positive pitch stiffness ($\frac{\partial C_m}{\partial \alpha} > 0$) at subsonic cruise. Under open-loop fixed controls without FLCS, any pitch perturbation produces exponential pitch divergence.

#### Compressibility & Speedbrake Corrections
The TP-1538 tables are low-speed wind-tunnel data carrying no Mach dependence. These are extended across the transonic and supersonic range the airframe can actually reach:
- **Prandtl-Glauert scaling** $1/\sqrt{|1 - M^2|}$ applied to lift, sideforce and moments, capped below $M = 1$ to avoid the singularity and switching to the Ackeret form supersonically.
- **Wave drag** rising from a critical Mach of 0.86 through a transonic peak of $\Delta C_D = 0.026$ at $M = 1.05$, decaying to a supersonic plateau of 0.017.
- **Mach tuck**: the aft aerodynamic-centre shift contributes $\Delta C_m = -0.030$ nose-down through the transonic region.
- **Speedbrake**: $\Delta C_D = +0.025$ at full extension, linear in position, with a small nose-up pitch increment.
- **Supersonic drag rise**: beyond the transonic peak, $C_{D,\text{wave}}$ grows again as $0.006\,(M - M_{\text{peak}})^2$ rather than settling to a flat plateau.
- **Dynamic-pressure drag**: $\Delta C_D = 0.095\,\left(\frac{q - q_0}{q_0}\right)^2$ for $q > q_0 = 60\ \text{kPa}$.

  The F-16's low-altitude speed limit is set by dynamic pressure, not Mach: the placarded 800 KEAS
  corresponds to $q \approx 102$ kPa, reached near Mach 1.2 on the deck but not until beyond Mach 2
  at altitude. A drag term depending only on Mach cannot reproduce both ends of the envelope, since
  it penalises high altitude — where the real aircraft is fastest — just as hard as sea level.
  This term represents losses that scale with absolute air loading: inlet spillage and bleed, hinge
  moments, and skin friction on a heated boundary layer.

##### Validated performance

| Condition | Simulated | Real F-16C |
|---|---|---|
| Max level speed, 11,000 m | Mach 2.06 | Mach 2.05 |
| Max level speed, sea level | Mach 1.14 ($q$ = 92 kPa) | Mach ~1.05, $q$ limit 102 kPa |
| Max dry (MIL) speed, 11,000 m | Mach 1.23 | subsonic–low supersonic |
| Military thrust, SLS | 75.6 kN | 76.3 kN (17,155 lbf) |
| Max afterburner, SLS | 129.0 kN | 131.6 kN (29,588 lbf) |
| Thrust-to-weight, combat | 1.17 | ~1.10 |

These are pinned by `test_performance_envelope.cpp`, which flies the full 6-DoF model with an
altitude-hold autopilot rather than asserting on constants.

All corrections default to `mach = 0`, which reproduces the original incompressible table data bit-for-bit.

### 7. Digital Fly-By-Wire Flight Control System (FLCS) & Actuators
Defined in [`include/fastjet/flcs/`](include/fastjet/flcs/):
- **Hydraulic Actuators** ([`actuator.hpp`](include/fastjet/flcs/actuator.hpp)):
  - First-order lag dynamics ($\dot{\delta} = \frac{1}{\tau}(\delta_{cmd} - \delta)$).
  - Stabilator: $\tau = 0.05$ s, rate limit = $60^\circ/\text{s}$, limits = $[-25^\circ, +25^\circ]$.
  - Flaperon: $\tau = 0.05$ s, rate limit = $52^\circ/\text{s}$, limits = $[-20^\circ, +20^\circ]$.
  - Rudder: $\tau = 0.05$ s, rate limit = $120^\circ/\text{s}$, limits = $[-30^\circ, +30^\circ]$.
  - Automatic Leading-Edge Flap (LEF) schedule vs $\alpha$ and $\bar{q}$.
- **Pitch Axis Control Law** ([`pitch_controller.hpp`](include/fastjet/flcs/pitch_controller.hpp)):
  - Blends pitch rate ($q_{cmd}$) at low dynamic pressure ($\bar{q} \le 2000$ Pa) and normal acceleration ($N_{z,cmd}$) at high dynamic pressure ($\bar{q} \ge 8000$ Pa).
  - Hard G-limiter strictly clamped to $[-3.0\text{ G}, +9.0\text{ G}]$.
  - Hard Angle-of-Attack limiter actively overrides pitch-up command to cap $\alpha \le 25.5^\circ$.
  - Closed-loop PI controller with pitch rate damping and integrator anti-windup.
- **Roll & Yaw Axes Control Laws** ([`lateral_directional_controller.hpp`](include/fastjet/flcs/lateral_directional_controller.hpp)):
  - Roll-rate command up to $300^\circ/\text{s}$ with automatic roll attenuation at high AoA ($\alpha \in [15^\circ, 25.5^\circ]$).
  - Aileron-Rudder Interconnect (ARI) providing turn coordination and eliminating adverse yaw.
  - Yaw rate damper and sideslip feedback for Dutch roll suppression.

### 8. Propulsion, Fuel System & Landing Gear
Defined in [`include/fastjet/propulsion/`](include/fastjet/propulsion/), [`include/fastjet/fdm/fuel_system.hpp`](include/fastjet/fdm/fuel_system.hpp) and [`include/fastjet/gear/`](include/fastjet/gear/):
- **F110-GE-129 Engine Model** ([`f110_engine.hpp`](include/fastjet/propulsion/f110_engine.hpp)):
  - Detent schedule (Cutoff / Idle / Military / Afterburner) driving sea-level static ratings of 4.45 kN idle, 75.6 kN military, and 129 kN full reheat.
  - **Installed thrust lapse** with density ratio and Mach: $F = F_{SL} \cdot \sigma^{n} (1 + k_{ram} M^2)$, where $n = 0.85$ dry and $1.00$ in reheat. Thrust falls from 75.5 kN at sea level to 23.6 kN at 12 km.
  - **Spool dynamics** as a first-order lag with direction-dependent time constants ($\tau_{up} = 3.2$ s, $\tau_{down} = 1.6$ s, $\tau_{AB} = 0.6$ s), integrated by the exact discrete solution so the response is stable at any timestep.
  - **Fuel burn** via thrust-specific fuel consumption (0.75 lb/lbf/hr dry, 2.00 wet) with flame-out when the tanks run dry.
- **Variable Mass & Inertia** ([`fuel_system.hpp`](include/fastjet/fdm/fuel_system.hpp)):
  - Gross mass and the full inertia tensor are rebuilt every timestep from the fuel remaining. A full internal load of 3,175 kg raises $I_{yy}$ by 34%, measurably reducing pitch agility.
- **Three-Point Landing Gear** ([`landing_gear.hpp`](include/fastjet/gear/landing_gear.hpp)):
  - Oleo-pneumatic struts modelled as spring-dampers, sized so the static stance carries **12% on the nose and 88% on the mains** — matching the real aircraft's CG position — with the reaction balancing weight to within 0.01%.
  - **Tyre cornering-stiffness** side-force model (force proportional to slip angle, saturating at the friction limit) rather than pure Coulomb friction, which is what makes ground handling stable and self-centring.
  - Lateral friction is capped at $\mu = 0.65$, deliberately below the geometric rollover threshold ($b/h = 1.13/1.59 = 0.71$), so the aircraft slides before it tips.
  - **Speed-scheduled nosewheel steering** tapering as $1/V^2$ above taxi speed, holding commanded lateral acceleration within the rollover limit.
  - Differential toe braking ($\mu = 0.55$), and structural failure above a 4.5 m/s sink rate.

### 9. Controller Input Processing Subsystem
Defined in [`include/fastjet/input/`](include/fastjet/input/):
- **Direct Hardware Polling** ([`joystick_driver.hpp`](include/fastjet/input/joystick_driver.hpp)):
  - SDL3 direct hardware polling backend (`SDL3InputDriver`) reading multi-axis analog controllers and hats/buttons with microsecond-level latency (~5 µs hardware polling, ~35 ns end-to-end update latency).
  - Mock driver (`VirtualHOTASDriver`) for automated deterministic testing and headless simulation.
- **Signal Conditioning** ([`signal_conditioner.hpp`](include/fastjet/input/signal_conditioner.hpp)):
  - Per-axis min/center/max calibration offsets and inversion toggles.
  - Configurable inner deadbands (suppressing center stick jitter/drift) and outer deadbands (guaranteeing 100% full-throw command at physical endstops).
- **Force-Sensing Side-Stick Simulation Curves**:
  - Emulates the rigid force-sensing feel of the real F-16 transducer side-stick on displacement desktop joysticks using the selectable polynomial transfer function:
    $$y = (1 - c)x + c x^3, \quad c \in [0.0, 1.0]$$
  - Tunable curvature $c$ provides ultra-precise micro-corrections around center stick while preserving full command authority at outer deflection.
- **Avionics Flight Controls** ([`avionics_controls.hpp`](include/fastjet/input/avionics_controls.hpp)):
  - 4-way digital pitch and roll trim hat with smooth rate slew ($0.05/\text{s}$) and manual zeroing.
  - Multi-detent throttle quadrant controller mapping raw throttle to Cutoff, Ground/Flight Idle, Military Power, and Afterburner (threshold at $0.85$).
  - Speedbrake deployment switch (Retracted, 50%, Extended) with physical hydraulic deployment slew ($60\%/\text{s}$).
  - Differential left/right toe brakes.
- **Configuration Persistence** ([`input_config.hpp`](include/fastjet/input/input_config.hpp)):
  - Structured JSON serialization and deserialization for all axes, deadbands, curvature $c$, and button/hat mappings without external dependencies.

### 10. First-Person Graphics, Collimated HUD & Cockpit Avionics
Defined in [`include/fastjet/graphics/`](include/fastjet/graphics/):
- **Camera Rig & G-Induced Head Dynamics** ([`camera_rig.hpp`](include/fastjet/graphics/camera_rig.hpp)):
  - First-person camera positioned at the true F-16 cockpit Design Eye Point (DEP: $[+1.80, 0.00, -0.65]\text{ m}$).
  - 2nd-order critically damped spring-damper ($\omega_n = 25\text{ rad/s}, \zeta = 0.85$) simulating instantaneous cockpit G-load head displacement (downward compression under $+9\text{G}$, upward under $-3\text{G}$, lateral tilt under $N_y$).
- **Optical-Infinity Collimated HUD** ([`hud_collimator.hpp`](include/fastjet/graphics/hud_collimator.hpp)):
  - Projects symbology at optical infinity ($R \to \infty$) so reticles remain horizon-locked without parallax as the pilot's head translates inside the cockpit.
  - Hardware Stencil Buffer clipping constrained to the physical 3D combiner glass polygon so symbology is clipped outside the glass viewing aperture.
  - Flight Path Marker (FPM) tracking true inertial velocity vector angles $(\gamma, \chi)$ driven by $(\theta - \alpha)$ vertically and $(\psi + \beta)$ horizontally.
  - Conformal pitch ladder, heading tape, Calibrated Airspeed (CAS) box, barometric altitude box, and digital G-meter with peak G memory.
- **Flight Instruments (Render-to-Texture)** ([`flight_instruments.hpp`](include/fastjet/graphics/flight_instruments.hpp)):
  - Off-screen $512 \times 512$ Framebuffer Object (FBO) dynamically rendering the three-light AoA indexer (slow chevron, on-speed donut, fast chevron), Attitude Director Indicator (ADI) ball with sky/ground split, and Mach number tape.
  - Mapped directly as a material texture onto the cockpit dashboard MFD panel.
- **Atmosphere & Environment** ([`sky_ground_renderer.hpp`](include/fastjet/graphics/sky_ground_renderer.hpp)):
  - Analytic single-scattering sky: wavelength-dependent Rayleigh ($\beta \propto \lambda^{-4}$) plus Henyey-Greenstein Mie aerosol scattering, with a sun disc and aureole.
  - Radiance along a view ray, where $\tau$ is the vertical optical depth and $m$ the air mass:

$$L = \left(\tau_R\,\Phi_R(\theta) + \tau_M\,\Phi_M(\theta)\right) m \cdot \frac{1 - e^{-\tau m}}{\tau m}$$

  - The trailing factor is the mean self-attenuation along the path; without it the horizon saturates to white.
  - Sky darkens and deepens with altitude via $\rho = e^{-h/H}$, $H = 8{,}500\text{ m}$.
  - **The terrain haze and the sky are the same function.** Aerial perspective fades ground into the radiance the sky quad would draw along that exact ray, and converges on the sky's own air mass in the far field, so the horizon has no seam by construction.
  - Aerosol haze is integrated through the boundary layer ($H_{aer} = 1{,}200\text{ m}$) rather than applied per unit distance, so the ground stays crisp from altitude but washes out along the deck.
- **Procedural Terrain** ([`terrain_field.hpp`](include/fastjet/graphics/terrain_field.hpp), [`terrain_mesh.hpp`](include/fastjet/graphics/terrain_mesh.hpp)):
  - Ridged fractional Brownian motion over a hash-based value-noise lattice: 5 octaves, non-integer lacunarity 2.13 to avoid axis-aligned repeats. Peaks reach $620\text{ m}$.
  - A pure function of position, so the mesh builder and any future collision query cannot disagree; identical on every machine and every run.
  - The airfield sits in a guaranteed-flat basin ($r = 2{,}600\text{ m}$, blending out over $5{,}200\text{ m}$) so takeoff and landing stay predictable.
  - Four concentric LOD rings ($125\text{ m}$ to $3{,}200\text{ m}$ spacing) reaching $91\text{ km}$: ~27k triangles total, built once at init, zero per-frame allocation.
  - Shaded by elevation and slope (lowland green, dry upland, exposed rock, snow only on high shallow ground) with sun and sky-ambient lighting in linear space.
  - $3,000\text{ m} \times 60\text{ m}$ runway with centerline, edge lines, threshold piano keys, touchdown-zone bars, and aiming-point blocks. The terrain mesh cuts a hole for the airfield rather than z-fighting decals that sit $20\text{ cm}$ above it.

---

## Directory Structure

```
.
├── CMakeLists.txt
├── LICENSE
├── Makefile
├── README.md
├── include/
│   └── fastjet/
│       ├── aero/
│       │   ├── control_surfaces.hpp  # F-16 control surface deflections and limits
│       │   ├── f16_aero_model.hpp    # Total force & moment synthesizer
│       │   ├── interpolator.hpp      # Zero-allocation 1D/2D/3D interpolators
│       │   └── tp1538_tables.hpp     # NASA TP-1538 wind-tunnel lookup tables
│       ├── environment/
│       │   └── atmosphere1976.hpp    # 1976 US Standard Atmosphere
│       ├── fdm/
│       │   ├── aircraft_forces.hpp   # Body-frame forces & moments interface
│       │   ├── flight_state.hpp      # FlightState & FlightStateDeriv structs
│       │   ├── fuel_system.hpp       # Variable gross mass & inertia from fuel state
│       │   ├── mass_properties.hpp   # F-16C mass & precomputed inertia inverse
│       │   ├── rk4_integrator.hpp    # Deterministic zero-allocation RK4 integrator
│       │   └── six_dof_fdm.hpp       # Core 6-DoF equations of motion
│       ├── gear/
│       │   └── landing_gear.hpp      # Three-point struts, tyre friction, steering
│       ├── propulsion/
│       │   └── f110_engine.hpp       # Thrust lapse, spool dynamics, fuel burn
│       ├── flcs/
│       │   ├── actuator.hpp          # Hydraulic actuator dynamics (lag, rate limits)
│       │   ├── f16_flcs.hpp          # Integrated FBW Flight Control System
│       │   ├── imu_sensor.hpp        # Synthetic IMU sensor model (Nz, Ny, rates)
│       │   ├── lateral_directional_controller.hpp # Roll-rate, ARI, yaw damper
│       │   ├── pilot_commands.hpp    # Cockpit inceptor command inputs
│       │   └── pitch_controller.hpp  # Blended q/Nz, +9G & 25.5 AoA limiters
│       ├── graphics/
│       │   ├── camera_rig.hpp        # F-16 DEP camera rig with G-load spring-damper
│       │   ├── cockpit_geometry.hpp  # 3D low-poly cockpit shell, MFD quad, combiner glass
│       │   ├── flight_instruments.hpp# FBO render-to-texture AoA indexer, ADI, Mach tape
│       │   ├── gl_common.hpp         # Mat4, Color4, and OpenGL utilities
│       │   ├── hud_collimator.hpp    # Optical-infinity collimated HUD and FPM
│       │   ├── render_engine.hpp     # Master multi-pass graphics rendering engine
│       │   ├── shader.hpp            # GLSL compilation and uniform bindings
│       │   ├── sky_ground_renderer.hpp# Scattering sky, aerial perspective, airfield
│       │   ├── terrain_field.hpp     # Deterministic ridged-fBm procedural heightfield
│       │   └── terrain_mesh.hpp      # Concentric LOD ring mesh built from the field
│       ├── input/
│       │   ├── avionics_controls.hpp # Digital trim hat, throttle detents, speedbrakes
│       │   ├── input_config.hpp      # Controller profiles & pure C++20 JSON persistence
│       │   ├── input_manager.hpp     # End-to-end input processing pipeline
│       │   ├── joystick_driver.hpp   # SDL3 hardware poller & VirtualHOTASDriver
│       │   └── signal_conditioner.hpp# Deadbands, calibration, & force-sensing curves
│       └── math/
│           ├── matrix3x3.hpp         # 3x3 matrix arithmetic and inversion
│           ├── quaternion.hpp        # Unit quaternion with DCM rotations
│           └── vector3.hpp           # 3D vector arithmetic and cross/dot products
├── src/
│   └── f16_sim_viewer.cpp            # Interactive real-time 3D flight sim demo
└── tests/
    ├── test_actuators.cpp            # Actuator lag, rate limits, and deflection bounds
    ├── test_aero_coefficients.cpp    # Validates NASA TP-1538 lookups and dCm/dalpha > 0
    ├── test_atmosphere.cpp           # Atmosphere verification vs NOAA 1976 tables
    ├── test_avionics_controls.cpp    # Digital trim hat, throttle detents, speedbrakes
    ├── test_ballistic.cpp            # Ballistic parabolic flight & equilibrium test
    ├── test_camera_rig.cpp           # F-16 DEP camera rig & G-load head dynamics
    ├── test_compressibility.cpp      # Prandtl-Glauert, wave drag, Mach tuck, speedbrake
    ├── test_config_persistence.cpp   # JSON configuration load/save round-trip test
    ├── test_flcs_level_flight.cpp    # 60-second hands-off level flight stabilization
    ├── test_flcs_pullup.cpp          # Full aft stick +9G and 25.5 AoA limiter test
    ├── test_flight_instruments.cpp   # FBO render-to-texture and instrument logic
    ├── test_force_curves.cpp         # F-16 force-sensing polynomial response curves
    ├── test_gyroscopic.cpp           # Rotational dynamics & momentum conservation
    ├── test_hud_collimation.cpp      # Collimated optical infinity math & FPM projection
    ├── test_input_pipeline.cpp       # End-to-end SDL3 polling and FLCS driving latency
    ├── test_integrated_flight.cpp    # Takeoff, landing, fuel burn across the full stack
    ├── test_interpolator.cpp         # Multidimensional interpolation verification
    ├── test_landing_gear.cpp         # Strut statics, braking, steering, gear failure
    ├── test_performance.cpp          # Zero-heap allocation check & 6M step/s benchmark
    ├── test_pitch_divergence.cpp     # Open-loop pitch divergence test (No FLCS)
    ├── test_propulsion.cpp           # Thrust lapse, spool lag, fuel burn, variable mass
    ├── test_render_pipeline.cpp      # Multi-pass OpenGL graphics pipeline verification
    ├── test_signal_conditioning.cpp  # Dual deadbands, calibration, and inversion
    ├── test_throttle_control.cpp     # Throttle slew, afterburner accel, deceleration
    ├── test_performance_envelope.cpp # Top speed, T/W and thrust lapse vs real F-16C
    ├── test_terrain_field.cpp        # Flat airfield basin, determinism, normals, bounds
    └── test_terrain_render.cpp       # Winding/culling, runway over terrain, horizon seam
```

---

## Building and Running Tests

### Requirements
- A C++20 compiler (GCC 11+ or Clang 14+)
- SDL3, libepoxy and OpenGL — needed only for the viewer and the graphics tests. The 19 physics test suites build with no external dependencies.

### Make

To compile and run all 26 verification test suites:

```bash
make run_all
```

To run the interactive real-time 3D cockpit flight simulator:

```bash
make viewer
./bin/f16_sim_viewer
```

> **Note:** the Makefile builds with `-march=native`, which tunes for the building machine and produces binaries that may fault elsewhere. Use the CMake build for anything portable or distributable.

### Simulator Controls

| Key | Function |
|-----|----------|
| Arrow Keys | Pitch and roll stick commands |
| A / D | Rudder pedals (also nosewheel steering on the ground) |
| Shift / Ctrl | Throttle increase / decrease (1.5 s idle to full reheat) |
| `+` / `-`, keypad `+` / `-`, PageUp / PageDown | Alternative throttle bindings |
| I | Invert hardware throttle axis (toggle forward=full power vs forward=cutoff) |
| J | Ignore a hardware throttle lever (keyboard only) |
| 1 / 2 / 3 / 4 | Snap throttle to Cutoff / Idle / Military / Max afterburner |
| B | Speedbrake extend / retract (2 s hydraulic travel) |
| Space | Wheel brakes, for the rollout |
| G | Landing gear up / down |
| T | Trim reset |
| R | Reset simulation |
| Esc | Exit |

### CMake

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

CMake builds with portable flags by default. Add `-DFASTJET_NATIVE_ARCH=ON` to opt into `-march=native`, or `-DFASTJET_BUILD_VIEWER=OFF` to build the physics tests alone when SDL3 is unavailable.

Individual test targets:
```bash
make test_input_pipeline     # Validates SDL3 polling and end-to-end FLCS driving latency
make test_signal_conditioning# Validates inner/outer deadbands and axis calibration
make test_force_curves       # Validates polynomial y = (1-c)*x + c*x^3 force curves
make test_avionics_controls  # Validates digital trim hats, throttle detents, speedbrakes
make test_config_persistence # Validates JSON configuration serialization and parsing
make test_actuators          # Validates hydraulic lag, rate limits, and deflection bounds
make test_flcs_level_flight  # Validates 60s hands-off level flight stability with FLCS
make test_flcs_pullup        # Validates full aft stick +9G and 25.5 deg AoA limiters
make test_pitch_divergence   # Validates open-loop pitch divergence under 200 Hz RK4
make test_aero_coefficients  # Validates NASA TP-1538 coefficients and dCm/dalpha > 0
make test_interpolator       # Validates 1D, 2D, and 3D interpolation and clamping
make test_atmosphere         # Validates atmosphere properties across troposphere/stratosphere
make test_ballistic          # Validates trajectory against closed-form analytical equations
make test_gyroscopic         # Validates torque-free angular momentum conservation
make test_performance        # Validates zero heap allocation and measures throughput
```

---

## Example Usage

```cpp
#include "fastjet/fdm/flight_state.hpp"
#include "fastjet/fdm/rk4_integrator.hpp"
#include "fastjet/environment/atmosphere1976.hpp"

using namespace fastjet::math;
using namespace fastjet::fdm;
using namespace fastjet::environment;

int main() {
    // 1. Initialize F-16 mass properties and 200 Hz integrator (dt = 0.005s)
    const MassProperties f16_mass = MassProperties::create_clean_f16();
    const RK4Integrator integrator(0.005);

    // 2. Set initial flight condition (cruise at 10,000 m altitude, 250 m/s airspeed)
    FlightState state;
    state.pos_ned = Vector3(0.0, 0.0, -10000.0); // NED z = -10,000 m (altitude = 10,000 m)
    state.vel_b   = Vector3(250.0, 0.0, 0.0);     // 250 m/s along forward body axis
    state.omega_b = Vector3::zero();
    state.q_att   = Quaternion::identity();       // Level flight

    double sim_time = 0.0;

    // 3. Hot simulation loop (allocates zero heap memory)
    for (int step = 0; step < 2000; ++step) { // 10 seconds of simulation
        // Query air data for aerodynamics
        const AirData air = Atmosphere1976::compute(state.altitude(), state.airspeed());

        // Step simulation using custom force provider lambda
        integrator.step(state, sim_time, f16_mass, [&](double t, const FlightState& s) noexcept {
            AircraftForces forces;
            // Example: Trim lift force opposing gravity
            forces.force_b = Vector3(0.0, 0.0, -f16_mass.mass_kg * SixDoFFDM::GRAVITY_ACCEL);
            return forces;
        });
    }

    return 0;
}
```
