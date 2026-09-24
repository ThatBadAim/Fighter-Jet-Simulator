#pragma once

/// @file
/// @brief GLSL source shared by every scene shader.
///
/// Each block is a self-contained chunk that a shader concatenates after its
/// `#version` line and uniforms. Keeping one copy of the atmosphere, tonemap,
/// BRDF and shadow code is what keeps the sky, terrain, airframe and clouds in
/// one lighting environment: none of them can drift out of calibration with the
/// others, because there is nothing to drift.
///
/// Dependency order when concatenating: COMMON, OUTPUT, ATMOSPHERE, then any of
/// LIGHTING, SHADOW, CLOUDS, and one of GROUND_BAKED (run time) or
/// TERRAIN_MATERIAL (the bake). Both need COMMON and define GroundSample.

namespace fastjet::graphics::glsl {

/// @brief Constants, integer hashing and value/cellular noise.
///
/// Hashes are integer based rather than the usual fract(sin()) trick: at the
/// 100 km coordinates this world uses, sin() of a large argument loses most of
/// its mantissa and the noise visibly tiles.
inline constexpr const char* COMMON = R"(
const float PI = 3.14159265;

uint hash_u32(uint x) {
    x ^= x >> 16u; x *= 0x7feb352du;
    x ^= x >> 15u; x *= 0x846ca68bu;
    x ^= x >> 16u;
    return x;
}
float hash21(ivec2 p) {
    return float(hash_u32(uint(p.x) * 0x8da6b343u ^ hash_u32(uint(p.y) + 0x68e31da4u))) * (1.0 / 4294967296.0);
}
vec2 hash22(ivec2 p) {
    uint h = hash_u32(uint(p.x) * 0x8da6b343u ^ hash_u32(uint(p.y) + 0x9e3779b9u));
    return vec2(float(h & 0xffffu), float(h >> 16u)) * (1.0 / 65535.0);
}

/// Smooth value noise in [0, 1].
float vnoise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = p - i;
    vec2 u = f * f * (3.0 - 2.0 * f);
    ivec2 c = ivec2(i);
    float a = hash21(c);
    float b = hash21(c + ivec2(1, 0));
    float d = hash21(c + ivec2(0, 1));
    float e = hash21(c + ivec2(1, 1));
    return mix(mix(a, b, u.x), mix(d, e, u.x), u.y);
}

/// Value noise with its analytic gradient: (value, d/dx, d/dy). One lattice
/// lookup gives a bump normal that finite differences would need three for.
vec3 vnoise_d(vec2 p) {
    vec2 i = floor(p);
    vec2 f = p - i;
    vec2 u = f * f * (3.0 - 2.0 * f);
    vec2 du = 6.0 * f * (1.0 - f);
    ivec2 c = ivec2(i);
    float a = hash21(c);
    float b = hash21(c + ivec2(1, 0));
    float d = hash21(c + ivec2(0, 1));
    float e = hash21(c + ivec2(1, 1));
    float k1 = b - a, k2 = d - a, k4 = a - b - d + e;
    return vec3(a + k1 * u.x + k2 * u.y + k4 * u.x * u.y,
                du * vec2(k1 + k4 * u.y, k2 + k4 * u.x));
}

/// Fractional Brownian motion in [0, 1]. Each octave is rotated so the
/// lattices never line up into visible grid artefacts.
float fbm(vec2 p, int octaves) {
    const mat2 ROT = mat2(0.80, 0.60, -0.60, 0.80);
    float sum = 0.0, amp = 0.5, norm = 0.0;
    for (int i = 0; i < octaves; ++i) {
        sum += amp * vnoise(p);
        norm += amp;
        p = ROT * p * 2.03 + vec2(17.1, 9.2);
        amp *= 0.5;
    }
    return sum / norm;
}

/// Cellular noise: distance to the nearest and second-nearest feature point,
/// and the integer id of the nearest cell.
vec2 voronoi(vec2 p, out ivec2 id) {
    ivec2 base = ivec2(floor(p));
    vec2 f = p - floor(p);
    float d1 = 8.0, d2 = 8.0;
    id = base;
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            ivec2 c = ivec2(x, y);
            vec2 feature = vec2(c) + 0.15 + 0.7 * hash22(base + c);
            float d = length(feature - f);
            if (d < d1) { d2 = d1; d1 = d; id = base + c; }
            else if (d < d2) { d2 = d; }
        }
    }
    return vec2(d1, d2);
}

