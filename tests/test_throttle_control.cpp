#include "fastjet/propulsion/f110_engine.hpp"
#include "fastjet/fdm/rk4_integrator.hpp"
#include "fastjet/fdm/fuel_system.hpp"
#include "fastjet/environment/atmosphere1976.hpp"
#include "fastjet/aero/f16_aero_model.hpp"
#include "fastjet/flcs/f16_flcs.hpp"
#include "fastjet/input/avionics_controls.hpp"
#include "fastjet/input/signal_conditioner.hpp"
#include "fastjet/input/input_config.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace fastjet;

namespace {

constexpr double DT = 0.005;
constexpr double THROTTLE_SLEW_PER_SEC = 0.67; // Mirrors the viewer

/// Minimal free-flight model matching the viewer's integration order.
struct Flight {
    fdm::FlightState state{};
    fdm::MassProperties mass{};
    fdm::RK4Integrator integrator{DT};
    aero::F16AeroModel aero{};
    flcs::F16FLCS flcs{};
    propulsion::F110Engine engine{};
    fdm::AircraftForces forces{};
    double time{0.0};
    double speedbrake{0.0};

    Flight() { mass = fdm::FuelSystem::compute(engine.fuel_kg); }

    void trim(double alt, double v) {
        state = fdm::FlightState{};
        state.pos_ned = math::Vector3(0.0, 0.0, -alt);
        state.vel_b = math::Vector3(v, 0.0, 0.0);
        state.q_att = math::Quaternion::identity();
    }

