#pragma once

#include <cmath>
#include <algorithm>

namespace fastjet {
namespace environment {

/**
 * @brief Output struct representing thermodynamic air properties at altitude.
 */
struct AirData {
    double geometric_altitude{0.0};    // h [m]
    double geopotential_altitude{0.0};  // H [m]
    double temperature{288.15};         // T [K]
    double pressure{101325.0};          // P [Pa]
    double density{1.2250};             // rho [kg/m^3]
    double speed_of_sound{340.294};     // a [m/s]
    double dynamic_pressure{0.0};       // q_bar [Pa]
    double mach_number{0.0};            // M [-]
};

/**
 * @brief 1976 US Standard Atmosphere Model.
 * Computes ambient conditions up to 84,852 m (86 km geometric) geopotential altitude.
 * Zero-allocation, fast, deterministic.
 */
class Atmosphere1976 {
public:
    // Physical Constants
    static constexpr double R_EARTH = 6356766.0;         // Effective Earth radius [m]
    static constexpr double G0      = 9.80665;           // Sea-level gravitational acceleration [m/s^2]
    static constexpr double R_GAS   = 287.05287;         // Specific gas constant for dry air [J/(kg*K)]
    static constexpr double GAMMA   = 1.4;               // Ratio of specific heats for air [-]
    static constexpr double T0      = 288.15;            // Sea-level standard temperature [K]
    static constexpr double P0      = 101325.0;          // Sea-level standard pressure [Pa]
    static constexpr double RHO0    = 1.2250;            // Sea-level standard density [kg/m^3]

    /**
     * @brief Computes atmospheric properties at a given geopotential altitude H and true airspeed.
     *
     * @param geopotential_altitude_m Geopotential altitude in meters.
     * @param true_airspeed_mps True airspeed in meters/second (default = 0).
     * @return AirData containing T, P, rho, a, q_bar, and Mach.
     */
    [[nodiscard]] static AirData compute_from_geopotential(double geopotential_altitude_m, double true_airspeed_mps = 0.0) noexcept {
        AirData data;
        const double H = std::clamp(geopotential_altitude_m, -500.0, 84852.0);
        data.geopotential_altitude = H;
        data.geometric_altitude = (R_EARTH * H) / (R_EARTH - H);

        double T = T0;
        double P = P0;

        if (H <= 11000.0) {
            // Layer 0: Troposphere (0 to 11 km)
            constexpr double L0 = -0.0065; // K/m
            T = T0 + L0 * H;
            P = P0 * std::pow(T / T0, -G0 / (L0 * R_GAS));
        } else if (H <= 20000.0) {
            // Layer 1: Tropopause / Lower Stratosphere (11 to 20 km, isothermal)
            constexpr double H1 = 11000.0;
            constexpr double T1 = 216.65;
            constexpr double P1 = 22632.0639734629;
            T = T1;
            P = P1 * std::exp(-G0 * (H - H1) / (R_GAS * T1));
        } else if (H <= 32000.0) {
            // Layer 2: Stratosphere 1 (20 to 32 km)
            constexpr double H2 = 20000.0;
            constexpr double T2 = 216.65;
            constexpr double P2 = 5474.88866768;
            constexpr double L2 = 0.0010; // K/m
            T = T2 + L2 * (H - H2);
            P = P2 * std::pow(T / T2, -G0 / (L2 * R_GAS));
        } else if (H <= 47000.0) {
            // Layer 3: Stratosphere 2 (32 to 47 km)
            constexpr double H3 = 32000.0;
            constexpr double T3 = 228.65;
            constexpr double P3 = 868.0186835;
            constexpr double L3 = 0.0028; // K/m
            T = T3 + L3 * (H - H3);
            P = P3 * std::pow(T / T3, -G0 / (L3 * R_GAS));
        } else if (H <= 51000.0) {
            // Layer 4: Stratopause (47 to 51 km, isothermal)
            constexpr double H4 = 47000.0;
            constexpr double T4 = 270.65;
            constexpr double P4 = 110.9063055;
            T = T4;
            P = P4 * std::exp(-G0 * (H - H4) / (R_GAS * T4));
        } else if (H <= 71000.0) {
            // Layer 5: Mesosphere 1 (51 to 71 km)
            constexpr double H5 = 51000.0;
            constexpr double T5 = 270.65;
            constexpr double P5 = 66.9388731;
            constexpr double L5 = -0.0028; // K/m
            T = T5 + L5 * (H - H5);
            P = P5 * std::pow(T / T5, -G0 / (L5 * R_GAS));
        } else {
            // Layer 6: Mesosphere 2 (71 to 84.852 km)
            constexpr double H6 = 71000.0;
            constexpr double T6 = 214.65;
            constexpr double P6 = 3.9564201;
            constexpr double L6 = -0.0020; // K/m
            T = T6 + L6 * (H - H6);
            P = P6 * std::pow(T / T6, -G0 / (L6 * R_GAS));
        }

        data.temperature = T;
        data.pressure = P;
        data.density = P / (R_GAS * T);
        data.speed_of_sound = std::sqrt(GAMMA * R_GAS * T);

        const double tas = std::max(0.0, true_airspeed_mps);
        data.dynamic_pressure = 0.5 * data.density * tas * tas;
        data.mach_number = (data.speed_of_sound > 0.0) ? (tas / data.speed_of_sound) : 0.0;

        return data;
    }

    /**
     * @brief Computes atmospheric properties at a given geometric altitude h and true airspeed.
     * Converts geometric altitude h to geopotential altitude H = (R_e * h) / (R_e + h).
     *
     * @param geometric_altitude_m Altitude above mean sea level in meters.
     * @param true_airspeed_mps True airspeed in meters/second (default = 0).
     * @return AirData containing T, P, rho, a, q_bar, and Mach.
     */
    [[nodiscard]] static AirData compute(double geometric_altitude_m, double true_airspeed_mps = 0.0) noexcept {
        const double h = std::clamp(geometric_altitude_m, -500.0, 84852.0);
        const double H = (R_EARTH * h) / (R_EARTH + h);
        AirData data = compute_from_geopotential(H, true_airspeed_mps);
        data.geometric_altitude = geometric_altitude_m;
        return data;
    }
};

} // namespace environment
} // namespace fastjet