/// Interleaved gradient noise: per-pixel dither and PCF rotation.
float ign(vec2 frag) {
    return fract(52.9829189 * fract(dot(frag, vec2(0.06711056, 0.00583715))));
}
)";

/// @brief Final colour encoding.
///
/// Scene shaders write linear radiance. When the frame goes through the HDR
/// post chain (`uHdrOutput == 1`) that radiance is stored as-is and tonemapped
/// once, after bloom. When a shader draws straight to the display, as the
/// cockpit and the standalone environment tests do, it encodes with the same
/// curve and exposure itself. The default uniform values (0) select direct
/// display output, so a shader used on its own still produces a correct image.
inline constexpr const char* OUTPUT = R"(
uniform int   uHdrOutput;
uniform float uExposure;

/// Scene-referred to display exposure. Calibrated so mid-grey reflectances
/// land where the previous Reinhard curve put them, with a deeper toe.
const float DEFAULT_EXPOSURE = 1.35;

/// ACES reference rendering transform, Stephen Hill's fitted approximation.
vec3 aces_fitted(vec3 c) {
    const mat3 ACES_IN = mat3(0.59719, 0.07600, 0.02840,
                              0.35458, 0.90834, 0.13383,
                              0.04823, 0.01566, 0.83777);
    const mat3 ACES_OUT = mat3( 1.60475, -0.10208, -0.00327,
                               -0.53108,  1.10813, -0.07276,
                               -0.07367, -0.00605,  1.07602);
    c = ACES_IN * c;
    vec3 a = c * (c + 0.0245786) - 0.000090537;
    vec3 b = c * (0.983729 * c + 0.4329510) + 0.238081;
    return clamp(ACES_OUT * (a / b), 0.0, 1.0);
}

vec3 encode_srgb(vec3 c) {
    return mix(12.92 * c, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, step(0.0031308, c));
}

float scene_exposure() { return uExposure > 0.0 ? uExposure : DEFAULT_EXPOSURE; }

vec3 display_encode(vec3 radiance) {
    return encode_srgb(aces_fitted(radiance * scene_exposure()));
}

vec4 scene_output(vec3 radiance, float alpha) {
    radiance = max(radiance, vec3(0.0));
    if (uHdrOutput == 1) return vec4(radiance, alpha);
    return vec4(display_encode(radiance), alpha);
}
)";

/// @brief Analytic single-scattering atmosphere.
///
/// Sky colour and terrain haze are the same physical quantity: light scattered
/// into the view ray. Sharing one function means the horizon can never show a
/// seam, because the terrain fades into exactly the radiance the sky quad draws
/// in that direction. graphics::AtmosphereModel mirrors sky_radiance_am() on
/// the CPU for ambient lighting; the two must be changed together.
inline constexpr const char* ATMOSPHERE = R"(
// Sea-level scattering coefficients [1/m]. Rayleigh goes as 1/lambda^4, which
// is what makes the zenith blue and the horizon pale; Mie is the
// wavelength-neutral aerosol term.
const vec3  RAYLEIGH = vec3(5.8e-6, 13.5e-6, 33.1e-6);
const float MIE      = 21e-6;
const float SCALE_H  = 8500.0;   // [m] atmospheric scale height
const float EXPOSURE = 1.6;      // sky radiance scale (not the camera exposure)
const vec3  SUN_TINT = vec3(1.0, 0.96, 0.90);
const float SUN_DISC_RADIANCE = 40.0; // HDR: enough to bloom, clipped on display

