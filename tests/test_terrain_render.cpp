#include "fastjet/graphics/sky_ground_renderer.hpp"
#include "fastjet/graphics/camera_rig.hpp"

#include <SDL3/SDL.h>

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>

using namespace fastjet;
using namespace fastjet::graphics;

namespace {

constexpr int W = 640;
constexpr int H = 360;

struct Frame {
    std::vector<unsigned char> rgb; // top-down, 3 bytes per pixel
    unsigned char at(int x, int y, int c) const {
        return rgb[static_cast<size_t>((y * W + x) * 3 + c)];
    }
};

/// Render one frame of sky + ground from the given pose, with back-face
/// culling enabled exactly as RenderEngine does it.
Frame render(SkyGroundRenderer& env, CameraRig& rig,
             double alt_m, double x_m, double pitch_rad) {
    fdm::FlightState st{};
    st.pos_ned = math::Vector3(x_m, 0.0, -alt_m);
    st.q_att = math::Quaternion::from_euler(0.0, pitch_rad, 0.0);

    const float aspect = static_cast<float>(W) / static_cast<float>(H);
    const Mat4 proj = Mat4::perspective(60.0f * 3.14159265f / 180.0f, aspect, 0.5f, 120000.0f);
    const Mat4 view_world = rig.compute_world_view_matrix(st);
    const Mat4 view_inf = rig.compute_infinity_view_matrix(st);

    glViewport(0, 0, W, H);
    // Magenta clear: any of it left in the frame means something failed to draw.
    glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const Mat4 vp_inf = proj * view_inf;
    env.render_sky(vp_inf.inverse(), static_cast<float>(alt_m));

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    env.render_ground(proj * view_world, rig.eye_pos_ned(st));

    glDisable(GL_CULL_FACE);

    Frame f;
    f.rgb.resize(static_cast<size_t>(W) * H * 3);
    std::vector<unsigned char> raw(f.rgb.size());
    glReadPixels(0, 0, W, H, GL_RGB, GL_UNSIGNED_BYTE, raw.data());
    // glReadPixels is bottom-up; flip so row 0 is the top of the screen.
    for (int y = 0; y < H; ++y) {
        std::memcpy(&f.rgb[static_cast<size_t>((H - 1 - y) * W * 3)],
                    &raw[static_cast<size_t>(y * W * 3)],
                    static_cast<size_t>(W) * 3);
    }
    return f;
}

int count_magenta(const Frame& f) {
    int n = 0;
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            if (f.at(x, y, 0) > 200 && f.at(x, y, 1) < 60 && f.at(x, y, 2) > 200) ++n;
        }
    }
    return n;
}

} // namespace

/// Terrain winding must be counter-clockwise when seen from above. NED is
/// left-handed relative to the screen convention, so this is easy to get
/// backwards; when it is wrong, back-face culling removes the entire world and
/// leaves the clear colour showing.
void test_terrain_survives_back_face_culling(SkyGroundRenderer& env, CameraRig& rig) {
    std::cout << "[Test] Terrain Render: Survives Back-Face Culling... ";

    const Frame f = render(env, rig, 2500.0, -14000.0, -0.45);

    // Looking down from 2.5 km, the lower half of the screen must be terrain.
    const int holes = count_magenta(f);
    assert(holes == 0 && "Culling must not remove the terrain (winding order)");

    // And the bottom of the frame must not be sky-blue: it must be ground.
    long r = 0, g = 0, b = 0;
    int n = 0;
    for (int y = H * 3 / 4; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            r += f.at(x, y, 0);
            g += f.at(x, y, 1);
            b += f.at(x, y, 2);
            ++n;
        }
    }
    r /= n; g /= n; b /= n;
    assert(g > b && "Ground must read green, not sky blue");

    std::printf("PASSED (no culled holes, ground RGB %ld %ld %ld) ", r, g, b);
    std::cout << "\n";
}

/// The runway decals sit 20 cm above a flat basin. Across a 120 km depth range
/// that separation is far below depth-buffer precision, so TerrainMesh cuts a
/// hole for the airfield. If that hole and the apron ever disagree, the seam
/// shows as sky through the ground.
void test_runway_is_visible_over_terrain(SkyGroundRenderer& env, CameraRig& rig) {
    std::cout << "[Test] Terrain Render: Runway Renders Over Terrain... ";

    const Frame f = render(env, rig, 300.0, 500.0, -0.30);

    assert(count_magenta(f) == 0 && "No un-drawn pixels over the airfield");

    // Asphalt is much darker than grass and, unlike grass, is neutral: its red
    // and green channels are nearly equal, while lit terrain is clearly green.
    // Measured values are asphalt ~(71,71,74) against grass ~(100,115,86).
    int dark = 0;
    for (int y = H / 2; y < H; ++y) {
        for (int x = W / 2 - 40; x < W / 2 + 40; ++x) {
            const int r = f.at(x, y, 0);
            const int g = f.at(x, y, 1);
            const int b = f.at(x, y, 2);
            const bool neutral = std::abs(r - g) < 8 && b >= g;
            if (r < 85 && neutral) ++dark;
        }
    }
    assert(dark > 400 && "Runway asphalt must be visible against the terrain");

    // Markings: the painted stripes read near 200 on every channel.
    int bright = 0;
    for (int y = H / 2; y < H; ++y) {
        for (int x = W / 2 - 40; x < W / 2 + 40; ++x) {
            if (f.at(x, y, 0) > 150 && f.at(x, y, 1) > 150 && f.at(x, y, 2) > 140) ++bright;
        }
    }
    assert(bright > 20 && "Runway markings must be visible");

    std::printf("PASSED (%d asphalt px, %d marking px) ", dark, bright);
    std::cout << "\n";
}

