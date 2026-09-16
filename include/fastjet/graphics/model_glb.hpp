#pragma once

#include "fastjet/graphics/gl_common.hpp"
#include "fastjet/graphics/shader.hpp"
#include "fastjet/fdm/flight_state.hpp"
#include "fastjet/graphics/sky_ground_renderer.hpp"

#include <vector>
#include <string>
#include <iostream>
#include <cmath>
#include "fastjet/graphics/cgltf.h"
#include "fastjet/graphics/stb_image.h"

namespace fastjet::graphics {

/// @brief Submesh classification within the F-16 airframe hierarchy
enum class F16PartType {
    AIRFRAME,
    CANOPY,
    COCKPIT,
    HUD,
    GEAR_UP,
    GEAR_DOWN,
    LIGHTS,
    RAILS,
    OTHER
};

/// @brief Single drawable submesh primitive with OpenGL vertex & index buffers
struct GLBSubmesh {
    std::string name;
    F16PartType part_type = F16PartType::OTHER;

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    GLsizei index_count = 0;
    GLenum index_type = GL_UNSIGNED_INT;

    // Material properties
    int texture_index = -1; // Index into textures_
    Color4 base_color_factor{1.0f, 1.0f, 1.0f, 1.0f};
    bool is_transparent = false;
};

/// @brief High-fidelity 3D glTF/GLB binary model loader & PBR/Phong renderer
class ModelGLB {
private:
    std::vector<GLBSubmesh> submeshes_;
    std::vector<GLuint> textures_;
    ShaderProgram shader_;

    bool initialized_ = false;
    bool loaded_ = false;

    // Scale from GLB model units to meters (10 units = 1 meter)
    static constexpr float MODEL_SCALE = 0.1f;

    // Center-of-gravity offset in meters along body X axis to align model CG with FDM CG
    static constexpr float CG_OFFSET_X = -3.50f;

    F16PartType classify_node(const std::string& name) {
        if (name.find("canopy") != std::string::npos) return F16PartType::CANOPY;
        if (name.find("cockpit") != std::string::npos) return F16PartType::COCKPIT;
        if (name.find("hud") != std::string::npos) return F16PartType::HUD;
        if (name.find("landingOff") != std::string::npos) return F16PartType::GEAR_UP;
        if (name.find("landingOnLight") != std::string::npos) return F16PartType::LIGHTS;
        if (name.find("landingOn") != std::string::npos) return F16PartType::GEAR_DOWN;
        if (name.find("rails") != std::string::npos) return F16PartType::RAILS;
        if (name.find("airframe") != std::string::npos) return F16PartType::AIRFRAME;
        return F16PartType::AIRFRAME;
    }

    bool init_shaders() {
        const char* vert_src = R"(
            #version 330 core
            layout (location = 0) in vec3 aPos;
            layout (location = 1) in vec3 aNormal;
            layout (location = 2) in vec2 aUV;

            uniform mat4 uModel;
            uniform mat4 uView;
            uniform mat4 uProjection;
            uniform mat3 uNormalMatrix;

            out vec3 vNormal;
            out vec2 vUV;
            out vec3 vWorldPos;

            void main() {
                vec4 worldPos = uModel * vec4(aPos, 1.0);
                vWorldPos = worldPos.xyz;
                vNormal   = normalize(uNormalMatrix * aNormal);
                vUV       = aUV;
                gl_Position = uProjection * uView * worldPos;
            }
        )";

        const char* frag_src = R"(
            #version 330 core
            in vec3 vNormal;
            in vec2 vUV;
            in vec3 vWorldPos;
            out vec4 FragColor;

            uniform sampler2D uBaseColorTexture;
            uniform int uHasTexture;
            uniform vec4 uBaseColorFactor;
            uniform vec3 uSunDirWorld;
            uniform vec3 uSkyColor;
            uniform float uAlphaMultiplier;