/// Scattered radiance looking along `ray` from `altitude`, linear space.
///
/// `air_mass` overrides the path length for rays that do not run to the top
/// of the atmosphere. A ray hitting terrain a few km away traverses far less
/// air than the full column, and using the column value there floods near
/// ground with bright blue in-scatter.
vec3 sky_radiance_am(vec3 ray, vec3 sun_dir, float altitude, bool with_sun, float air_mass) {
    float cos_sun = clamp(dot(ray, sun_dir), -1.0, 1.0);
    float density = exp(-altitude / SCALE_H);

    vec3  tau_r = RAYLEIGH * SCALE_H * density;
    float tau_m = MIE * SCALE_H * density;

    float phase_r = 0.75 * (1.0 + cos_sun * cos_sun);
    const float g = 0.76;
    float g2 = g * g;
    float phase_m = (1.0 - g2) / (4.0 * PI * pow(1.0 + g2 - 2.0 * g * cos_sun, 1.5));

    // Single scattering with self-extinction. The (1-exp(-t))/t factor is the
    // mean attenuation along the path, which stops the horizon saturating.
    vec3 od = (tau_r + vec3(tau_m)) * air_mass;
    vec3 atten = (vec3(1.0) - exp(-od)) / max(od, vec3(1e-6));
    vec3 inscat = (tau_r * phase_r + vec3(tau_m * phase_m)) * air_mass * atten;

    vec3 sky = SUN_TINT * inscat * EXPOSURE;

    if (with_sun) {
        // Sun disc (~0.53 deg) with limb darkening, plus a soft aureole.
        float disc = smoothstep(0.99990, 0.99996, cos_sun);
        float limb = mix(0.55, 1.0, smoothstep(0.99990, 0.99999, cos_sun));
        float aureole = pow(max(cos_sun, 0.0), 1200.0) * 0.35;
        sky += SUN_TINT * (disc * limb * SUN_DISC_RADIANCE + aureole);
    }
    return sky;
}

/// Sky dome radiance: the ray runs to the top of the atmosphere.
vec3 sky_radiance(vec3 ray, vec3 sun_dir, float altitude, bool with_sun) {
    float mu = max(-ray.z, 0.015); // NED: up is -Z
    return sky_radiance_am(ray, sun_dir, altitude, with_sun, 1.0 / mu);
}

/// In-scattered light between the eye and a surface `dist` away.
///
/// Where haze becomes opaque the surface must be exactly the colour the sky
/// quad draws, or the horizon shows a rim, so the air mass and reference
/// altitude converge on the sky's own values with distance.
vec3 haze_radiance(vec3 ray, vec3 sun_dir, float eye_alt, float surf_alt, float dist) {
    float mean_alt = 0.5 * (eye_alt + surf_alt);
    float dh = eye_alt - surf_alt;
    float rho;
    if (abs(dh) < 1.0) {
        rho = exp(-max(mean_alt, 0.0) / SCALE_H);
    } else {
        rho = (exp(-max(surf_alt, 0.0) / SCALE_H) - exp(-max(eye_alt, 0.0) / SCALE_H)) * SCALE_H / dh;
    }
    float near_am = rho * dist / SCALE_H;
    float sky_am = 1.0 / max(-ray.z, 0.015);
    float t = smoothstep(20000.0, 60000.0, dist);
    float am = mix(near_am, sky_am, t);
    float alt = mix(mean_alt, eye_alt, t);
    return sky_radiance_am(ray, sun_dir, alt, false, am);
}

/// Fraction of a view ray's radiance that is haze rather than surface.
///
/// Aerosol lives in the boundary layer, so optical depth is integrated along
/// the ray through an exponential profile (closed form), which keeps the
/// ground crisp from high up while still washing out along the deck.
float haze_fraction(float eye_alt, float surf_alt, float dist) {
    const float BETA0 = 5.2e-5;   // [1/m] sea-level extinction (~75 km visibility)
    const float H_AER = 1250.0;   // [m] aerosol scale height
    float dh = eye_alt - surf_alt;
    float rho_eye  = exp(-max(eye_alt, 0.0) / H_AER);
    float rho_surf = exp(-max(surf_alt, 0.0) / H_AER);
    float mean_rho = abs(dh) < 1.0 ? rho_surf : (rho_surf - rho_eye) * H_AER / dh;
    float fog = 1.0 - exp(-BETA0 * mean_rho * dist);
    // The terrain mesh stops at ~95 km on axis.
    float edge = smoothstep(45000.0, 72000.0, dist);
    return clamp(max(fog, edge), 0.0, 1.0);
}

