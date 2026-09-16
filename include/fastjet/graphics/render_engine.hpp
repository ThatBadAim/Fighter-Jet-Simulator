#pragma once

#include "fastjet/graphics/gl_common.hpp"
#include "fastjet/graphics/shader.hpp"
#include "fastjet/graphics/camera_rig.hpp"
#include "fastjet/graphics/cockpit_geometry.hpp"
#include "fastjet/graphics/hud_collimator.hpp"
#include "fastjet/graphics/flight_instruments.hpp"
#include "fastjet/graphics/cockpit_telemetry.hpp"
#include "fastjet/graphics/sky_ground_renderer.hpp"
#include "fastjet/fdm/flight_state.hpp"
#include "fastjet/flcs/imu_sensor.hpp"
#include "fastjet/graphics/model_glb.hpp"
#include "fastjet/graphics/aircraft_menu.hpp"
#include <cmath>

namespace fastjet::graphics {

/// @brief Integrated Real-Time Rendering Engine coordinating first-person camera,
/// collimated HUD, MFD instruments, cockpit shell, and sky/ground environment.
class RenderEngine {
private:
    CameraRig camera_rig_;
    CockpitGeometry cockpit_;
    HUDCollimator hud_;
    FlightInstruments instruments_;
    SkyGroundRenderer environment_;
    ModelGLB f16_model_;
    AircraftMenu aircraft_menu_;

    ShaderProgram cockpit_shader_;
    ShaderProgram mfd_shader_;

    int viewport_width_  = 1280;
    int viewport_height_ = 720;
    float fov_deg_       = 60.0f;

    bool initialized_ = false;

    /// @brief Draws the aircraft selection overlay, if it is open.
    ///
    /// Single owner of the menu draw call: both the chase and cockpit paths
    /// route through here, so the translucent panel is never blended twice.
    void render_menu_overlay(float aspect, const AvionicsTelemetry& telemetry) {
        if (aircraft_menu_.is_open()) {
            aircraft_menu_.render(aspect, telemetry.aircraft_type);
        }
    }

    bool init_cockpit_shaders() {
        // Cockpit shell shader with directional lighting
        const char* shell_vert = R"(
            #version 330 core
            layout (location = 0) in vec3 aPos;
            layout (location = 1) in vec3 aNormal;
            layout (location = 2) in vec2 aUV;
            layout (location = 3) in vec4 aColor;

            uniform mat4 uProjection;
            uniform mat4 uView;

            out vec3 vNormal;
            out vec4 vColor;
            out vec3 vPosB;

            void main() {
                vNormal = aNormal;
                vColor  = aColor;
                vPosB   = aPos;               // body frame, for the canopy term
                gl_Position = uProjection * uView * vec4(aPos, 1.0);
            }
        )";

        const char* shell_frag = R"(
            #version 330 core
            in vec3 vNormal;
            in vec4 vColor;
            in vec3 vPosB;
            out vec4 FragColor;

            uniform vec3 uSunDirBody;   // sun direction in BODY frame
            uniform vec3 uSkyColor;     // ambient from the sky, linear

            void main() {
                vec3 n = normalize(vNormal);

                // Sunlight entering through the canopy. The bubble is open
                // above and forward, so surfaces facing up (-Z in body) and
                // forward see far more of it than ones tucked under the coaming.
                float ndl = max(dot(n, uSunDirBody), 0.0);

                // Sky dome ambient, weighted by how much sky each surface can
                // see: upward faces are lit, deep footwell faces stay dark.
                float openness = clamp(-n.z * 0.5 + 0.5, 0.0, 1.0);

                // Depth shading: the further aft and lower in the tub, the less
                // light reaches it. Cheap stand-in for cockpit occlusion.
                float depth = clamp((vPosB.x - 0.9) / 1.5, 0.0, 1.0);
                float occl = mix(0.35, 1.0, depth);

                vec3 sun_col = vec3(1.0, 0.96, 0.90) * 2.4;
                vec3 lit = vColor.rgb * (sun_col * ndl * 0.55
                                       + uSkyColor * openness * 1.8
                                       + vec3(0.02)) * occl;

                // Match the world tonemap and gamma so the cockpit and the view
                // through the canopy share one response curve.
                lit = lit / (lit + vec3(1.0));
                lit = pow(lit, vec3(1.0 / 2.2));
                FragColor = vec4(lit, vColor.a);
            }
        )";

