#pragma once

#include "fastjet/graphics/gl_common.hpp"
#include "fastjet/graphics/shader.hpp"
#include "fastjet/graphics/shader_library.hpp"
#include "fastjet/graphics/camera_rig.hpp"
#include "fastjet/graphics/cockpit_geometry.hpp"
#include "fastjet/graphics/hud_collimator.hpp"
#include "fastjet/graphics/flight_instruments.hpp"
#include "fastjet/graphics/gpu_profiler.hpp"
#include "fastjet/graphics/cockpit_telemetry.hpp"
#include "fastjet/graphics/sky_ground_renderer.hpp"
#include "fastjet/graphics/cloud_layer.hpp"
#include "fastjet/graphics/exhaust_plume.hpp"
#include "fastjet/graphics/frame_context.hpp"
#include "fastjet/graphics/hdr_pipeline.hpp"
#include "fastjet/graphics/render_quality.hpp"
#include "fastjet/graphics/scene_lighting.hpp"
#include "fastjet/graphics/shadow_map.hpp"
#include "fastjet/fdm/flight_state.hpp"
#include "fastjet/flcs/imu_sensor.hpp"
#include "fastjet/graphics/model_glb.hpp"
#include "fastjet/graphics/aircraft_menu.hpp"
#include "fastjet/graphics/ui_canvas.hpp"
#include "fastjet/ui/theme.hpp"
#include <algorithm>
#include <cmath>
#include <string>

namespace fastjet::graphics {

/// @brief Integrated real-time rendering engine: first-person and chase
/// cameras, HDR world, collimated HUD, MFD instruments, cockpit and menus.
///
/// Frame structure:
///   1. Shadow pass: the airframe (and in the cockpit, the cockpit shell)
///      into the sun shadow map.
///   2. World pass into the HDR target: sky with cirrus, terrain and
///      airfield, airframe, exhaust, cumulus deck. Linear radiance, MSAA.
///   3. Post: resolve, bloom, exposure, ACES, vignette, G-LOC greyout, into
///      the window framebuffer.
///   4. Cockpit (display-encoded, same curve): shell, airframe and canopy
///      around the pilot, MFDs, combiner stencil and HUD.
///   5. Vision overlay, colour-vision filter and menus.
class RenderEngine {
private:
    CameraRig camera_rig_;
    CockpitGeometry cockpit_;
    HUDCollimator hud_;
    FlightInstruments instruments_;
    CockpitGauges gauges_;
    SkyGroundRenderer environment_;
    ModelGLB f16_model_;
    AircraftMenu aircraft_menu_;
    UiCanvas ui_canvas_;
    ColorFilterPass color_filter_;
    ui::ColorblindMode color_filter_mode_ = ui::ColorblindMode::OFF;

    HdrPipeline hdr_;
    ShadowMap shadow_map_;
    CloudLayer clouds_;
    ExhaustPlume exhaust_;
    RenderQuality quality_{};
    WeatherState weather_{};
    double time_s_ = 0.0;

    GpuProfiler gpu_profiler_;

    ShaderProgram cockpit_shader_;
    ShaderProgram mfd_shader_;
    ShaderProgram caster_shader_;

    int viewport_width_  = 1280;
    int viewport_height_ = 720;
    float fov_deg_       = 60.0f;

    bool initialized_ = false;
    bool model_cockpit_ = false; ///< Cockpit view uses the airframe asset's cockpit

    /// Scene exposure (see glsl::OUTPUT). One value for world and cockpit.
    static constexpr float kExposure = 1.35f;
    static constexpr float kBloomStrength = 0.045f;
    static constexpr float kVignette = 0.18f;
    /// World projection range [m]. The near plane keeps runway decals out of
    /// z-fighting; the far plane reaches past the terrain mesh edge.
    static constexpr float kWorldNear = 0.5f;
    static constexpr float kWorldFar = 60000.0f;
    static constexpr const char* kModelPath = "assets/models/f16.glb";
    static constexpr float kCockpitNear = 0.05f;
    static constexpr float kCockpitFar = 20.0f;
    /// CG height above the wheels' contact plane on the ground [m].
    static constexpr double kCgHeightOnGear = 1.56;
    /// Throttle above which the afterburner lights (matches the engine model).
    static constexpr double kAfterburnerThrottle = 0.85;