            void main() {
                vec4 baseColor = uBaseColorFactor;
                if (uHasTexture == 1) {
                    vec4 texColor = texture(uBaseColorTexture, vUV);
                    baseColor *= texColor;
                }

                // Normal lighting in NED world coordinates (-Z is Up)
                vec3 n = normalize(vNormal);

                // Directional sunlight
                float ndl = max(dot(n, uSunDirWorld), 0.0);
                vec3 sunColor = vec3(1.0, 0.96, 0.90) * 2.5;

                // Sky dome ambient: upward facing surfaces (-Z in NED) receive more sky bounce
                float skyBounce = clamp(-n.z * 0.5 + 0.5, 0.0, 1.0);
                vec3 ambient = uSkyColor * skyBounce * 1.6 + vec3(0.06);

                // Specular highlight (sun)
                vec3 viewDir = normalize(-vWorldPos); // approximate camera eye ray
                vec3 halfDir = normalize(uSunDirWorld + viewDir);
                float spec = pow(max(dot(n, halfDir), 0.0), 32.0) * 0.25;

                vec3 lit = baseColor.rgb * (sunColor * ndl * 0.7 + ambient) + sunColor * spec;

                // Tone mapping & gamma correction matching FastJet renderer
                lit = lit / (lit + vec3(1.0));
                lit = pow(lit, vec3(1.0 / 2.2));

                FragColor = vec4(lit, baseColor.a * uAlphaMultiplier);
            }
        )";

        return shader_.init_from_source(vert_src, frag_src);
    }

