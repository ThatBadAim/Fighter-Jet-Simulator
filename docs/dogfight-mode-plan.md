# Dogfight Mode — Implementation Plan

A realism-first air combat mode: within-visual-range (WVR) basic fighter
manoeuvres (BFM) against AI adversaries that fly the **same** 6-DoF physics,
control laws and pilot-physiology model as the player. Winning should come from
energy management, turn performance and weapons employment inside real
envelopes, not from game rules.

---

## 1. Design pillars

1. **Same physics for everyone.** AI pilots produce the same `PilotCommands` and
   throttle a human does and go through the same FLCS, OFC (GLOC, Auto-GCAS),
   engine and RK4 integrator. An AI can never turn tighter, accelerate harder or
   ignore G than the airframe allows. This is the single most important realism
   rule and it is enforced structurally, not by tuning.
2. **Real envelopes, visible to the player.** Guns fly ballistic rounds with real
   muzzle velocity and rate of fire; missiles have real motors, seeker limits and
   minimum/maximum ranges. The HUD shows the envelope so the player learns *why*
   a shot missed.
3. **Energy is the currency.** Turn rate, turn radius, specific excess power and
   corner speed come out of the existing aero/propulsion models — the mode adds
   readouts, not special cases.
4. **Realism with honest assists.** Every assist (target box, lead cue, relaxed
   GLOC) is an explicit, labelled setting, off in the "Realistic" preset.
5. **Testable like the rest of the sim.** Every system lands with deterministic
   closed-loop tests in the style of `tests/test_ofc_closed_loop.cpp`.

## 2. What already exists to build on

| Area | Existing asset | Where |
|---|---|---|
| Flight dynamics | Validated 6-DoF RK4 for five airframes, corrected compressibility, per-profile limits | `include/fastjet/fdm/`, `aero/` |
| Flight controls | G/alpha-limited FBW (F-16/F-15EX/Typhoon/F-22), feel-limited A-10, roll-rate trim | `include/fastjet/flcs/aircraft_flcs.hpp`, `f16_flcs.hpp` |
| Pilot physiology | GLOC exposure model, greyout/blackout fade | `onboard_flight_computer.hpp` |
| Safety systems | Auto-GCAS, departure recovery (airframe-aware) | `onboard_flight_computer.hpp` |
| World | Procedural terrain, collision, atmosphere, wind/turbulence | `environment/`, `graphics/terrain_field.hpp` |
| Visuals | Cockpit + chase camera, collimated HUD, MFDs, GLB model renderer with camera-relative matrices | `graphics/render_engine.hpp`, `model_glb.hpp`, `hud_collimator.hpp` |
| Audio | Engine synthesis, sample one-shots (over-G, stall) | `audio/audio_engine.hpp` |
| UI | Menu system, settings persistence | `ui/` |
| Test harness | Closed-loop per-airframe sim with IMU→OFC→FLCS→engine→RK4 | `tests/test_ofc_closed_loop.cpp` (`ClosedLoopSim`) |

## 3. Gaps

- **Single-aircraft main loop.** Every per-aircraft object (engine, gear, FLCS,
  OFC, mass, forces, integrator step, collision) is a local in `main()` of
  `src/f16_sim_viewer.cpp`. There is no aircraft entity to instantiate twice.
- **One visual model.** `assets/models/f16.glb` is the only airframe model, and
  the renderer draws exactly one aircraft (the ownship).
- **No weapons, sensors, AI, damage, or mission flow.**

## 4. Architecture

```
World (fixed capacity, zero heap in the 200 Hz loop)
 ├── Aircraft[kMaxAircraft = 8]       ← extracted from main(); one per jet
 │     ├── FlightState, MassProperties, forces
 │     ├── MultiEngine, LandingGear, AircraftFLCS, OnBoardFlightComputer
 │     ├── PilotSource: Human(InputManager) | AI(BfmPilot)
 │     ├── WeaponsStore (gun rounds, missiles, flares)
 │     ├── Sensors (radar/IRST track, RWR, visual)
 │     └── Damage / alive state
 ├── Projectile pool[kMaxRounds ≈ 4096]   ← gun rounds, swept-segment hit tests
 ├── Missile pool[kMaxMissiles = 32]
 ├── Countermeasure pool (flares)
 └── Engagement (setup, scoring, end conditions, event log for debrief)
```

- **`sim::Aircraft`** owns exactly what `ClosedLoopSim` owns today plus the
  viewer's gear, wind and collision handling, and exposes
  `step(dt, PilotCommands, throttle, const WorldView&)`. The ownship becomes
  `aircraft[0]`; the viewer loop becomes "gather inputs → step world → render".
