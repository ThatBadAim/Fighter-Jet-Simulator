# Fast Jet Simulator - Authentic Audio Asset Directory

Place real recorded audio samples (`.wav` format) in this directory. The simulator's audio engine will automatically detect, load, convert, and stream them.

If a file is missing, the engine gracefully remains completely silent for that channel without emitting synthetic noise.

## Supported Sound Files

| Filename | Description | Behavior | Recommended Source |
|---|---|---|---|
| `engine_spool.wav` | F-16 cockpit turbofan interior spool | Looped; pitch/volume scales with core RPM% | FlightGear F-16 (`Sounds/jet-interior.wav`) |
| `afterburner.wav` | Deep F110 afterburner combustion rumble | Looped; fades in when throttle > 85% detent | FlightGear F-16 (`Sounds/reheat.wav`) |
| `wind_rush.wav` | Canopy boundary-layer aerodynamic airflow hiss | Looped; volume scales with dynamic pressure $q$ | FlightGear (`Sounds/wind.wav`) or Freesound CC0 |
| `touchdown.wav` | Tire contact chirp on runway touchdown | One-shot on gear touchdown transition | FlightGear / DCS tire screech |
| `gear_transit.wav` | Mechanical gear actuator unlock/transit clunk | One-shot when gear deployed/retracted | FlightGear F-16 gear sound |
| `over_g.wav` | Cockpit aural Over-G warning tone | Repeated while $N_z > 8.5\text{G}$ | Real F-16 Betty aural warning |
| `stall.wav` | Stall warning horn / high-AoA tone | Repeated while $\alpha > 20^\circ$ | Authentic military horn |
| `pull_up.wav` | Bitchin' Betty "PULL UP" voice callout | One-shot on critical sink rate | Kim Crow F-16 Betty recording |

## Where to get authentic sounds
1. **FlightGear F-16 Falcon (GPL v2)**:
   The open-source FlightGear aircraft repository provides authentic F-16 recordings in:
   `https://github.com/FlightGear/f16/tree/master/Sounds`
2. **NASA Audio Collection (Public Domain)**:
   Dryden Flight Research Center F-16 / jet engine acoustic recordings.
3. **Freesound.org (CC0 / CC-BY)**:
   Search for "jet cockpit ambience", "turbofan idle", "afterburner roar".