/// Sky and terrain must meet without a visible step. They share one
/// sky_radiance() function precisely so this holds; a regression that gives
/// the haze its own colour shows up here as a hard band.
void test_horizon_has_no_seam(SkyGroundRenderer& env, CameraRig& rig) {
    std::cout << "[Test] Terrain Render: Horizon Blends Without A Seam... ";

    // Look along the flat basin from low altitude. The ground/sky boundary here
    // is the far haze limit rather than a nearby ridge, so any step in the
    // column is a shading discontinuity rather than real terrain geometry.
    const Frame f = render(env, rig, 900.0, -60000.0, -0.02);

    // Locate the last row that is still pure sky: sky is uniform, so walk down
    // until the colour first departs from the value at the top of the frame.
    const int sky_r = f.at(W / 2, 4, 0);
    const int sky_g = f.at(W / 2, 4, 1);
    const int sky_b = f.at(W / 2, 4, 2);

    int first_ground = -1;
    for (int y = 5; y < H; ++y) {
        const int d = std::abs(static_cast<int>(f.at(W / 2, y, 0)) - sky_r) +
                      std::abs(static_cast<int>(f.at(W / 2, y, 1)) - sky_g) +
                      std::abs(static_cast<int>(f.at(W / 2, y, 2)) - sky_b);
        if (d > 10) { first_ground = y; break; }
    }
    assert(first_ground > 0 && "Terrain must appear somewhere below the horizon");

    // The largest single-row step anywhere in the transition. Sharing one
    // sky_radiance() between the sky quad and the terrain haze keeps this small;
    // before that change the same measurement was ~111.
    int worst = 0;
    const int hi = (first_ground + 10 < H) ? first_ground + 10 : H;
    for (int y = first_ground; y < hi; ++y) {
        const int d =
            std::abs(static_cast<int>(f.at(W / 2, y, 0)) - static_cast<int>(f.at(W / 2, y - 1, 0))) +
            std::abs(static_cast<int>(f.at(W / 2, y, 1)) - static_cast<int>(f.at(W / 2, y - 1, 1))) +
            std::abs(static_cast<int>(f.at(W / 2, y, 2)) - static_cast<int>(f.at(W / 2, y - 1, 2)));
        if (d > worst) worst = d;
    }

    assert(worst < 45 && "Horizon must not show a hard colour step");

    std::printf("PASSED (largest step %d entering terrain at row %d) ", worst, first_ground);
    std::cout << "\n";
}

/// The sky must darken with altitude and be brighter at the horizon than the
/// zenith. A flat or inverted gradient means the scattering model is broken.
void test_sky_gradient_and_altitude_response(SkyGroundRenderer& env, CameraRig& rig) {
    std::cout << "[Test] Terrain Render: Sky Gradient And Altitude Response... ";

    // Look up so the frame is all sky.
    const Frame low  = render(env, rig, 1000.0,  -40000.0, 0.75);
    const Frame high = render(env, rig, 16000.0, -40000.0, 0.75);

    auto blue_at = [](const Frame& f, int y) {
        long s = 0;
        for (int x = 0; x < W; ++x) s += f.at(x, y, 2);
        return s / W;
    };

    // Top of frame looks nearer the zenith than the bottom.
    const long zenith_low  = blue_at(low, 10);
    const long horizon_low = blue_at(low, H - 10);
    assert(horizon_low > zenith_low && "Horizon must be paler than the zenith");

    // Climbing thins the air, so the same direction gets darker.
    const long zenith_high = blue_at(high, 10);
    assert(zenith_high < zenith_low && "Sky must darken with altitude");

    std::printf("PASSED (zenith %ld->%ld climbing, horizon %ld) ",
                zenith_low, zenith_high, horizon_low);
    std::cout << "\n";
}

int main() {
    std::cout << "=== Procedural Terrain & Atmosphere Render Verification ===\n";

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        return 1;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window* window = SDL_CreateWindow("Terrain Render Test", W, H,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    assert(window != nullptr);
    SDL_GLContext ctx = SDL_GL_CreateContext(window);
    assert(ctx != nullptr);

    SkyGroundRenderer env;
    CameraRig rig;
    const bool ok = env.init();
    assert(ok && "Sky/terrain renderer must initialise");
    assert(env.terrain_vertex_count() > 10000 && "Terrain mesh must be built");

    test_terrain_survives_back_face_culling(env, rig);
    test_runway_is_visible_over_terrain(env, rig);
    test_horizon_has_no_seam(env, rig);
    test_sky_gradient_and_altitude_response(env, rig);

    assert(glGetError() == GL_NO_ERROR && "No OpenGL errors during terrain tests");

    env.destroy();
    SDL_GL_DestroyContext(ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << "All Terrain Render tests passed successfully!\n\n";
    return 0;
}
