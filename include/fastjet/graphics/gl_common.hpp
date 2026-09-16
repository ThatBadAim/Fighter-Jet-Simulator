#pragma once

#include <epoxy/gl.h>
#include <cmath>
#include <cstring>
#include <array>
#include "fastjet/math/vector3.hpp"
#include "fastjet/math/quaternion.hpp"

namespace fastjet::graphics {

/// @brief Lightweight 4x4 column-major matrix for OpenGL rendering
struct alignas(16) Mat4 {
    std::array<float, 16> m{}; // m[col * 4 + row]

    static constexpr Mat4 identity() noexcept {
        Mat4 res{};
        res.m[0] = 1.0f; res.m[5] = 1.0f; res.m[10] = 1.0f; res.m[15] = 1.0f;
        return res;
    }

    static constexpr Mat4 zero() noexcept {
        return Mat4{};
    }

    const float* data() const noexcept { return m.data(); }
    float* data() noexcept { return m.data(); }

    float operator()(int row, int col) const noexcept { return m[col * 4 + row]; }
    float& operator()(int row, int col) noexcept { return m[col * 4 + row]; }

    Mat4 operator*(const Mat4& rhs) const noexcept {
        Mat4 out{};
        for (int col = 0; col < 4; ++col) {
            for (int row = 0; row < 4; ++row) {
                float sum = 0.0f;
                for (int k = 0; k < 4; ++k) {
                    sum += (*this)(row, k) * rhs(k, col);
                }
                out(row, col) = sum;
            }
        }
        return out;
    }

    /// @brief Creates standard perspective projection matrix
    /// @param fov_y_rad Vertical field of view in radians
    /// @param aspect Aspect ratio (width / height)
    /// @param z_near Near clipping plane
    /// @param z_far Far clipping plane
    static Mat4 perspective(float fov_y_rad, float aspect, float z_near, float z_far) noexcept {
        Mat4 res = zero();
        const float tan_half_fov = std::tan(fov_y_rad * 0.5f);
        res(0, 0) = 1.0f / (aspect * tan_half_fov);
        res(1, 1) = 1.0f / tan_half_fov;
        res(2, 2) = -(z_far + z_near) / (z_far - z_near);
        res(3, 2) = -1.0f;
        res(2, 3) = -(2.0f * z_far * z_near) / (z_far - z_near);
        return res;
    }

    /// @brief Creates standard orthographic projection matrix
    static Mat4 ortho(float left, float right, float bottom, float top, float z_near, float z_far) noexcept {
        Mat4 res = identity();
        res(0, 0) = 2.0f / (right - left);
        res(1, 1) = 2.0f / (top - bottom);
        res(2, 2) = -2.0f / (z_far - z_near);
        res(0, 3) = -(right + left) / (right - left);
        res(1, 3) = -(top + bottom) / (top - bottom);
        res(2, 3) = -(z_far + z_near) / (z_far - z_near);
        return res;
    }

    /// @brief Translation matrix
    static Mat4 translate(float tx, float ty, float tz) noexcept {
        Mat4 res = identity();
        res(0, 3) = tx;
        res(1, 3) = ty;
        res(2, 3) = tz;
        return res;
    }

    /// @brief Scaling matrix
    static Mat4 scale(float sx, float sy, float sz) noexcept {
        Mat4 res = identity();
        res(0, 0) = sx;
        res(1, 1) = sy;
        res(2, 2) = sz;
        return res;
    }

    /// @brief Rotation around X axis (radians)
    static Mat4 rotate_x(float angle_rad) noexcept {
        Mat4 res = identity();
        const float c = std::cos(angle_rad);
        const float s = std::sin(angle_rad);
        res(1, 1) = c;  res(1, 2) = -s;
        res(2, 1) = s;  res(2, 2) = c;
        return res;
    }

    /// @brief Rotation around Y axis (radians)
    static Mat4 rotate_y(float angle_rad) noexcept {
        Mat4 res = identity();
        const float c = std::cos(angle_rad);
        const float s = std::sin(angle_rad);
        res(0, 0) = c;  res(0, 2) = s;
        res(2, 0) = -s; res(2, 2) = c;
        return res;
    }

    /// @brief Rotation around Z axis (radians)
    static Mat4 rotate_z(float angle_rad) noexcept {
        Mat4 res = identity();
        const float c = std::cos(angle_rad);
        const float s = std::sin(angle_rad);
        res(0, 0) = c;  res(0, 1) = -s;
        res(1, 0) = s;  res(1, 1) = c;
        return res;
    }