public:
    ModelGLB() = default;
    ~ModelGLB() { destroy(); }

    ModelGLB(const ModelGLB&) = delete;
    ModelGLB& operator=(const ModelGLB&) = delete;

    /// @brief Load binary glTF model from disk and initialize OpenGL resources
    bool load(const std::string& filepath) {
        cgltf_options options = {};
        cgltf_data* data = nullptr;

        const std::vector<std::string> candidates = {
            filepath,
            "../" + filepath,
            "../../" + filepath
        };
        std::string actual_path;
        cgltf_result result = cgltf_result_file_not_found;

        for (const auto& p : candidates) {
            result = cgltf_parse_file(&options, p.c_str(), &data);
            if (result == cgltf_result_success) {
                actual_path = p;
                break;
            }
        }

        if (result != cgltf_result_success) {
            std::cerr << "[ModelGLB] Failed to parse " << filepath << " (error " << result << ")\n";
            return false;
        }

        result = cgltf_load_buffers(&options, data, actual_path.c_str());
        if (result != cgltf_result_success) {
            std::cerr << "[ModelGLB] Failed to load buffers for " << actual_path << "\n";
            cgltf_free(data);
            return false;
        }

        // 1. Load embedded images into OpenGL textures
        textures_.resize(data->images_count, 0);
        for (cgltf_size i = 0; i < data->images_count; ++i) {
            const cgltf_image& img = data->images[i];
            if (!img.buffer_view) continue;

            const uint8_t* raw_data = reinterpret_cast<const uint8_t*>(img.buffer_view->buffer->data) + img.buffer_view->offset;
            int width = 0, height = 0, channels = 0;
            unsigned char* pixels = stbi_load_from_memory(raw_data, static_cast<int>(img.buffer_view->size),
                                                          &width, &height, &channels, 4);
            if (!pixels) {
                std::cerr << "[ModelGLB] Failed to decode image " << i << ": " << stbi_failure_reason() << "\n";
                continue;
            }

            GLuint tex = 0;
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            glGenerateMipmap(GL_TEXTURE_2D);

            stbi_image_free(pixels);
            textures_[i] = tex;
        }

        // 2. Iterate nodes and extract mesh primitives
        for (cgltf_size ni = 0; ni < data->nodes_count; ++ni) {
            const cgltf_node& node = data->nodes[ni];
            if (!node.mesh) continue;

            // Determine part classification from node or parent node name
            std::string part_name = node.name ? node.name : "";
            if (node.parent && node.parent->name) {
                part_name += " " + std::string(node.parent->name);
            }
            F16PartType ptype = classify_node(part_name);

            // Compute composite world transform matrix for this node
            float node_world_mat[16];
            cgltf_node_transform_world(&node, node_world_mat);

            for (cgltf_size pi = 0; pi < node.mesh->primitives_count; ++pi) {
                const cgltf_primitive& prim = node.mesh->primitives[pi];
                if (prim.type != cgltf_primitive_type_triangles) continue;

                // Find POSITION, NORMAL, and TEXCOORD_0
                const cgltf_accessor* pos_acc = nullptr;
                const cgltf_accessor* norm_acc = nullptr;
                const cgltf_accessor* uv_acc = nullptr;

                for (cgltf_size ai = 0; ai < prim.attributes_count; ++ai) {
                    const cgltf_attribute& attr = prim.attributes[ai];
                    if (attr.type == cgltf_attribute_type_position) pos_acc = attr.data;
                    else if (attr.type == cgltf_attribute_type_normal) norm_acc = attr.data;
                    else if (attr.type == cgltf_attribute_type_texcoord && attr.index == 0) uv_acc = attr.data;
                }

                if (!pos_acc) continue;
                cgltf_size num_verts = pos_acc->count;

                std::vector<float> positions(num_verts * 3);
                cgltf_accessor_unpack_floats(pos_acc, positions.data(), positions.size());

                std::vector<float> normals;
                if (norm_acc) {
                    normals.resize(num_verts * 3);
                    cgltf_accessor_unpack_floats(norm_acc, normals.data(), normals.size());
                }

                std::vector<float> uvs;
                if (uv_acc) {
                    uvs.resize(num_verts * 2);
                    cgltf_accessor_unpack_floats(uv_acc, uvs.data(), uvs.size());
                }

                // Interleave into Vertex3D-like stream: [Px, Py, Pz, Nx, Ny, Nz, U, V] (8 floats)
                std::vector<float> vertex_data;
                vertex_data.reserve(num_verts * 8);

                for (cgltf_size v = 0; v < num_verts; ++v) {
                    // Local position
                    float px = positions[v * 3 + 0];
                    float py = positions[v * 3 + 1];
                    float pz = positions[v * 3 + 2];

                    // Transform by node world matrix: pos' = M * pos
                    float tx = node_world_mat[0]*px + node_world_mat[4]*py + node_world_mat[8]*pz  + node_world_mat[12];
                    float ty = node_world_mat[1]*px + node_world_mat[5]*py + node_world_mat[9]*pz  + node_world_mat[13];
                    float tz = node_world_mat[2]*px + node_world_mat[6]*py + node_world_mat[10]*pz + node_world_mat[14];

                    // Normal transformed by rotation part of node matrix
                    float nx = 0.0f, ny = 0.0f, nz = -1.0f;
                    if (!normals.empty()) {
                        float lnx = normals[v * 3 + 0];
                        float lny = normals[v * 3 + 1];
                        float lnz = normals[v * 3 + 2];
                        nx = node_world_mat[0]*lnx + node_world_mat[4]*lny + node_world_mat[8]*lnz;
                        ny = node_world_mat[1]*lnx + node_world_mat[5]*lny + node_world_mat[9]*lnz;
                        nz = node_world_mat[2]*lnx + node_world_mat[6]*lny + node_world_mat[10]*lnz;
                        float len = std::sqrt(nx*nx + ny*ny + nz*nz);
                        if (len > 1e-6f) { nx /= len; ny /= len; nz /= len; }
                    }

                    float u = uvs.empty() ? 0.0f : uvs[v * 2 + 0];
                    float u_v = uvs.empty() ? 0.0f : uvs[v * 2 + 1];

                    vertex_data.push_back(tx);
                    vertex_data.push_back(ty);
                    vertex_data.push_back(tz);
                    vertex_data.push_back(nx);
                    vertex_data.push_back(ny);
                    vertex_data.push_back(nz);
                    vertex_data.push_back(u);
                    vertex_data.push_back(u_v);
                }

                // Unpack indices
                std::vector<uint32_t> indices;
                if (prim.indices) {
                    indices.resize(prim.indices->count);
                    cgltf_accessor_unpack_indices(prim.indices, indices.data(), sizeof(uint32_t), indices.size());
                } else {
                    indices.resize(num_verts);
                    for (uint32_t vi = 0; vi < num_verts; ++vi) indices[vi] = vi;
                }

                // Create GL VAO / VBO / EBO
                GLuint vao = 0, vbo = 0, ebo = 0;
                glGenVertexArrays(1, &vao);
                glGenBuffers(1, &vbo);
                glGenBuffers(1, &ebo);

                glBindVertexArray(vao);
                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBufferData(GL_ARRAY_BUFFER, vertex_data.size() * sizeof(float), vertex_data.data(), GL_STATIC_DRAW);

                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

                // Attributes: 0 = Position (3 floats), 1 = Normal (3 floats), 2 = UV (2 floats)
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(0));

                glEnableVertexAttribArray(1);
                glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));

                glEnableVertexAttribArray(2);
                glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(6 * sizeof(float)));

                glBindVertexArray(0);

                // Build submesh metadata
                GLBSubmesh sm;
                sm.name = part_name;
                sm.part_type = ptype;
                sm.vao = vao;
                sm.vbo = vbo;
                sm.ebo = ebo;
                sm.index_count = static_cast<GLsizei>(indices.size());
                sm.index_type = GL_UNSIGNED_INT;

                if (prim.material) {
                    if (prim.material->has_pbr_metallic_roughness) {
                        const auto& pbr = prim.material->pbr_metallic_roughness;
                        if (pbr.base_color_texture.texture && pbr.base_color_texture.texture->image) {
                            // Find index of texture
                            for (cgltf_size im = 0; im < data->images_count; ++im) {
                                if (&data->images[im] == pbr.base_color_texture.texture->image) {
                                    sm.texture_index = static_cast<int>(im);
                                    break;
                                }
                            }
                        }
                        sm.base_color_factor = Color4{pbr.base_color_factor[0],
                                                      pbr.base_color_factor[1],
                                                      pbr.base_color_factor[2],
                                                      pbr.base_color_factor[3]};
                    }
                    if (prim.material->alpha_mode == cgltf_alpha_mode_blend || sm.part_type == F16PartType::CANOPY) {
                        sm.is_transparent = true;
                    }
                }

                submeshes_.push_back(sm);
            }
        }

        cgltf_free(data);
        loaded_ = true;
        std::cout << "[ModelGLB] Loaded " << submeshes_.size() << " submeshes and "
                  << textures_.size() << " textures successfully from " << filepath << "\n";
        return true;
    }

    bool init() {
        if (initialized_) return true;
        if (!init_shaders()) return false;
        initialized_ = true;
        return true;
    }

    void destroy() noexcept {
        for (auto& sm : submeshes_) {
            if (sm.vao) glDeleteVertexArrays(1, &sm.vao);
            if (sm.vbo) glDeleteBuffers(1, &sm.vbo);
            if (sm.ebo) glDeleteBuffers(1, &sm.ebo);
            sm.vao = sm.vbo = sm.ebo = 0;
        }
        submeshes_.clear();

        for (auto tex : textures_) {
            if (tex) glDeleteTextures(1, &tex);
        }
        textures_.clear();

        shader_.destroy();
        initialized_ = false;
        loaded_ = false;
    }

    bool is_loaded() const noexcept { return loaded_; }

    /// @brief Render external 3D F-16 airframe in world space (Chase camera view)
    void render_world(const Mat4& projection,
                      const Mat4& view,
                      const fdm::FlightState& state,
                      bool gear_down) {
        if (!loaded_ || !initialized_) return;

        shader_.use();
        shader_.set_mat4("uProjection", projection);
        shader_.set_mat4("uView", view);

        // Sun direction in NED
        float sx, sy, sz;
        SkyGroundRenderer::sun_dir_ned(sx, sy, sz);
        shader_.set_vec3("uSunDirWorld", sx, sy, sz);

        // Sky ambient color
        const auto alt = static_cast<float>(-state.pos_ned.z);
        const float sky_density = std::exp(-alt / 8500.0f);
        const float amb = 0.30f * sky_density + 0.06f;
        shader_.set_vec3("uSkyColor", amb * 0.65f, amb * 0.80f, amb * 1.0f);

        // Compute Aircraft World Transform Matrix
        // M_world = T(Pos_NED) * R(q_att) * T(CG_OFFSET) * S(MODEL_SCALE)
        const math::Matrix3x3 C_nb = state.q_att.to_dcm_body_to_ned();

        Mat4 R_att = Mat4::identity();
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                R_att(r, c) = static_cast<float>(C_nb(r, c));
            }
        }

        const Mat4 T_pos = Mat4::translate(static_cast<float>(state.pos_ned.x),
                                           static_cast<float>(state.pos_ned.y),
                                           static_cast<float>(state.pos_ned.z));

        const Mat4 T_cg  = Mat4::translate(CG_OFFSET_X, 0.0f, 0.0f);
        const Mat4 S_mod = Mat4::scale(MODEL_SCALE, MODEL_SCALE, MODEL_SCALE);

        const Mat4 M_model = T_pos * R_att * T_cg * S_mod;
        shader_.set_mat4("uModel", M_model);

        // Normal matrix (upper-left 3x3 of rotation matrix).
        // nmat is filled row-major below, so it is uploaded with transpose=GL_FALSE
        // to match the column-major convention ShaderProgram::set_mat4 already uses.
        // Passing GL_TRUE here transposed an already-transposed buffer and inverted
        // the rotation, lighting the airframe from the opposite hemisphere.
        float nmat[9];
        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                nmat[c * 3 + r] = R_att(r, c);
            }
        }
        glUniformMatrix3fv(glGetUniformLocation(shader_.id(), "uNormalMatrix"), 1, GL_FALSE, nmat);

        // Pass 1: Render opaque submeshes
        for (const auto& sm : submeshes_) {
            if (sm.is_transparent) continue;
            if (sm.part_type == F16PartType::GEAR_DOWN && !gear_down) continue;
            if (sm.part_type == F16PartType::GEAR_UP && gear_down) continue;
            if (sm.part_type == F16PartType::LIGHTS && !gear_down) continue;

            draw_submesh(sm, 1.0f);
        }

        // Pass 2: Render transparent submeshes (canopy glass) with blending
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        for (const auto& sm : submeshes_) {
            if (!sm.is_transparent) continue;
            draw_submesh(sm, 0.65f);
        }

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    /// @brief Render F-16 airframe in cockpit-local body frame (First-person view framing)
    void render_cockpit(const Mat4& projection,
                        const Mat4& view_cockpit,
                        const fdm::FlightState& state) {
        if (!loaded_ || !initialized_) return;

        shader_.use();
        shader_.set_mat4("uProjection", projection);
        shader_.set_mat4("uView", view_cockpit);

        // In cockpit view, lighting is calculated in Body space
        float sx, sy, sz;
        SkyGroundRenderer::sun_dir_ned(sx, sy, sz);
        const math::Vector3 sun_b = state.q_att.rotate_ned_to_body(math::Vector3(sx, sy, sz));
        shader_.set_vec3("uSunDirWorld", static_cast<float>(sun_b.x),
                                         static_cast<float>(sun_b.y),
                                         static_cast<float>(sun_b.z));

        const auto alt = static_cast<float>(-state.pos_ned.z);
        const float sky_density = std::exp(-alt / 8500.0f);
        const float amb = 0.30f * sky_density + 0.06f;
        shader_.set_vec3("uSkyColor", amb * 0.65f, amb * 0.80f, amb * 1.0f);

        // Model matrix in body coordinates: T(CG_OFFSET) * S(MODEL_SCALE)
        const Mat4 M_body = Mat4::translate(CG_OFFSET_X, 0.0f, 0.0f) * Mat4::scale(MODEL_SCALE, MODEL_SCALE, MODEL_SCALE);
        shader_.set_mat4("uModel", M_body);

        float nmat[9] = {1,0,0, 0,1,0, 0,0,1};
        glUniformMatrix3fv(glGetUniformLocation(shader_.id(), "uNormalMatrix"), 1, GL_FALSE, nmat);

        // The AIRFRAME submesh is the whole fuselage: after CG offset and scaling it
        // spans body X in [-8.45, +6.64] m, while the pilot Design Eye Point sits at
        // X = +1.80 m. Drawing it here put the inside of the hull between the pilot
        // and the cockpit, which is what corrupted the first-person view. Only the
        // canopy rails are legitimately visible from the DEP, so only they are drawn
        // opaque; the canopy glass is drawn blended afterwards.
        for (const auto& sm : submeshes_) {
            if (sm.is_transparent) continue;
            if (sm.part_type == F16PartType::RAILS) {
                draw_submesh(sm, 1.0f);
            }
        }

        // Canopy glass over the rails: blended, and without writing depth so the
        // world beyond the canopy is not masked out.
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        for (const auto& sm : submeshes_) {
            if (!sm.is_transparent) continue;
            if (sm.part_type != F16PartType::CANOPY) continue;
            draw_submesh(sm, 0.35f);
        }

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

private:
    void draw_submesh(const GLBSubmesh& sm, float alpha_mult) {
        if (sm.texture_index >= 0 && sm.texture_index < static_cast<int>(textures_.size()) && textures_[sm.texture_index] != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textures_[sm.texture_index]);
            shader_.set_int("uBaseColorTexture", 0);
            shader_.set_int("uHasTexture", 1);
        } else {
            shader_.set_int("uHasTexture", 0);
        }

        shader_.set_vec4("uBaseColorFactor",
                         sm.base_color_factor.r,
                         sm.base_color_factor.g,
                         sm.base_color_factor.b,
                         sm.base_color_factor.a);
        shader_.set_float("uAlphaMultiplier", alpha_mult);

        glBindVertexArray(sm.vao);
        glDrawElements(GL_TRIANGLES, sm.index_count, sm.index_type, nullptr);
        glBindVertexArray(0);
    }
};

} // namespace fastjet::graphics
