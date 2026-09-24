#pragma once

#include "fastjet/aircraft/aircraft_type.hpp"
#include <cstdint>

namespace fastjet::graphics {

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
};

} // namespace fastjet::graphics
