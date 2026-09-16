#pragma once

#include "../input/avionics_controls.hpp"
#include "../environment/atmosphere1976.hpp"
#include <algorithm>
#include <cmath>

namespace fastjet {
namespace propulsion {

/**
 * @brief F110-GE-129 augmented turbofan engine model.
 *
 * Wraps the throttle-detent lever logic in input::ThrottleController (which maps
 * lever position to a sea-level static thrust rating) and adds the two physical
 * effects that rating alone cannot represent:
 *
 *  1. Installed thrust lapse with density ratio and Mach number.
 *  2. First-order spool dynamics (thrust cannot change instantaneously).
 *
 * Also integrates fuel burn via thrust-specific fuel consumption (TSFC), so the
 * airframe mass can be updated as fuel is consumed.
 *
 * Zero dynamic heap allocation; safe to call at 200 Hz in the hot loop.
 */
class F110Engine {
public:
    // --- Thrust lapse ---------------------------------------------------
    // Installed thrust scales approximately with density ratio sigma raised to
    // an exponent slightly below unity for a low-bypass augmented turbofan.
    static constexpr double LAPSE_EXPONENT_DRY = 0.85;
    static constexpr double LAPSE_EXPONENT_AB  = 1.00; // Reheat lapses ~linearly with density

    // Ram recovery: net thrust recovers with Mach as inlet momentum rises.
    // F_ram(M) = 1 + K_RAM_* * M^2, bounded to keep the model physical.
    static constexpr double K_RAM_DRY = 0.16;
    static constexpr double K_RAM_AB  = 0.36;
    static constexpr double MACH_RAM_LIMIT = 2.05;

    // --- Spool dynamics -------------------------------------------------
    // Time constants differ by direction: spool-up is slower than spool-down.
    static constexpr double TAU_SPOOL_UP   = 3.2;  // [s] idle -> mil
    static constexpr double TAU_SPOOL_DOWN = 1.6;  // [s] mil -> idle
    static constexpr double TAU_AB_LIGHT   = 0.6;  // [s] afterburner light-off is rapid

    // --- Fuel ------------------------------------------------------------
    // Thrust-specific fuel consumption [kg / (N * s)].
    static constexpr double TSFC_DRY = 2.12e-5; // ~0.75 lb/(lbf*hr)
    static constexpr double TSFC_AB  = 5.66e-5; // ~2.00 lb/(lbf*hr)

    static constexpr double JP8_DENSITY = 800.0; // [kg/m^3]

    // Internal fuel capacity of an F-16C, ~7,000 lb.
    static constexpr double INTERNAL_FUEL_CAPACITY_KG = 3175.0;

    /// Lever/detent logic (Cutoff, Idle, Mil, Afterburner) and static rating.
    input::ThrottleController lever{};

    double commanded_thrust_n{0.0}; // Steady-state installed thrust demand [N]
    double net_thrust_n{0.0};       // Actual thrust after spool lag [N]
    double fuel_kg{INTERNAL_FUEL_CAPACITY_KG};
    double fuel_flow_kgs{0.0};      // Instantaneous fuel flow [kg/s]

    constexpr F110Engine() noexcept = default;

    /**
     * @brief Installed thrust lapse multiplier for the given air data.
     *
     * @param air     Ambient air data (density and speed of sound).
     * @param in_ab   True when operating in the afterburner range.
     * @return Multiplier applied to the sea-level static rating.
     */
    [[nodiscard]] static double lapse_factor(
        const environment::AirData& air,
        bool in_ab
    ) noexcept {
        const double sigma = std::clamp(
            air.density / environment::Atmosphere1976::RHO0, 0.0, 1.5);

        const double exponent = in_ab ? LAPSE_EXPONENT_AB : LAPSE_EXPONENT_DRY;
        const double density_term = std::pow(sigma, exponent);

        const double mach = std::clamp(air.mach_number, 0.0, MACH_RAM_LIMIT);
        const double k_ram = in_ab ? K_RAM_AB : K_RAM_DRY;
        const double ram_term = 1.0 + k_ram * mach * mach;

        return density_term * ram_term;
    }

    /**
     * @brief Advances the engine one timestep.
     *
     * @param throttle_position Lever position in [0.0, 1.0].
     * @param air               Ambient air data at the current state.
     * @param dt                Timestep [s].
     * @param ab_gate_open      Physical afterburner detent gate.
     * @return Net installed thrust [N] after lapse and spool lag.
     */
    double update(
        double throttle_position,
        const environment::AirData& air,
        double dt,
        bool ab_gate_open = true
    ) noexcept {
        // Flame-out when dry: the lever commands nothing without fuel.
        const bool has_fuel = (fuel_kg > 0.0);
        const double lever_pos = has_fuel ? throttle_position : 0.0;

        // 1. Sea-level static rating from the detent schedule.
        const double static_thrust = lever.update(lever_pos, ab_gate_open);
        const bool in_ab =
            (lever.state == input::ThrottleController::DetentState::AFTERBURNER);

        // 2. Apply installed lapse with altitude and Mach.
        commanded_thrust_n = static_thrust * lapse_factor(air, in_ab);

        // 3. First-order spool dynamics toward the commanded thrust.
        if (dt > 0.0) {
            double tau;
            if (in_ab) {
                tau = TAU_AB_LIGHT;
            } else if (commanded_thrust_n >= net_thrust_n) {
                tau = TAU_SPOOL_UP;
            } else {
                tau = TAU_SPOOL_DOWN;
            }

            // Exact discrete solution of the first-order lag: stable for any dt.
            const double alpha = 1.0 - std::exp(-dt / tau);
            net_thrust_n += (commanded_thrust_n - net_thrust_n) * alpha;
        } else {
            net_thrust_n = commanded_thrust_n;
        }

        net_thrust_n = std::max(0.0, net_thrust_n);

        // 4. Fuel burn proportional to actual thrust produced.
        const double tsfc = in_ab ? TSFC_AB : TSFC_DRY;
        fuel_flow_kgs = has_fuel ? (net_thrust_n * tsfc) : 0.0;
        if (dt > 0.0) {
            fuel_kg = std::max(0.0, fuel_kg - fuel_flow_kgs * dt);
        }

        return net_thrust_n;
    }

    /// Fuel remaining as a fraction of internal capacity, in [0, 1].
    [[nodiscard]] double fuel_fraction() const noexcept {
        return (INTERNAL_FUEL_CAPACITY_KG > 0.0)
             ? std::clamp(fuel_kg / INTERNAL_FUEL_CAPACITY_KG, 0.0, 1.0)
             : 0.0;
    }

    /// True once internal fuel is exhausted.
    [[nodiscard]] bool is_flamed_out() const noexcept {
        return fuel_kg <= 0.0;
    }

    void reset(double fuel_load_kg = INTERNAL_FUEL_CAPACITY_KG) noexcept {
        commanded_thrust_n = 0.0;
        net_thrust_n = 0.0;
        fuel_kg = std::clamp(fuel_load_kg, 0.0, INTERNAL_FUEL_CAPACITY_KG);
        fuel_flow_kgs = 0.0;
    }
};

} // namespace propulsion
} // namespace fastjet
