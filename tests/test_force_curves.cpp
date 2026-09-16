#include "../include/fastjet/input/signal_conditioner.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace fastjet::input;

void test_force_curve_endpoints_and_symmetry() {
    std::cout << "[Test] Force-Sensing Curve: Endpoints & Symmetry... " << std::flush;

    for (double c = 0.0; c <= 1.0; c += 0.25) {
        // Zero point
        assert(std::abs(SignalConditioner::evaluate_force_curve(0.0, c)) < 1e-12);

        // Maximum endpoints
        assert(std::abs(SignalConditioner::evaluate_force_curve(1.0, c) - 1.0) < 1e-12);
        assert(std::abs(SignalConditioner::evaluate_force_curve(-1.0, c) - (-1.0)) < 1e-12);

        // Odd symmetry: y(-x) == -y(x)
        for (double x = 0.1; x < 1.0; x += 0.1) {
            const double y_pos = SignalConditioner::evaluate_force_curve(x, c);
            const double y_neg = SignalConditioner::evaluate_force_curve(-x, c);
            assert(std::abs(y_pos + y_neg) < 1e-12);
        }
    }
    std::cout << "PASSED\n";
}

void test_curvature_tuning() {
    std::cout << "[Test] Curvature Parameter c Tuning: Linear, Cubic, and Blended... " << std::flush;

    constexpr double x = 0.5;

    // c = 0.0 (Pure Linear): y = (1-0)*0.5 + 0*(0.5^3) = 0.5
    const double y_linear = SignalConditioner::evaluate_force_curve(x, 0.0);
    assert(std::abs(y_linear - 0.5) < 1e-12);

    // c = 1.0 (Pure Cubic): y = (1-1)*0.5 + 1*(0.5^3) = 0.125
    const double y_cubic = SignalConditioner::evaluate_force_curve(x, 1.0);
    assert(std::abs(y_cubic - 0.125) < 1e-12);

    // c = 0.5 (Blended): y = 0.5*0.5 + 0.5*0.125 = 0.25 + 0.0625 = 0.3125
    const double y_blend = SignalConditioner::evaluate_force_curve(x, 0.5);
    assert(std::abs(y_blend - 0.3125) < 1e-12);

    // Monotonic sensitivity progression: y_cubic < y_blend < y_linear
    assert(y_cubic < y_blend);
    assert(y_blend < y_linear);

    std::cout << "PASSED (c=0.0: " << y_linear << ", c=0.5: " << y_blend << ", c=1.0: " << y_cubic << ")\n";
}

void test_strict_monotonicity() {
    std::cout << "[Test] Force-Sensing Curve Monotonicity & Center Precision... " << std::flush;

    constexpr double c = 0.60; // 60% cubic curvature (standard F-16 HOTAS stick curve)

    // Verify center slope dy/dx = 1 - c = 0.40
    constexpr double dx = 1e-5;
    const double y_eps = SignalConditioner::evaluate_force_curve(dx, c);
    const double center_slope = y_eps / dx;
    assert(std::abs(center_slope - (1.0 - c)) < 1e-4);

    // Verify strict monotonicity across entire range [-1.0, 1.0]
    double prev_y = -1.0;
    for (double x = -0.99; x <= 1.00; x += 0.01) {
        const double cur_y = SignalConditioner::evaluate_force_curve(x, c);
        assert(cur_y > prev_y);
        prev_y = cur_y;
    }

    std::cout << "PASSED (Center slope: " << center_slope << ", Strictly monotonic)\n";
}

int main() {
    std::cout << "=== Force-Sensing Simulation Curve Verification ===\n";
    test_force_curve_endpoints_and_symmetry();
    test_curvature_tuning();
    test_strict_monotonicity();
    std::cout << "All Force Curve tests passed successfully!\n\n";
    return 0;
}
