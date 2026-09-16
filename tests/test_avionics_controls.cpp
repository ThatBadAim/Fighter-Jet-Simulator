#include "../include/fastjet/input/avionics_controls.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace fastjet::input;

void test_digital_trim_hat() {
    std::cout << "[Test] Avionics: Digital Trim Hat Slew & Bounds... " << std::flush;

    DigitalTrimHat trim;
    trim.slew_rate = 0.05; // 5% per second
    constexpr double DT = 0.10; // 100 ms steps

    // Trim Nose Up for 2 seconds -> +0.10
    for (int i = 0; i < 20; ++i) {
        trim.step(+1.0, 0.0, DT);
    }
    assert(std::abs(trim.trim_pitch - 0.10) < 1e-4);
    assert(std::abs(trim.trim_roll) < 1e-6);

    // Continue trimming up to hit +0.50 limit
    for (int i = 0; i < 200; ++i) {
        trim.step(+1.0, 0.0, DT);
    }
    assert(std::abs(trim.trim_pitch - 0.50) < 1e-6);

    // Trim Roll Left for 3 seconds -> -0.15
    for (int i = 0; i < 30; ++i) {
        trim.step(0.0, -1.0, DT);
    }
    assert(std::abs(trim.trim_roll - (-0.15)) < 1e-4);

    // Reset trim
    trim.reset();
    assert(trim.trim_pitch == 0.0);
    assert(trim.trim_roll == 0.0);

    std::cout << "PASSED\n";
}

void test_throttle_detents() {
    std::cout << "[Test] Avionics: Throttle Detents (Cutoff, Idle, Mil, Afterburner)... " << std::flush;

    ThrottleController throttle;

    // 1. Cutoff Detent (u < 0.03)
    throttle.update(0.01);
    assert(throttle.state == ThrottleController::DetentState::CUTOFF);
    assert(throttle.net_thrust_n == 0.0);

    // 2. Idle Detent (u = 0.08)
    throttle.update(0.08);
    assert(throttle.state == ThrottleController::DetentState::IDLE);
    assert(throttle.net_thrust_n == ThrottleController::THRUST_IDLE_N); // 4,450 N
    assert(throttle.dry_power == 0.0);
    assert(throttle.ab_power == 0.0);

    // 3. 50% Dry Thrust (u ~ 0.465)
    throttle.update((0.08 + 0.85) / 2.0);
    assert(throttle.state == ThrottleController::DetentState::MIL_POWER);
    assert(std::abs(throttle.dry_power - 0.50) < 1e-3);
    assert(throttle.ab_power == 0.0);

    // 4. Military Power Detent (u = 0.85)
    throttle.update(0.85);
    assert(throttle.state == ThrottleController::DetentState::MIL_POWER);
    assert(std::abs(throttle.dry_power - 1.0) < 1e-6);
    assert(throttle.ab_power == 0.0);
    assert(std::abs(throttle.net_thrust_n - ThrottleController::THRUST_MIL_N) < 1e-3); // 75,600 N

    // 5. Afterburner Zone (u = 0.925 -> 50% AB)
    throttle.update(0.925);
    assert(throttle.state == ThrottleController::DetentState::AFTERBURNER);
    assert(throttle.dry_power == 1.0);
    assert(std::abs(throttle.ab_power - 0.50) < 1e-3);
    const double expected_50ab = ThrottleController::THRUST_MIL_N +
        0.5 * (ThrottleController::THRUST_MAX_AB - ThrottleController::THRUST_MIL_N);
    assert(std::abs(throttle.net_thrust_n - expected_50ab) < 1e-3);

    // 6. Full Afterburner (u = 1.00 -> 100% AB)
    throttle.update(1.00);
    assert(throttle.state == ThrottleController::DetentState::AFTERBURNER);
    assert(std::abs(throttle.ab_power - 1.0) < 1e-6);
    assert(std::abs(throttle.net_thrust_n - ThrottleController::THRUST_MAX_AB) < 1e-3); // 129,000 N

    std::cout << "PASSED\n";
}

void test_speedbrake_and_wheel_brakes() {
    std::cout << "[Test] Avionics: Speedbrake Deployment & Wheel Brakes... " << std::flush;

    SpeedbrakeController sb;
    constexpr double DT = 0.05;

    // Full extension in 2 seconds (slew_rate = 0.50 / s)
    for (int i = 0; i < 40; ++i) {
        sb.update(SpeedbrakeController::SwitchPosition::EXTEND, DT);
    }
    assert(std::abs(sb.position - 1.0) < 1e-6);
    assert(std::abs(sb.get_drag_coefficient_increment() - 0.025) < 1e-6);

    // Hold at position
    sb.update(SpeedbrakeController::SwitchPosition::OFF, DT);
    assert(std::abs(sb.position - 1.0) < 1e-6);

    // Retract halfway (1 second)
    for (int i = 0; i < 20; ++i) {
        sb.update(SpeedbrakeController::SwitchPosition::RETRACT, DT);
    }
    assert(std::abs(sb.position - 0.50) < 1e-6);
    assert(std::abs(sb.get_drag_coefficient_increment() - 0.0125) < 1e-6);

    // Wheel Brakes
    WheelBrakes brakes;
    brakes.left_brake = 0.70;
    brakes.right_brake = 0.30;
    assert(brakes.effective_left() == 0.70);
    assert(brakes.effective_right() == 0.30);

    // Parking brake locks both at 100%
    brakes.parking_brake = true;
    assert(brakes.effective_left() == 1.00);
    assert(brakes.effective_right() == 1.00);

    std::cout << "PASSED\n";
}

int main() {
    std::cout << "=== Avionics Controls Verification ===\n";
    test_digital_trim_hat();
    test_throttle_detents();
    test_speedbrake_and_wheel_brakes();
    std::cout << "All Avionics Controls tests passed successfully!\n\n";
    return 0;
}
