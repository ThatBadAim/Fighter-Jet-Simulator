#pragma once

#include "fastjet/graphics/gl_common.hpp"
#include "fastjet/graphics/shader.hpp"
#include "fastjet/graphics/terrain_field.hpp"
#include "fastjet/graphics/terrain_mesh.hpp"
#include "fastjet/fdm/flight_state.hpp"
#include <cmath>
#include <string>
#include <vector>

namespace fastjet::graphics {

/// @brief Visual environment: scattering sky dome, procedural terrain, and the
/// airfield surface with its markings.
///
/// The sky is an analytic single-scattering approximation (Rayleigh + Mie)
/// rather than a fixed gradient, so the horizon reddens near the sun, the zenith
/// deepens with altitude, and terrain haze matches the sky it fades into. The
/// same sun direction and haze constants feed the terrain and cockpit shaders,
/// which is what keeps the scene looking like one coherent lighting environment.
class SkyGroundRenderer {
public:
    /// @brief Sun direction in NED, pointing *toward* the sun. Late-morning,
    /// high and slightly forward-left of the runway heading.
    static constexpr float SUN_NED_X = 0.42f;
    static constexpr float SUN_NED_Y = -0.34f;
    static constexpr float SUN_NED_Z = -0.84f; // negative z = upward

    static void sun_dir_ned(float& x, float& y, float& z) noexcept {
        const float inv = 1.0f / std::sqrt(SUN_NED_X * SUN_NED_X +
                                           SUN_NED_Y * SUN_NED_Y +
                                           SUN_NED_Z * SUN_NED_Z);
        x = SUN_NED_X * inv;
        y = SUN_NED_Y * inv;
        z = SUN_NED_Z * inv;
    }

private:
    GLuint sky_vao_ = 0;
    GLuint sky_vbo_ = 0;
    ShaderProgram sky_shader_;

    GLuint ground_vao_ = 0;
    GLuint ground_vbo_ = 0;
    GLsizei ground_vertex_count_ = 0;
    ShaderProgram ground_shader_;

    TerrainMesh terrain_;
    ShaderProgram terrain_shader_;

    bool initialized_ = false;

    void build_sky_quad() {
        // Fullscreen quad in NDC [-1, 1]
        const float quad_verts[] = {
            -1.0f, -1.0f,
             1.0f, -1.0f,
             1.0f,  1.0f,
            -1.0f, -1.0f,
             1.0f,  1.0f,
            -1.0f,  1.0f
        };

        glGenVertexArrays(1, &sky_vao_);
        glGenBuffers(1, &sky_vbo_);

        glBindVertexArray(sky_vao_);
        glBindBuffer(GL_ARRAY_BUFFER, sky_vbo_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad_verts), quad_verts, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
    }

