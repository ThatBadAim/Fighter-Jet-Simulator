#include "../include/fastjet/aero/interpolator.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace fastjet::aero;

void test_1d_interpolation() {
    std::cout << "[Test] 1D Linear Interpolator & Edge Clamping... " << std::flush;

    constexpr std::array<double, 5> x_grid{0.0, 10.0, 20.0, 30.0, 40.0};
    constexpr std::array<double, 5> y_data{10.0, 30.0, 50.0, 70.0, 90.0}; // y = 2x + 10

    // Exact grid points
    assert(std::abs(Interpolator::interp1d(x_grid, y_data, 0.0) - 10.0) < 1e-12);
    assert(std::abs(Interpolator::interp1d(x_grid, y_data, 20.0) - 50.0) < 1e-12);
    assert(std::abs(Interpolator::interp1d(x_grid, y_data, 40.0) - 90.0) < 1e-12);

    // Midpoints
    assert(std::abs(Interpolator::interp1d(x_grid, y_data, 15.0) - 40.0) < 1e-12);
    assert(std::abs(Interpolator::interp1d(x_grid, y_data, 35.0) - 80.0) < 1e-12);

    // Clamping outside bounds
    assert(std::abs(Interpolator::interp1d(x_grid, y_data, -10.0) - 10.0) < 1e-12);
    assert(std::abs(Interpolator::interp1d(x_grid, y_data, 100.0) - 90.0) < 1e-12);

    std::cout << "PASSED\n";
}

void test_2d_bilinear_interpolation() {
    std::cout << "[Test] 2D Bilinear Interpolator... " << std::flush;

    constexpr std::size_t NX = 3;
    constexpr std::size_t NY = 4;
    constexpr std::array<double, NX> x_grid{0.0, 5.0, 10.0};
    constexpr std::array<double, NY> y_grid{0.0, 2.0, 4.0, 6.0};

    // f(x, y) = 3*x + 2*y + 1
    std::array<double, NX * NY> table{};
    for (std::size_t i = 0; i < NX; ++i) {
        for (std::size_t j = 0; j < NY; ++j) {
            table[i * NY + j] = 3.0 * x_grid[i] + 2.0 * y_grid[j] + 1.0;
        }
    }

    // Interior evaluation at (2.5, 3.0) -> 3*2.5 + 2*3.0 + 1 = 7.5 + 6.0 + 1 = 14.5
    const double val = Interpolator::interp2d<NX, NY>(x_grid, y_grid, table, 2.5, 3.0);
    assert(std::abs(val - 14.5) < 1e-12);

    // Edge clamping check
    const double clamped_val = Interpolator::interp2d<NX, NY>(x_grid, y_grid, table, -5.0, 20.0);
    // Should evaluate at x=0, y=6 -> 3*0 + 2*6 + 1 = 13.0
    assert(std::abs(clamped_val - 13.0) < 1e-12);

    std::cout << "PASSED\n";
}

void test_3d_trilinear_interpolation() {
    std::cout << "[Test] 3D Trilinear Interpolator... " << std::flush;

    constexpr std::size_t NX = 3;
    constexpr std::size_t NY = 3;
    constexpr std::size_t NZ = 3;
    constexpr std::array<double, NX> x_grid{0.0, 10.0, 20.0};
    constexpr std::array<double, NY> y_grid{0.0, 5.0, 10.0};
    constexpr std::array<double, NZ> z_grid{0.0, 2.0, 4.0};

    // f(x, y, z) = x + 2*y + 4*z
    std::array<double, NX * NY * NZ> table{};
    for (std::size_t i = 0; i < NX; ++i) {
        for (std::size_t j = 0; j < NY; ++j) {
            for (std::size_t k = 0; k < NZ; ++k) {
                const std::size_t idx = (i * NY + j) * NZ + k;
                table[idx] = x_grid[i] + 2.0 * y_grid[j] + 4.0 * z_grid[k];
            }
        }
    }

    // Evaluate at interior point (5.0, 2.5, 3.0) -> 5.0 + 2*2.5 + 4*3.0 = 5.0 + 5.0 + 12.0 = 22.0
    const double val = Interpolator::interp3d<NX, NY, NZ>(x_grid, y_grid, z_grid, table, 5.0, 2.5, 3.0);
    assert(std::abs(val - 22.0) < 1e-12);

    // Clamped evaluation
    const double clamped = Interpolator::interp3d<NX, NY, NZ>(x_grid, y_grid, z_grid, table, -10.0, 50.0, 2.0);
    // x=0, y=10, z=2 -> 0 + 2*10 + 4*2 = 28.0
    assert(std::abs(clamped - 28.0) < 1e-12);

    std::cout << "PASSED\n";
}

int main() {
    std::cout << "=== Interpolation Engine Verification ===\n";
    test_1d_interpolation();
    test_2d_bilinear_interpolation();
    test_3d_trilinear_interpolation();
    std::cout << "All Interpolator tests passed successfully!\n\n";
    return 0;
}
