#include "../include/fastjet/environment/atmosphere1976.hpp"
#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>

using namespace fastjet::environment;

void test_sea_level() {
    std::cout << "[Test] 1976 US Standard Atmosphere: Sea Level (0 m)... " << std::flush;
    const AirData ad = Atmosphere1976::compute(0.0, 0.0);

    // Standard sea level: T = 288.15 K, P = 101325 Pa, rho = 1.2250 kg/m^3, a = 340.294 m/s
    assert(std::abs(ad.temperature - 288.15) < 1e-3);
    assert(std::abs(ad.pressure - 101325.0) < 1e-1);
    assert(std::abs(ad.density - 1.2250) < 1e-3);
    assert(std::abs(ad.speed_of_sound - 340.294) < 0.1);
    std::cout << "PASSED (T=" << ad.temperature << " K, P=" << ad.pressure
              << " Pa, rho=" << ad.density << " kg/m^3, a=" << ad.speed_of_sound << " m/s)\n";
}

void test_troposphere_5000m() {
    std::cout << "[Test] 1976 US Standard Atmosphere: 5,000 m... " << std::flush;
    const AirData ad = Atmosphere1976::compute(5000.0, 200.0); // 200 m/s airspeed

    // 5000 m standard: T ~ 255.676 K, P ~ 54048 Pa, rho ~ 0.7364 kg/m^3, a ~ 320.5 m/s
    assert(std::abs(ad.temperature - 255.68) < 0.2);
    assert(std::abs(ad.pressure - 54048.0) / 54048.0 < 0.005);
    assert(std::abs(ad.density - 0.7364) / 0.7364 < 0.005);
    assert(std::abs(ad.speed_of_sound - 320.5) < 0.5);

    // Check dynamic pressure: q_bar = 0.5 * rho * V^2
    const double expected_q = 0.5 * ad.density * 200.0 * 200.0;
    assert(std::abs(ad.dynamic_pressure - expected_q) < 1e-5);

    // Check Mach: M = V / a
    const double expected_mach = 200.0 / ad.speed_of_sound;
    assert(std::abs(ad.mach_number - expected_mach) < 1e-5);

    std::cout << "PASSED (T=" << ad.temperature << " K, P=" << ad.pressure
              << " Pa, rho=" << ad.density << " kg/m^3, q_bar=" << ad.dynamic_pressure
              << " Pa, Mach=" << ad.mach_number << ")\n";
}

void test_tropopause_11000m() {
    std::cout << "[Test] 1976 US Standard Atmosphere: Tropopause (H = 11,000 m geopotential)... " << std::flush;
    const AirData ad = Atmosphere1976::compute_from_geopotential(11000.0, 0.0);

    // Tropopause standard: T = 216.65 K, P ~ 22632 Pa, rho ~ 0.3639 kg/m^3, a ~ 295.07 m/s
    assert(std::abs(ad.temperature - 216.65) < 1e-4);
    assert(std::abs(ad.pressure - 22632.06) / 22632.06 < 0.001);
    assert(std::abs(ad.density - 0.3639) / 0.3639 < 0.005);
    assert(std::abs(ad.speed_of_sound - 295.07) < 0.2);
    std::cout << "PASSED (T=" << ad.temperature << " K, P=" << ad.pressure
              << " Pa, rho=" << ad.density << " kg/m^3)\n";

    // Also test geometric altitude h = 11,000 m (where H ~ 10,981 m)
    const AirData ad_geom = Atmosphere1976::compute(11000.0, 0.0);
    assert(std::abs(ad_geom.temperature - 216.77) < 0.1);
}

void test_stratosphere_25000m() {
    std::cout << "[Test] 1976 US Standard Atmosphere: Stratosphere (H = 25,000 m geopotential)... " << std::flush;
    const AirData ad = Atmosphere1976::compute_from_geopotential(25000.0, 400.0);

    // 25 km standard: T ~ 221.55 K, P ~ 2549 Pa, rho ~ 0.04008 kg/m^3
    assert(std::abs(ad.temperature - 221.55) < 0.2);
    assert(std::abs(ad.pressure - 2549.0) / 2549.0 < 0.02);
    assert(std::abs(ad.density - 0.04008) / 0.04008 < 0.02);
    std::cout << "PASSED (T=" << ad.temperature << " K, P=" << ad.pressure
              << " Pa, rho=" << ad.density << " kg/m^3, Mach=" << ad.mach_number << ")\n";
}

int main() {
    std::cout << "=== 1976 US Standard Atmosphere Model Verification ===\n";
    test_sea_level();
    test_troposphere_5000m();
    test_tropopause_11000m();
    test_stratosphere_25000m();
    std::cout << "All Atmosphere1976 tests passed successfully!\n\n";
    return 0;
}