/// Applies aerial perspective to a lit surface radiance.
vec3 apply_haze(vec3 lit, vec3 world, vec3 eye, vec3 sun_dir) {
    float dist = length(world - eye);
    vec3 ray = (world - eye) / max(dist, 1e-3);
    vec3 haze = haze_radiance(ray, sun_dir, -eye.z, -world.z, dist);
    return mix(lit, haze, haze_fraction(-eye.z, -world.z, dist));
}
)";

/// @brief Frame lighting uniforms and a Cook-Torrance BRDF.
///
/// Sun and ambient terms come from graphics::SceneLighting, computed once per
/// frame on the CPU from the same scattering model the sky uses.
inline constexpr const char* LIGHTING = R"(
uniform vec3 uSunDir;          // toward the sun, in the shading frame
uniform vec3 uSunRadiance;     // sun colour at the eye, linear
uniform vec3 uSkyAmbient;      // cosine-weighted sky radiance, upper hemisphere
uniform vec3 uGroundAmbient;   // radiance bounced up from the ground
uniform int  uFrameIsBody;     // 1 when shading in aircraft body axes (cockpit)
uniform mat3 uBodyToNed;       // body -> NED rotation when uFrameIsBody == 1

/// Converts a shading-frame direction to NED, where "up" and the sky live.
vec3 to_ned(vec3 v) { return uFrameIsBody == 1 ? uBodyToNed * v : v; }

/// Fallbacks for shaders used without a SceneLighting upload (tests).
vec3 sun_radiance() {
    return dot(uSunRadiance, uSunRadiance) > 0.0 ? uSunRadiance : vec3(1.0, 0.95, 0.86) * 3.0;
}
vec3 sky_ambient() {
    return dot(uSkyAmbient, uSkyAmbient) > 0.0 ? uSkyAmbient : vec3(0.22, 0.30, 0.44);
}
vec3 ground_ambient() {
    return dot(uGroundAmbient, uGroundAmbient) > 0.0 ? uGroundAmbient : vec3(0.10, 0.11, 0.08);
}

/// Hemispherical ambient for a normal: sky from above, bounce from below.
vec3 hemi_ambient(vec3 n) {
    float up = clamp(-to_ned(n).z * 0.5 + 0.5, 0.0, 1.0);
    return mix(ground_ambient(), sky_ambient(), up);
}

float d_ggx(float ndh, float a) {
    float a2 = a * a;
    float d = ndh * ndh * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}
float v_smith_ggx(float ndv, float ndl, float a) {
    float k = a * 0.5;
    return 0.25 / ((ndv * (1.0 - k) + k) * (ndl * (1.0 - k) + k));
}
vec3 f_schlick(vec3 f0, float vdh) {
    return f0 + (1.0 - f0) * pow(1.0 - vdh, 5.0);
}
/// Fresnel with roughness damping, for image-based (ambient) reflection.
vec3 f_schlick_rough(vec3 f0, float ndv, float rough) {
    return f0 + (max(vec3(1.0 - rough), f0) - f0) * pow(1.0 - ndv, 5.0);
}

/// Direct sun contribution for a metallic-roughness surface.
vec3 sun_brdf(vec3 albedo, float metallic, float rough, vec3 n, vec3 v, vec3 l) {
    float ndl = max(dot(n, l), 0.0);
    if (ndl <= 0.0) return vec3(0.0);
    float ndv = max(dot(n, v), 1e-3);
    vec3 h = normalize(v + l);
    float a = max(rough * rough, 0.002);
    vec3 f0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = f_schlick(f0, max(dot(v, h), 0.0));
    vec3 spec = F * d_ggx(max(dot(n, h), 0.0), a) * v_smith_ggx(ndv, ndl, a);
    vec3 diff = (1.0 - F) * (1.0 - metallic) * albedo;
    // Diffuse is albedo * E/pi; sun_radiance() already carries the 1/pi.
    return (diff + spec * PI) * sun_radiance() * ndl;
}