    /// @brief Draws the aircraft selection overlay, if it is open.
    ///
    /// Single owner of the menu draw call: both the chase and cockpit paths
    /// route through here, so the translucent panel is never blended twice.
    void render_menu_overlay(float aspect, const AvionicsTelemetry& telemetry,
                             const input::ControllerProfile* ctrl_profile = nullptr,
                             const input::InputManager* input_mgr = nullptr) {
        // Correct the scene and HUD for colour-vision deficiency before any
        // menu is composited: menus carry their own colour-blind-safe palette.
        color_filter_.apply(static_cast<int>(color_filter_mode_), viewport_width_, viewport_height_);
        if (aircraft_menu_.is_open()) {
            aircraft_menu_.render(aspect, telemetry.aircraft_type, ctrl_profile, input_mgr);
        }
    }

    bool init_cockpit_shaders() {
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
                vPosB   = aPos;               // body frame
                gl_Position = uProjection * uView * vec4(aPos, 1.0);
            }
        )";

        // Cockpit interior in body axes. The sun arrives through the canopy
        // and is shadowed by the real airframe (canopy sill, hull, spine) and
        // by the cockpit itself, so light and shadow sweep across the panels
        // as the jet rolls.
        const std::string shell_frag = std::string(R"(
            #version 330 core
            in vec3 vNormal;
            in vec4 vColor;
            in vec3 vPosB;
            out vec4 FragColor;

            uniform vec3 uEyeBody;
        )") + glsl::COMMON + glsl::OUTPUT + glsl::ATMOSPHERE + glsl::LIGHTING + glsl::SHADOW + R"(
            // Satin anti-glare paint.
            const float PAINT_ROUGHNESS = 0.62;
            // Interior ambient relative to open sky: the tub sees the sky only
            // through the canopy above it.
            const float CANOPY_AMBIENT = 1.25;

            void main() {
                vec3 n = normalize(vNormal);
                vec3 v = normalize(uEyeBody - vPosB);
                if (dot(n, v) < 0.0) n = -n;

                float vis = sun_shadow(vPosB, n, gl_FragCoord.xy);

                // Sky seen through the canopy: faces turned up (-Z body) see
                // most of it, faces tucked under the coaming very little.
                float openness = clamp(-n.z * 0.5 + 0.5, 0.0, 1.0);
                // Deeper in the tub (aft and low) less light reaches.
                float depth = clamp((vPosB.x - 0.9) / 1.5, 0.0, 1.0);
                float occl = mix(0.35, 1.0, depth);

                vec3 albedo = vColor.rgb;
                vec3 lit = sun_brdf(albedo, 0.0, PAINT_ROUGHNESS, n, v, uSunDir) * vis
                         + albedo * hemi_ambient(n) * openness * occl * CANOPY_AMBIENT
                         + albedo * 0.02;
                FragColor = scene_output(lit, vColor.a);
            }
        )";

        if (!cockpit_shader_.init_from_source(shell_vert, shell_frag.c_str())) {
            return false;
        }
        cockpit_shader_.use();
        cockpit_shader_.set_int("uShadowMap", FrameContext::kShadowUnit);

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
            out vec3 vTint;

