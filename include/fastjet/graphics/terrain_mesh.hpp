#pragma once

#include "fastjet/core/thread_pool.hpp"
#include "fastjet/graphics/gl_common.hpp"
#include "fastjet/graphics/terrain_field.hpp"
#include <cmath>
#include <vector>

namespace fastjet::graphics {

/// @brief Static terrain mesh built from TerrainField as concentric LOD rings.
///
/// The aircraft spends most of its time near the airfield, so resolution is
/// spent there: an inner patch at fine spacing, then successively coarser rings
/// out to the visible horizon. Rings are built once at startup into a single
/// interleaved buffer, keeping the zero-allocation-per-frame property of the
/// rest of the simulation.
///
/// Vertex layout: pos(3), normal(3), height-above-field(1) = 7 floats.
/// Colour is derived in the fragment shader from elevation and slope, so the
/// buffer stays small and the palette can change without a rebuild.
class TerrainMesh {
private:
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLsizei vertex_count_ = 0;
    bool initialized_ = false;

    struct Ring {
        float half_extent; // [m] outer half-size of this ring
        float cell;        // [m] grid spacing
    };

    // The airfield decals are drawn as a separate coplanar layer. Rather than
    // fight them for depth (20 cm of separation is far below the precision of a
    // 120 km depth range), the terrain simply leaves a hole for the apron and
    // the decals fill it. Kept slightly inside the apron edge so there is no gap.
    static constexpr float APRON_X0 = -690.0f;
    static constexpr float APRON_X1 =  3690.0f;
    static constexpr float APRON_Y0 = -890.0f;
    static constexpr float APRON_Y1 =   890.0f;

    /// @brief True if a cell lies within the airfield footprint and must be
    /// skipped so the apron and its markings own those pixels outright.
    [[nodiscard]] static bool in_airfield(float x0, float y0, float x1, float y1) noexcept {
        return x1 > APRON_X0 && x0 < APRON_X1 &&
               y1 > APRON_Y0 && y0 < APRON_Y1;
    }

    // Ring 0 is a solid patch; the rest are hollow frames around it. Spacing
    // roughly doubles each step, so screen-space triangle size stays even.
    static constexpr Ring RINGS[] = {
        {  4500.0f,    50.0f }, // Basin & immediate airfield vicinity (high fidelity)
        { 14000.0f,   150.0f }, // Near mountain foothills & ridges
        { 36000.0f,   400.0f }, // Mid mountain ranges
        { 65000.0f,  1000.0f }, // Outer mountain chains
        { 95000.0f,  2500.0f }, // Horizon silhouettes
    };
    static constexpr int RING_COUNT = 5;

    static void push_vertex(std::vector<float>& v, float x, float y) {
        const float h = TerrainField::height(x, y);
        float nx, ny, nz;
        // Differentiate over a span tied to feature size, not cell size: this
        // keeps lighting continuous across the LOD ring seams, where cell size
        // jumps by 3x and per-cell normals would visibly crease.
        TerrainField::normal_ned(x, y, 35.0f, nx, ny, nz);

        v.push_back(x);
        v.push_back(y);
        v.push_back(-h); // NED: elevation is negative z
        v.push_back(nx);
        v.push_back(ny);
        v.push_back(nz);
        v.push_back(h);
    }

    /// @brief Emit one cell as two triangles.
    ///
    /// Winding is counter-clockwise as seen from above the terrain. NED has +X
    /// north, +Y east and +Z *down*, so it is left-handed with respect to the
    /// usual screen convention: walking the cell in +X then +Y order gives a
    /// clockwise face when viewed from above. The reversed vertex order below
    /// is what makes GL_CCW front-facing correct for a camera looking down.
    static void push_quad(std::vector<float>& v, float x0, float y0, float x1, float y1) {
        push_vertex(v, x0, y0);
        push_vertex(v, x1, y1);
        push_vertex(v, x1, y0);

        push_vertex(v, x0, y0);
        push_vertex(v, x0, y1);
        push_vertex(v, x1, y1);
    }

    void build() {
        std::vector<float> verts;
        // Sized for the high-density ring table (~70k-85k cells x 6 verts x 7 floats).
        verts.reserve(800'000);

        const float cx = TerrainField::FIELD_CENTER_X;

        float prev_extent = 0.0f;
        for (int r = 0; r < RING_COUNT; ++r) {
            const float ext  = RINGS[r].half_extent;
            const float cell = RINGS[r].cell;

            // Snap the ring bounds to the cell grid so quads align exactly.
            const float span = std::ceil(2.0f * ext / cell) * cell;
            const float lo_x = cx - span * 0.5f;
            const float lo_y = -span * 0.5f;

            const int n = static_cast<int>(span / cell);
            // Rows are sampled across the thread pool, each into its own
            // buffer, then joined in row order: the mesh is identical to a
            // serial build.
            std::vector<std::vector<float>> rows(static_cast<size_t>(n));
            core::ThreadPool::shared().parallel_for(0, n, [&](int i) {
                std::vector<float>& row = rows[static_cast<size_t>(i)];
                const float x0 = lo_x + static_cast<float>(i) * cell;
                const float x1 = x0 + cell;
                for (int j = 0; j < n; ++j) {
                    const float y0 = lo_y + static_cast<float>(j) * cell;
                    const float y1 = y0 + cell;

                    if (prev_extent > 0.0f) {
                        const bool inside_x = (x0 >= cx - prev_extent) && (x1 <= cx + prev_extent);
                        const bool inside_y = (y0 >= -prev_extent) && (y1 <= prev_extent);
                        if (inside_x && inside_y) continue;
                    }

                    if (in_airfield(x0, y0, x1, y1)) continue;

                    push_quad(row, x0, y0, x1, y1);
                }
            });
            for (const auto& row : rows) verts.insert(verts.end(), row.begin(), row.end());
            prev_extent = span * 0.5f;
        }

        vertex_count_ = static_cast<GLsizei>(verts.size() / 7);

        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);

        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(verts.size() * sizeof(float)),
                     verts.data(), GL_STATIC_DRAW);

        constexpr GLsizei stride = 7 * sizeof(float);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(0));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
    }

public:
    TerrainMesh() = default;
    ~TerrainMesh() { destroy(); }

    TerrainMesh(const TerrainMesh&) = delete;
    TerrainMesh& operator=(const TerrainMesh&) = delete;

    void init() {
        if (initialized_) return;
        build();
        initialized_ = true;
    }

    void destroy() noexcept {
        if (vao_) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
        if (vbo_) { glDeleteBuffers(1, &vbo_); vbo_ = 0; }
        vertex_count_ = 0;
        initialized_ = false;
    }

    void draw() const noexcept {
        if (!vao_) return;
        glBindVertexArray(vao_);
        glDrawArrays(GL_TRIANGLES, 0, vertex_count_);
        glBindVertexArray(0);
    }

    [[nodiscard]] GLsizei vertex_count() const noexcept { return vertex_count_; }
    [[nodiscard]] bool is_initialized() const noexcept { return initialized_; }
};

} // namespace fastjet::graphics