/// Environment reflection: the real sky above the horizon, lit ground below,
/// blurred toward the ambient average as roughness rises.
vec3 environment_radiance(vec3 r, float rough, float altitude) {
    vec3 blurred = hemi_ambient(r);
    float blur = clamp(rough * 1.3, 0.0, 1.0);
    if (blur >= 1.0) return blurred; // rough: the sharp sky is never seen
    vec3 rn = to_ned(r);
    vec3 sharp = rn.z < 0.0 ? sky_radiance(rn, to_ned(uSunDir), altitude, false) : ground_ambient() * 0.9;
    return mix(sharp, blurred, blur);
}

/// Ambient (image-based) contribution for a metallic-roughness surface.
vec3 ambient_brdf(vec3 albedo, float metallic, float rough, vec3 n, vec3 v, float altitude, float ao) {
    float ndv = max(dot(n, v), 1e-3);
    vec3 f0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = f_schlick_rough(f0, ndv, rough);
    vec3 diff = (1.0 - F) * (1.0 - metallic) * albedo * hemi_ambient(n);
    vec3 r = reflect(-v, n);
    // Horizon occlusion: reflections that point into the surface are dimmed.
    float horizon = clamp(1.0 + dot(r, n), 0.0, 1.0);
    vec3 spec = F * environment_radiance(r, rough, altitude) * horizon * horizon;
    return (diff + spec) * ao;
}
)";

/// @brief Sun shadow map lookup (the aircraft is the only caster).
inline constexpr const char* SHADOW = R"(
uniform sampler2DShadow uShadowMap;
uniform mat4  uShadowMatrix;   // world -> shadow texture space
uniform int   uShadowEnabled;
uniform float uShadowTexel;    // shadow texel size in texture units
uniform float uShadowStrength; // receiver-specific fade (penumbra with height)
uniform float uShadowBias;     // receiver depth bias in normalised depth

/// Visibility of the sun in [0, 1] at a world position. A rotated 8-tap
/// Poisson kernel on top of hardware 2x2 PCF gives a soft, stable edge.
float sun_shadow(vec3 world, vec3 n, vec2 frag) {
    if (uShadowEnabled != 1 || uShadowStrength <= 0.0) return 1.0;
    // Normal offset keeps curved surfaces from shadowing themselves.
    vec4 p = uShadowMatrix * vec4(world + n * 0.04, 1.0);
    vec3 s = p.xyz / p.w;
    if (any(lessThan(s.xy, vec2(0.0))) || any(greaterThan(s.xy, vec2(1.0)))) return 1.0;
    const vec2 POISSON[8] = vec2[8](
        vec2(-0.613, 0.617), vec2(0.170, -0.040), vec2(-0.299, -0.792), vec2(0.645, 0.493),
        vec2(-0.651, -0.162), vec2(0.421, -0.628), vec2(-0.050, 0.980), vec2(0.954, -0.198));
    float ang = ign(frag) * 6.2831853;
    mat2 rot = mat2(cos(ang), sin(ang), -sin(ang), cos(ang));
    float radius = uShadowTexel * 1.6;
    float lit = 0.0;
    for (int i = 0; i < 8; ++i) {
        lit += texture(uShadowMap, vec3(s.xy + rot * POISSON[i] * radius, min(s.z, 1.0) - uShadowBias));
    }
    lit /= 8.0;
    // Fade out toward the map border so the shadow never ends on a hard line.
    vec2 edge = min(s.xy, 1.0 - s.xy);
    float border = smoothstep(0.0, 0.06, min(edge.x, edge.y));
    return mix(1.0, lit, uShadowStrength * border);
}
)";

/// @brief Scattered cumulus layer: coverage field, shadows and thickness.
///
/// A 2D field rather than a volume: it is what an integrated GPU can afford at
/// 1080p, and from the distances a jet sees clouds at, a coverage map with
/// thickness shading reads as cumulus. The field lives in "cloud space"
/// (world position plus wind drift), so the drifting deck is one static
/// pattern. The cloud sheet evaluates it procedurally at full resolution; the
/// shadows it casts on terrain and airframe read a baked map of the same
/// field (CloudLayer), so every shadow sits under the cloud that casts it.
inline constexpr const char* CLOUDS = R"(
uniform float uCloudCoverage; // 0 disables the layer (and its shadows)
uniform float uCloudBase;     // [m] cloud base altitude
uniform float uCloudTop;      // [m] cloud top altitude
uniform vec2  uCloudOffset;   // [m] wind drift: cloud space = world + offset
uniform sampler2D uCloudMap;  // baked field for shadows
uniform vec3  uCloudRect;     // map origin in cloud space [m], 1/size