- **Pilot sources are interchangeable.** The human path is today's
  `InputManager` + keyboard smoothing. The AI path produces the same struct. AI
  jets run their own OFC, so Auto-GCAS protects them from terrain and GLOC can
  knock an AI pilot out mid-fight exactly as it can the player.
- **Rendering others.** Draw each non-ownship `Aircraft` with
  `ModelGLB::relative_matrix(state, camera_origin, WORLD)` (already camera-relative,
  so no precision loss at range), plus a distance-scaled "spotting dot" so a
  bandit is visible at realistic ranges on a 1080p screen.
- **Determinism.** Fixed 200 Hz step, seeded RNG per engagement → an
  engagement can be replayed from its inputs (debrief, regression tests).

## 5. Phases

Each phase ends playable and fully tested. Sizes: S ≈ days, M ≈ 1–2 weeks,
L ≈ 2–4 weeks of focused work.

### Phase 0 — Extract the aircraft entity (M)
- Move the per-aircraft pipeline out of `main()` into `sim::Aircraft`; add
  `sim::World` holding the fixed array.
- **No behaviour change.** Acceptance: every existing suite passes, and a new
  test flies the same inputs through the old path and `sim::Aircraft` with
  bit-identical state after 60 s.

### Phase 1 — Second aircraft, BFM range (M)
- Spawn N aircraft; render them; chase camera can cycle targets.
- **"Range" sub-mode:** a scripted target (straight-and-level, constant-G turn,
  weave) for gunnery and pursuit practice.
- HUD additions: target designator box, range, closure (Vc), aspect angle,
  antenna-train angle, own turn rate, Ps (specific excess power) readout.
- Mid-air collision between aircraft (bounding capsules).
- Tests: target script follows its path; HUD geometry (range, aspect) matches
  analytic values for known placements.

### Phase 2 — Guns (L)
- Per-airframe gun data (verify figures against primary sources before shipping):

  | Airframe | Gun | Rate | Muzzle vel. | Rounds |
  |---|---|---|---|---|
  | F-16C | M61A1 20 mm | ~6,000 rpm | ~1,050 m/s | 511 |
  | F-15EX | M61A1 20 mm | ~6,000 rpm | ~1,050 m/s | ~510 (verify) |
  | F-22A | M61A2 20 mm | ~6,000 rpm | ~1,050 m/s | 480 |
  | Typhoon | Mauser BK-27 27 mm | ~1,700 rpm | ~1,025 m/s | 150 |
  | A-10C | GAU-8/A 30 mm | ~3,900 rpm | ~1,010 m/s | 1,174 |

- Rounds are pooled projectiles inheriting the shooter's velocity, with gravity,
  drag (atmosphere-dependent) and a dispersion cone; hits via swept segment vs
  target capsules each step (rounds move ~5 m per step, so point tests miss).
- Gunsight: LCOS pipper and an EEGS-style funnel computed by integrating a
  projected bullet stream against the target's current turn — the same
  ballistics as the real rounds, so the funnel is honest.
- Recoil/vibration on the camera rig; gun audio; ammo counter on HUD/MFD.
- Tests: projectile drop/time-of-flight vs analytic ballistics; a
  non-manoeuvring target at 600 m is hit when the pipper is on it; funnel
  predicts the rounds' actual position within tolerance.

### Phase 3 — Damage model (M)
- Component damage rather than hit points: engine(s) (thrust loss, fire), fuel
  leak (fuel_kg drain), control surfaces (reduced actuator authority/rate in
  `Actuator`), hydraulics, pilot. Twin-engine jets survive losing one engine;
  the A-10's redundancy is a real advantage.
- Damage feeds the existing models (thrust limit, actuator limits, mass), so a
  damaged jet *flies* damaged.
- Ejection and kill/score events.

### Phase 4 — AI BFM pilot (L, the core of the mode)
- Layered pilot: **tactical layer** (situation assessment every ~0.2 s:
  offensive / defensive / neutral from range, aspect, angle-off, energy state)
  → **manoeuvre layer** (pure/lead/lag pursuit, high/low yo-yo, break turn,
  jink, scissors, one-circle vs two-circle choice from relative turn
  performance, extension/separation) → **control layer** (G/roll/throttle
  controllers that output `PilotCommands`).
- Energy logic uses the jet's real corner speed and Ps (computed from the aero
  and engine models at the current condition, not tables).
- Skill tiers as physiological/perceptual limits, not cheats: reaction delay,
  G tolerance (feeds the GLOC model), tracking error, tactical choices, and how
  often it checks six.
