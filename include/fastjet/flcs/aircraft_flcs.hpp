#pragma once

#include "f16_flcs.hpp"
#include "../aircraft/aircraft_type.hpp"
#include "../aircraft/aircraft_config.hpp"

namespace fastjet::flcs {

/**
 * @brief Unified Flight Control System supporting all 5 combat aircraft.
 * Adapts control laws, limiters, actuator dynamics, and control surface scheduling:
 * - F-16: Quad digital FBW with G/AoA limiter (25.2 deg)
 * - F-15EX: Advanced digital FBW with +9g limiter and 30 deg AoA protection
 * - Typhoon: Quad carefree handling FBW with delta-canard scheduling and 35 deg AoA protection
 * - F-22A: 5th-gen FBW with integrated 2D pitch thrust vectoring allocation
 * - A-10C: Hydromechanical flight controls with dual Pitch/Yaw Stability Augmentation System (SAS)
 *
 * Zero dynamic heap allocation.
 */
class AircraftFLCS {
public:
    aircraft::AircraftType aircraft_type{aircraft::AircraftType::F16_FIGHTING_FALCON};
    aircraft::FLCSConfig config{};

    F16FLCS f16_core{};

    // Actuators
    Actuator stabilator{Actuator::create_stabilator(0.0)};
    Actuator aileron{Actuator::create_flaperon(0.0)};
    Actuator rudder{Actuator::create_rudder(0.0)};

    // Thrust vectoring command [-1.0, +1.0] for F-22
    double tvc_pitch_cmd{0.0};

    // Internal SAS damper states for A-10
    double sas_pitch_damper{0.0};
    double sas_yaw_damper{0.0};

    AircraftFLCS() noexcept {
        configure(aircraft::AircraftType::F16_FIGHTING_FALCON);
    }

    explicit AircraftFLCS(aircraft::AircraftType type) noexcept {
        configure(type);
    }

    void configure(aircraft::AircraftType type) noexcept {
        aircraft_type = type;
        config = aircraft::AircraftConfig::get(type).flcs;
        f16_core.reset();
        stabilator.reset();
        aileron.reset();
        rudder.reset();
        tvc_pitch_cmd = 0.0;
        sas_pitch_damper = 0.0;
        sas_yaw_damper = 0.0;
    }

    void reset() noexcept {
        f16_core.reset();
        stabilator.reset();
        aileron.reset();
        rudder.reset();
        tvc_pitch_cmd = 0.0;
        sas_pitch_damper = 0.0;
        sas_yaw_damper = 0.0;
    }

