#pragma once

#include "fastjet/aircraft/aircraft_type.hpp"
#include <cstdint>

namespace fastjet::graphics {

/// @brief Dogfight symbology: target track, gunsight, weapons and the fight's state.
/// Directions are unit vectors in the ownship's body axes (X nose, Y right wing, Z belly).
struct CombatTelemetry {
    bool active = false;              ///< Dogfight mode running (draws the combat HUD)
    bool target_valid = false;        ///< Bandit tracked
    double target_dir_b[3] = {1.0, 0.0, 0.0};
    double target_range_m = 0.0;
    double closure_mps = 0.0;
    double aspect_deg = 0.0;
    double ata_deg = 0.0;
    bool pipper_valid = false;        ///< Director gunsight solution available
    double pipper_dir_b[3] = {1.0, 0.0, 0.0};
    double gun_max_range_m = 1200.0;  ///< Outer edge of the range ring
    bool in_gun_range = false;
    int ammo = 0;
    bool gun_firing = false;
    char gun_name[12] = "";
    double turn_rate_dps = 0.0;
    double ps_mps = 0.0;
    int hits_scored = 0;
    int hits_taken = 0;
    double hit_cue_s = 0.0;           ///< "HIT" flash time remaining
    bool engine_damage = false;
    bool fuel_leak = false;
    bool engine_fire = false;
    bool control_damage = false;
    char status[64] = "";             ///< Fight description (skill, set-up)
    char banner[40] = "";             ///< Outcome, empty while the fight is on
    char debrief[96] = "";            ///< One-line score under the banner
    char debrief2[96] = "";           ///< Second debrief line (evade: missiles and chaff)

    // Radar warning receiver and countermeasures. Bearings are relative
    // azimuths: 0 on the nose, positive clockwise, degrees.
    bool rwr_active = false;          ///< Draw the RWR azimuth display
    int rwr_level = 0;                ///< 0 clear, 1 search (painted), 2 lock, 3 missile launch
    bool rwr_emitter_valid = false;
    double rwr_emitter_bearing_deg = 0.0;
    char rwr_symbol[4] = "";          ///< Emitter type, e.g. "16"
    bool rwr_missile_valid = false;
    double rwr_missile_bearing_deg = 0.0;
    double rwr_missile_range_m = 0.0;
    int chaff = -1;                   ///< Cartridges left (-1: no dispenser shown)
    bool blink = false;               ///< Warning flash phase (~3 Hz)
};

/// @brief The fire-control radar (FCR) page: a B-scope of azimuth against range.
/// Azimuths are from the ownship's heading in the stabilised scan frame,
/// positive right, degrees.
struct RadarTelemetry {
    static constexpr int kMaxHits = 32;
    struct Hit {
        float az_deg = 0.0f;
        float range_nm = 0.0f;
        int age = 0;                  ///< Scan frames since the paint (0: this frame)
    };

    bool page_fcr = false;            ///< Left MFD shows FCR (else the FLCS page)
    bool fitted = false;              ///< The airframe carries an air-to-air radar
    int mode = 0;                     ///< 0 off, 1 RWS, 2 STT
    double range_scale_nm = 20.0;
    double az_limit_deg = 60.0;
    int bars = 4;
    double ant_az_deg = 0.0;          ///< Antenna position (the carets)
    double ant_el_deg = 0.0;
    double pitch_deg = 0.0;           ///< Ownship attitude for the artificial horizon
    double roll_deg = 0.0;
    int hit_count = 0;
    Hit hits[kMaxHits]{};
    double cursor_az_deg = 0.0;       ///< Acquisition cursor
    double cursor_range_nm = 10.0;
    int cov_top_kft = 0;              ///< Altitude block the scan covers at the cursor's range
    int cov_bottom_kft = 0;
    // STT
    bool track_memory = false;        ///< Coasting on an extrapolated track
    double tgt_az_deg = 0.0;
    double tgt_range_nm = 0.0;
    double tgt_alt_kft = 0.0;
    double tgt_rel_heading_deg = 0.0; ///< Target track relative to the ownship's heading
    double tgt_heading_deg = 0.0;     ///< True track, 0-360
    double tgt_gs_kt = 0.0;
    double closure_kt = 0.0;
    int aspect_tens = 0;              ///< Target aspect in tens of degrees (18: head-on)
    char aspect_side = 'R';           ///< Which side of the target the ownship is on
    bool blink = false;               ///< ~3 Hz flash phase
};

/// @brief Comprehensive real-time cockpit avionics and propulsion telemetry
/// All data required for in-cockpit displays and aural alerts.
struct AvionicsTelemetry {
    // Aircraft Type
    aircraft::AircraftType aircraft_type = aircraft::AircraftType::F16_FIGHTING_FALCON;

