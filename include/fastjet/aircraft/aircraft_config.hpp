#pragma once

#include "aircraft_type.hpp"
#include "../math/vector3.hpp"

namespace fastjet::aircraft {

/**
 * @brief Verified mass and inertia properties for an aircraft type.
 */
struct MassConfig {
    double empty_mass_kg{9298.64};
    double internal_fuel_capacity_kg{3175.1};
    double mtow_kg{19187.0};
    double Ixx{12875.0};
    double Iyy{75674.0};
    double Izz{85552.0};
    double Ixz{1331.0};
};

/**
 * @brief Engine ratings and propulsion installation parameters.
 */
struct PropulsionConfig {
    int engine_count{1};
    double static_idle_thrust_n{4450.0};
    double static_dry_thrust_n{75600.0};
    double static_ab_thrust_n{129000.0};
    bool has_afterburner{true};
    bool has_supercruise{false};
    double supercruise_mach{1.0};
    double tau_spool_up{3.2};
    double tau_spool_down{1.6};
    double tau_ab_light{0.6};
    double tsfc_dry{2.12e-5};
    double tsfc_ab{5.66e-5};
    double thrust_z_offset_m{0.0};      ///< Pitch-thrust arm (A-10 high nacelle = -1.2m)
    bool has_thrust_vectoring{false};   ///< 2D pitch thrust vectoring (F-22)
    double tvc_max_deflection_deg{0.0}; ///< Max TVC deflection (+/- 20 deg for F-22)
    double tvc_moment_arm_m{0.0};       ///< Distance from CG to TVC nozzle (~6.0m for F-22)
};

/**
 * @brief Aerodynamic geometric references and polar coefficients.
 */
struct AeroConfig {
    double s_ref{27.87};               ///< Wing reference area [m^2]
    double b_span{9.144};              ///< Wingspan [m]
    double c_bar{3.450};               ///< Mean aerodynamic chord [m]
    double aspect_ratio{3.0};          ///< Wing aspect ratio
    double sweep_le_deg{40.0};         ///< Leading edge sweep [deg]
    double cd0{0.020};                 ///< Parasitic zero-lift drag coefficient
    double k_induced{0.16};            ///< Induced drag factor: CD = CD0 + k * CL^2
    double cl_alpha{4.0};              ///< Lift-curve slope [1/rad]
    double cl_max{1.4};                ///< Clean max lift coefficient
    double mach_crit{0.86};            ///< Critical Mach drag-divergence onset
    double mach_peak{1.05};            ///< Transonic drag peak Mach
    double cd_wave_peak{0.026};        ///< Peak transonic wave drag
    double cd_wave_super{0.017};       ///< Supersonic wave drag plateau
    double cd_speedbrake{0.025};       ///< Full speedbrake drag increment
    double cm_speedbrake{0.006};       ///< Full speedbrake pitching moment
    double pitch_damping{ -10.0 };     ///< Cmq pitch damping derivative
    double roll_damping{ -0.45 };      ///< Clp roll damping derivative
    double yaw_damping{ -0.20 };       ///< Cnr yaw damping derivative
};

/**
 * @brief Flight control laws, envelope limits, and augmentation.
 */
struct FLCSConfig {
    enum class LawType {
        DIGITAL_FBW_G_ALPHA,  ///< F-16 & F-15EX: G-command / Alpha-limited digital FBW
        CAREFREE_DELTA_CANARD,///< Eurofighter: Relaxed pitch stability carefree FBW
        FBW_TVC_ALLOCATED,    ///< F-22: 5th-gen FBW with unified aero and 2D TVC
        HYDRO_SAS_AUGMENTED   ///< A-10: Mechanical linkage with dual pitch/yaw SAS
    };

    LawType law_type{LawType::DIGITAL_FBW_G_ALPHA};
    double max_g_positive{9.0};
    double max_g_negative{-3.0};
    double alpha_limit_deg{25.2};
    double max_roll_rate_dps{308.0};
};

/**
 * @brief Tricycle landing gear geometry and strut characteristics.
 */
struct LandingGearConfig {
    math::Vector3 nose_pos_b{3.60, 0.0, 0.30};
    double nose_spring_k{1.03e5};
    double nose_damping_c{1.20e4};
    math::Vector3 main_l_pos_b{-0.50, -1.13, 0.30};
    math::Vector3 main_r_pos_b{-0.50, 1.13, 0.30};
    double main_spring_k{3.70e5};
    double main_damping_c{4.30e4};
    double rest_length{1.40};
    double max_stroke{0.46};
    double max_steer_deg{32.0};
};

/**
 * @brief Complete unified aircraft configuration specification.
 */
struct AircraftConfig {
    AircraftType type{AircraftType::F16_FIGHTING_FALCON};
    const char* display_name{"F-16C Fighting Falcon"};
    MassConfig mass{};
    PropulsionConfig propulsion{};
    AeroConfig aero{};
    FLCSConfig flcs{};
    LandingGearConfig gear{};