/// Density in [0, 1] at a cloud-space position.
float cloud_field(vec2 c, int octaves) {
    vec2 p = c * (1.0 / 3000.0);
    // Domain warp breaks the value-noise blobs into lumpy cumulus outlines.
    vec2 warp = vec2(vnoise(p * 1.7 + 3.1), vnoise(p * 1.7 + 11.7)) - 0.5;
    float n = fbm(p + warp * 0.55, octaves);
    // fbm clusters around 0.5 (sd ~0.1), so the threshold is placed on that
    // distribution: coverage 0.5 thresholds at the median.
    float lo = 0.5 + (0.5 - uCloudCoverage) * 0.36;
    return smoothstep(lo, lo + 0.09, n);
}

/// Density at a world position, evaluated procedurally.
float cloud_density(vec2 xy, int octaves) {
    return cloud_field(xy + uCloudOffset, octaves);
}

/// Density at a world position from the baked map (clear outside it).
float cloud_density_map(vec2 xy) {
    vec2 uv = (xy + uCloudOffset - uCloudRect.xy) * uCloudRect.z;
    if (any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0)))) return 0.0;
    return texture(uCloudMap, uv).r;
}

/// Fraction of sunlight that passes the cloud layer to reach `world`.
float cloud_shadow(vec3 world, vec3 sun_dir) {
    if (uCloudCoverage <= 0.0 || sun_dir.z >= -0.05) return 1.0;
    float alt = -world.z;
    float mid = 0.5 * (uCloudBase + uCloudTop);
    if (alt >= mid) return 1.0;
    // Follow the sun ray from the receiver up to the layer midplane.
    float t = (mid - alt) / -sun_dir.z;
    float d = cloud_density_map(world.xy + sun_dir.xy * t);
    return mix(1.0, 0.28, d);
}
)";

/// @brief Baked ground material lookup (TerrainTextures) plus the close-range
/// detail the bake leaves out.
inline constexpr const char* GROUND_BAKED = R"(
uniform sampler2D uGround0;   // finest level (inner 16 km)
uniform sampler2D uGround1;
uniform sampler2D uGround2;   // coarsest level (192 km)
uniform vec4 uGroundLevels;   // 1 / level edge [1/m] per level
uniform vec2 uGroundCenter;   // world XY the levels are centred on

struct GroundSample {
    vec3  albedo;
    float roughness;
    float water;   // 1 on open water
    float forest;  // 1 under tree canopy (occludes ambient)
};

float level_weight(vec2 uv) {
    vec2 e = min(uv, 1.0 - uv);
    return smoothstep(0.0, 0.05, min(e.x, e.y));
}

GroundSample ground_baked(vec3 world, float dist) {
    // All three levels are always sampled: branching around the fetches
    // would break the implicit derivatives that select mip levels.
    vec2 d = world.xy - uGroundCenter;
    vec2 u0 = d * uGroundLevels.x + 0.5;
    vec2 u1 = d * uGroundLevels.y + 0.5;
    vec2 u2 = d * uGroundLevels.z + 0.5;
    vec4 t = texture(uGround2, u2);
    t = mix(t, texture(uGround1, u1), level_weight(u1));
    t = mix(t, texture(uGround0, u0), level_weight(u0));

    GroundSample g;
    g.albedo = t.rgb * t.rgb;
    g.water = step(0.975, t.a);
    g.forest = (1.0 - g.water) * clamp(t.a / 0.95, 0.0, 1.0);
    float lum = dot(g.albedo, vec3(0.2126, 0.7152, 0.0722));
    g.roughness = mix(mix(0.92, 0.55, smoothstep(0.35, 0.6, lum)), 0.06, g.water);

    // Close-range grain and tree crowns, finer than the finest level.
    float near_fade = 1.0 - smoothstep(1500.0, 6000.0, dist);
    if (near_fade > 0.0) {
        vec2 xy = world.xy;
        float micro = vnoise(xy * 0.045) * 0.65 + vnoise(xy * 0.20) * 0.35;
        g.albedo *= mix(1.0, 0.88 + 0.24 * micro, near_fade);
        if (g.forest > 0.05) {
            float crowns = vnoise(xy * 0.13) * 0.6 + vnoise(xy * 0.41) * 0.4;
            g.albedo *= mix(1.0, 0.7 + 0.6 * crowns, g.forest * near_fade);
        }
    }
    return g;
}
)";

