#pragma once

#include "flight_state.hpp"
#include "mass_properties.hpp"
#include "aircraft_forces.hpp"
#include "six_dof_fdm.hpp"
#include <concepts>

namespace fastjet {
namespace fdm {

/**
 * @brief Concept for a callable force generator:
 * Accepts (double time, const FlightState& state) -> AircraftForces
 */
template <typename F>
concept ForceProvider = requires(F f, double t, const FlightState& s) {
    { f(t, s) } -> std::convertible_to<AircraftForces>;
};

/**
 * @brief Deterministic 4th-Order Runge-Kutta (RK4) numerical integrator for 6-DoF FDM.
 * Guarantees zero heap allocation and deterministic fixed-step simulation.
 */
class RK4Integrator {
public:
    static constexpr double DEFAULT_DT = 0.005; // 200 Hz

    double dt{DEFAULT_DT};

    constexpr RK4Integrator() noexcept = default;
    constexpr explicit RK4Integrator(double fixed_dt) noexcept : dt(fixed_dt) {}

    /**
     * @brief Performs one deterministic RK4 integration step.
     * All intermediate stage buffers are allocated on the stack with zero dynamic memory.
     *
     * @tparam ForceFn Callable conforming to ForceProvider concept.
     * @param state Current flight state (mutated in-place to state at t + dt).
     * @param time Current simulation time in seconds (mutated in-place to t + dt).
     * @param mass Mass and inertia properties of the aircraft.
     * @param force_fn Evaluator for external aero/thrust forces and moments.
     */
    template <ForceProvider ForceFn>
    void step(
        FlightState& state,
        double& time,
        const MassProperties& mass,
        ForceFn&& force_fn
    ) const noexcept {
        const double half_dt = 0.5 * dt;
        const double sixth_dt = dt / 6.0;

        // Stage 1: k1 = f(t, x)
        const AircraftForces f1 = force_fn(time, state);
        const FlightStateDeriv k1 = SixDoFFDM::compute_derivatives(state, f1, mass);

        // Stage 2: k2 = f(t + dt/2, x + k1 * dt/2)
        FlightState x2 = state + (k1 * half_dt);
        x2.normalize_quaternion();
        const AircraftForces f2 = force_fn(time + half_dt, x2);
        const FlightStateDeriv k2 = SixDoFFDM::compute_derivatives(x2, f2, mass);

        // Stage 3: k3 = f(t + dt/2, x + k2 * dt/2)
        FlightState x3 = state + (k2 * half_dt);
        x3.normalize_quaternion();
        const AircraftForces f3 = force_fn(time + half_dt, x3);
        const FlightStateDeriv k3 = SixDoFFDM::compute_derivatives(x3, f3, mass);

        // Stage 4: k4 = f(t + dt, x + k3 * dt)
        FlightState x4 = state + (k3 * dt);
        x4.normalize_quaternion();
        const AircraftForces f4 = force_fn(time + dt, x4);
        const FlightStateDeriv k4 = SixDoFFDM::compute_derivatives(x4, f4, mass);

        // State update: x_{n+1} = x_n + (dt / 6) * (k1 + 2*k2 + 2*k3 + k4)
        const FlightStateDeriv combined_deriv = k1 + (k2 * 2.0) + (k3 * 2.0) + k4;
        state += (combined_deriv * sixth_dt);

        // Re-normalize attitude quaternion to maintain exact unit norm
        state.normalize_quaternion();

        // Increment time
        time += dt;
    }

    /**
     * @brief Specialized overload for constant or zero external force (e.g. ballistic flight).
     */
    void step(
        FlightState& state,
        double& time,
        const MassProperties& mass,
        const AircraftForces& constant_forces = AircraftForces::zero()
    ) const noexcept {
        step(state, time, mass, [&constant_forces](double, const FlightState&) noexcept -> AircraftForces {
            return constant_forces;
        });
    }
};

} // namespace fdm
} // namespace fastjet
