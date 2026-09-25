#pragma once

#include "fastjet/aircraft/aircraft_type.hpp"
#include "fastjet/aircraft/aircraft_config.hpp"
#include "fastjet/aero/aircraft_aero_model.hpp"
#include "fastjet/environment/atmosphere1976.hpp"
#include "fastjet/environment/ground_collision.hpp"
#include "fastjet/environment/wind_turbulence.hpp"
#include "fastjet/fdm/fuel_system.hpp"
#include "fastjet/fdm/rk4_integrator.hpp"
#include "fastjet/flcs/aircraft_flcs.hpp"
#include "fastjet/flcs/imu_sensor.hpp"
#include "fastjet/flcs/onboard_flight_computer.hpp"
#include "fastjet/flcs/pilot_commands.hpp"
#include "fastjet/gear/landing_gear.hpp"
#include "fastjet/input/avionics_controls.hpp"
#include "fastjet/propulsion/multi_engine.hpp"
#include "fastjet/sim/damage_model.hpp"
#include "fastjet/sim/gun.hpp"
#include <algorithm>
#include <cstdint>

namespace fastjet::sim {

/// @brief Everything a pilot (human or AI) puts into the cockpit for one step.
///
/// A human pilot and the AI produce exactly this struct, and both go through
/// the same OFC, FLCS, engine and integrator. No pilot has a path around the
/// airframe's limits.
struct AircraftControls {
    flcs::PilotCommands stick{};   ///< Raw inceptors, before the OFC
    double throttle{0.0};          ///< Lever position [0, 1]
    bool speedbrake_out{false};    ///< Speedbrake switch
    double brake_left{0.0};        ///< Toe brakes [0, 1]
    double brake_right{0.0};
    bool trigger{false};           ///< Gun trigger held
    bool dispense{false};          ///< Countermeasure dispense switch (one program per press)
};

/**
 * @brief One jet: the complete per-aircraft pipeline that used to live in the
 * viewer's main loop.
 *
 * Each step runs IMU -> OFC -> FLCS -> speedbrake -> engine -> fuel/mass ->
 * gear -> turbulence -> RK4 -> ground collision, in that order and with the
 * same arithmetic as the original loop, so a single ownship flies
 * bit-identically through it (see tests/test_aircraft_entity.cpp).
 *
 * Every jet carries its own turbulence state: gusts are felt where the
 * aircraft is, not shared across the sky.
 *
 * Zero heap allocation; safe in the 200 Hz loop.
 */
class Aircraft {
public:
    static constexpr double DT = fdm::RK4Integrator::DEFAULT_DT;

    aircraft::AircraftType type{aircraft::AircraftType::F16_FIGHTING_FALCON};

    fdm::FlightState state{};
    fdm::FlightState prev_state{};        ///< State before the latest step (render interpolation, hit sweeps)
    fdm::MassProperties mass{};
    fdm::AircraftForces forces{};         ///< Forces at the current state (IMU / HUD)
    flcs::IMUData imu{};

    propulsion::MultiEngine engine;
    gear::LandingGear landing_gear;
    aero::AircraftAeroModel aero_model;
    flcs::AircraftFLCS flcs;
    flcs::OnBoardFlightComputer ofc;
    input::SpeedbrakeController speedbrake{};
    environment::WindTurbulenceModel wind{};
    GunSystem gun{};
    DamageState damage{};

    bool crashed{false};
    double time{0.0};

    Aircraft() noexcept : Aircraft(aircraft::AircraftType::F16_FIGHTING_FALCON) {}

    explicit Aircraft(aircraft::AircraftType t) noexcept
        : type(t), engine(t), landing_gear(t), aero_model(t), flcs(t), ofc(t) {
        mass = fdm::FuelSystem::compute(engine.fuel_kg, t);
        wind.base_wind_speed_mps = 5.0;
        gun.configure(t);
        damage = DamageState{t};
        imu.Nz = 1.0;
    }

    /// @brief Adopt a different airframe. Fuel is carried across as a fraction
    /// (an in-flight change must not refuel the jet).
    void configure(aircraft::AircraftType t) noexcept {
        type = t;
        engine.configure(t, /*preserve_fuel=*/true);
        aero_model.configure(t);
        flcs.configure(t);
        ofc.configure(t);
        landing_gear.configure(t);
        mass = fdm::FuelSystem::compute(engine.fuel_kg, t);
        gun.configure(t);
        damage = DamageState{t};
    }

    /// @brief Fresh jet at @p start: full fuel, systems reset, undamaged.
    void reset(const fdm::FlightState& start) noexcept {
        state = start;
        prev_state = start;
        crashed = false;
        engine.reset();
        landing_gear.reset(type);
        ofc.reset();
        flcs.reset();
        speedbrake.position = 0.0;
        mass = fdm::FuelSystem::compute(engine.fuel_kg, type);
        forces = fdm::AircraftForces{};
        imu = flcs::IMUData{};
        imu.Nz = 1.0;
        gun.configure(type);
        damage = DamageState{type};
    }

    /// @brief Re-seed this jet's turbulence so jets in one sky do not share gusts.
    void seed_turbulence(uint32_t seed) noexcept { wind.seed(seed); }