/// @brief Procedural ground material shared by the terrain and the airfield
/// apron, so the two meet with no visible edge.
inline constexpr const char* TERRAIN_MATERIAL = R"(
struct GroundSample {
    vec3  albedo;
    float roughness;
    float water;
    float forest;
};

const vec2 FIELD_CENTER = vec2(1500.0, 0.0);

/// Ground albedo at a world position. `h` is elevation [m], `level` is the
/// up-component of the surface normal (1 = level), `dist` the view distance.
GroundSample ground_material(vec3 world, float h, float level, float dist) {
    GroundSample g;
    vec2 xy = world.xy;
    float slope = 1.0 - level;
    float near_fade = 1.0 - smoothstep(2500.0, 9000.0, dist);

    // Natural base: valley grass through steppe to scree with elevation.
    vec3 lowland = vec3(0.060, 0.105, 0.040);
    vec3 meadow  = vec3(0.092, 0.132, 0.052);
    vec3 upland  = vec3(0.140, 0.128, 0.078);
    vec3 scree   = vec3(0.128, 0.122, 0.112);
    vec3 rock    = vec3(0.150, 0.142, 0.132);
    vec3 snow    = vec3(0.800, 0.830, 0.880);
    vec3 col = mix(lowland, meadow, smoothstep(0.0, 110.0, h));
    col = mix(col, upland, smoothstep(100.0, 260.0, h));
    col = mix(col, scree, smoothstep(260.0, 390.0, h));
    col = mix(col, rock, smoothstep(390.0, 520.0, h));

    // Airfield infield: mown grass inside the perimeter, with no crops, woods
    // or water, blending out over a few hundred metres beyond the apron.
    vec2 fence_d = abs(xy - FIELD_CENTER) - vec2(2250.0, 1000.0);
    float fence = length(max(fence_d, 0.0)) + min(max(fence_d.x, fence_d.y), 0.0);
    float infield = 1.0 - smoothstep(0.0, 350.0, fence);

    // Farmland: blocks on a gently warped grid, each divided into one to
    // four strips along a random axis, the way enclosed fields are laid out.
    // Blocks are bounded by hedgerows, strips by paler headland tracks.
    const mat2 FIELD_ROT = mat2(0.866, 0.5, -0.5, 0.866);
    const vec2 BLOCK = vec2(620.0, 410.0); // [m]
    vec2 fp = FIELD_ROT * xy;
    fp += (vec2(vnoise(xy * 0.0006), vnoise(xy * 0.0006 + 7.3)) - 0.5) * 300.0;
    vec2 bcell = floor(fp / BLOCK);
    vec2 bf = fp / BLOCK - bcell;
    ivec2 bid = ivec2(bcell);
    float strips = floor(1.0 + hash21(bid + ivec2(9, 3)) * 3.999);
    bool along_x = hash21(bid + ivec2(2, 11)) > 0.5;
    float coord = along_x ? bf.x : bf.y;
    float strip = floor(coord * strips);
    ivec2 fid = bid * 4 + ivec2(int(strip), along_x ? 1 : 2);
    float crop = hash21(fid);
    vec3 crops[6] = vec3[6](
        vec3(0.058, 0.102, 0.038),  // pasture
        vec3(0.078, 0.112, 0.040),  // young cereal
        vec3(0.140, 0.126, 0.068),  // ripe cereal
        vec3(0.092, 0.078, 0.056),  // ploughed soil
        vec3(0.098, 0.106, 0.052),  // hay
        vec3(0.070, 0.098, 0.044)); // grass ley
    vec3 field_col = crops[int(crop * 5.999)];
    field_col *= 0.90 + 0.20 * hash21(fid + ivec2(31, 7));
    // Drill rows / plough furrows along each strip, only up close.
    vec2 rd = along_x ? FIELD_ROT[1] : FIELD_ROT[0];
    float rows = 0.5 + 0.5 * sin(dot(xy, rd) * 1.6);
    field_col *= mix(1.0, 0.86 + 0.14 * rows, near_fade * step(0.33, crop) * step(crop, 0.83));
    // Distance to the block edge and to the strip edge [m].
    vec2 edge_m = min(bf, 1.0 - bf) * BLOCK;
    float within = fract(coord * strips);
    float strip_m = min(within, 1.0 - within) * (along_x ? BLOCK.x : BLOCK.y) / strips;
    float hedge = 1.0 - smoothstep(3.0, 9.0, min(edge_m.x, edge_m.y));
    float track = (1.0 - smoothstep(1.5, 4.0, strip_m)) * step(1.5, strips);
    field_col = mix(field_col, vec3(0.105, 0.098, 0.070), track * 0.6);
    field_col = mix(field_col, vec3(0.030, 0.052, 0.020), hedge * 0.85);
    float field_mask = smoothstep(200.0, 40.0, h) * smoothstep(0.62, 0.90, level) * (1.0 - infield);
    col = mix(col, field_col, field_mask * 0.85);
    // Mowing stripes run along the runway.
    vec3 mown = vec3(0.066, 0.094, 0.043) * (1.0 + 0.08 * near_fade * sign(sin(xy.y * 0.21)));
    col = mix(col, mown, infield);

    // Woodland: blotches on the lower slopes and in field corners, with
    // canopy clumping that reads as individual crowns up close.
    float wood_n = fbm(xy * (1.0 / 2600.0) + 41.0, 4);
    float wood = smoothstep(0.56, 0.62, wood_n + slope * 0.25) * smoothstep(480.0, 330.0, h) * (1.0 - infield);
    float crowns = vnoise(xy * 0.13) * 0.6 + vnoise(xy * 0.41) * 0.4;
    vec3 wood_col = mix(vec3(0.020, 0.040, 0.016), vec3(0.042, 0.068, 0.026), crowns);
    col = mix(col, wood_col, wood * 0.92);

    // Rock strata on steep faces.
    float strata_n = vnoise(xy * 0.008) * 14.0;
    float strata = sin((world.z + strata_n) * 0.20) * 0.5 + 0.5;
    vec3 rock_tint = mix(rock, rock * 0.78 + vec3(0.012, 0.012, 0.008), strata);
    float rock_mask = smoothstep(0.24, 0.58, slope);
    col = mix(col, rock_tint, rock_mask);

    // Snow where it can lie: high and not too steep, with a ragged line.
    float snow_line = smoothstep(520.0, 600.0, h + (vnoise(xy * 0.004) - 0.5) * 70.0)
                    * smoothstep(0.50, 0.80, level);
    col = mix(col, snow, snow_line);

    // Lakes in low, level ground away from the airfield basin.
    float lake_n = fbm(xy * (1.0 / 5200.0) + 97.0, 4);
    float away = smoothstep(4200.0, 6000.0, length(xy - FIELD_CENTER));
    float water = smoothstep(0.705, 0.715, lake_n) * step(h, 1.5) * smoothstep(0.985, 0.995, level) * away;
    float shore = smoothstep(0.68, 0.705, lake_n) * (1.0 - water) * step(h, 1.5) * away;
    col = mix(col, vec3(0.110, 0.100, 0.070), shore * 0.7);

    // Broad mottling and fine grain.
    float macro = vnoise(xy * 0.0014) * 0.55 + vnoise(xy * 0.0055) * 0.45;
    float micro = (vnoise(xy * 0.045) * 0.65 + vnoise(xy * 0.20) * 0.35);
    col *= (0.84 + 0.32 * macro) * mix(1.0, 0.90 + 0.20 * micro, near_fade);

    g.albedo = mix(col, vec3(0.012, 0.022, 0.026), water);
    g.roughness = mix(mix(0.92, 0.55, snow_line), 0.06, water);
    g.water = water;
    g.forest = wood * (1.0 - snow_line) * (1.0 - rock_mask);
    return g;
}
)";

} // namespace fastjet::graphics::glsl
