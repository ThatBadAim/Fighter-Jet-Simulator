#pragma once

#include "../math/vector3.hpp"
#include "../math/matrix3x3.hpp"

#include "../aircraft/aircraft_type.hpp"
#include "../aircraft/aircraft_config.hpp"

namespace fastjet {
namespace fdm {

/**
 * @brief Mass and Inertia properties of an aircraft.
 * Precomputes inertia tensor and analytical inverse for real-time 6-DoF dynamics.
 */
struct MassProperties {
    // Mass in kilograms
    double mass_kg{9298.643585}; // 20,500 lbs empty weight
    double inv_mass{1.0 / 9298.643585};

    // Moments and products of inertia [kg*m^2]
    double Ixx{12874.8472}; // Roll inertia
    double Iyy{75673.6230}; // Pitch inertia
    double Izz{85552.1125}; // Yaw inertia
    double Ixz{1331.4132};  // Cross-coupling inertia (X-Z)

    // Precomputed inertia matrix and inverse
    math::Matrix3x3 I{};
    math::Matrix3x3 I_inv{};

    constexpr MassProperties() noexcept {
        update_matrices();
    }

    constexpr MassProperties(double mass, double ixx, double iyy, double izz, double ixz) noexcept
        : mass_kg(mass), inv_mass(1.0 / mass), Ixx(ixx), Iyy(iyy), Izz(izz), Ixz(ixz) {
        update_matrices();
    }

    /**
     * @brief Creates mass properties for a specified aircraft type.
     */
    static constexpr MassProperties create(aircraft::AircraftType type) noexcept {
        const auto cfg = aircraft::AircraftConfig::get(type);
        return MassProperties(
            cfg.mass.empty_mass_kg,
            cfg.mass.Ixx,
            cfg.mass.Iyy,
            cfg.mass.Izz,
            cfg.mass.Ixz
        );
    }

    /**
     * @brief The Stevens & Lewis / NASA Langley F-16 reference test article.
     * Flight weight: 20,500 lbs (~9,298.64 kg) — the weight the aero tables and
     * this inertia tensor were published at, used by the model-validation tests:
     * Ixx = 9,496 slug-ft^2, Iyy = 55,814 slug-ft^2, Izz = 63,100 slug-ft^2, Ixz = 982 slug-ft^2.
     * The flyable F-16C uses its real empty weight via create() / FuelSystem.
     */
    static constexpr MassProperties create_clean_f16() noexcept {
        return MassProperties(9298.64, 12875.0, 75674.0, 85552.0, 1331.0);
    }

    constexpr void update_matrices() noexcept {
        inv_mass = (mass_kg > 0.0) ? (1.0 / mass_kg) : 0.0;

        // Inertia tensor in body frame:
        // [  Ixx    0   -Ixz ]
        // [   0    Iyy    0  ]
        // [ -Ixz    0    Izz ]
        I = math::Matrix3x3(
             Ixx,  0.0, -Ixz,
             0.0,  Iyy,  0.0,
            -Ixz,  0.0,  Izz
        );

        // Analytical inverse using Gamma = Ixx*Izz - Ixz^2:
        const double gamma = Ixx * Izz - Ixz * Ixz;
        if (gamma > 1e-12 && Iyy > 1e-12) {
            I_inv = math::Matrix3x3(
                Izz / gamma, 0.0,       Ixz / gamma,
                0.0,         1.0 / Iyy, 0.0,
                Ixz / gamma, 0.0,       Ixx / gamma
            );
        } else {
            I_inv = math::Matrix3x3::zero();
        }
    }
};

} // namespace fdm
} // namespace fastjet
