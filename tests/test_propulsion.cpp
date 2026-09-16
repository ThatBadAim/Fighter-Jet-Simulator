#include "fastjet/propulsion/f110_engine.hpp"
#include "fastjet/fdm/fuel_system.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace fastjet;

namespace {

using environment::Atmosphere1976;
using propulsion::F110Engine;

void test_sea_level_static_ratings() {
    std::cout << "[Test] Engine: Sea-Level Static Thrust Ratings... ";

    F110Engine eng;
    const auto sl = Atmosphere1976::compute(0.0, 0.0);

    // Settle the spool by running to steady state at full military power.
    for (int i = 0; i < 4000; ++i) eng.update(0.85, sl, 0.005);
    const double mil = eng.net_thrust_n;

    // At sea level static, lapse factor is 1.0, so thrust matches the rating.
    assert(std::abs(mil - input::ThrottleController::THRUST_MIL_N) < 500.0);

    // Full afterburner.
    for (int i = 0; i < 4000; ++i) eng.update(1.0, sl, 0.005);
    const double ab = eng.net_thrust_n;
    assert(std::abs(ab - input::ThrottleController::THRUST_MAX_AB) < 500.0);
    assert(ab > mil * 1.5 && "afterburner must substantially exceed mil power");

    // Cutoff produces no thrust.
    for (int i = 0; i < 4000; ++i) eng.update(0.0, sl, 0.005);
    assert(eng.net_thrust_n < 1.0);

    std::cout << "PASSED (MIL " << mil / 1000.0 << " kN, AB " << ab / 1000.0 << " kN)\n";
}

void test_altitude_lapse() {
    std::cout << "[Test] Engine: Thrust Lapse with Altitude... ";

    const auto sl   = Atmosphere1976::compute(0.0, 0.0);
    const auto alt6 = Atmosphere1976::compute(6000.0, 0.0);
    const auto alt12 = Atmosphere1976::compute(12000.0, 0.0);

    F110Engine e0, e6, e12;
    for (int i = 0; i < 4000; ++i) {
        e0.update(0.85, sl, 0.005);
        e6.update(0.85, alt6, 0.005);
        e12.update(0.85, alt12, 0.005);
    }

    // Thrust must fall monotonically with altitude.
    assert(e6.net_thrust_n < e0.net_thrust_n && "thrust must lapse by 6 km");
    assert(e12.net_thrust_n < e6.net_thrust_n && "thrust must lapse further by 12 km");

    // At ~12 km the density ratio is ~0.31, so static thrust should be far below
    // sea level but still non-trivial.
    const double ratio12 = e12.net_thrust_n / e0.net_thrust_n;
    assert(ratio12 > 0.15 && ratio12 < 0.60);

    std::cout << "PASSED (SL " << e0.net_thrust_n / 1000.0
              << " kN -> 6 km " << e6.net_thrust_n / 1000.0
              << " kN -> 12 km " << e12.net_thrust_n / 1000.0 << " kN)\n";
}

void test_ram_recovery() {
    std::cout << "[Test] Engine: Ram Recovery with Mach Number... ";

    const auto slow = Atmosphere1976::compute(9000.0, 150.0);
    const auto fast = Atmosphere1976::compute(9000.0, 500.0);

    F110Engine e_slow, e_fast;
    for (int i = 0; i < 4000; ++i) {
        e_slow.update(1.0, slow, 0.005);
        e_fast.update(1.0, fast, 0.005);
    }

    // At identical altitude, higher Mach must recover more thrust.
    assert(e_fast.net_thrust_n > e_slow.net_thrust_n &&
           "ram effect must raise thrust with Mach");

    std::cout << "PASSED (M" << slow.mach_number << ": " << e_slow.net_thrust_n / 1000.0
              << " kN -> M" << fast.mach_number << ": " << e_fast.net_thrust_n / 1000.0 << " kN)\n";
}

void test_spool_dynamics() {
    std::cout << "[Test] Engine: Spool-Up and Spool-Down Lag... ";

    const auto sl = Atmosphere1976::compute(0.0, 0.0);

    F110Engine eng;
    // Stabilise at idle.
    for (int i = 0; i < 4000; ++i) eng.update(0.08, sl, 0.005);
    const double idle = eng.net_thrust_n;

    // Slam to military power: thrust must NOT jump instantly.
    const double after_one_step = eng.update(0.85, sl, 0.005);
    assert(after_one_step < input::ThrottleController::THRUST_MIL_N * 0.2 &&
           "thrust must not step instantaneously to commanded value");
    assert(after_one_step > idle && "thrust must begin rising");

    // After ~1 time constant it should have covered roughly 60% of the gap.
    F110Engine e2;
    for (int i = 0; i < 4000; ++i) e2.update(0.08, sl, 0.005);
    const double start = e2.net_thrust_n;
    const int steps_tau = static_cast<int>(F110Engine::TAU_SPOOL_UP / 0.005);
    for (int i = 0; i < steps_tau; ++i) e2.update(0.85, sl, 0.005);
    const double target = e2.commanded_thrust_n;
    const double frac = (e2.net_thrust_n - start) / (target - start);
    assert(frac > 0.5 && frac < 0.75 && "one tau must cover ~63% of the step");

    // Spool-down must be faster than spool-up.
    assert(F110Engine::TAU_SPOOL_DOWN < F110Engine::TAU_SPOOL_UP);

    std::cout << "PASSED (1 tau = " << frac * 100.0 << "% of commanded step)\n";
}

void test_fuel_burn_and_flameout() {
    std::cout << "[Test] Engine: Fuel Burn, Endurance & Flame-Out... ";

    const auto sl = Atmosphere1976::compute(0.0, 0.0);

    F110Engine eng;
    const double start_fuel = eng.fuel_kg;
    assert(start_fuel > 3000.0 && "F-16 carries ~3,175 kg internal fuel");

    // Burn at military power for 60 s.
    for (int i = 0; i < 12000; ++i) eng.update(0.85, sl, 0.005);
    const double burned = start_fuel - eng.fuel_kg;

    assert(burned > 0.0 && "fuel must be consumed");
    assert(eng.fuel_flow_kgs > 0.0);

    // Afterburner must burn markedly faster than dry thrust.
    F110Engine dry, wet;
    for (int i = 0; i < 2000; ++i) { dry.update(0.85, sl, 0.005); wet.update(1.0, sl, 0.005); }
    assert(wet.fuel_flow_kgs > dry.fuel_flow_kgs * 1.5 &&
           "afterburner fuel flow must far exceed dry");

    // Run to dry tanks: the engine must flame out and produce no thrust.
    F110Engine burner;
    burner.reset(5.0); // Nearly empty
    for (int i = 0; i < 40000; ++i) burner.update(1.0, sl, 0.005);
    assert(burner.is_flamed_out() && "engine must flame out when fuel is exhausted");
    assert(burner.net_thrust_n < 1.0 && "no thrust without fuel");
    assert(burner.fuel_kg == 0.0);

    std::cout << "PASSED (MIL burn " << burned << " kg/min, AB flow "
              << wet.fuel_flow_kgs << " vs dry " << dry.fuel_flow_kgs << " kg/s)\n";
}

void test_variable_mass() {
    std::cout << "[Test] Fuel System: Variable Mass & Inertia... ";

    const auto full = fdm::FuelSystem::compute(F110Engine::INTERNAL_FUEL_CAPACITY_KG);
    const auto empty = fdm::FuelSystem::compute(0.0);

    // Full must be heavier than empty by exactly the fuel load.
    const double delta = full.mass_kg - empty.mass_kg;
    assert(std::abs(delta - F110Engine::INTERNAL_FUEL_CAPACITY_KG) < 1e-6);

    // Empty mass must match the published empty weight.
    assert(std::abs(empty.mass_kg - fdm::FuelSystem::EMPTY_MASS_KG) < 1e-6);

    // Inertia must scale up with mass, and the inverse must stay consistent.
    assert(full.Ixx > empty.Ixx && full.Iyy > empty.Iyy && full.Izz > empty.Izz);
    assert(full.inv_mass < empty.inv_mass);

    // I * I_inv must be the identity (validates the analytical inverse).
    const math::Matrix3x3 prod = full.I * full.I_inv;
    assert(std::abs(prod.m[0][0] - 1.0) < 1e-9);
    assert(std::abs(prod.m[1][1] - 1.0) < 1e-9);
    assert(std::abs(prod.m[2][2] - 1.0) < 1e-9);
    assert(std::abs(prod.m[0][2]) < 1e-9);

    // A heavier jet must pitch more slowly under identical moment.
    const double q_accel_full = 1.0 / full.Iyy;
    const double q_accel_empty = 1.0 / empty.Iyy;
    assert(q_accel_full < q_accel_empty && "fuel load must reduce pitch agility");

    std::cout << "PASSED (Empty " << empty.mass_kg << " kg -> Full "
              << full.mass_kg << " kg, Iyy +"
              << 100.0 * (full.Iyy / empty.Iyy - 1.0) << "%)\n";
}

} // namespace

int main() {
    std::cout << "=== F110 Engine, Fuel & Variable Mass Verification ===\n";

    test_sea_level_static_ratings();
    test_altitude_lapse();
    test_ram_recovery();
    test_spool_dynamics();
    test_fuel_burn_and_flameout();
    test_variable_mass();

    std::cout << "All Propulsion tests passed successfully!\n";
    return 0;
}