    /// @brief Airfield surface: apron, runway, and markings. These sit on the
    /// flat basin the terrain field guarantees around the field, so they need no
    /// height lookup, only a small bias to stay clear of the terrain mesh.
    void build_airfield() {
        std::vector<float> verts; // pos(3), color(4)

        auto add_quad = [&](float x0, float y0, float z0,
                            float x1, float y1, float z1,
                            float x2, float y2, float z2,
                            float x3, float y3, float z3,
                            const Color4& col) {
            // Reversed winding, matching TerrainMesh: NED is left-handed with
            // respect to the screen convention, so listing the corners in their
            // natural order yields a clockwise (back) face seen from above.
            const float v[6][7] = {
                {x0, y0, z0, col.r, col.g, col.b, col.a},
                {x2, y2, z2, col.r, col.g, col.b, col.a},
                {x1, y1, z1, col.r, col.g, col.b, col.a},
                {x0, y0, z0, col.r, col.g, col.b, col.a},
                {x3, y3, z3, col.r, col.g, col.b, col.a},
                {x2, y2, z2, col.r, col.g, col.b, col.a}
            };
            for (int i = 0; i < 6; ++i) {
                for (int j = 0; j < 7; ++j) {
                    verts.push_back(v[i][j]);
                }
            }
        };

        // 1. Airfield apron: a graded pad that visually seats the runway in the
        // surrounding terrain instead of letting asphalt meet raw hillside.
        //
        // TerrainMesh cuts a hole here so the two never fight for depth. That
        // hole is snapped out to the 125 m cell grid, so the apron must cover
        // the snapped bounds (-750..3750, -1000..1000), not just its own
        // nominal footprint, or the seam would show through to the sky.
        const Color4 apron_col{0.075f, 0.088f, 0.045f, 1.0f};
        add_quad(-750.0f, -1000.0f, -0.05f,
                 3750.0f, -1000.0f, -0.05f,
                 3750.0f,  1000.0f, -0.05f,
                 -750.0f,  1000.0f, -0.05f,
                 apron_col);

        // 2. Runway surface (asphalt: 3,000 m x 60 m)
        const Color4 asphalt_col{0.026f, 0.026f, 0.029f, 1.0f};
        add_quad(0.0f, -30.0f, -0.20f,
                 3000.0f, -30.0f, -0.20f,
                 3000.0f,  30.0f, -0.20f,
                 0.0f,  30.0f, -0.20f,
                 asphalt_col);

        // 3. Runway shoulders (lighter graded margin either side)
        const Color4 shoulder_col{0.048f, 0.047f, 0.042f, 1.0f};
        add_quad(0.0f, -46.0f, -0.14f, 3000.0f, -46.0f, -0.14f,
                 3000.0f, -30.0f, -0.14f, 0.0f, -30.0f, -0.14f, shoulder_col);
        add_quad(0.0f, 30.0f, -0.14f, 3000.0f, 30.0f, -0.14f,
                 3000.0f, 46.0f, -0.14f, 0.0f, 46.0f, -0.14f, shoulder_col);

        // 4. Centreline stripes (30 m dash, 20 m gap, width 0.9 m)
        const Color4 white{0.62f, 0.62f, 0.60f, 1.0f};
        for (float x = 120.0f; x < 2880.0f; x += 50.0f) {
            add_quad(x, -0.45f, -0.30f,
                     x + 30.0f, -0.45f, -0.30f,
                     x + 30.0f,  0.45f, -0.30f,
                     x,  0.45f, -0.30f,
                     white);
        }

        // 5. Runway edge lines
        add_quad(0.0f, -29.4f, -0.30f, 3000.0f, -29.4f, -0.30f,
                 3000.0f, -28.5f, -0.30f, 0.0f, -28.5f, -0.30f, white);
        add_quad(0.0f, 28.5f, -0.30f, 3000.0f, 28.5f, -0.30f,
                 3000.0f, 29.4f, -0.30f, 0.0f, 29.4f, -0.30f, white);

        // 6. Threshold piano keys at both runway heads
        for (float y = -24.0f; y <= 24.0f; y += 4.0f) {
            add_quad(30.0f, y - 1.2f, -0.30f, 80.0f, y - 1.2f, -0.30f,
                     80.0f, y + 1.2f, -0.30f, 30.0f, y + 1.2f, -0.30f, white);
            add_quad(2920.0f, y - 1.2f, -0.30f, 2970.0f, y - 1.2f, -0.30f,
                     2970.0f, y + 1.2f, -0.30f, 2920.0f, y + 1.2f, -0.30f, white);
        }

        // 7. Touchdown zone markers, 150 m past each threshold
        for (float side : {-1.0f, 1.0f}) {
            for (float y0 : {8.0f, 14.0f}) {
                add_quad(230.0f, side * y0, -0.30f, 275.0f, side * y0, -0.30f,
                         275.0f, side * (y0 + 2.4f), -0.30f, 230.0f, side * (y0 + 2.4f), -0.30f, white);
                add_quad(2725.0f, side * y0, -0.30f, 2770.0f, side * y0, -0.30f,
                         2770.0f, side * (y0 + 2.4f), -0.30f, 2725.0f, side * (y0 + 2.4f), -0.30f, white);
            }
        }

        // 8. Aiming point blocks
        for (float side : {-1.0f, 1.0f}) {
            add_quad(400.0f, side * 8.0f, -0.30f, 460.0f, side * 8.0f, -0.30f,
                     460.0f, side * 14.0f, -0.30f, 400.0f, side * 14.0f, -0.30f, white);
            add_quad(2540.0f, side * 8.0f, -0.30f, 2600.0f, side * 8.0f, -0.30f,
                     2600.0f, side * 14.0f, -0.30f, 2540.0f, side * 14.0f, -0.30f, white);
        }

        ground_vertex_count_ = static_cast<GLsizei>(verts.size() / 7);

        glGenVertexArrays(1, &ground_vao_);
        glGenBuffers(1, &ground_vbo_);

        glBindVertexArray(ground_vao_);
        glBindBuffer(GL_ARRAY_BUFFER, ground_vbo_);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(verts.size() * sizeof(float)),
                     verts.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

public:
    SkyGroundRenderer() = default;

    ~SkyGroundRenderer() {
        destroy();
    }

    /// @brief GLSL common to every shader that touches the atmosphere.
    ///
    /// Sky colour and terrain haze are the same physical quantity: light
    /// scattered into the view ray. Sharing one function means the horizon can
    /// never show a seam, because the terrain fades into exactly the radiance
    /// the sky quad would have drawn in that direction.
    static constexpr const char* ATMOSPHERE_GLSL = R"(
        // Sea-level scattering coefficients [1/m]. Rayleigh goes as 1/lambda^4,
        // which is what makes the zenith blue and the horizon pale; Mie is the
        // wavelength-neutral aerosol term.
        const vec3  RAYLEIGH = vec3(5.8e-6, 13.5e-6, 33.1e-6);
        const float MIE      = 21e-6;
        const float SCALE_H  = 8500.0;   // [m] atmospheric scale height
        const float EXPOSURE = 1.6;
        const vec3  SUN_TINT = vec3(1.0, 0.96, 0.90);

        /// Scattered radiance looking along `ray` from `altitude`, linear space.
        ///
        /// `path_mass` overrides the air mass for rays that do not run to the
        /// top of the atmosphere. A ray hitting terrain a few km away traverses
        /// far less air than the full column, and using the column value there
        /// floods near ground with bright blue in-scatter.
        vec3 sky_radiance_am(vec3 ray, vec3 sun_dir, float altitude,
                             bool with_sun, float air_mass) {
            float cos_sun = clamp(dot(ray, sun_dir), -1.0, 1.0);

            float density = exp(-altitude / SCALE_H);

            vec3  tau_r = RAYLEIGH * SCALE_H * density;
            float tau_m = MIE * SCALE_H * density;

            float phase_r = 0.75 * (1.0 + cos_sun * cos_sun);
            const float g = 0.76;
            float g2 = g * g;
            float phase_m = (1.0 - g2) /
                (4.0 * 3.14159265 * pow(1.0 + g2 - 2.0 * g * cos_sun, 1.5));

            // Single scattering with self-extinction. The (1-exp(-t))/t factor
            // is the mean attenuation along the path, which is what stops the
            // horizon saturating to white.
            vec3 od = (tau_r + vec3(tau_m)) * air_mass;
            vec3 atten = (vec3(1.0) - exp(-od)) / max(od, vec3(1e-6));
            vec3 inscat = (tau_r * phase_r + vec3(tau_m * phase_m)) * air_mass * atten;

            vec3 sky = SUN_TINT * inscat * EXPOSURE;

            if (with_sun) {
                // Sun disc (~0.53 deg) plus a soft aureole.
                float sun_disc = smoothstep(0.99990, 0.99996, cos_sun);
                float aureole  = pow(max(cos_sun, 0.0), 1200.0) * 0.35;
                sky += SUN_TINT * (sun_disc * 14.0 + aureole);
            }
            return sky;
        }

        /// Sky dome radiance: the ray runs to the top of the atmosphere, so the
        /// air mass is the full slant column.
        vec3 sky_radiance(vec3 ray, vec3 sun_dir, float altitude, bool with_sun) {
            float up = -ray.z;                     // NED: up is -Z
            float mu = max(up, 0.015);             // 1 at zenith, large at horizon
            return sky_radiance_am(ray, sun_dir, altitude, with_sun, 1.0 / mu);
        }

        /// In-scattered light between the eye and a surface `dist` away.
        ///
        /// Air mass is the density-weighted path length in units of the vertical
        /// scale height. Close in, a short ray scatters little and the haze is
        /// dim. Far out, the ray grazes through dense low air for tens of km and
        /// its air mass approaches that of the sky column in the same direction.
        ///
        /// The far limit matters as much as the near one: where haze becomes
        /// opaque the terrain must be *exactly* the colour the sky quad draws,
        /// or the horizon shows a rim. Blending the air mass toward the sky's
        /// own value makes the two agree in the limit by construction.
        vec3 haze_radiance(vec3 ray, vec3 sun_dir, float eye_alt,
                           float surf_alt, float dist) {
            float mean_alt = 0.5 * (eye_alt + surf_alt);

            // Mean relative density along the ray.
            float dh = eye_alt - surf_alt;
            float rho;
            if (abs(dh) < 1.0) {
                rho = exp(-max(mean_alt, 0.0) / SCALE_H);
            } else {
                rho = (exp(-max(surf_alt, 0.0) / SCALE_H) -
                       exp(-max(eye_alt, 0.0) / SCALE_H)) * SCALE_H / dh;
            }
            float near_am = rho * dist / SCALE_H;

            // The air mass the sky quad uses for this same direction.
            float up = -ray.z;
            float sky_am = 1.0 / max(up, 0.015);

            // Converge on the sky as the haze saturates, so distant ground and
            // sky meet with no step.
            float t = smoothstep(20000.0, 60000.0, dist);
            float am = mix(near_am, sky_am, t);

            // Likewise let the reference altitude drift to the eye altitude, so
            // the far field matches the sky's own vertical density.
            float alt = mix(mean_alt, eye_alt, t);

            return sky_radiance_am(ray, sun_dir, alt, false, am);
        }

        /// Filmic-ish tonemap then gamma. Applied identically everywhere so the
        /// sky and the ground share one response curve.
        vec3 tonemap(vec3 c) {
            c = c / (c + vec3(1.0));
            return pow(c, vec3(1.0 / 2.2));
        }

        /// Fraction of a view ray's radiance that is haze rather than surface.
        ///
        /// Aerosol haze lives in the boundary layer, not throughout the column,
        /// so optical depth must be integrated along the ray rather than taken
        /// as a constant per metre. For an exponential profile the integral
        /// between two altitudes has a closed form, which is what keeps the
        /// ground crisp from high up while still washing out along the deck.
        float haze_fraction(float eye_alt, float surf_alt, float dist) {
            const float BETA0 = 7.4e-5;   // [1/m] sea-level extinction (~53 km visibility)
            const float H_AER = 1200.0;   // [m] aerosol scale height

            float dh = eye_alt - surf_alt;
            float rho_eye  = exp(-max(eye_alt, 0.0)  / H_AER);
            float rho_surf = exp(-max(surf_alt, 0.0) / H_AER);

            // Mean density along the ray. When the two ends are at nearly the
            // same height the closed form degenerates, so fall back to the
            // local density there.
            float mean_rho;
            if (abs(dh) < 1.0) {
                mean_rho = rho_surf;
            } else {
                mean_rho = (rho_surf - rho_eye) * H_AER / dh;
            }

            float od = BETA0 * mean_rho * dist;
            float fog = 1.0 - exp(-od);

            // The terrain mesh stops at ~91 km on axis. Exponential haze alone
            // is still only ~0.98 opaque out there, and that last two percent of
            // dark ground against a bright sky is a visible rim at the horizon.
            // Force the far field fully into haze well inside the mesh edge, so
            // the ground dissolves rather than ending.
            const float FADE_START = 42000.0;
            const float FADE_END   = 70000.0;
            float edge = smoothstep(FADE_START, FADE_END, dist);

            return clamp(max(fog, edge), 0.0, 1.0);
        }
    )";