        if (!cockpit_shader_.init_from_source(shell_vert, shell_frag)) {
            return false;
        }

        // MFD Texture Display Shader
        const char* mfd_vert = R"(
            #version 330 core
            layout (location = 0) in vec3 aPos;
            layout (location = 1) in vec3 aNormal;
            layout (location = 2) in vec2 aUV;
            layout (location = 3) in vec4 aColor;

            uniform mat4 uProjection;
            uniform mat4 uView;

            out vec2 vUV;

            void main() {
                vUV = aUV;
                gl_Position = uProjection * uView * vec4(aPos, 1.0);
            }
        )";

        const char* mfd_frag = R"(
            #version 330 core
            in vec2 vUV;
            out vec4 FragColor;

            uniform sampler2D uTexture;

            void main() {
                // Emissive instrument display: the panel generates its own
                // light, so it bypasses the scene lighting but still shares the
                // gamma curve everything else is written with.
                vec4 tex_col = texture(uTexture, vUV);
                FragColor = vec4(tex_col.rgb, tex_col.a);
            }
        )";

        if (!mfd_shader_.init_from_source(mfd_vert, mfd_frag)) {
            return false;
        }

        return true;
    }

public:
    RenderEngine() = default;

    ~RenderEngine() {
        destroy();
    }

    bool init(int width = 1280, int height = 720, float fov_deg = 60.0f) {
        if (initialized_) return true;

        viewport_width_  = width;
        viewport_height_ = height;
        fov_deg_         = fov_deg;

        if (!init_cockpit_shaders()) return false;
        cockpit_.init();
        if (!hud_.init()) return false;
        if (!instruments_.init()) return false;
        if (!environment_.init()) return false;
        if (!aircraft_menu_.init()) return false;

        f16_model_.init();
        f16_model_.load("assets/models/f16.glb");

        glEnable(GL_MULTISAMPLE);

        initialized_ = true;
        return true;
    }

    void set_viewport(int width, int height) noexcept {
        viewport_width_  = width;
        viewport_height_ = height;
    }

    void set_fov(float fov_deg) noexcept {
        fov_deg_ = fov_deg;
    }

    void destroy() noexcept {
        cockpit_.destroy();
        hud_.destroy();
        instruments_.destroy();
        environment_.destroy();
        f16_model_.destroy();
        aircraft_menu_.destroy();
        cockpit_shader_.destroy();
        mfd_shader_.destroy();
        initialized_ = false;
    }

    /// @brief Main multi-pass render function
    /// @param dt Simulation delta time [s]
    /// @param state 6-DoF aircraft flight state
    /// @param imu Instantaneous IMU acceleration and rate sensor measurements
    /// @param is_crashed Ground collision / crash status
    /// @param telemetry Live cockpit telemetry for instruments and HUD
    void render_frame(double dt,
                      const fdm::FlightState& state,
                      const flcs::IMUData& imu,
                      bool is_crashed = false,
                      const AvionicsTelemetry& telemetry = AvionicsTelemetry{}) {
        if (!initialized_) return;

        // -------------------------------------------------------------------------
        // 0. Update dynamic systems
        // -------------------------------------------------------------------------
        camera_rig_.update(dt, imu);
        instruments_.update_and_render(state, telemetry);

        // Compute Camera Projection Matrices (Dual-Projection Pipeline)
        const float aspect = static_cast<float>(viewport_width_) / static_cast<float>(viewport_height_);
        const float fov_rad = fov_deg_ * (3.14159265f / 180.0f);
        // External World & HUD infinity projection: z_near=0.5m eliminates runway z-fighting
        const Mat4 proj_world   = Mat4::perspective(fov_rad, aspect, 0.5f, 50000.0f);
        // Cockpit interior projection: z_near=0.05m retains near-field geometry (DEP at X=1.8m, panels at X=2.15-2.45m)
        const Mat4 proj_cockpit = Mat4::perspective(fov_rad, aspect, 0.05f, 20.0f);

        // View matrices
        const Mat4 view_world   = camera_rig_.compute_world_view_matrix(state);
        const Mat4 view_cockpit = camera_rig_.compute_cockpit_view_matrix();
        const Mat4 view_inf     = camera_rig_.compute_infinity_view_matrix(state);

        // Eye altitude, needed by both the environment and cockpit passes.
        const auto eye_alt_cockpit = static_cast<float>(-camera_rig_.eye_pos_ned(state).z);

        // Clear framebuffers (ensure stencil mask is writable for glClear)
        glViewport(0, 0, viewport_width_, viewport_height_);
        glClearColor(0.05f, 0.05f, 0.06f, 1.0f);
        glStencilMask(0xFF);
        glClearStencil(0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // -------------------------------------------------------------------------
        // PASS 1: Sky & Ground Environment
        // -------------------------------------------------------------------------
        {
            // Eye position drives both the scattering density and the range
            // based aerial perspective, so the sky and terrain agree.
            const math::Vector3 eye_ned = camera_rig_.eye_pos_ned(state);
            const auto eye_alt = static_cast<float>(-eye_ned.z);

            // Sky gradient rendered at infinity
            const Mat4 view_proj_inf = proj_world * view_inf;
            const Mat4 inv_view_proj_inf = view_proj_inf.inverse();
            environment_.render_sky(inv_view_proj_inf, eye_alt);

            // Terrain, runway and markings rendered with depth test
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
            glDepthMask(GL_TRUE);

            // Terrain is a closed heightfield: back-face culling halves the
            // fragment load with no visible change.
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            glFrontFace(GL_CCW);

            const Mat4 view_proj_world = proj_world * view_world;
            environment_.render_ground(view_proj_world, eye_ned);

            glDisable(GL_CULL_FACE);
        }

        // -------------------------------------------------------------------------
        // CHASE CAMERA VIEW: Render full external 3D F-16 Fighting Falcon model
        // -------------------------------------------------------------------------
        if (camera_rig_.mode() == CameraMode::CHASE) {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
            glDepthMask(GL_TRUE);

            f16_model_.render_world(proj_world, view_world, state, telemetry.gear_deployed);

            // Subtle avionics overlay (airspeed, altitude, G-meter)
            const Mat4 view_hud = Mat4::identity();
            hud_.build_symbology(state, imu, is_crashed, telemetry);
            hud_.render_unmasked(proj_world, view_hud);

            // The menu overlay is drawn by PASS 5, which is shared with the
            // cockpit path. Drawing it here as well would blend the
            // translucent glass backdrop over itself.
            render_menu_overlay(aspect, telemetry);
            return;
        }

        // -------------------------------------------------------------------------
        // PASS 2: Cockpit Shell & MFD Panel
        // -------------------------------------------------------------------------
        {
            // Clear depth buffer so cockpit interior geometry always renders crisply
            // in front of distant world terrain with dedicated near-range precision
            glClear(GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
            glDepthMask(GL_TRUE);

            // Rotate the world sun into the body frame. Doing the lighting in
            // body coordinates is what makes the cockpit interior sweep through
            // light and shadow as the aircraft rolls, instead of being lit by a
            // lamp bolted to the airframe.
            float sx, sy, sz;
            SkyGroundRenderer::sun_dir_ned(sx, sy, sz);
            const math::Vector3 sun_b = state.q_att.rotate_ned_to_body(
                math::Vector3(sx, sy, sz));

            // Ambient tint from the sky, dimmed with altitude the same way the
            // sky itself is, so the cockpit cools off as the jet climbs.
            const float sky_density = std::exp(-eye_alt_cockpit / 8500.0f);
            const float amb = 0.30f * sky_density + 0.06f;

            // Cockpit shell
            cockpit_shader_.use();
            cockpit_shader_.set_mat4("uProjection", proj_cockpit);
            cockpit_shader_.set_mat4("uView", view_cockpit);
            cockpit_shader_.set_vec3("uSunDirBody",
                                     static_cast<float>(sun_b.x),
                                     static_cast<float>(sun_b.y),
                                     static_cast<float>(sun_b.z));
            cockpit_shader_.set_vec3("uSkyColor", amb * 0.62f, amb * 0.78f, amb * 1.0f);
            cockpit_.draw_shell();

            // Render high-detail F-16 airframe (nose cone, canopy rails, wings)
            f16_model_.render_cockpit(proj_cockpit, view_cockpit, state);

            // MFD textured display panel
            mfd_shader_.use();
            mfd_shader_.set_mat4("uProjection", proj_cockpit);
            mfd_shader_.set_mat4("uView", view_cockpit);
            mfd_shader_.set_int("uTexture", 0);
            instruments_.bind_texture(GL_TEXTURE0);
            cockpit_.draw_mfd();
        }

        // -------------------------------------------------------------------------
        // PASS 3: Combiner Glass Stencil Aperture Mask
        // -------------------------------------------------------------------------
        {
            glEnable(GL_STENCIL_TEST);
            glStencilMask(0xFF);
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
            // Only write stencil where combiner glass passes depth test against cockpit geometry
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LEQUAL);
            glDepthMask(GL_FALSE); // Don't modify depth buffer
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

            cockpit_shader_.use();
            cockpit_shader_.set_mat4("uProjection", proj_cockpit);
            cockpit_shader_.set_mat4("uView", view_cockpit);
            cockpit_.draw_combiner_glass();

            // Restore color writes
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

            // Render subtle glass tint with alpha blend
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            cockpit_.draw_combiner_glass();
            glDisable(GL_BLEND);
        }

        // -------------------------------------------------------------------------
        // PASS 4: Optical-Infinity Collimated HUD Symbology
        // Stencil masked to physical combiner glass polygon
        // -------------------------------------------------------------------------
        {
            // Only draw where stencil == 1 (inside combiner glass aperture)
            glStencilMask(0x00);
            glStencilFunc(GL_EQUAL, 1, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

            // HUD is collimated at optical infinity: disable depth test, additive blend
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);

            // Render complete collimated HUD symbology masked to physical combiner glass aperture
            const Mat4 view_hud = camera_rig_.head_look_matrix();
            hud_.build_symbology(state, imu, is_crashed, telemetry);
            hud_.render_masked(proj_world, view_hud);

            // Reset OpenGL states and restore stencil mask for subsequent clear
            glDisable(GL_STENCIL_TEST);
            glDisable(GL_BLEND);
            glStencilMask(0xFF);
            glEnable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
        }

        // -------------------------------------------------------------------------
        // PASS 5: Interactive Aircraft Selection Menu Overlay
        // -------------------------------------------------------------------------
        render_menu_overlay(aspect, telemetry);
    }

    CameraRig& camera_rig() noexcept { return camera_rig_; }
    const CameraRig& camera_rig() const noexcept { return camera_rig_; }

    HUDCollimator& hud() noexcept { return hud_; }
    const HUDCollimator& hud() const noexcept { return hud_; }

    FlightInstruments& instruments() noexcept { return instruments_; }
    const FlightInstruments& instruments() const noexcept { return instruments_; }

    ModelGLB& model() noexcept { return f16_model_; }
    const ModelGLB& model() const noexcept { return f16_model_; }

    AircraftMenu& menu() noexcept { return aircraft_menu_; }
    const AircraftMenu& menu() const noexcept { return aircraft_menu_; }

    bool is_initialized() const noexcept { return initialized_; }
};

} // namespace fastjet::graphics
