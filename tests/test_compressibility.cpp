#include "fastjet/aero/f16_aero_model.hpp"
#include "fastjet/environment/atmosphere1976.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace fastjet;
using aero::F16AeroModel;
using aero::ControlSurfaces;

namespace {

fdm::FlightState make_state(double alpha_deg, double v_mps) {
    fdm::FlightState s{};
    const double a = alpha_deg * M_PI / 180.0;
    s.vel_b = math::Vector3(v_mps * std::cos(a), 0.0, v_mps * std::sin(a));
    s.q_att = math::Quaternion::identity();
    s.pos_ned = math::Vector3(0.0, 0.0, -9000.0);
    return s;
}

void test_incompressible_backward_compatible() {
    std::cout << "[Test] Compressibility: Mach 0 Reproduces Table Data Exactly... ";

    const auto state = make_state(5.0, 250.0);
    const ControlSurfaces ctrl{0.0, 0.0, 0.0, 0.0};

    // Default (no Mach argument) must equal an explicit mach = 0.
    const auto a = F16AeroModel::compute_coefficients(state, ctrl, 20000.0);
    const auto b = F16AeroModel::compute_coefficients(state, ctrl, 20000.0, 0.0);

    assert(a.CL == b.CL && a.CD == b.CD && a.Cm == b.Cm);
    assert(a.CY == b.CY && a.Cl == b.Cl && a.Cn == b.Cn);

    std::cout << "PASSED (CL=" << a.CL << ", CD=" << a.CD << " unchanged)\n";
}

void test_prandtl_glauert_lift_rise() {
    std::cout << "[Test] Compressibility: Prandtl-Glauert Subsonic Lift Rise... ";

    const auto state = make_state(5.0, 250.0);
    const ControlSurfaces ctrl{0.0, 0.0, 0.0, 0.0};

    const auto low  = F16AeroModel::compute_coefficients(state, ctrl, 20000.0, 0.30);
    const auto high = F16AeroModel::compute_coefficients(state, ctrl, 20000.0, 0.75);

    // Lift coefficient must grow with Mach in the subsonic range.
    assert(high.CL > low.CL && "PG factor must raise CL with Mach");

    // Verify the factor itself against the closed form at M = 0.5.
    const double pg = F16AeroModel::prandtl_glauert(0.5);
    const double expected = 1.0 / std::sqrt(1.0 - 0.25);
    assert(std::abs(pg - expected) < 1e-12);

    // The factor must stay finite through M = 1 (no singularity).
    for (double m = 0.0; m <= 2.5; m += 0.05) {
        const double f = F16AeroModel::prandtl_glauert(m);
        assert(std::isfinite(f) && f > 0.0 && f < 10.0);
    }

    std::cout << "PASSED (M0.30 CL=" << low.CL << " -> M0.75 CL=" << high.CL
              << ", PG(0.5)=" << pg << ")\n";
}

/// Dynamic-pressure drag is what makes the aircraft q-limited at low altitude.
void test_dynamic_pressure_drag() {
    std::cout << "[Test] Compressibility: Dynamic-Pressure Drag Rise... ";

    // Nothing in the normal flight envelope.
    assert(F16AeroModel::dynamic_pressure_drag(0.0) == 0.0);
    assert(F16AeroModel::dynamic_pressure_drag(30000.0) == 0.0);
    assert(F16AeroModel::dynamic_pressure_drag(F16AeroModel::Q_DRAG_ONSET) == 0.0);

    // Rising steeply as the airframe approaches its structural placard.
    const double at_80 = F16AeroModel::dynamic_pressure_drag(80000.0);
    const double at_100 = F16AeroModel::dynamic_pressure_drag(100000.0);
    assert(at_80 > 0.0);
    assert(at_100 > at_80 && "drag must grow with dynamic pressure");

    // Quadratic: doubling the excess above onset quadruples the increment.
    const double e1 = F16AeroModel::dynamic_pressure_drag(
        F16AeroModel::Q_DRAG_ONSET * 2.0);
    const double e2 = F16AeroModel::dynamic_pressure_drag(
        F16AeroModel::Q_DRAG_ONSET * 3.0);
    assert(std::abs(e2 / e1 - 4.0) < 1e-6 && "must scale quadratically");

    // At altitude the same Mach number produces far less q, so the term must
    // not penalise the high-altitude supersonic envelope.
    const auto high = environment::Atmosphere1976::compute(11000.0, 600.0);
    assert(F16AeroModel::dynamic_pressure_drag(high.dynamic_pressure) <
           at_100 * 0.5 && "must be small at altitude");

    std::cout << "PASSED (onset " << F16AeroModel::Q_DRAG_ONSET / 1000.0
              << " kPa, 100 kPa -> +" << at_100 << " CD)\n";
}

void test_wave_drag() {
    std::cout << "[Test] Compressibility: Transonic Wave Drag Rise... ";

    // No wave drag below the critical Mach number.
    assert(F16AeroModel::wave_drag(0.50) == 0.0);
    assert(F16AeroModel::wave_drag(0.85) == 0.0);

    // Drag must rise through the transonic region.
    const double cd_090 = F16AeroModel::wave_drag(0.90);
    const double cd_098 = F16AeroModel::wave_drag(0.98);
    const double cd_peak = F16AeroModel::wave_drag(F16AeroModel::MACH_PEAK);

    assert(cd_090 > 0.0);
    assert(cd_098 > cd_090 && "drag must rise toward the transonic peak");
    assert(cd_peak >= cd_098);
    assert(std::abs(cd_peak - F16AeroModel::CD_WAVE_PEAK) < 1e-9);

    // Beyond the peak, drag first falls away from the transonic spike as the
    // shock system stabilises, then climbs again with Mach: shock losses and
    // inlet spillage keep growing. A flat plateau would leave the airframe
    // with no aerodynamic speed limit at all.
    const double cd_12 = F16AeroModel::wave_drag(1.20);
    const double cd_15 = F16AeroModel::wave_drag(1.50);
    const double cd_20 = F16AeroModel::wave_drag(2.00);
    assert(cd_12 < cd_peak && "drag must fall past the transonic peak");
    assert(cd_20 > cd_15 && "supersonic drag must rise with Mach");
    assert(cd_15 > F16AeroModel::CD_WAVE_SUPER * 0.9 &&
           "must stay near or above the supersonic level");

    // And it must appear in the actual drag coefficient.
    const auto state = make_state(2.0, 300.0);
    const ControlSurfaces ctrl{0.0, 0.0, 0.0, 0.0};
    const auto sub = F16AeroModel::compute_coefficients(state, ctrl, 40000.0, 0.70);
    const auto tra = F16AeroModel::compute_coefficients(state, ctrl, 40000.0, 1.05);
    assert(tra.CD > sub.CD && "transonic CD must exceed subsonic CD");

    std::cout << "PASSED (M0.90 " << cd_090 << " -> peak " << cd_peak
              << " -> M2.0 " << cd_20 << ")\n";
}

void test_mach_tuck() {
    std::cout << "[Test] Compressibility: Transonic Mach Tuck (Nose-Down Cm)... ";

    const auto state = make_state(2.0, 300.0);
    const ControlSurfaces ctrl{0.0, 0.0, 0.0, 0.0};

    const auto sub = F16AeroModel::compute_coefficients(state, ctrl, 40000.0, 0.70);
    const auto tra = F16AeroModel::compute_coefficients(state, ctrl, 40000.0, 1.05);

    // The aft AC shift must drive the pitching moment nose-down relative to
    // the purely PG-scaled subsonic value.
    assert(tra.Cm < sub.Cm && "Mach tuck must reduce Cm (nose-down)");

    std::cout << "PASSED (Cm " << sub.Cm << " at M0.70 -> " << tra.Cm << " at M1.05)\n";
}

void test_speedbrake_drag() {
    std::cout << "[Test] Speedbrake: Drag Increment & Force Synthesis... ";

    const auto state = make_state(4.0, 200.0);

    const ControlSurfaces closed{0.0, 0.0, 0.0, 0.0, 0.0};
    const ControlSurfaces half{0.0, 0.0, 0.0, 0.0, 0.5};
    const ControlSurfaces open{0.0, 0.0, 0.0, 0.0, 1.0};

    const auto c0 = F16AeroModel::compute_coefficients(state, closed, 25000.0);
    const auto c5 = F16AeroModel::compute_coefficients(state, half, 25000.0);
    const auto c1 = F16AeroModel::compute_coefficients(state, open, 25000.0);

    // Drag must rise monotonically with extension.
    assert(c5.CD > c0.CD && c1.CD > c5.CD && "speedbrake must add drag");

    // Full extension must add exactly the specified increment.
    const double delta = c1.CD - c0.CD;
    assert(std::abs(delta - F16AeroModel::CD_SPEEDBRAKE_FULL) < 1e-9);

    // Half extension must add half the increment (linear in position).
    assert(std::abs((c5.CD - c0.CD) - 0.5 * F16AeroModel::CD_SPEEDBRAKE_FULL) < 1e-9);

    // The increment must reach the dimensional forces, not just coefficients.
    const auto f0 = F16AeroModel::compute_forces_and_moments(state, closed, 25000.0);
    const auto f1 = F16AeroModel::compute_forces_and_moments(state, open, 25000.0);
    assert(f1.force_b.x < f0.force_b.x && "extended brakes must add retarding force");

    // Clamping must reject out-of-range commands.
    const ControlSurfaces over{0.0, 0.0, 0.0, 0.0, 3.0};
    const auto co = F16AeroModel::compute_coefficients(state, over, 25000.0);
    assert(std::abs(co.CD - c1.CD) < 1e-12 && "speedbrake must clamp at 1.0");

    std::cout << "PASSED (Delta CD = " << delta << " at full extension, "
              << (f0.force_b.x - f1.force_b.x) << " N retarding force)\n";
}

} // namespace

int main() {
    std::cout << "=== Compressibility & Speedbrake Aerodynamics Verification ===\n";

    test_incompressible_backward_compatible();
    test_prandtl_glauert_lift_rise();
    test_wave_drag();
    test_dynamic_pressure_drag();
    test_mach_tuck();
    test_speedbrake_drag();

    std::cout << "All Compressibility and Speedbrake tests passed successfully!\n";
    return 0;
}