            void main() {
                vUV = aUV;
                vTint = aColor.rgb;
                gl_Position = uProjection * uView * vec4(aPos, 1.0);
            }
        )";

        const char* mfd_frag = R"(
            #version 330 core
            in vec2 vUV;
            in vec3 vTint;
            out vec4 FragColor;

            uniform sampler2D uTexture;

            void main() {
                // Emissive instrument display: the panel generates its own
                // light, so it bypasses the scene lighting. The vertex tint
                // dims faces that are lit by the cockpit rather than self-lit.
                vec4 c = texture(uTexture, vUV);
                // Transparent texels (round dials' corners) leave the
                // cockpit's own bezel showing.
                if (c.a < 0.5) discard;
                FragColor = vec4(c.rgb * vTint, 1.0);
            }
        )";

        if (!mfd_shader_.init_from_source(mfd_vert, mfd_frag)) {
            return false;
        }

        // Depth-only caster for the cockpit shell (position at location 0).
        const char* caster_vert = R"(
            #version 330 core
            layout (location = 0) in vec3 aPos;
            uniform mat4 uLightMVP;
            void main() { gl_Position = uLightMVP * vec4(aPos, 1.0); }
        )";
        const char* caster_frag = R"(
            #version 330 core
            void main() {}
        )";
        if (!caster_shader_.init_from_source(caster_vert, caster_frag)) {
            return false;
        }
        glUseProgram(0);
        return true;
    }

    /// @brief Lighting, clouds and shadow for this frame.
    [[nodiscard]] FrameContext build_frame_context(const fdm::FlightState& state,
                                                   const math::Vector3& eye_ned) const {
        FrameContext ctx;
        const auto eye_alt = static_cast<float>(-eye_ned.z);
        ctx.lighting = SceneLighting::compute(SkyGroundRenderer::sun_dir_ned(), eye_alt, kExposure);
        if (quality_.clouds) {
            ctx.clouds.coverage = weather_.cumulus_coverage;
            ctx.clouds.base_m = weather_.cumulus_base_m;
            ctx.clouds.top_m = weather_.cumulus_top_m;
            // The field moves downwind: sampling at (xy - wind * t).
            ctx.clouds.offset_x = static_cast<float>(-weather_.wind_north_mps * time_s_);
            ctx.clouds.offset_y = static_cast<float>(-weather_.wind_east_mps * time_s_);
        }
        ctx.shadow = shadow_map_.enabled() ? &shadow_map_ : nullptr;
        ctx.eye_ned = eye_ned;
        ctx.time_s = static_cast<float>(time_s_);
        const double ground = TerrainField::height(static_cast<float>(state.pos_ned.x),
                                                   static_cast<float>(state.pos_ned.y));
        ctx.ground_shadow = ShadowMap::ground_strength(-state.pos_ned.z - ground - kCgHeightOnGear);
        return ctx;
    }

    /// @brief Pass 1: sun shadow casters.
    void render_shadow_pass(const fdm::FlightState& state, bool gear_down, bool chase) {
        if (!shadow_map_.enabled()) return;
        shadow_map_.fit(SkyGroundRenderer::sun_dir_ned(), state.pos_ned);
        shadow_map_.begin();
        const ModelAlignment align = chase ? ModelAlignment::WORLD : ModelAlignment::COCKPIT;
        f16_model_.render_shadow(shadow_map_, state, gear_down, align);
        if (!chase && !model_cockpit_) {
            // The cockpit shades itself too: glare shield onto the panel,
            // consoles onto the floor.
            caster_shader_.use();
            caster_shader_.set_mat4("uLightMVP",
                                    shadow_map_.caster_matrix(state.pos_ned) * ModelGLB::attitude_matrix(state));
            cockpit_.draw_shell();
        }
        shadow_map_.end();
        glViewport(0, 0, viewport_width_, viewport_height_);
    }

    /// @brief Afterburner fraction and nozzle glow from the telemetry.
    static void exhaust_levels(const AvionicsTelemetry& t, float& afterburner, float& glow) noexcept {
        const double thr = std::clamp(t.throttle_input, 0.0, 1.0);
        afterburner = static_cast<float>(std::clamp((thr - kAfterburnerThrottle) / (1.0 - kAfterburnerThrottle), 0.0, 1.0));
        // FTIT spans ~420 C at idle to ~980 C in reheat in the telemetry model.
        glow = static_cast<float>(std::clamp((t.engine_ftit_deg_c - 420.0) / 560.0, 0.0, 1.0));
        if (std::string(t.detent_str) == "CUTOFF") afterburner = glow = 0.0f;
    }

    /// @brief Eye-relative model matrix for the exhaust (nozzle frame).
    static Mat4 exhaust_matrix(const fdm::FlightState& state, const math::Vector3& eye) {
        const math::Vector3 nozzle = ModelGLB::nozzle_exit_body();
        // Nozzle frame +X runs aft: a half turn about body Z.
        Mat4 aft = Mat4::identity();
        aft(0, 0) = -1.0f;
        aft(1, 1) = -1.0f;
        return ModelGLB::relative_matrix_body(state, eye) *
               Mat4::translate(static_cast<float>(nozzle.x), static_cast<float>(nozzle.y), static_cast<float>(nozzle.z)) *
               aft * Mat4::scale(ExhaustPlume::kPlumeLength, ExhaustPlume::kNozzleRadius, ExhaustPlume::kNozzleRadius);
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

        // The airframe's file read and texture decoding run on the worker
        // threads while the GL resources below are built on this one.
        f16_model_.prefetch(kModelPath);

        if (!init_cockpit_shaders()) return false;
        cockpit_.init();
        if (!hud_.init()) return false;
        if (!gauges_.init()) return false;
        if (!environment_.init()) return false;
        if (!aircraft_menu_.init()) return false;
        if (!ui_canvas_.init()) return false;
        if (!color_filter_.init()) return false;
        if (!hdr_.init()) return false;
        if (!clouds_.init()) return false;
        if (!exhaust_.init()) return false;
        if (!shadow_map_.init(quality_.shadow_map_size)) {
            std::cerr << "[RenderEngine] Shadow map unavailable; continuing without sun shadows\n";
        }

        if (!f16_model_.init()) return false;
        f16_model_.load(kModelPath);
        // Fly from the asset's own cockpit when it has one; the live displays
        // move onto its MFD screens. The built-in shell remains the fallback.
        model_cockpit_ = f16_model_.has_cockpit();
        // The asset cockpit has no centre display, so its atlas is the MFD row only.
        if (!instruments_.init(/*centre_page=*/!model_cockpit_)) return false;
        if (model_cockpit_) {
            cockpit_.use_screens(ModelGLB::cockpit_screens(instruments_.mfd_row_v0()));
            // Working faces over the asset's painted dials and lamps.
            cockpit_.use_gauges(ModelGLB::cockpit_gauges());
        }

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

    /// @brief Applies a renderer quality level. Takes effect on the next frame.
    void set_quality(const RenderQuality& q) {
        const bool shadow_changed = q.shadow_map_size != quality_.shadow_map_size;
        quality_ = q;
        if (initialized_ && shadow_changed) shadow_map_.init(quality_.shadow_map_size);
    }

    [[nodiscard]] const RenderQuality& quality() const noexcept { return quality_; }

    void set_weather(const WeatherState& w) noexcept { weather_ = w; }
    [[nodiscard]] const WeatherState& weather() const noexcept { return weather_; }

    void destroy() noexcept {
        cockpit_.destroy();
        hud_.destroy();
        instruments_.destroy();
        gauges_.destroy();
        environment_.destroy();
        f16_model_.destroy();
        aircraft_menu_.destroy();
        ui_canvas_.destroy();
        color_filter_.destroy();
        hdr_.destroy();
        shadow_map_.destroy();
        clouds_.destroy();
        exhaust_.destroy();
        gpu_profiler_.destroy();
        cockpit_shader_.destroy();
        mfd_shader_.destroy();
        caster_shader_.destroy();
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
                      const AvionicsTelemetry& telemetry = AvionicsTelemetry{},
                      const input::ControllerProfile* ctrl_profile = nullptr,
                      const input::InputManager* input_mgr = nullptr) {
        if (!initialized_) return;

        // -------------------------------------------------------------------
        // 0. Update dynamic systems and cameras
        // -------------------------------------------------------------------
        gpu_profiler_.begin_frame();
        gpu_profiler_.mark("instruments");
        camera_rig_.update(dt, imu);
        // The displays are only seen from the cockpit: the chase view skips
        // redrawing them. Gauge pointers are re-primed rather than lagged
        // across a return from the chase view.
        if (camera_rig_.mode() != CameraMode::CHASE) {
            instruments_.update_and_render(state, telemetry);
            gpu_profiler_.mark("gauges");
            if (cockpit_.has_gauges()) gauges_.update_and_render(dt, state, telemetry);
        } else {
            gauges_.reset();
        }
        time_s_ += dt;

        const float aspect = static_cast<float>(viewport_width_) / static_cast<float>(viewport_height_);
        const float fov_rad = fov_deg_ * (3.14159265f / 180.0f);
        const Mat4 proj_world   = Mat4::perspective(fov_rad, aspect, kWorldNear, kWorldFar);
        // Cockpit interior projection keeps near-field geometry (DEP at
        // X=1.8 m, panels at X=2.15-2.45 m) with its own depth precision.
        const Mat4 proj_cockpit = Mat4::perspective(fov_rad, aspect, kCockpitNear, kCockpitFar);

        const Mat4 view_world   = camera_rig_.compute_world_view_matrix(state);
        const Mat4 view_cockpit = camera_rig_.compute_cockpit_view_matrix();
        const Mat4 view_inf     = camera_rig_.compute_infinity_view_matrix(state);

        // Rotation-only view for eye-relative geometry (airframe, exhaust).
        Mat4 view_rot = view_world;
        view_rot(0, 3) = view_rot(1, 3) = view_rot(2, 3) = 0.0f;

        const bool chase = camera_rig_.mode() == CameraMode::CHASE;
        const math::Vector3 eye_ned = camera_rig_.eye_pos_ned(state);
        const auto eye_alt = static_cast<float>(-eye_ned.z);
        FrameContext ctx = build_frame_context(state, eye_ned);
        if (quality_.clouds) clouds_.update_map(ctx.clouds, eye_ned);

        // -------------------------------------------------------------------
        // PASS 1: Sun shadow map
        // -------------------------------------------------------------------
        gpu_profiler_.mark("shadow");
        render_shadow_pass(state, telemetry.gear_deployed, chase);
        gpu_profiler_.mark("sky");

        // -------------------------------------------------------------------
        // PASS 2: World into the HDR target
        // -------------------------------------------------------------------
        const bool hdr = hdr_.ensure(viewport_width_, viewport_height_, quality_.scene_samples);
        ctx.hdr_output = hdr;
        if (hdr) {
            hdr_.begin_scene();
        } else {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, viewport_width_, viewport_height_);
        }
        glDisable(GL_STENCIL_TEST);
        glDisable(GL_BLEND);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_TRUE);
        glStencilMask(0xFF);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClearStencil(0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
            if (!chase && model_cockpit_) {
                // Most of the lower screen is cockpit: keep the world from
                // shading pixels the cockpit pass will paint over.
                f16_model_.render_cockpit_occluder(proj_cockpit, view_cockpit);
            }
            const Mat4 view_proj_inf = proj_world * view_inf;
            environment_.render_sky(view_proj_inf.inverse(), eye_alt, &ctx,
                                    quality_.cirrus ? weather_.cirrus_cover : 0.0f);

            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
            glDepthMask(GL_TRUE);
            // Terrain is a closed heightfield: back-face culling halves the
            // fragment load with no visible change.
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            glFrontFace(GL_CCW);
            const Mat4 view_proj_world = proj_world * view_world;
            gpu_profiler_.mark("terrain");
            environment_.render_ground(view_proj_world, eye_ned, &ctx);
            gpu_profiler_.mark("airframe");
            glDisable(GL_CULL_FACE);

            if (chase) {
                f16_model_.render_world(proj_world * view_rot, state, telemetry.gear_deployed, ctx);
                if (quality_.exhaust && hdr) {
                    float afterburner = 0.0f;
                    float glow = 0.0f;
                    exhaust_levels(telemetry, afterburner, glow);
                    exhaust_.render(proj_world * view_rot, exhaust_matrix(state, eye_ned), afterburner, glow,
                                    static_cast<float>(time_s_));
                }
            }
            gpu_profiler_.mark("clouds");
            if (hdr) clouds_.render(view_proj_world, ctx);
        }
        gpu_profiler_.mark("post");

        // -------------------------------------------------------------------
        // PASS 3: Bloom and display transform into the window
        // -------------------------------------------------------------------
        const auto greyout = static_cast<float>(std::clamp(telemetry.ofc_gloc_blackout_frac, 0.0, 1.0));
        if (hdr) {
            hdr_.resolve();
            PostParams post;
            post.exposure = kExposure;
            post.bloom = quality_.bloom;
            post.bloom_strength = kBloomStrength;
            post.vignette = kVignette;
            post.greyout = greyout;
            hdr_.composite(post, 0);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, viewport_width_, viewport_height_);
        glStencilMask(0xFF);
        glDepthMask(GL_TRUE);
        glClearStencil(0);
        glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        // -------------------------------------------------------------------
        // CHASE VIEW: avionics overlay, then menus
        // -------------------------------------------------------------------
        if (chase) {
            gpu_profiler_.mark("hud+menus");
            const Mat4 view_hud = Mat4::identity();
            hud_.build_symbology(state, imu, is_crashed, telemetry);
            hud_.render_unmasked(proj_world, view_hud);
            hdr_.vision_overlay(greyout, viewport_width_, viewport_height_);
            render_menu_overlay(aspect, telemetry, ctrl_profile, input_mgr);
            gpu_profiler_.end_frame();
            return;
        }

        // -------------------------------------------------------------------
        // PASS 4: Cockpit shell, airframe around the pilot, MFDs
        // -------------------------------------------------------------------
        gpu_profiler_.mark("cockpit");
        {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
            glDepthMask(GL_TRUE);

            // Light in body axes: the interior sweeps through light and
            // shadow as the aircraft rolls instead of being lit by a lamp
            // bolted to the airframe.
            const auto& sun = ctx.lighting.sun_dir;
            const math::Vector3 sun_b = state.q_att.rotate_ned_to_body(math::Vector3(sun[0], sun[1], sun[2]));
            const math::Matrix3x3 c_nb = state.q_att.to_dcm_body_to_ned();
            float body_to_ned[9];
            for (int r = 0; r < 3; ++r) {
                for (int c = 0; c < 3; ++c) body_to_ned[c * 3 + r] = static_cast<float>(c_nb(r, c));
            }
            const math::Vector3 eye_b = camera_rig_.eye_pos_body();

            cockpit_shader_.use();
            ctx.lighting.upload(cockpit_shader_);
            cockpit_shader_.set_int("uHdrOutput", 0);
            cockpit_shader_.set_int("uFrameIsBody", 1);
            cockpit_shader_.set_mat3("uBodyToNed", body_to_ned);
            cockpit_shader_.set_vec3("uSunDir", static_cast<float>(sun_b.x), static_cast<float>(sun_b.y),
                                     static_cast<float>(sun_b.z));
            cockpit_shader_.set_vec3("uEyeBody", static_cast<float>(eye_b.x), static_cast<float>(eye_b.y),
                                     static_cast<float>(eye_b.z));
            cockpit_shader_.set_mat4("uProjection", proj_cockpit);
            cockpit_shader_.set_mat4("uView", view_cockpit);
            if (ctx.shadow) {
                ctx.shadow->bind_receiver(cockpit_shader_, FrameContext::kShadowUnit, 1.0f,
                                          ctx.shadow->receiver_matrix(state.pos_ned) * ModelGLB::attitude_matrix(state));
            } else {
                ctx.bind_shadow(cockpit_shader_, 0.0f);
            }
            if (!model_cockpit_) cockpit_.draw_shell();

            // The cockpit interior, hull, wings and canopy around the pilot.
            f16_model_.render_cockpit(proj_cockpit, view_cockpit, state, ctx, eye_b, model_cockpit_);

            // Live MFD pages: self-lit, so they bypass the scene lighting.
            mfd_shader_.use();
            mfd_shader_.set_mat4("uProjection", proj_cockpit);
            mfd_shader_.set_mat4("uView", view_cockpit);
            mfd_shader_.set_int("uTexture", 0);
            instruments_.bind_texture(GL_TEXTURE0);
            cockpit_.draw_mfd();
            if (cockpit_.has_gauges()) {
                gauges_.bind_texture(GL_TEXTURE0);
                cockpit_.draw_gauges();
            }
        }

        // -------------------------------------------------------------------
        // PASS 5: Combiner glass stencil aperture mask
        // -------------------------------------------------------------------
        gpu_profiler_.mark("hud+menus");
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

            if (model_cockpit_) {
                // The asset's combiner glass is the aperture; it already drew
                // its own coating with the cockpit.
                f16_model_.render_hud_mask(proj_cockpit, view_cockpit);
                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            } else {
                cockpit_shader_.use();
                cockpit_.draw_combiner_glass();

                // Restore color writes
                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

                // Render subtle glass tint with alpha blend
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                cockpit_.draw_combiner_glass();
                glDisable(GL_BLEND);
            }
        }

        // -------------------------------------------------------------------
        // PASS 6: Optical-infinity collimated HUD symbology, stencil masked
        // to the physical combiner glass
        // -------------------------------------------------------------------
        {
            glStencilMask(0x00);
            glStencilFunc(GL_EQUAL, 1, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

            // HUD is collimated at optical infinity: no depth test, additive.
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);

            const Mat4 view_hud = camera_rig_.head_look_matrix();
            hud_.build_symbology(state, imu, is_crashed, telemetry);
            hud_.render_masked(proj_world, view_hud);

            // Reset states and restore the stencil mask for the next clear
            glDisable(GL_STENCIL_TEST);
            glDisable(GL_BLEND);
            glStencilMask(0xFF);
            glEnable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
        }

        // -------------------------------------------------------------------
        // PASS 7: G-LOC vision, then menus
        // -------------------------------------------------------------------
        hdr_.vision_overlay(greyout, viewport_width_, viewport_height_);
        render_menu_overlay(aspect, telemetry, ctrl_profile, input_mgr);
        gpu_profiler_.end_frame();
    }

    /// @brief Per-pass GPU timing (developer tool; off by default).
    GpuProfiler& gpu_profiler() noexcept { return gpu_profiler_; }

    CameraRig& camera_rig() noexcept { return camera_rig_; }
    const CameraRig& camera_rig() const noexcept { return camera_rig_; }

    HUDCollimator& hud() noexcept { return hud_; }
    const HUDCollimator& hud() const noexcept { return hud_; }

    FlightInstruments& instruments() noexcept { return instruments_; }
    CockpitGauges& gauges() noexcept { return gauges_; }
    const CockpitGauges& gauges() const noexcept { return gauges_; }
    const FlightInstruments& instruments() const noexcept { return instruments_; }

    ModelGLB& model() noexcept { return f16_model_; }
    const ModelGLB& model() const noexcept { return f16_model_; }

    const HdrPipeline& hdr() const noexcept { return hdr_; }
    const ShadowMap& shadow_map() const noexcept { return shadow_map_; }

    /// @brief Draws a menu draw list over the finished frame.
    void render_ui(const ui::DrawList& list) { ui_canvas_.render(list, viewport_width_, viewport_height_); }

    /// @brief Full-screen colour-vision correction applied to the scene and HUD.
    void set_color_filter(ui::ColorblindMode mode) noexcept { color_filter_mode_ = mode; }

    /// @brief Re-skins the tactical aircraft menu from the shared design tokens.
    void apply_theme(const ui::Theme& theme) noexcept { aircraft_menu_.apply_theme(theme); }

    AircraftMenu& menu() noexcept { return aircraft_menu_; }
    const AircraftMenu& menu() const noexcept { return aircraft_menu_; }

    bool is_initialized() const noexcept { return initialized_; }
};

} // namespace fastjet::graphics
