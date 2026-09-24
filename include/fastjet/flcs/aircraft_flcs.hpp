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

    // Internal integrator and SAS damper states
    double pitch_integrator{0.0};
    double sas_pitch_damper{0.0};
    double sas_yaw_damper{0.0};
    RollRateTrim roll_trim{};

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
        pitch_integrator = 0.0;
        sas_pitch_damper = 0.0;
        sas_yaw_damper = 0.0;
        roll_trim.reset();
    }

    void reset() noexcept {
        f16_core.reset();
        stabilator.reset();
        aileron.reset();
        rudder.reset();
        tvc_pitch_cmd = 0.0;
        pitch_integrator = 0.0;
        sas_pitch_damper = 0.0;
        sas_yaw_damper = 0.0;
        roll_trim.reset();
    }

    /// Prediction horizon for the AoA limiters [s].
    static constexpr double ALPHA_LEAD_S = 0.45;

    /**
     * @brief Alpha predicted @p lead_s ahead from alpha's own rate [deg].
     *
     * Alpha rate is pitch rate less the flight-path turn rate the current Nz
     * accounts for.  Leading on raw pitch rate instead charged every sustained
     * pull as if alpha were climbing, which forced a choice between capping the
     * pull well short of the limit and overshooting it in transients (Typhoon to
     * ~40 deg of a 35 deg limit, the F-22 to ~80 of 65).
     */
    [[nodiscard]] static double predicted_alpha_deg(const IMUData& imu, double lead_s) noexcept {
        const double alpha_rate = imu.q_deg - steady_pull_rate_dps(imu.Nz, imu.airspeed);
        return imu.alpha_deg + lead_s * alpha_rate;
    }

    /**
     * @brief Pitch rate a steady wings-level pull at @p nz_cmd flies at [deg/s].
     *
     * The pitch-rate damper acts on the rate *beyond* this.  Damping the whole
     * rate fought every sustained pull: the damper opposed the very rate the G
     * command needs, the integrator had to wind up to overcome it, and the
     * F-15EX, Typhoon and F-22 settled a full G or more short of command (7-8 G
     * on a full-aft stick rated for 9 G).
     */
    [[nodiscard]] static double steady_pull_rate_dps(double nz_cmd, double airspeed) noexcept {
        constexpr double G0 = 9.80665;
        return (180.0 / M_PI) * G0 * (nz_cmd - 1.0) / std::max(airspeed, 50.0);
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

        const double q_bar = dynamic_pressure;
        // Dynamic pressure blending weight: pitch rate at low speed, Nz at high speed
        const double w_Nz = std::clamp((q_bar - 2000.0) / 4000.0, 0.0, 1.0);
        // Dynamic pressure gain scheduling: stabilizes loop gain from 100 to 800+ knots
        const double q_scale = std::clamp(std::sqrt(15000.0 / std::max(2500.0, q_bar)), 0.45, 1.40);

        switch (config.law_type) {
            case aircraft::FLCSConfig::LawType::DIGITAL_FBW_G_ALPHA: {
                // F-15EX Digital FBW: G command blending with smooth predictive AoA protection
                const double pitch_in = std::clamp(pilot.pitch_stick, -1.0, 1.0);
                double Nz_cmd = (pitch_in >= 0.0) ? (1.0 + pitch_in * (config.max_g_positive - 1.0))
                                                  : (1.0 + pitch_in * (1.0 - config.max_g_negative));
                Nz_cmd = std::clamp(Nz_cmd, config.max_g_negative, config.max_g_positive);

                const double q_cmd = pitch_in * 30.0; // deg/s

                // Predictive AoA Limiter: anticipates stall entry and smoothly arrests pitch rate
                const double alpha_pred = predicted_alpha_deg(imu, ALPHA_LEAD_S);

                double aoa_override_de = 0.0;
                if (alpha_pred > (config.alpha_limit_deg - 5.0)) {
                    const double alpha_headroom = std::max(0.0, config.alpha_limit_deg - alpha_pred);
                    Nz_cmd = std::min(Nz_cmd, 1.0 + (alpha_headroom / 5.0) * (config.max_g_positive - 1.0));
                    if (alpha_pred > (config.alpha_limit_deg - 2.5)) {
                        const double excess = alpha_pred - (config.alpha_limit_deg - 2.5);
                        aoa_override_de = 2.2 * excess + 0.15 * excess * excess;
                    }
                }

                const double err_Nz = Nz_cmd - imu.Nz;
                const double err_q  = (q_cmd - imu.q_deg) * 0.04;
                const double error  = w_Nz * err_Nz + (1.0 - w_Nz) * err_q;

                // Anti-windup & AoA integrator bleed
                if (alpha_pred >= (config.alpha_limit_deg - 2.5) && pitch_integrator > 0.0 && dt > 0.0) {
                    pitch_integrator *= std::max(0.0, 1.0 - 15.0 * dt);
                } else if (dt > 0.0) {
                    pitch_integrator += error * dt;
                    pitch_integrator = std::clamp(pitch_integrator, -25.0, 25.0);
                }

                // G-Limiter anticipatory pitch damping (prevents high-speed violent rocking).
                // It brakes pitch rate *beyond* the steady rate of the commanded pull:
                // braking the whole rate held a full-aft 9 G pull at ~7.7 G.
                const double q_steady = w_Nz * steady_pull_rate_dps(Nz_cmd, imu.airspeed);
                double g_damping = 0.0;
                if (imu.Nz > (config.max_g_positive - 1.5) && imu.q_deg > 0.0) {
                    g_damping = 0.20 * (imu.Nz - (config.max_g_positive - 1.5));
                }

                constexpr double Kp = 2.00;
                constexpr double Ki = 1.00;
                constexpr double Kd = 0.28;
                de_cmd = -q_scale * (Kp * error + Ki * pitch_integrator) + Kd * (imu.q_deg - q_steady)
                       + g_damping * std::max(0.0, imu.q_deg - 0.8 * q_steady) + aoa_override_de;
                de_cmd = std::clamp(de_cmd, -25.0, 25.0);

                // Roll & Yaw: roll rate command + yaw rate damper + turn coordination
                const double p_cmd = pilot.roll_stick * config.max_roll_rate_dps;
                const double da_p = -0.20 * (p_cmd - imu.p_deg);
                da_cmd = std::clamp(da_p + roll_trim.update(dt, p_cmd, imu.p_deg, 0.80, da_p, 25.0), -25.0, 25.0);
                dr_cmd = std::clamp(-pilot.rudder_pedal * 25.0 + 0.40 * imu.r_deg - 0.20 * imu.beta_deg, -30.0, 30.0);
                break;
            }

            case aircraft::FLCSConfig::LawType::CAREFREE_DELTA_CANARD: {
                // Eurofighter Typhoon Carefree Handling
                const double pitch_in = std::clamp(pilot.pitch_stick, -1.0, 1.0);
                double Nz_cmd = (pitch_in >= 0.0) ? (1.0 + pitch_in * (config.max_g_positive - 1.0))
                                                  : (1.0 + pitch_in * (1.0 - config.max_g_negative));
                Nz_cmd = std::clamp(Nz_cmd, config.max_g_negative, config.max_g_positive);

                const double q_cmd = pitch_in * 30.0; // deg/s

                const double alpha_pred = predicted_alpha_deg(imu, ALPHA_LEAD_S);

                double aoa_override_de = 0.0;
                if (alpha_pred > (config.alpha_limit_deg - 5.0)) {
                    const double alpha_headroom = std::max(0.0, config.alpha_limit_deg - alpha_pred);
                    Nz_cmd = std::min(Nz_cmd, 1.0 + (alpha_headroom / 5.0) * (config.max_g_positive - 1.0));
                    if (alpha_pred > (config.alpha_limit_deg - 2.5)) {
                        const double excess = alpha_pred - (config.alpha_limit_deg - 2.5);
                        aoa_override_de = 2.0 * excess + 0.12 * excess * excess;
                    }
                }

                const double err_Nz = Nz_cmd - imu.Nz;
                const double err_q  = (q_cmd - imu.q_deg) * 0.04;
                const double error  = w_Nz * err_Nz + (1.0 - w_Nz) * err_q;

                if (alpha_pred >= (config.alpha_limit_deg - 2.5) && pitch_integrator > 0.0 && dt > 0.0) {
                    pitch_integrator *= std::max(0.0, 1.0 - 15.0 * dt);
                } else if (dt > 0.0) {
                    pitch_integrator += error * dt;
                    pitch_integrator = std::clamp(pitch_integrator, -15.0, 15.0);
                }

                double g_damping = 0.0;
                if (imu.Nz > (config.max_g_positive - 1.5) && imu.q_deg > 0.0) {
                    g_damping = 0.12 * (imu.Nz - (config.max_g_positive - 1.5));
                }

                constexpr double Kp = 1.30;
                constexpr double Ki = 0.60;
                constexpr double Kd = 0.35;
                de_cmd = -q_scale * (Kp * error + Ki * pitch_integrator) + Kd * (imu.q_deg - w_Nz * steady_pull_rate_dps(Nz_cmd, imu.airspeed)) + g_damping * imu.q_deg + aoa_override_de;
                de_cmd = std::clamp(de_cmd, -30.0, 30.0);

                const double p_cmd = pilot.roll_stick * config.max_roll_rate_dps;
                const double da_p = -0.12 * (p_cmd - imu.p_deg);
                da_cmd = std::clamp(da_p + roll_trim.update(dt, p_cmd, imu.p_deg, 0.48, da_p, 25.0), -25.0, 25.0);
                dr_cmd = std::clamp(-pilot.rudder_pedal * 28.0 + 0.45 * imu.r_deg - 0.25 * imu.beta_deg, -30.0, 30.0);
                break;
            }

            case aircraft::FLCSConfig::LawType::FBW_TVC_ALLOCATED: {
                // F-22A Raptor: Unified aerodynamic surfaces + 2D thrust vectoring
                const double pitch_in = std::clamp(pilot.pitch_stick, -1.0, 1.0);
                double Nz_cmd = (pitch_in >= 0.0) ? (1.0 + pitch_in * (config.max_g_positive - 1.0))
                                                  : (1.0 + pitch_in * (1.0 - config.max_g_negative));
                Nz_cmd = std::clamp(Nz_cmd, config.max_g_negative, config.max_g_positive);

                const double q_cmd = pitch_in * 32.0;

                const double alpha_pred = predicted_alpha_deg(imu, ALPHA_LEAD_S);

                double aoa_override_de = 0.0;
                if (alpha_pred > (config.alpha_limit_deg - 5.0)) {
                    const double alpha_headroom = std::max(0.0, config.alpha_limit_deg - alpha_pred);
                    Nz_cmd = std::min(Nz_cmd, 1.0 + (alpha_headroom / 5.0) * (config.max_g_positive - 1.0));
                    if (alpha_pred > (config.alpha_limit_deg - 2.5)) {
                        const double excess = alpha_pred - (config.alpha_limit_deg - 2.5);
                        aoa_override_de = 1.8 * excess + 0.10 * excess * excess;
                    }
                }

                const double err_Nz = Nz_cmd - imu.Nz;
                const double err_q  = (q_cmd - imu.q_deg) * 0.04;
                const double error  = w_Nz * err_Nz + (1.0 - w_Nz) * err_q;

                if (alpha_pred >= (config.alpha_limit_deg - 2.5) && pitch_integrator > 0.0 && dt > 0.0) {
                    pitch_integrator *= std::max(0.0, 1.0 - 15.0 * dt);
                } else if (dt > 0.0) {
                    pitch_integrator += error * dt;
                    pitch_integrator = std::clamp(pitch_integrator, -15.0, 15.0);
                }

                double g_damping = 0.0;
                if (imu.Nz > (config.max_g_positive - 1.5) && imu.q_deg > 0.0) {
                    g_damping = 0.12 * (imu.Nz - (config.max_g_positive - 1.5));
                }

                constexpr double Kp = 1.35;
                constexpr double Ki = 0.60;
                constexpr double Kd = 0.32;
                de_cmd = -q_scale * (Kp * error + Ki * pitch_integrator) + Kd * (imu.q_deg - w_Nz * steady_pull_rate_dps(Nz_cmd, imu.airspeed)) + g_damping * imu.q_deg + aoa_override_de;
                de_cmd = std::clamp(de_cmd, -25.0, 25.0);

                // TVC Pitch Allocation: Pitch-up elevator (de < 0) corresponds to nozzle pitch-up.
                // The nose-up stick feed-forward fades out as predicted alpha closes on
                // the limit, leaving the nozzles to the control law's own command (which
                // carries the AoA limiter).  Unfaded, the raw stick kept the nozzles
                // pitching up against the limiter's push: full aft stick below ~180 m/s
                // drove alpha through the 65 deg limit and tumbled the jet end over end.
                const double q_factor = std::clamp(1.0 - dynamic_pressure / 15000.0, 0.0, 1.0);
                const double alpha_factor = std::clamp((imu.alpha_deg - 20.0) / 20.0, 0.0, 1.0);
                const double tvc_weight = (std::max)(q_factor, alpha_factor);
                const double stick_ff = (pilot.pitch_stick > 0.0)
                    ? pilot.pitch_stick * std::clamp((config.alpha_limit_deg - alpha_pred) / 10.0, 0.0, 1.0)
                    : pilot.pitch_stick;

                tvc_pitch_cmd = std::clamp(stick_ff * (0.35 + 0.65 * tvc_weight) - (de_cmd / 25.0) * 0.5 * tvc_weight, -1.0, 1.0);

                const double p_cmd = pilot.roll_stick * config.max_roll_rate_dps;
                const double da_p = -0.10 * (p_cmd - imu.p_deg);
                da_cmd = std::clamp(da_p + roll_trim.update(dt, p_cmd, imu.p_deg, 0.40, da_p, 22.0), -22.0, 22.0);
                dr_cmd = std::clamp(-pilot.rudder_pedal * 26.0 + 0.40 * imu.r_deg - 0.20 * imu.beta_deg, -28.0, 28.0);
                break;
            }

            case aircraft::FLCSConfig::LawType::HYDRO_SAS_AUGMENTED: {
                // A-10C Warthog: Direct hydromechanical linkage + dual Pitch/Yaw SAS dampers
                // Pulling stick back (pitch_stick > 0) commands elevator UP (de_cmd < 0).
                // Gearing puts full aft stick just past stall alpha, as on any
                // conventional jet: 22 deg per unit stick would have trimmed to ~77 deg
                // and pulled 9-17 G.  Above Q_FEEL the feel system's stick force per G
                // leaves a pilot short of full elevator, so a full pull near Vne bends
                // the jet a little past 7.33 G rather than breaking it.
                constexpr double ELEVATOR_PER_STICK_DEG = 10.0;
                constexpr double Q_FEEL_PA = 20000.0;
                const double feel_authority = std::clamp(Q_FEEL_PA / std::max(q_bar, 1.0), 0.4, 1.0);
                de_cmd = -pilot.pitch_stick * ELEVATOR_PER_STICK_DEG * feel_authority;

                // Pitch SAS damper (damps pitch rate)
                sas_pitch_damper += (imu.q_deg - sas_pitch_damper) * (1.0 - std::exp(-dt / 0.20));
                de_cmd += sas_pitch_damper * 0.25;
                de_cmd = std::clamp(de_cmd, -25.0, 15.0);

                // Direct ailerons: right stick (roll_stick > 0) commands right roll (da_cmd < 0)
                da_cmd = -pilot.roll_stick * 20.0;

                // Direct rudder: right pedal (rudder_pedal > 0) commands right yaw (dr_cmd < 0)
                sas_yaw_damper += (imu.r_deg - sas_yaw_damper) * (1.0 - std::exp(-dt / 0.15));
                dr_cmd = -pilot.rudder_pedal * 25.0 + sas_yaw_damper * 0.35;
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
