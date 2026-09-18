#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <span>

namespace fastjet {
namespace aero {

/**
 * @brief High-performance, zero-allocation multi-dimensional interpolation engine.
 * Supports 1D linear, 2D bilinear, and 3D trilinear interpolation with boundary clamping.
 */
class Interpolator {
public:
    /**
     * @brief Finds the bounding index and interpolation fraction t in [0, 1].
     * Clamps to [0, N-2] and [0.0, 1.0].
     *
     * Optimization: Uses linear scan for small grid sizes (N <= 16), which avoids
     * branch mispredictions and logarithmic loop overhead in hot aerodynamic table lookup loops.
     *
     * @tparam N Size of grid array
     * @param grid Strictly monotonically increasing array of breakpoints
     * @param x Query value
     * @param[out] idx Lower bounding index
     * @param[out] t Interpolation fraction: x = grid[idx] + t * (grid[idx+1] - grid[idx])
     */
    template <std::size_t N>
    static constexpr void find_index_and_weight(
        const std::array<double, N>& grid,
        double x,
        std::size_t& idx,
        double& t
    ) noexcept {
        static_assert(N >= 2, "Grid must have at least 2 points");

        if (x <= grid[0]) {
            idx = 0;
            t = 0.0;
            return;
        }
        if (x >= grid[N - 1]) {
            idx = N - 2;
            t = 1.0;
            return;
        }

        std::size_t low = 0;

        if constexpr (N <= 16) {
            // Linear scan for small grid sizes
            while (low + 1 < N - 1 && grid[low + 1] <= x) {
                ++low;
            }
        } else {
            // Binary search for larger grid sizes
            std::size_t high = N - 1;
            while (high - low > 1) {
                const std::size_t mid = low + (high - low) / 2;
                if (grid[mid] <= x) {
                    low = mid;
                } else {
                    high = mid;
                }
            }
        }

        idx = low;
        const double span = grid[idx + 1] - grid[idx];
        t = (span > 1e-12) ? ((x - grid[idx]) / span) : 0.0;
    }

    /**
     * @brief 1D Linear Interpolation with edge clamping.
     */
    template <std::size_t N>
    static constexpr double interp1d(
        const std::array<double, N>& grid,
        const std::array<double, N>& values,
        double x
    ) noexcept {
        std::size_t i{0};
        double t{0.0};
        find_index_and_weight(grid, x, i, t);
        return values[i] + t * (values[i + 1] - values[i]);
    }

    /**
     * @brief 2D Bilinear Interpolation over rectilinear grid with edge clamping.
     * Data layout: values[i_x * NY + i_y]
     */
    template <std::size_t NX, std::size_t NY>
    static constexpr double interp2d(
        const std::array<double, NX>& x_grid,
        const std::array<double, NY>& y_grid,
        const std::array<double, NX * NY>& values,
        double x,
        double y
    ) noexcept {
        std::size_t ix{0}, iy{0};
        double tx{0.0}, ty{0.0};

        find_index_and_weight(x_grid, x, ix, tx);
        find_index_and_weight(y_grid, y, iy, ty);

        const double v00 = values[ix * NY + iy];
        const double v10 = values[(ix + 1) * NY + iy];
        const double v01 = values[ix * NY + (iy + 1)];
        const double v11 = values[(ix + 1) * NY + (iy + 1)];

        const double c0 = v00 + tx * (v10 - v00);
        const double c1 = v01 + tx * (v11 - v01);

        return c0 + ty * (c1 - c0);
    }

    /**
     * @brief 3D Trilinear Interpolation over rectilinear grid with edge clamping.
     * Data layout: values[(i_x * NY + i_y) * NZ + i_z]
     */
    template <std::size_t NX, std::size_t NY, std::size_t NZ>
    static constexpr double interp3d(
        const std::array<double, NX>& x_grid,
        const std::array<double, NY>& y_grid,
        const std::array<double, NZ>& z_grid,
        const std::array<double, NX * NY * NZ>& values,
        double x,
        double y,
        double z
    ) noexcept {
        std::size_t ix{0}, iy{0}, iz{0};
        double tx{0.0}, ty{0.0}, tz{0.0};

        find_index_and_weight(x_grid, x, ix, tx);
        find_index_and_weight(y_grid, y, iy, ty);
        find_index_and_weight(z_grid, z, iz, tz);

        constexpr auto idx = [](std::size_t i, std::size_t j, std::size_t k) constexpr noexcept -> std::size_t {
            return (i * NY + j) * NZ + k;
        };

        const double c000 = values[idx(ix,     iy,     iz)];
        const double c100 = values[idx(ix + 1, iy,     iz)];
        const double c010 = values[idx(ix,     iy + 1, iz)];
        const double c110 = values[idx(ix + 1, iy + 1, iz)];
        const double c001 = values[idx(ix,     iy,     iz + 1)];
        const double c101 = values[idx(ix + 1, iy,     iz + 1)];
        const double c011 = values[idx(ix,     iy + 1, iz + 1)];
        const double c111 = values[idx(ix + 1, iy + 1, iz + 1)];

        // Interpolate along X
        const double c00 = c000 + tx * (c100 - c000);
        const double c10 = c010 + tx * (c110 - c010);
        const double c01 = c001 + tx * (c101 - c001);
        const double c11 = c011 + tx * (c111 - c011);

        // Interpolate along Y
        const double c0 = c00 + ty * (c10 - c00);
        const double c1 = c01 + ty * (c11 - c01);

        // Interpolate along Z
        return c0 + tz * (c1 - c0);
    }
};

} // namespace aero
} // namespace fastjet