    /// @brief Computes exact 4x4 matrix inverse
    Mat4 inverse() const noexcept {
        Mat4 inv{};
        const float* m = this->m.data();

        inv.m[0] = m[5]  * m[10] * m[15] - 
                   m[5]  * m[11] * m[14] - 
                   m[9]  * m[6]  * m[15] + 
                   m[9]  * m[7]  * m[14] +
                   m[13] * m[6]  * m[11] - 
                   m[13] * m[7]  * m[10];

        inv.m[4] = -m[4]  * m[10] * m[15] + 
                    m[4]  * m[11] * m[14] + 
                    m[8]  * m[6]  * m[15] - 
                    m[8]  * m[7]  * m[14] - 
                    m[12] * m[6]  * m[11] + 
                    m[12] * m[7]  * m[10];

        inv.m[8] = m[4]  * m[9] * m[15] - 
                   m[4]  * m[11] * m[13] - 
                   m[8]  * m[5] * m[15] + 
                   m[8]  * m[7] * m[13] + 
                   m[12] * m[5] * m[11] - 
                   m[12] * m[7] * m[9];

        inv.m[12] = -m[4]  * m[9] * m[14] + 
                     m[4]  * m[10] * m[13] +
                     m[8]  * m[5] * m[14] - 
                     m[8]  * m[6] * m[13] - 
                     m[12] * m[5] * m[10] + 
                     m[12] * m[6] * m[9];

        inv.m[1] = -m[1]  * m[10] * m[15] + 
                    m[1]  * m[11] * m[14] + 
                    m[9]  * m[2] * m[15] - 
                    m[9]  * m[3] * m[14] - 
                    m[13] * m[2] * m[11] + 
                    m[13] * m[3] * m[10];

        inv.m[5] = m[0]  * m[10] * m[15] - 
                   m[0]  * m[11] * m[14] - 
                   m[8]  * m[2] * m[15] + 
                   m[8]  * m[3] * m[14] + 
                   m[12] * m[2] * m[11] - 
                   m[12] * m[3] * m[10];

        inv.m[9] = -m[0]  * m[9] * m[15] + 
                    m[0]  * m[11] * m[13] + 
                    m[8]  * m[1] * m[15] - 
                    m[8]  * m[3] * m[13] - 
                    m[12] * m[1] * m[11] + 
                    m[12] * m[3] * m[9];

        inv.m[13] = m[0]  * m[9] * m[14] - 
                    m[0]  * m[10] * m[13] - 
                    m[8]  * m[1] * m[14] + 
                    m[8]  * m[2] * m[13] + 
                    m[12] * m[1] * m[10] - 
                    m[12] * m[2] * m[9];

        inv.m[2] = m[1]  * m[6] * m[15] - 
                   m[1]  * m[7] * m[14] - 
                   m[5]  * m[2] * m[15] + 
                   m[5]  * m[3] * m[14] + 
                   m[13] * m[2] * m[7] - 
                   m[13] * m[3] * m[6];

        inv.m[6] = -m[0]  * m[6] * m[15] + 
                    m[0]  * m[7] * m[14] + 
                    m[4]  * m[2] * m[15] - 
                    m[4]  * m[3] * m[14] - 
                    m[12] * m[2] * m[7] + 
                    m[12] * m[3] * m[6];

        inv.m[10] = m[0]  * m[5] * m[15] - 
                    m[0]  * m[7] * m[13] - 
                    m[4]  * m[1] * m[15] + 
                    m[4]  * m[3] * m[13] + 
                    m[12] * m[1] * m[7] - 
                    m[12] * m[3] * m[5];

        inv.m[14] = -m[0]  * m[5] * m[14] + 
                     m[0]  * m[6] * m[13] + 
                     m[4]  * m[1] * m[14] - 
                     m[4]  * m[2] * m[13] - 
                     m[12] * m[1] * m[6] + 
                     m[12] * m[2] * m[5];

        inv.m[3] = -m[1] * m[6] * m[11] + 
                    m[1] * m[7] * m[10] + 
                    m[5] * m[2] * m[11] - 
                    m[5] * m[3] * m[10] - 
                    m[9] * m[2] * m[7] + 
                    m[9] * m[3] * m[6];

        inv.m[7] = m[0] * m[6] * m[11] - 
                   m[0] * m[7] * m[10] - 
                   m[4] * m[2] * m[11] + 
                   m[4] * m[3] * m[10] + 
                   m[8] * m[2] * m[7] - 
                   m[8] * m[3] * m[6];

        inv.m[11] = -m[0] * m[5] * m[11] + 
                     m[0] * m[7] * m[9] + 
                     m[4] * m[1] * m[11] - 
                     m[4] * m[3] * m[9] - 
                     m[8] * m[1] * m[7] + 
                     m[8] * m[3] * m[5];

        inv.m[15] = m[0] * m[5] * m[10] - 
                    m[0] * m[6] * m[9] - 
                    m[4] * m[1] * m[10] + 
                    m[4] * m[2] * m[9] + 
                    m[8] * m[1] * m[6] - 
                    m[8] * m[2] * m[5];

        float det = m[0] * inv.m[0] + m[1] * inv.m[4] + m[2] * inv.m[8] + m[3] * inv.m[12];
        if (std::abs(det) < 1e-12f) {
            return identity();
        }

        det = 1.0f / det;
        for (int i = 0; i < 16; ++i) {
            inv.m[i] *= det;
        }

        return inv;
    }

    /// @brief Converts a 3x3 matrix (row-major) to a 4x4 matrix
    static Mat4 from_mat3(const fastjet::math::Matrix3x3& m3) noexcept {
        Mat4 res = identity();
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                res(r, c) = static_cast<float>(m3(r, c));
            }
        }
        return res;
    }
};

/// @brief RGBA Color
struct Color4 {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;

    static constexpr Color4 hud_green() noexcept {
        return Color4{0.0f, 1.0f, 0.25f, 1.0f}; // F-16 P-43 phosphor green
    }
    static constexpr Color4 white() noexcept {
        return Color4{1.0f, 1.0f, 1.0f, 1.0f};
    }
    static constexpr Color4 black() noexcept {
        return Color4{0.0f, 0.0f, 0.0f, 1.0f};
    }
    static constexpr Color4 amber() noexcept {
        return Color4{1.0f, 0.75f, 0.0f, 1.0f};
    }
    static constexpr Color4 red() noexcept {
        return Color4{1.0f, 0.1f, 0.1f, 1.0f};
    }
    static constexpr Color4 sky_blue() noexcept {
        return Color4{0.2f, 0.45f, 0.85f, 1.0f};
    }
    static constexpr Color4 earth_brown() noexcept {
        return Color4{0.45f, 0.28f, 0.15f, 1.0f};
    }
};

} // namespace fastjet::graphics