    /// @brief Still in the fight: flying, intact and with a conscious pilot at the controls.
    [[nodiscard]] bool alive() const noexcept {
        return !crashed && !damage.destroyed && !damage.pilot_incapacitated;
    }

    /// @brief One fixed 200 Hz physics step.
    void step(double dt, const AircraftControls& in) noexcept {
        prev_state = state;
        if (crashed) return;

        // A destroyed jet has no pilot on the controls and no working engine;
        // it falls out of the sky on its own aerodynamics.
        damage.update(dt);
        AircraftControls cmd = in;
        if (damage.destroyed || damage.pilot_incapacitated) {
            cmd.stick = {};
            cmd.trigger = false;
        }

        const auto air = environment::Atmosphere1976::compute(state.altitude(), state.airspeed());
        imu = flcs::IMUData::read(state, forces, mass, air.dynamic_pressure);

        // OFC intercept: overrides the pilot while GLOC or Auto-GCAS is active.
        const flcs::PilotCommands effective = ofc.update(dt, state, imu, cmd.stick, landing_gear.deployed);

        auto surfaces = flcs.update(dt, state, effective, air.dynamic_pressure, forces, mass);
        damage.apply_to_surfaces(surfaces);

        speedbrake.update(cmd.speedbrake_out ? input::SpeedbrakeController::SwitchPosition::EXTEND
                                             : input::SpeedbrakeController::SwitchPosition::RETRACT,
                          dt);
        surfaces.speedbrake = speedbrake.position;

        // Auto-GCAS throttle authority overrides the lever.
        double lever = cmd.throttle;
        if (ofc.has_throttle_override()) lever = ofc.throttle_override_value();
        engine.tvc_pitch_cmd = flcs.tvc_pitch_cmd;
        const double thrust_n = engine.update(lever, air, dt) * damage.thrust_factor();
        if (damage.fuel_leak_kgs > 0.0) {
            engine.fuel_kg = std::max(0.0, engine.fuel_kg - damage.fuel_leak_kgs * dt);
        }
        mass = fdm::FuelSystem::compute(engine.fuel_kg, type);

        landing_gear.steer_cmd = cmd.stick.rudder_pedal;
        landing_gear.brake_left = cmd.brake_left;
        landing_gear.brake_right = cmd.brake_right;

        const double agl = environment::GroundCollision::get_agl(state);
        wind.update(dt, agl, state.airspeed());

        // RK4 step with forces re-evaluated at each stage, in the air-relative frame.
        const double pitch_moment = engine.pitch_moment();
        integrator_.step(state, time, mass,
            [&](double, const fdm::FlightState& s) noexcept -> fdm::AircraftForces {
                const double s_agl = environment::GroundCollision::get_agl(s);
                const math::Vector3 v_rel = wind.compute_relative_velocity(s.vel_b, s_agl, s.q_att);
                const double v_rel_norm = (std::max)(1.0, v_rel.norm());
                const auto stage_air = environment::Atmosphere1976::compute(s.altitude(), v_rel_norm);

                fdm::FlightState s_aero = s;
                s_aero.vel_b = v_rel;
                fdm::AircraftForces f = aero_model.compute_forces_and_moments(
                    s_aero, surfaces, stage_air.dynamic_pressure, stage_air.mach_number);
                f.force_b.x += thrust_n;
                f.moment_b.y += pitch_moment;

                const auto gear_f = landing_gear.compute(s, mass);
                f.force_b = f.force_b + gear_f.force_b;
                f.moment_b = f.moment_b + gear_f.moment_b;
                return f;
            });

        // Forces at the new state, for the next IMU read and the HUD.
        const double state_agl = environment::GroundCollision::get_agl(state);
        fdm::FlightState state_aero = state;
        state_aero.vel_b = wind.compute_relative_velocity(state.vel_b, state_agl, state.q_att);
        forces = aero_model.compute_forces_and_moments(state_aero, surfaces, air.dynamic_pressure, air.mach_number);
        forces.force_b.x += thrust_n;
        forces.moment_b.y += engine.pitch_moment();

        const auto col = environment::GroundCollision::check_collision(
            state, type, landing_gear.deployed, landing_gear.collapsed);
        if (col.has_collided) {
            crashed = true;
            environment::GroundCollision::clamp_to_surface(state);
            imu.Nz = 0.0;
        }
    }

    // ---------------------------------------------------------------------
    // Geometry helpers used by weapons, sensors and the AI
    // ---------------------------------------------------------------------

    [[nodiscard]] math::Vector3 position() const noexcept { return state.pos_ned; }
    [[nodiscard]] math::Vector3 velocity() const noexcept { return state.velocity_ned(); }

    /// @brief Inertial acceleration [m/s^2, NED] from the latest forces (gravity included).
    [[nodiscard]] math::Vector3 acceleration() const noexcept {
        const math::Vector3 specific_b = forces.force_b / std::max(1.0, mass.mass_kg);
        return state.q_att.rotate_body_to_ned(specific_b) + math::Vector3(0.0, 0.0, fdm::SixDoFFDM::GRAVITY_ACCEL);
    }

private:
    fdm::RK4Integrator integrator_{DT};
};

} // namespace fastjet::sim