    /**
     * @brief Returns the verified technical configuration for any supported aircraft.
     */
    [[nodiscard]] static constexpr AircraftConfig get(AircraftType type) noexcept {
        AircraftConfig cfg{};
        cfg.type = type;

        switch (type) {
            case AircraftType::F16_FIGHTING_FALCON: {
                cfg.display_name = "F-16C Fighting Falcon";
                // 19,700 lb empty (USAF fact sheet), 7,000 lb internal fuel, 42,300 lb MTOW.
                // Inertias are the Stevens & Lewis tensor (measured at 20,500 lb)
                // scaled to the empty weight; FuelSystem scales them back up with fuel.
                cfg.mass = MassConfig{
                    8935.8, 3175.1, 19187.0,
                    12373.0, 72723.0, 82216.0, 1279.0
                };
                cfg.propulsion = PropulsionConfig{
                    1, 4450.0, 75600.0, 129000.0,
                    true, false, 1.0,
                    3.2, 1.6, 0.6,
                    2.12e-5, 5.66e-5, 0.0,
                    false, 0.0, 0.0
                };
                cfg.aero = AeroConfig{
                    27.87, 9.144, 3.450, 3.0, 40.0,
                    0.020, 0.16, 4.0, 1.4,
                    0.86, 1.05, 0.026, 0.017,
                    0.025, 0.006, -10.0, -0.45, -0.20
                };
                cfg.flcs = FLCSConfig{
                    FLCSConfig::LawType::DIGITAL_FBW_G_ALPHA,
                    9.0, -3.0, 25.2, 308.0
                };
                cfg.gear = LandingGearConfig{
                    math::Vector3(3.60, 0.0, 0.30), 1.03e5, 1.20e4,
                    math::Vector3(-0.50, -1.13, 0.30),
                    math::Vector3(-0.50, 1.13, 0.30),
                    3.70e5, 4.30e4, 1.40, 0.46, 32.0
                };
                break;
            }

            case AircraftType::F15EX_EAGLE_II: {
                cfg.display_name = "F-15EX Eagle II";
                // 31,700 lb empty, 13,550 lb internal fuel, 81,000 lb MTOW
                cfg.mass = MassConfig{
                    14379.0, 6146.0, 36741.0,
                    42500.0, 255000.0, 290000.0, 3100.0
                };
                // 2x GE F110-GE-129 engines: 34,000 lbf dry (151.2 kN), 59,000 lbf AB (262.4 kN)
                cfg.propulsion = PropulsionConfig{
                    2, 8900.0, 151200.0, 262400.0,
                    true, false, 1.0,
                    2.8, 1.4, 0.5,
                    2.12e-5, 5.66e-5, 0.0,
                    false, 0.0, 0.0
                };
                // S_ref = 608 sq ft (56.49 m^2), b = 42.8 ft (13.05 m), c_bar = 15.94 ft (4.86 m)
                cfg.aero = AeroConfig{
                    56.49, 13.045, 4.858, 3.01, 45.0,
                    0.0175, 0.12, 4.4, 1.45,
                    0.88, 1.15, 0.028, 0.018,
                    0.045, 0.008, -12.5, -0.42, -0.22
                };
                // Modern Digital Fly-By-Wire system
                cfg.flcs = FLCSConfig{
                    FLCSConfig::LawType::DIGITAL_FBW_G_ALPHA,
                    9.0, -3.0, 30.0, 260.0
                };
                // Wheelbase: 5.50m, Track: 2.76m
                cfg.gear = LandingGearConfig{
                    math::Vector3(4.80, 0.0, 0.40), 1.50e5, 1.80e4,
                    math::Vector3(-0.70, -1.38, 0.40),
                    math::Vector3(-0.70, 1.38, 0.40),
                    5.20e5, 6.00e4, 1.55, 0.50, 30.0
                };
                break;
            }

            case AircraftType::EUROFIGHTER_TYPHOON: {
                cfg.display_name = "Eurofighter Typhoon";
                // 11,000 kg empty, 4,998 kg fuel, 23,500 kg MTOW
                cfg.mass = MassConfig{
                    11000.0, 4998.0, 23500.0,
                    19500.0, 98000.0, 115000.0, 1500.0
                };
                // 2x Eurojet EJ200: 120.0 kN dry, 180.0 kN AB. Dry supercruise at Mach 1.50!
                cfg.propulsion = PropulsionConfig{
                    2, 6000.0, 120000.0, 180000.0,
                    true, true, 1.50,
                    2.4, 1.2, 0.4,
                    2.10e-5, 4.80e-5, 0.0,
                    false, 0.0, 0.0
                };
                // S_ref = 51.2 m^2, b = 10.95 m, c_bar = 4.22 m, delta-canard
                cfg.aero = AeroConfig{
                    51.20, 10.950, 4.220, 2.34, 53.0,
                    0.0150, 0.14, 3.8, 1.60,
                    0.90, 1.10, 0.020, 0.014,
                    0.040, 0.005, -9.0, -0.38, -0.18
                };
                // Quadruple carefree handling digital FBW
                cfg.flcs = FLCSConfig{
                    FLCSConfig::LawType::CAREFREE_DELTA_CANARD,
                    9.0, -3.0, 35.0, 270.0
                };
                // Wheelbase: 4.40m, Track: 2.80m
                cfg.gear = LandingGearConfig{
                    math::Vector3(3.80, 0.0, 0.35), 1.25e5, 1.50e4,
                    math::Vector3(-0.60, -1.40, 0.35),
                    math::Vector3(-0.60, 1.40, 0.35),
                    4.40e5, 5.20e4, 1.45, 0.48, 35.0
                };
                break;
            }

            case AircraftType::F22_RAPTOR: {
                cfg.display_name = "F-22A Raptor";
                // 19,659 kg empty (43,340 lb), 8,165 kg fuel (18,000 lb), 37,875 kg MTOW
                cfg.mass = MassConfig{
                    19659.0, 8165.0, 37875.0,
                    61012.0, 379629.0, 427082.0, 4745.0
                };
                // 2x F119-PW-100: 232.0 kN dry, 312.0 kN AB. Dry supercruise at Mach 1.82!
                // 2D Pitch TVC nozzles (+/- 20 deg)
                cfg.propulsion = PropulsionConfig{
                    2, 12000.0, 232000.0, 312000.0,
                    true, true, 1.82,
                    2.2, 1.2, 0.5,
                    1.98e-5, 5.20e-5, 0.0,
                    true, 20.0, 6.0
                };
                // S_ref = 78.04 m^2 (840 sq ft), b = 13.56 m, c_bar = 6.40 m
                cfg.aero = AeroConfig{
                    78.04, 13.564, 6.401, 2.36, 48.0,
                    0.0165, 0.13, 3.9, 1.65,
                    0.92, 1.12, 0.022, 0.015,
                    0.035, 0.004, -14.0, -0.40, -0.25
                };
                // Integrated FBW + 2D TVC Mixer
                cfg.flcs = FLCSConfig{
                    FLCSConfig::LawType::FBW_TVC_ALLOCATED,
                    9.0, -3.0, 65.0, 280.0
                };
                // Wheelbase: 5.40m, Track: 3.40m
                cfg.gear = LandingGearConfig{
                    math::Vector3(4.60, 0.0, 0.45), 2.10e5, 2.50e4,
                    math::Vector3(-0.80, -1.70, 0.45),
                    math::Vector3(-0.80, 1.70, 0.45),
                    7.50e5, 8.50e4, 1.60, 0.52, 30.0
                };
                break;
            }

            case AircraftType::A10_THUNDERBOLT: {
                cfg.display_name = "A-10C Thunderbolt II";
                // 11,321 kg empty (24,959 lb), 4,990 kg fuel (11,000 lb), 22,680 kg MTOW
                cfg.mass = MassConfig{
                    11321.0, 4990.0, 22680.0,
                    47047.0, 69689.0, 106432.0, -2440.0
                };
                // 2x GE TF34-GE-100A: 80.6 kN dry, NO AFTERBURNER!
                // High nacelle mount pitch coupling: thrust line Z offset = -1.2 m
                cfg.propulsion = PropulsionConfig{
                    2, 4000.0, 80600.0, 80600.0,
                    false, false, 0.0,
                    4.5, 2.2, 0.0,
                    1.05e-5, 1.05e-5, -1.2,
                    false, 0.0, 0.0
                };
                // High-aspect-ratio straight wing: AR = 6.54, high lift, high drag
                // S_ref = 47.01 m^2 (506 sq ft), b = 17.53 m, c_bar = 3.03 m
                // CD0 is the installed figure with 11 pylons: at 0.032 the jet ran
                // ~450 kt level against a published 381 kt.  Thick-wing drag rise
                // is steep past the M0.65 critical Mach.
                cfg.aero = AeroConfig{
                    47.01, 17.526, 3.030, 6.54, 0.0,
                    0.043, 0.055, 5.1, 1.80,
                    0.65, 0.85, 0.055, 0.045,
                    0.060, 0.000, -11.0, -0.55, -0.28
                };
                // Mechanical linkage with dual Pitch/Yaw Stability Augmentation System (SAS)
                cfg.flcs = FLCSConfig{
                    FLCSConfig::LawType::HYDRO_SAS_AUGMENTED,
                    7.33, -3.0, 18.0, 130.0
                };
                // Wide-stance gear. Nose gear offset 0.3m right for GAU-8 cannon!
                // Wheelbase: 4.60m, Track: 5.24m
                cfg.gear = LandingGearConfig{
                    math::Vector3(4.20, 0.30, 0.35), 1.30e5, 1.60e4,
                    math::Vector3(-0.40, -2.62, 0.35),
                    math::Vector3(-0.40, 2.62, 0.35),
                    4.80e5, 5.50e4, 1.50, 0.48, 35.0
                };
                break;
            }
        }

        return cfg;
    }
};

} // namespace fastjet::aircraft
