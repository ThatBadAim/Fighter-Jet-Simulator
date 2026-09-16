#include "../include/fastjet/input/signal_conditioner.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace fastjet::input;

void test_deadband_filtering() {
    std::cout << "[Test] Signal Conditioner: Inner & Outer Deadbands... " << std::flush;

    // 5% inner deadband, 3% outer deadband
    constexpr double inner_db = 0.05;
    constexpr double outer_db = 0.03;

    // Within inner deadband -> exactly 0.0
    assert(SignalConditioner::apply_deadbands(0.0, inner_db, outer_db) == 0.0);
    assert(SignalConditioner::apply_deadbands(0.04, inner_db, outer_db) == 0.0);
    assert(SignalConditioner::apply_deadbands(-0.049, inner_db, outer_db) == 0.0);

    // Within outer deadband -> exactly 1.0 (or -1.0)
    assert(SignalConditioner::apply_deadbands(0.97, inner_db, outer_db) == 1.0);
    assert(SignalConditioner::apply_deadbands(1.0, inner_db, outer_db) == 1.0);
    assert(SignalConditioner::apply_deadbands(-0.98, inner_db, outer_db) == -1.0);

    // Midpoint scaling: between 0.05 and 0.97 (span = 0.92)
    // x = 0.51 -> (0.51 - 0.05) / 0.92 = 0.46 / 0.92 = 0.50 exactly
    const double mid = SignalConditioner::apply_deadbands(0.51, inner_db, outer_db);
    assert(std::abs(mid - 0.50) < 1e-12);

    // Continuity: right above deadband threshold, output is smoothly epsilon above 0
    const double just_above = SignalConditioner::apply_deadbands(0.0501, inner_db, outer_db);
    assert(just_above > 0.0 && just_above < 0.001);

    std::cout << "PASSED\n";
}

void test_axis_calibration_and_inversion() {
    std::cout << "[Test] Axis Calibration Offsets & Inversion... " << std::flush;

    // Asymmetric potentiometer: raw_min = 1000, raw_center = 2000, raw_max = 4000
    // Zero deadband, linear curve
    AxisCalibration cal(1000, 2000, 4000, 0.0, 0.0, false, 0.0);

    // Neutral center -> 0.0
    assert(std::abs(SignalConditioner::process_bipolar(2000, cal)) < 1e-6);

    // Minimum -> -1.0
    assert(std::abs(SignalConditioner::process_bipolar(1000, cal) - (-1.0)) < 1e-6);

    // Maximum -> +1.0
    assert(std::abs(SignalConditioner::process_bipolar(4000, cal) - (+1.0)) < 1e-6);

    // 50% left: 1500 -> -0.5
    assert(std::abs(SignalConditioner::process_bipolar(1500, cal) - (-0.5)) < 1e-6);

    // 50% right: 3000 -> +0.5
    assert(std::abs(SignalConditioner::process_bipolar(3000, cal) - (+0.5)) < 1e-6);

    // Inversion test
    cal.inverted = true;
    assert(std::abs(SignalConditioner::process_bipolar(1000, cal) - (+1.0)) < 1e-6);
    assert(std::abs(SignalConditioner::process_bipolar(4000, cal) - (-1.0)) < 1e-6);
    assert(std::abs(SignalConditioner::process_bipolar(3000, cal) - (-0.5)) < 1e-6);

    std::cout << "PASSED\n";
}

void test_unipolar_axis_conditioning() {
    std::cout << "[Test] Unipolar Conditioning (Throttle / Brakes)... " << std::flush;

    // Throttle range: 0 to 1000 with 5% inner deadband and 2% outer deadband
    AxisCalibration thr_cal(0, 0, 1000, 0.05, 0.02, false, 0.0);

    // 0 raw -> 0.0
    assert(SignalConditioner::process_unipolar(0, thr_cal) == 0.0);
    assert(SignalConditioner::process_unipolar(40, thr_cal) == 0.0); // within 5% inner deadband

    // 990 raw -> 1.0 (within 2% outer deadband)
    assert(SignalConditioner::process_unipolar(990, thr_cal) == 1.0);
    assert(SignalConditioner::process_unipolar(1000, thr_cal) == 1.0);

    // Inverted throttle
    thr_cal.inverted = true;
    assert(SignalConditioner::process_unipolar(0, thr_cal) == 1.0);
    assert(SignalConditioner::process_unipolar(1000, thr_cal) == 0.0);

    std::cout << "PASSED\n";
}

int main() {
    std::cout << "=== Signal Conditioning & Axis Calibration Verification ===\n";
    test_deadband_filtering();
    test_axis_calibration_and_inversion();
    test_unipolar_axis_conditioning();
    std::cout << "All Signal Conditioning tests passed successfully!\n\n";
    return 0;
}
