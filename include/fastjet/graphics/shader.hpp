#pragma once

#include "fastjet/graphics/gl_common.hpp"
#include <string>
#include <iostream>

namespace fastjet::graphics {

class ShaderProgram {
private:
    GLuint program_id_ = 0;
    bool valid_ = false;

    static GLuint compile_stage(GLenum type, const char* source) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint success = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char log[1024];
            glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
            std::cerr << "[Shader Error] Compilation failed ("
                      << (type == GL_VERTEX_SHADER ? "Vertex" : "Fragment")
                      << "):\n" << log << "\n";
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }

public:
    ShaderProgram() = default;

    ~ShaderProgram() {
        destroy();
    }

    // Move-only semantics
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

    ShaderProgram(ShaderProgram&& other) noexcept
        : program_id_(other.program_id_), valid_(other.valid_) {
        other.program_id_ = 0;
        other.valid_ = false;
    }

    ShaderProgram& operator=(ShaderProgram&& other) noexcept {
        if (this != &other) {
            destroy();
            program_id_ = other.program_id_;
            valid_ = other.valid_;
            other.program_id_ = 0;
            other.valid_ = false;
        }
        return *this;
    }

    bool init_from_source(const char* vert_src, const char* frag_src) {
        destroy();

        GLuint vert = compile_stage(GL_VERTEX_SHADER, vert_src);
        if (!vert) return false;

        GLuint frag = compile_stage(GL_FRAGMENT_SHADER, frag_src);
        if (!frag) {
            glDeleteShader(vert);
            return false;
        }

        program_id_ = glCreateProgram();
        glAttachShader(program_id_, vert);
        glAttachShader(program_id_, frag);
        glLinkProgram(program_id_);

        GLint success = 0;
        glGetProgramiv(program_id_, GL_LINK_STATUS, &success);
        if (!success) {
            char log[1024];
            glGetProgramInfoLog(program_id_, sizeof(log), nullptr, log);
            std::cerr << "[Shader Error] Program linking failed:\n" << log << "\n";
            glDeleteShader(vert);
            glDeleteShader(frag);
            glDeleteProgram(program_id_);
            program_id_ = 0;
            valid_ = false;
            return false;
        }

        glDeleteShader(vert);
        glDeleteShader(frag);
        valid_ = true;
        return true;
    }

    void destroy() noexcept {
        if (program_id_ != 0) {
            glDeleteProgram(program_id_);
            program_id_ = 0;
        }
        valid_ = false;
    }

    bool is_valid() const noexcept { return valid_; }
    GLuint id() const noexcept { return program_id_; }

    void use() const noexcept {
        if (valid_) {
            glUseProgram(program_id_);
        }
    }

    GLint get_uniform_loc(const char* name) const noexcept {
        return glGetUniformLocation(program_id_, name);
    }

    void set_mat4(const char* name, const Mat4& mat) const noexcept {
        GLint loc = get_uniform_loc(name);
        if (loc >= 0) {
            glUniformMatrix4fv(loc, 1, GL_FALSE, mat.data());
        }
    }

    void set_vec3(const char* name, float x, float y, float z) const noexcept {
        GLint loc = get_uniform_loc(name);
        if (loc >= 0) {
            glUniform3f(loc, x, y, z);
        }
    }

    void set_vec4(const char* name, float x, float y, float z, float w) const noexcept {
        GLint loc = get_uniform_loc(name);
        if (loc >= 0) {
            glUniform4f(loc, x, y, z, w);
        }
    }

    void set_color4(const char* name, const Color4& col) const noexcept {
        set_vec4(name, col.r, col.g, col.b, col.a);
    }

    void set_float(const char* name, float val) const noexcept {
        GLint loc = get_uniform_loc(name);
        if (loc >= 0) {
            glUniform1f(loc, val);
        }
    }

    void set_int(const char* name, int val) const noexcept {
        GLint loc = get_uniform_loc(name);
        if (loc >= 0) {
            glUniform1i(loc, val);
        }
    }
};

} // namespace fastjet::graphics