    bool init() {
        if (initialized_) return true;

        // -------------------------------------------------------------------
        // Sky: analytic single-scattering dome
        // -------------------------------------------------------------------
        const char* sky_vert = R"(
            #version 330 core
            layout (location = 0) in vec2 aPos;
            out vec2 vPos;
            void main() {
                vPos = aPos;
                gl_Position = vec4(aPos, 0.9999, 1.0); // Render at far plane
            }
        )";

        const std::string sky_frag = std::string(R"(
            #version 330 core
            in vec2 vPos;
            out vec4 FragColor;

            uniform mat4  uInvViewProj;
            uniform vec3  uSunDir;    // NED, points toward the sun
            uniform float uAltitude;  // [m] above sea level
        )") + ATMOSPHERE_GLSL + R"(
            void main() {
                vec4 near_world = uInvViewProj * vec4(vPos, -1.0, 1.0);
                vec4 far_world  = uInvViewProj * vec4(vPos,  1.0, 1.0);
                vec3 ray = normalize(far_world.xyz / far_world.w - near_world.xyz / near_world.w);

                vec3 sky = sky_radiance(ray, uSunDir, uAltitude, true);
                FragColor = vec4(tonemap(sky), 1.0);
            }
        )";

        if (!sky_shader_.init_from_source(sky_vert, sky_frag.c_str())) {
            return false;
        }

