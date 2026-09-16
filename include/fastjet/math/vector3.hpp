#pragma once

#include <cmath>
#include <iostream>

namespace fastjet {
namespace math {

/**
 * @brief High-performance 3D vector representing Cartesian components (x, y, z).
 * Zero heap allocation, fully constexpr and inline.
 */
struct Vector3 {
    double x{0.0};
    double y{0.0};
    double z{0.0};

    constexpr Vector3() noexcept = default;
    constexpr Vector3(double x_, double y_, double z_) noexcept : x(x_), y(y_), z(z_) {}

    [[nodiscard]] constexpr Vector3 operator+(const Vector3& rhs) const noexcept {
        return {x + rhs.x, y + rhs.y, z + rhs.z};
    }

    [[nodiscard]] constexpr Vector3 operator-(const Vector3& rhs) const noexcept {
        return {x - rhs.x, y - rhs.y, z - rhs.z};
    }

    [[nodiscard]] constexpr Vector3 operator-() const noexcept {
        return {-x, -y, -z};
    }

    [[nodiscard]] constexpr Vector3 operator*(double scalar) const noexcept {
        return {x * scalar, y * scalar, z * scalar};
    }

    [[nodiscard]] constexpr Vector3 operator/(double scalar) const noexcept {
        const double inv = 1.0 / scalar;
        return {x * inv, y * inv, z * inv};
    }

    constexpr Vector3& operator+=(const Vector3& rhs) noexcept {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    constexpr Vector3& operator-=(const Vector3& rhs) noexcept {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    constexpr Vector3& operator*=(double scalar) noexcept {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    constexpr Vector3& operator/=(double scalar) noexcept {
        const double inv = 1.0 / scalar;
        x *= inv;
        y *= inv;
        z *= inv;
        return *this;
    }

    [[nodiscard]] constexpr double dot(const Vector3& rhs) const noexcept {
        return x * rhs.x + y * rhs.y + z * rhs.z;
    }

    [[nodiscard]] constexpr Vector3 cross(const Vector3& rhs) const noexcept {
        return {
            y * rhs.z - z * rhs.y,
            z * rhs.x - x * rhs.z,
            x * rhs.y - y * rhs.x
        };
    }

    [[nodiscard]] constexpr double norm_squared() const noexcept {
        return x * x + y * y + z * z;
    }

    [[nodiscard]] double norm() const noexcept {
        return std::sqrt(norm_squared());
    }

    [[nodiscard]] Vector3 normalized() const noexcept {
        const double n = norm();
        if (n > 1e-15) {
            return *this / n;
        }
        return {0.0, 0.0, 0.0};
    }

    static constexpr Vector3 zero() noexcept {
        return {0.0, 0.0, 0.0};
    }
};

[[nodiscard]] constexpr inline Vector3 operator*(double scalar, const Vector3& v) noexcept {
    return v * scalar;
}

inline std::ostream& operator<<(std::ostream& os, const Vector3& v) {
    os << "[" << v.x << ", " << v.y << ", " << v.z << "]";
    return os;
}

} // namespace math
} // namespace fastjet
