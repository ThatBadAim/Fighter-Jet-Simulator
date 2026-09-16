#pragma once

#include "vector3.hpp"
#include <array>
#include <cmath>
#include <stdexcept>

namespace fastjet {
namespace math {

/**
 * @brief 3x3 Matrix for rigid-body transformations and inertia tensor calculations.
 * Fixed stack storage, zero dynamic allocation, cache-line friendly.
 */
struct Matrix3x3 {
    // Row-major order: data[row][col]
    double m[3][3]{
        {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0}
    };

    constexpr Matrix3x3() noexcept = default;

    constexpr Matrix3x3(
        double m00, double m01, double m02,
        double m10, double m11, double m12,
        double m20, double m21, double m22
    ) noexcept : m{
        {m00, m01, m02},
        {m10, m11, m12},
        {m20, m21, m22}
    } {}

    static constexpr Matrix3x3 identity() noexcept {
        return {
            1.0, 0.0, 0.0,
            0.0, 1.0, 0.0,
            0.0, 0.0, 1.0
        };
    }

    static constexpr Matrix3x3 zero() noexcept {
        return {
            0.0, 0.0, 0.0,
            0.0, 0.0, 0.0,
            0.0, 0.0, 0.0
        };
    }

    constexpr double operator()(int row, int col) const noexcept {
        return m[row][col];
    }

    constexpr double& operator()(int row, int col) noexcept {
        return m[row][col];
    }

    [[nodiscard]] constexpr Vector3 operator*(const Vector3& v) const noexcept {
        return {
            m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z,
            m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
            m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z
        };
    }

    [[nodiscard]] constexpr Matrix3x3 operator*(const Matrix3x3& o) const noexcept {
        Matrix3x3 res{};
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                res.m[r][c] = m[r][0] * o.m[0][c] +
                              m[r][1] * o.m[1][c] +
                              m[r][2] * o.m[2][c];
            }
        }
        return res;
    }

    [[nodiscard]] constexpr Matrix3x3 operator*(double s) const noexcept {
        return {
            m[0][0] * s, m[0][1] * s, m[0][2] * s,
            m[1][0] * s, m[1][1] * s, m[1][2] * s,
            m[2][0] * s, m[2][1] * s, m[2][2] * s
        };
    }

    [[nodiscard]] constexpr Matrix3x3 transpose() const noexcept {
        return {
            m[0][0], m[1][0], m[2][0],
            m[0][1], m[1][1], m[2][1],
            m[0][2], m[1][2], m[2][2]
        };
    }

    [[nodiscard]] constexpr double determinant() const noexcept {
        return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
               m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
               m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
    }

    /**
     * @brief Computes the analytical inverse using adjugate matrix.
     * Returns zero matrix if singular.
     */
    [[nodiscard]] constexpr Matrix3x3 inverse() const noexcept {
        const double det = determinant();
        const double abs_det = det < 0.0 ? -det : det;
        if (abs_det < 1e-15) {
            return zero();
        }
        const double inv_det = 1.0 / det;

        return {
            (m[1][1] * m[2][2] - m[1][2] * m[2][1]) * inv_det,
            (m[0][2] * m[2][1] - m[0][1] * m[2][2]) * inv_det,
            (m[0][1] * m[1][2] - m[0][2] * m[1][1]) * inv_det,

            (m[1][2] * m[2][0] - m[1][0] * m[2][2]) * inv_det,
            (m[0][0] * m[2][2] - m[0][2] * m[2][0]) * inv_det,
            (m[0][2] * m[1][0] - m[0][0] * m[1][2]) * inv_det,

            (m[1][0] * m[2][1] - m[1][1] * m[2][0]) * inv_det,
            (m[0][1] * m[2][0] - m[0][0] * m[2][1]) * inv_det,
            (m[0][0] * m[1][1] - m[0][1] * m[1][0]) * inv_det
        };
    }
};

} // namespace math
} // namespace fastjet