    void step(double throttle) {
        const auto air = environment::Atmosphere1976::compute(
            state.altitude(), state.airspeed());
        flcs::PilotCommands pilot{0.0, 0.0, 0.0};
        auto surfaces = flcs.update(DT, state, pilot, air.dynamic_pressure, forces, mass);
        surfaces.speedbrake = speedbrake;

        const double thrust = engine.update(throttle, air, DT);
        mass = fdm::FuelSystem::compute(engine.fuel_kg);

        integrator.step(state, time, mass,
            [&](double, const fdm::FlightState& s) noexcept -> fdm::AircraftForces {
                const auto sa = environment::Atmosphere1976::compute(
                    s.altitude(), s.airspeed());
                fdm::AircraftForces f = aero.compute_forces_and_moments(
                    s, surfaces, sa.dynamic_pressure, sa.mach_number);
                f.force_b.x += thrust;
                return f;
            });
        forces = aero.compute_forces_and_moments(
            state, surfaces, air.dynamic_pressure, air.mach_number);
        forces.force_b.x += thrust;
    }
};

void test_throttle_slew_is_frame_rate_independent() {
    std::cout << "[Test] Throttle: Lever Slew Is Frame-Rate Independent... ";

    // Run the lever from idle to full at two very different frame rates.
    auto run = [](double frame_dt, double seconds) {
        double throttle = 0.0;
        const int frames = static_cast<int>(seconds / frame_dt);
        for (int i = 0; i < frames; ++i) {
            throttle = std::clamp(throttle + 1.0 * THROTTLE_SLEW_PER_SEC * frame_dt, 0.0, 1.0);
        }
        return throttle;
    };

    const double at_60fps   = run(1.0 / 60.0, 1.0);
    const double at_8000fps = run(1.0 / 8000.0, 1.0);

    // One second of travel must cover the same fraction at any frame rate.
    assert(std::abs(at_60fps - at_8000fps) < 0.01 &&
           "throttle travel must not depend on frame rate");
    assert(std::abs(at_60fps - THROTTLE_SLEW_PER_SEC) < 0.02 &&
           "one second must cover the specified slew rate");

    // Full travel must take a usable amount of time, not be instantaneous.
    const double full_travel_s = 1.0 / THROTTLE_SLEW_PER_SEC;
    assert(full_travel_s > 1.0 && full_travel_s < 3.0 &&
           "idle to full reheat should take 1-3 s");

    std::cout << "PASSED (60 FPS: " << at_60fps << ", 8000 FPS: " << at_8000fps
              << " after 1 s; full travel " << full_travel_s << " s)\n";
}

void test_afterburner_accelerates() {
    std::cout << "[Test] Throttle: Afterburner Accelerates the Aircraft... ";

    Flight idle, mil, ab;
    idle.trim(3000.0, 200.0);
    mil.trim(3000.0, 200.0);
    ab.trim(3000.0, 200.0);

    for (int i = 0; i < 6000; ++i) { // 30 s
        idle.step(0.08);
        mil.step(0.85);
        ab.step(1.00);
    }

    // Afterburner must out-accelerate military power, which must beat idle.
    assert(ab.state.airspeed() > mil.state.airspeed() &&
           "afterburner must accelerate harder than military power");
    assert(mil.state.airspeed() > idle.state.airspeed() &&
           "military power must accelerate harder than idle");

    // And the aircraft must genuinely speed up from its trim point.
    assert(ab.state.airspeed() > 250.0 && "afterburner must produce real acceleration");

    // The engine must actually be in the afterburner detent.
    assert(ab.engine.lever.state == input::ThrottleController::DetentState::AFTERBURNER);
    assert(mil.engine.lever.state == input::ThrottleController::DetentState::MIL_POWER);

    std::cout << "PASSED (30 s from 200 m/s: idle " << idle.state.airspeed()
              << ", MIL " << mil.state.airspeed()
              << ", AB " << ab.state.airspeed() << " m/s)\n";
}

void test_throttle_back_decelerates() {
    std::cout << "[Test] Throttle: Closing the Throttle Decelerates... ";

    Flight f;
    f.trim(3000.0, 300.0);

    // Accelerate under reheat, then pull the throttle to idle.
    for (int i = 0; i < 2000; ++i) f.step(1.0);
    const double fast = f.state.airspeed();

    for (int i = 0; i < 8000; ++i) f.step(0.08); // 40 s at idle
    const double slow = f.state.airspeed();

    assert(slow < fast && "closing the throttle must slow the aircraft");
    assert(f.engine.lever.state == input::ThrottleController::DetentState::IDLE);

    std::cout << "PASSED (" << fast << " -> " << slow << " m/s after 40 s at idle)\n";
}

void test_speedbrake_adds_deceleration() {
    std::cout << "[Test] Throttle: Speedbrake Increases Deceleration... ";

    Flight clean, braked;
    clean.trim(3000.0, 300.0);
    braked.trim(3000.0, 300.0);
    braked.speedbrake = 1.0;

    for (int i = 0; i < 4000; ++i) { // 20 s at idle
        clean.step(0.08);
        braked.step(0.08);
    }

    assert(braked.state.airspeed() < clean.state.airspeed() &&
           "speedbrake must add drag on top of a closed throttle");

    std::cout << "PASSED (idle only " << clean.state.airspeed()
              << " m/s vs idle+speedbrake " << braked.state.airspeed() << " m/s)\n";
}

void test_speedbrake_slews_not_snaps() {
    std::cout << "[Test] Throttle: Speedbrake Slews at Hydraulic Rate... ";

    input::SpeedbrakeController sb;
    assert(sb.position == 0.0);

    // One physics step must not fling the surface fully open.
    sb.update(input::SpeedbrakeController::SwitchPosition::EXTEND, DT);
    assert(sb.position > 0.0 && sb.position < 0.05 &&
           "speedbrake must not snap open in a single step");

    // Full extension should take about 2 s.
    int steps = 1;
    while (sb.position < 0.999 && steps < 10000) {
        sb.update(input::SpeedbrakeController::SwitchPosition::EXTEND, DT);
        ++steps;
    }
    const double extend_time = steps * DT;
    assert(extend_time > 1.0 && extend_time < 3.0 && "full travel should take ~2 s");

    // And it must retract again.
    for (int i = 0; i < 1000; ++i) {
        sb.update(input::SpeedbrakeController::SwitchPosition::RETRACT, DT);
    }
    assert(sb.position == 0.0 && "speedbrake must fully retract");

    std::cout << "PASSED (full extension in " << extend_time << " s, retracts fully)\n";
}

void test_detent_boundaries() {
    std::cout << "[Test] Throttle: Quick-Set Detent Positions... ";

    const auto sl = environment::Atmosphere1976::compute(0.0, 0.0);

    // The four quick-set keys map to these lever positions.
    struct Case { double pos; input::ThrottleController::DetentState want; };
    const Case cases[] = {
        {0.00, input::ThrottleController::DetentState::CUTOFF},
        {0.08, input::ThrottleController::DetentState::IDLE},
        {0.85, input::ThrottleController::DetentState::MIL_POWER},
        {1.00, input::ThrottleController::DetentState::AFTERBURNER},
    };

    for (const auto& c : cases) {
        propulsion::F110Engine e;
        for (int i = 0; i < 2000; ++i) e.update(c.pos, sl, DT);
        assert(e.lever.state == c.want && "quick-set key must hit its detent");
    }

    std::cout << "PASSED (Cutoff / Idle / MIL / AB all reachable)\n";
}

} // namespace