    // Airframe Change Confirmation
    // Counts down from AIRCRAFT_SWITCH_BANNER_SEC after a type change, so the
    // HUD can confirm on-screen that the switch reached the flight model.
    // Zero means no confirmation is in progress.
    double aircraft_switch_timer = 0.0;

    // Engine & Propulsion
    double throttle_input    = 0.0;     // Commanded throttle [0.0 - 1.0]
    double net_thrust_n      = 0.0;     // Net engine thrust [N]
    double engine_rpm_pct    = 65.0;    // Core rotor speed [65% idle - 100% MIL - 104% AB]
    double engine_ftit_deg_c = 400.0;   // Fan Turbine Inlet Temperature [°C]
    double nozzle_pos_pct    = 10.0;    // Variable exhaust nozzle opening [0% - 100%]
    double oil_pressure_psi  = 50.0;    // Engine lube oil pressure [PSI]
    double hyd_press_a_psi   = 3000.0;  // Flight control hydraulic system A [PSI]
    double hyd_press_b_psi   = 3000.0;  // Flight control hydraulic system B [PSI]
    const char* detent_str   = "IDLE";  // "CUTOFF", "IDLE", "MIL", "AFTERBURNER"

    // Fuel System
    double fuel_remaining_kg = 3175.0;  // Total internal fuel remaining [kg]
    double fuel_fraction     = 1.0;     // Fuel remaining fraction [0.0 - 1.0]
    double fuel_flow_kg_hr   = 850.0;   // Current burn rate [kg/hr]

    // Landing Gear & Brakes
    bool gear_deployed       = true;    // True if landing gear handle is DOWN
    bool gear_collapsed      = false;   // True if structural collapse occurred on touchdown
    double gear_transit_pos  = 1.0;     // 0.0 = UP and locked, 1.0 = DOWN and locked
    double brake_left        = 0.0;     // Left toe brake [0.0 - 1.0]
    double brake_right       = 0.0;     // Right toe brake [0.0 - 1.0]
    double touchdown_sink_mps= 0.0;     // Touchdown sink rate [m/s]
    bool on_ground           = false;   // Any gear strut compressed

    // Aerodynamic Surfaces
    double speedbrake_pos    = 0.0;     // Speedbrake hydraulic extension [0.0 - 1.0]

    // System Warnings & Faults
    bool is_crashed          = false;   // Ground collision / airframe impact
    bool over_g_alert        = false;   // Nz > 8.5G or < -2.5G
    bool high_aoa_alert      = false;   // AoA > 20.0 deg
    bool bingo_fuel_alert    = false;   // Fuel < 800 kg
    bool low_altitude_alert  = false;   // Low altitude sink rate / pull up
    bool master_caution      = false;   // Master caution lamp illuminated

    // Hardware & Flight Control Status
    bool is_hardware_hotas   = false;
    const char* input_name   = "KEYBOARD";

    // On-Board Flight Computer (OFC) Status
    bool   ofc_gloc_active        = false;   ///< True while pilot is incapacitated (GLOC or recovery)
    bool   ofc_gcas_active        = false;   ///< True while Auto-GCAS manoeuvre is executing
    bool   ofc_tumble_active      = false;   ///< True while Auto-Tumble & Spin recovery is active
    double ofc_gloc_blackout_frac = 0.0;     ///< Screen-space fade [0=clear, 1=full black]
    double ofc_gcas_tti_sec       = 999.0;   ///< Seconds to terrain impact at current trajectory
    double ofc_g_exposure         = 0.0;     ///< G-exposure accumulator value (debug / HUD readout)

    CombatTelemetry combat{};
    RadarTelemetry radar{};
};

} // namespace fastjet::graphics