- AI uses Auto-GCAS like the player; it will also respect a minimum altitude
  "hard deck" setting.
- Tests: AI-vs-AI 1v1 in identical jets always terminates (kill, bingo fuel or
  timeout) with no terrain impacts; an offensive AI converts a 3,000 ft
  perch set-up into a guns solution within a bounded time; a defensive AI
  defeats a gun pass from a lower-skill attacker a measurable share of the time.

### Phase 5 — Short-range missiles & countermeasures (L)
- IR missiles per airframe: AIM-9X (F-16, F-15EX, F-22), ASRAAM/IRIS-T
  (Typhoon), AIM-9M (A-10). Rocket motor with burn time, drag, max-G
  airframe limit, proportional navigation guidance, seeker gimbal/field-of-view
  limits, off-boresight capability per variant, fuze and warhead lethal radius.
- Seeker tone (growl → lock), uncage, dynamic launch zone (Rmin / Rmax /
  no-escape) on the HUD.
- Flares with burn time and IR intensity; seeker decoy probability from
  relative intensity, aspect and flare/target separation.
- Tests: PN intercept of a non-manoeuvring target; missile cannot pull more than
  its limit; launches outside Rmax fall short; seeker loses a target outside
  its gimbal.

### Phase 6 — Sensors & awareness (M)
- Radar air-combat modes (boresight, vertical scan, auto-acquisition) with
  realistic lock ranges; RWR for AI radar locks; padlock view (hold eyes on the
  bandit, respecting head-movement limits); optional IRST.
- Visual detection limits: the bandit is only boxed on HUD when a sensor or the
  eyes could plausibly see it (assist setting can relax this).

### Phase 7 — Game flow & UI (M)
- Main menu: **DOGFIGHT** entry → setup screen: own/adversary airframe,
  adversary count (1v1 first, then 1v2/2v2), start geometry (neutral head-on
  merge at 2 nm / offensive perch / defensive perch / high-aspect BVR-to-merge),
  altitude, weather, AI skill, weapons (guns only / guns + IR), realism preset.
- Realism presets: **Realistic** (all effects, no assists), **Standard** (target
  box, lead cue), **Relaxed** (reduced GLOC onset, spotting dots) — all built
  from individual toggles in `UserSettings`.
- End-of-engagement debrief: outcome, shots/hits, time in G, energy history,
  plus a top-down replay from the deterministic event log (optionally export
  Tacview ACMI for external review).

### Phase 8 — Performance & polish (S–M)
- Budget: 8 aircraft + 4,096 rounds + 32 missiles within ~1 ms of the 5 ms
  physics step on the target machine; profile `compute_forces_and_moments`
  (4 calls per RK4 step per aircraft).
- Distinct visual models for the F-15EX, Typhoon, F-22 and A-10 (currently all
  render as the F-16; sourcing licensed GLBs is a dependency).
- Audio: gun, missile launch/growl, RWR, Betty calls.

## 6. Milestones

1. **MVP — Guns BFM (Phases 0–4):** 1v1, same-type jets, guns only, neutral /
   offensive / defensive set-ups, three AI skill tiers. This is already a
   complete, realistic training product.
2. **Missiles (Phase 5):** short-range IR + flares.
3. **Full mode (Phases 6–8):** sensors, multi-bandit, mixed airframes, debrief.

## 7. Testing strategy

- Deterministic closed-loop tests for every system (as in
  `test_ofc_closed_loop.cpp`); engagements replayed from seeds in CI.
- Analytic checks where physics allows: ballistics, PN miss distance, turn-rate
  and radius vs `(n, V)` relations.
- Behavioural statistics for AI: win-rate bands per skill tier across seeded
  set-ups, zero terrain impacts, bounded engagement length.
- Performance test with the full entity budget.

## 8. Decisions needed from you

1. **Missiles in the first release, or guns-only MVP first?** (Recommended:
   guns-only MVP — it exercises every system except guidance and is where BFM
   realism lives.)
2. **BVR (AIM-120/Meteor) in scope at all?** It is a different game (radar,
   datalink, notching); recommended to leave out of this mode.
3. **Mixed-type matchups at launch** (e.g. F-16 vs Typhoon), or same-type only
   until the AI is proven? Mixed types need the AI's energy logic to be solid.
4. **Aircraft models:** source/licence GLBs for the other four airframes, or
   accept the F-16 model for all in the MVP.
5. **Damage model depth:** component damage (recommended, realistic) vs simple
   hit points (faster to build).