        // -------------------------------------------------------------------
        // Terrain: elevation/slope palette with sun lighting and aerial haze
        // -------------------------------------------------------------------
        const char* terr_vert = R"(
            #version 330 core
            layout (location = 0) in vec3 aPos;
            layout (location = 1) in vec3 aNormal;
            layout (location = 2) in float aHeight;

            uniform mat4 uViewProj;
            uniform vec3 uEyeNed;

            out vec3  vNormal;
            out float vHeight;
            out float vDist;
            out vec3  vWorld;

            void main() {
                vNormal = aNormal;
                vHeight = aHeight;
                vWorld  = aPos;
                vDist   = length(aPos - uEyeNed);
                gl_Position = uViewProj * vec4(aPos, 1.0);
            }
        )";

        const std::string terr_frag = std::string(R"(
            #version 330 core
            in vec3  vNormal;
            in float vHeight;
            in float vDist;
            in vec3  vWorld;
            out vec4 FragColor;

            uniform vec3  uSunDir;
            uniform float uAltitude;
            uniform vec3  uEyeNed;
        )") + ATMOSPHERE_GLSL + R"(
            // Cheap hash-based value noise, used only to break up flat colour
            // bands. Matching the CPU field exactly is unnecessary here.
            float hash(vec2 p) {
                return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
            }
            float noise(vec2 p) {
                vec2 i = floor(p), f = fract(p);
                f = f * f * (3.0 - 2.0 * f);
                return mix(mix(hash(i), hash(i + vec2(1, 0)), f.x),
                           mix(hash(i + vec2(0, 1)), hash(i + vec2(1, 1)), f.x), f.y);
            }

            void main() {
                vec3 n = normalize(vNormal);

                // Slope: 1.0 on flat ground (normal straight up in NED = -Z).
                float flatness = clamp(-n.z, 0.0, 1.0);
                float slope = 1.0 - flatness;

                // Elevation palette, authored as linear reflectances. Real
                // landscape albedo is low: grass ~0.15, dry scrub ~0.20,
                // rock ~0.18. Values much above that read as painted plastic.
                vec3 lowland  = vec3(0.055, 0.085, 0.042);
                vec3 meadow   = vec3(0.085, 0.105, 0.050);
                vec3 upland   = vec3(0.125, 0.110, 0.062);
                vec3 rock     = vec3(0.105, 0.098, 0.090);
                vec3 snow     = vec3(0.700, 0.730, 0.780);

                float h = vHeight;
                vec3 col = mix(lowland, meadow, smoothstep(0.0, 120.0, h));
                col = mix(col, upland,  smoothstep(110.0, 300.0, h));
                col = mix(col, rock,    smoothstep(300.0, 480.0, h));

                // Steep faces expose rock regardless of height.
                col = mix(col, rock, smoothstep(0.30, 0.68, slope));

                // Snow only where it can lie: high, and not on steep faces.
                float snow_line = smoothstep(540.0, 615.0, h) * smoothstep(0.55, 0.82, flatness);
                col = mix(col, snow, snow_line);

                // Multi-scale mottling so large triangles do not read as flat.
                float m = noise(vWorld.xy * 0.0016) * 0.5 + noise(vWorld.xy * 0.0071) * 0.5;
                col *= 0.82 + 0.34 * m;

                // Direct sun + sky ambient, in linear space. The ambient term is
                // weighted by openness so gullies stay darker than ridges.
                float ndl = max(dot(n, uSunDir), 0.0);
                vec3 sun_col = vec3(1.0, 0.95, 0.86) * 2.9;
                vec3 sky_col = vec3(0.30, 0.42, 0.62) * 0.65;
                float sky_occ = 0.55 + 0.45 * flatness;
                vec3 lit = col * (sun_col * ndl + sky_col * sky_occ);

                // Aerial perspective: fade into the radiance the sky itself
                // would show along this exact view ray. Because both sides use
                // sky_radiance(), the horizon has no seam by construction.
                vec3 ray = normalize(vWorld - uEyeNed);
                vec3 haze = haze_radiance(ray, uSunDir, uAltitude, -vWorld.z, vDist);
                float fog = haze_fraction(uAltitude, -vWorld.z, vDist);
                lit = mix(lit, haze, fog);

                FragColor = vec4(tonemap(lit), 1.0);
            }
        )";