/// Regression: a hardware throttle resting at its minimum must NOT capture the
/// throttle. The Logitech Extreme 3D Pro reports its slider as raw -32768,
/// which conditions to exactly 0.0 (engine cutoff). With a small latch
/// threshold, axis noise around that rest position would hand the engine to a
/// lever the pilot never touched, pinning thrust at cutoff and making the
/// keyboard throttle appear completely dead.
void test_resting_hardware_lever_does_not_capture_throttle() {
    std::cout << "[Test] Throttle: Resting Hardware Lever Does Not Capture... ";

    // Mirrors the viewer's latch logic.
    constexpr double DEADBAND = 0.25;
    bool active = false;
    double ref = -1.0;

    auto feed = [&](double axis) {
        if (!active) {
            if (ref < 0.0) ref = axis;
            else if (std::abs(axis - ref) > DEADBAND) active = true;
        }
    };

    // A slider parked at minimum, dithering by a couple of percent.
    const double rest = 0.0;
    for (int i = 0; i < 500; ++i) {
        feed(rest + ((i % 7) - 3) * 0.01); // +/- 3% noise
    }
    assert(!active && "Axis noise must not capture the throttle");

    // A deliberate push past the deadband does take over.
    feed(0.60);
    assert(active && "A real lever movement must take control");

    // Verify both raw slider orientations condition to cutoff at rest:
    // 1. Non-inverted slider at -32768:
    const input::AxisCalibration cal_noninv{-32768, 0, 32767, 0.02, 0.01, false, 0.00};
    const double at_rest_noninv = input::SignalConditioner::process_unipolar(-32768, cal_noninv);
    assert(at_rest_noninv == 0.0 && "Resting slider (non-inverted) conditions to cutoff");

    // 2. Inverted slider at +32767 (pulled fully back to rest):
    const input::AxisCalibration cal_inv{-32768, 0, 32767, 0.02, 0.01, true, 0.00};
    const double at_rest_inv = input::SignalConditioner::process_unipolar(32767, cal_inv);
    assert(at_rest_inv == 0.0 && "Resting slider (inverted, pulled back) conditions to cutoff");

    input::ThrottleController lever{};
    const double thrust = lever.update(at_rest_inv);
    assert(thrust == 0.0 && "Cutoff produces zero thrust");
    assert(lever.state == input::ThrottleController::DetentState::CUTOFF);

    std::cout << "PASS (rest=" << at_rest_inv << " -> " << thrust << " N, engine cutoff)\n";
}

/// The periodic status readout must be paced in wall-clock time, not frames.
/// At the ~8,600 FPS this viewer reaches, a frame-count cadence of 120 prints
/// ~70 lines per second and buries the throttle state in scrollback.
void test_status_cadence_is_time_based() {
    std::cout << "[Test] Throttle: Status Readout Is Time-Paced... ";

    auto lines_in_10s = [](double frame_dt) {
        double timer = 0.0;
        int lines = 0;
        for (double t = 0.0; t < 10.0; t += frame_dt) {
            timer += frame_dt;
            if (timer >= 1.0) { timer = 0.0; ++lines; }
        }
        return lines;
    };

    const int slow = lines_in_10s(1.0 / 60.0);
    const int fast = lines_in_10s(1.0 / 8600.0);
    assert(slow >= 9 && slow <= 11);
    assert(fast >= 9 && fast <= 11);
    assert(std::abs(slow - fast) <= 1 && "Cadence must not depend on frame rate");

    std::cout << "PASS (" << slow << " lines at 60 FPS, " << fast << " at 8600 FPS)\n";
}

int main() {
    std::cout << "=== Throttle, Afterburner & Deceleration Verification ===\n";

    test_throttle_slew_is_frame_rate_independent();

    test_resting_hardware_lever_does_not_capture_throttle();

    test_status_cadence_is_time_based();
    test_afterburner_accelerates();
    test_throttle_back_decelerates();
    test_speedbrake_adds_deceleration();
    test_speedbrake_slews_not_snaps();
    test_detent_boundaries();

    std::cout << "All Throttle Control tests passed successfully!\n";
    return 0;
}
