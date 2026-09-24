#pragma once

#include "fastjet/graphics/gl_common.hpp"
#include "fastjet/graphics/shader.hpp"
#include <vector>

namespace fastjet::graphics {

/// @brief Gun tracers: short glowing streaks drawn eye-relative, additively
/// into the HDR target so they bloom.
///
/// Positions arrive already relative to the eye (the renderer subtracts the
/// eye position in double precision), so streaks stay sharp kilometres from
/// the world origin.
class TracerRenderer {
public:
    TracerRenderer() = default;
    ~TracerRenderer() { destroy(); }
    TracerRenderer(const TracerRenderer&) = delete;
    TracerRenderer& operator=(const TracerRenderer&) = delete;

    bool init() {
        if (vao_) return true;
        static constexpr const char* kVert = R"(
            #version 330 core
            layout(location = 0) in vec3 aPos;
            layout(location = 1) in float aHeat;
            uniform mat4 uViewProj;
            out float vHeat;
            void main() {
                vHeat = aHeat;
                gl_Position = uViewProj * vec4(aPos, 1.0);
            }
        )";
        static constexpr const char* kFrag = R"(
            #version 330 core
            in float vHeat;
            out vec4 FragColor;
            void main() {
                // Burning tracer compound: white-yellow head, orange tail. HDR values bloom.
                vec3 col = mix(vec3(3.0, 1.1, 0.25), vec3(9.0, 7.0, 3.5), vHeat);
                FragColor = vec4(col, 1.0);
            }
        )";
        if (!shader_.init_from_source(kVert, kFrag)) return false;
        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);
        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                              reinterpret_cast<void*>(3 * sizeof(float)));
        glBindVertexArray(0);
        return true;
    }

    void destroy() noexcept {
        if (vao_) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
        if (vbo_) { glDeleteBuffers(1, &vbo_); vbo_ = 0; }
        shader_.destroy();
    }

    /// @brief Replace this frame's streaks: head and tail, eye-relative [m].
    void clear() noexcept { verts_.clear(); }
    void add(float hx, float hy, float hz, float tx, float ty, float tz) {
        verts_.insert(verts_.end(), {hx, hy, hz, 1.0f, tx, ty, tz, 0.0f});
    }
    [[nodiscard]] bool empty() const noexcept { return verts_.empty(); }

    void render(const Mat4& view_proj_rot) {
        if (!vao_ || verts_.empty()) return;
        shader_.use();
        shader_.set_mat4("uViewProj", view_proj_rot);
        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(verts_.size() * sizeof(float)), verts_.data(),
                     GL_STREAM_DRAW);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glDepthMask(GL_FALSE);
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(verts_.size() / 4));
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glBindVertexArray(0);
    }

private:
    ShaderProgram shader_{};
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    std::vector<float> verts_;
};

} // namespace fastjet::graphics