        if (!terrain_shader_.init_from_source(terr_vert, terr_frag.c_str())) {
            return false;
        }

        // -------------------------------------------------------------------
        // Airfield: flat vertex-coloured geometry, same lighting and haze
        // -------------------------------------------------------------------
        const char* gnd_vert = R"(
            #version 330 core
            layout (location = 0) in vec3 aPos;
            layout (location = 1) in vec4 aColor;
            uniform mat4 uViewProj;
            uniform vec3 uEyeNed;
            out vec4  vColor;
            out float vDist;
            out vec3  vWorld;
            void main() {
                vColor = aColor;
                vWorld = aPos;
                vDist  = length(aPos - uEyeNed);
                gl_Position = uViewProj * vec4(aPos, 1.0);
            }
        )";

        const std::string gnd_frag = std::string(R"(
            #version 330 core
            in vec4  vColor;
            in float vDist;
            in vec3  vWorld;
            out vec4 FragColor;

            uniform vec3  uSunDir;
            uniform float uAltitude;
            uniform vec3  uEyeNed;
        )") + ATMOSPHERE_GLSL + R"(
            void main() {
                // The airfield is flat, so its normal is straight up in NED.
                const vec3 n = vec3(0.0, 0.0, -1.0);
                float ndl = max(dot(n, uSunDir), 0.0);
                vec3 sun_col = vec3(1.0, 0.95, 0.86) * 2.9;
                vec3 sky_col = vec3(0.30, 0.42, 0.62) * 0.65;
                vec3 lit = vColor.rgb * (sun_col * ndl + sky_col);

                vec3 ray = normalize(vWorld - uEyeNed);
                vec3 haze = haze_radiance(ray, uSunDir, uAltitude, -vWorld.z, vDist);
                lit = mix(lit, haze, haze_fraction(uAltitude, -vWorld.z, vDist));

                FragColor = vec4(tonemap(lit), vColor.a);
            }
        )";

        if (!ground_shader_.init_from_source(gnd_vert, gnd_frag.c_str())) {
            return false;
        }

        build_sky_quad();
        build_airfield();
        terrain_.init();

        initialized_ = true;
        return true;
    }

    void destroy() noexcept {
        if (sky_vao_) { glDeleteVertexArrays(1, &sky_vao_); sky_vao_ = 0; }
        if (sky_vbo_) { glDeleteBuffers(1, &sky_vbo_); sky_vbo_ = 0; }
        if (ground_vao_) { glDeleteVertexArrays(1, &ground_vao_); ground_vao_ = 0; }
        if (ground_vbo_) { glDeleteBuffers(1, &ground_vbo_); ground_vbo_ = 0; }
        terrain_.destroy();
        sky_shader_.destroy();
        ground_shader_.destroy();
        terrain_shader_.destroy();
        initialized_ = false;
    }

    /// @brief Render the scattering sky dome.
    /// @param inv_view_proj Inverse of the infinity view-projection matrix
    /// @param altitude_m Eye altitude above sea level [m]
    void render_sky(const Mat4& inv_view_proj, float altitude_m = 0.0f) {
        if (!initialized_) return;

        float sx, sy, sz;
        sun_dir_ned(sx, sy, sz);

        glDepthMask(GL_FALSE); // Don't write depth for sky
        sky_shader_.use();
        sky_shader_.set_mat4("uInvViewProj", inv_view_proj);
        sky_shader_.set_vec3("uSunDir", sx, sy, sz);
        sky_shader_.set_float("uAltitude", altitude_m);

        glBindVertexArray(sky_vao_);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
    }

    /// @brief Render procedural terrain, then the airfield surface on top of it.
    /// @param view_proj World view-projection matrix
    /// @param eye_ned Eye position in NED [m], for range-based haze
    void render_ground(const Mat4& view_proj, const math::Vector3& eye_ned) {
        if (!initialized_) return;

        float sx, sy, sz;
        sun_dir_ned(sx, sy, sz);

        const auto ex = static_cast<float>(eye_ned.x);
        const auto ey = static_cast<float>(eye_ned.y);
        const auto ez = static_cast<float>(eye_ned.z);
        const float altitude = -ez;

        // Terrain first, at true depth.
        terrain_shader_.use();
        terrain_shader_.set_mat4("uViewProj", view_proj);
        terrain_shader_.set_vec3("uSunDir", sx, sy, sz);
        terrain_shader_.set_vec3("uEyeNed", ex, ey, ez);
        terrain_shader_.set_float("uAltitude", altitude);
        terrain_.draw();

        // Airfield markings are coplanar decals on the flat basin: bias them
        // toward the viewer so they never z-fight with the terrain mesh.
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-2.0f, -2.0f);

        ground_shader_.use();
        ground_shader_.set_mat4("uViewProj", view_proj);
        ground_shader_.set_vec3("uSunDir", sx, sy, sz);
        ground_shader_.set_vec3("uEyeNed", ex, ey, ez);
        ground_shader_.set_float("uAltitude", altitude);

        glBindVertexArray(ground_vao_);
        glDrawArrays(GL_TRIANGLES, 0, ground_vertex_count_);
        glBindVertexArray(0);

        glDisable(GL_POLYGON_OFFSET_FILL);
    }

    [[nodiscard]] GLsizei terrain_vertex_count() const noexcept {
        return terrain_.vertex_count();
    }

    bool is_initialized() const noexcept { return initialized_; }
};

} // namespace fastjet::graphics