    /**
     * @brief Steps the flight control laws and physical actuators.
     */
    aero::ControlSurfaces update(
        double dt,
        const fdm::FlightState& state,
        const PilotCommands& pilot,
        double dynamic_pressure,
        const fdm::AircraftForces& aero_forces,
        const fdm::MassProperties& mass
    ) noexcept {
        // Delegate F-16 directly to original tested implementation
        if (aircraft_type == aircraft::AircraftType::F16_FIGHTING_FALCON) {
            return f16_core.update(dt, state, pilot, dynamic_pressure, aero_forces, mass);
        }

        const IMUData imu = IMUData::read(state, aero_forces, mass, dynamic_pressure);

        double de_cmd = 0.0;
        double da_cmd = 0.0;
        double dr_cmd = 0.0;
        double dlef_cmd = 0.0;

        switch (config.law_type) {
            case aircraft::FLCSConfig::LawType::DIGITAL_FBW_G_ALPHA: {
                // F-15EX Digital FBW: G command blending with 30 deg AoA limiter
                // Pitch stick demands G at speed, pitch rate at low speed
                constexpr double G_PER_STICK = 9.0;
                double commanded_g = 1.0 + pilot.pitch_stick * G_PER_STICK;
                commanded_g = std::clamp(commanded_g, config.max_g_negative, config.max_g_positive);

                // AoA Limiter back-off
                if (imu.alpha_deg > config.alpha_limit_deg) {
                    const double alpha_excess = imu.alpha_deg - config.alpha_limit_deg;
                    commanded_g -= alpha_excess * 0.40;
                }

                // Proportional-Derivative pitch command
                const double g_err = commanded_g - imu.Nz;
                de_cmd = -g_err * 3.2 - state.omega_b.y * 5.0;
                de_cmd = std::clamp(de_cmd, -25.0, 25.0);

                // Roll & Yaw
                da_cmd = pilot.roll_stick * 21.5;
                dr_cmd = pilot.rudder_pedal * 30.0 - state.omega_b.z * 4.0; // Yaw damper
                break;
            }

            case aircraft::FLCSConfig::LawType::CAREFREE_DELTA_CANARD: {
                // Eurofighter Typhoon Carefree Handling:
                // Active canard/elevon control law with 35 deg AoA protection & +9g limit
                double commanded_g = 1.0 + pilot.pitch_stick * 9.0;
                commanded_g = std::clamp(commanded_g, config.max_g_negative, config.max_g_positive);

                if (imu.alpha_deg > config.alpha_limit_deg) {
                    const double alpha_excess = imu.alpha_deg - config.alpha_limit_deg;
                    commanded_g -= alpha_excess * 0.50;
                }

                const double g_err = commanded_g - imu.Nz;
                de_cmd = -g_err * 3.5 - state.omega_b.y * 6.0;
                de_cmd = std::clamp(de_cmd, -30.0, 30.0);

                // Extreme agility roll rate with roll-yaw cross-coupling suppression
                da_cmd = pilot.roll_stick * 25.0;
                dr_cmd = pilot.rudder_pedal * 30.0 - state.omega_b.z * 5.0 - imu.beta_deg * 0.4;
                break;
            }

            case aircraft::FLCSConfig::LawType::FBW_TVC_ALLOCATED: {
                // F-22A Raptor: Unified aerodynamic surfaces + 2D thrust vectoring
                double commanded_g = 1.0 + pilot.pitch_stick * 9.5;
                commanded_g = std::clamp(commanded_g, config.max_g_negative, config.max_g_positive);

                const double g_err = commanded_g - imu.Nz;
                de_cmd = -g_err * 3.2 - state.omega_b.y * 5.0;
                de_cmd = std::clamp(de_cmd, -25.0, 25.0);

                // TVC Pitch Allocation:
                // When dynamic pressure is low (q < 10 kPa) or AoA is extreme (> 25 deg),
                // thrust vectoring takes over the primary pitch authority
                const double q_factor = std::clamp(1.0 - dynamic_pressure / 15000.0, 0.0, 1.0);
                const double alpha_factor = std::clamp((imu.alpha_deg - 20.0) / 20.0, 0.0, 1.0);
                const double tvc_weight = (std::max)(q_factor, alpha_factor);

                tvc_pitch_cmd = std::clamp(pilot.pitch_stick * (0.35 + 0.65 * tvc_weight), -1.0, 1.0);

                da_cmd = pilot.roll_stick * 22.0;
                dr_cmd = pilot.rudder_pedal * 28.0 - state.omega_b.z * 4.5;
                break;
            }

            case aircraft::FLCSConfig::LawType::HYDRO_SAS_AUGMENTED: {
                // A-10C Warthog: Direct hydromechanical linkage + dual Pitch/Yaw SAS dampers
                // Pilot stick directly drives elevator position with natural aerodynamic feel
                de_cmd = pilot.pitch_stick * 25.0; // Direct elevator travel

                // Pitch SAS damper (washes out steady pitch rate, damps phugoid/short period)
                sas_pitch_damper += (state.omega_b.y - sas_pitch_damper) * (1.0 - std::exp(-dt / 0.20));
                de_cmd += -sas_pitch_damper * 3.0;
                de_cmd = std::clamp(de_cmd, -25.0, 15.0);

                // Direct ailerons (up to +/- 20 deg)
                da_cmd = pilot.roll_stick * 20.0;

                // Direct rudder + Yaw SAS damper
                sas_yaw_damper += (state.omega_b.z - sas_yaw_damper) * (1.0 - std::exp(-dt / 0.15));
                dr_cmd = pilot.rudder_pedal * 25.0 - sas_yaw_damper * 4.0;
                dr_cmd = std::clamp(dr_cmd, -25.0, 25.0);

                tvc_pitch_cmd = 0.0;
                break;
            }
        }

        // Step hydraulic actuators
        const double de_actual = stabilator.step(de_cmd, dt);
        const double da_actual = aileron.step(da_cmd, dt);
        const double dr_actual = rudder.step(dr_cmd, dt);

        return {de_actual, da_actual, dr_actual, dlef_cmd};
    }
};

} // namespace fastjet::flcs
