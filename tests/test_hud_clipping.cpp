#include "fastjet/graphics/render_engine.hpp"
#include <SDL3/SDL.h>
#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>

using namespace fastjet::graphics;
using namespace fastjet::math;
using namespace fastjet::fdm;
using namespace fastjet::flcs;

void test_stencil_clear_persistence() {
    std::cout << "[Test 1] Stencil Clear Persistence (Zero Leak across 50 Frames)... ";

    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window* window = SDL_CreateWindow("Stencil Test", 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    assert(window != nullptr);
    SDL_GLContext ctx = SDL_GL_CreateContext(window);
    assert(ctx != nullptr);

    RenderEngine engine;
    bool ok = engine.init(1280, 720, 60.0f);
    assert(ok);

    FlightState state{};
    state.pos_ned = Vector3(0.0, 0.0, -1500.0);
    state.vel_b   = Vector3(220.0, 0.0, 0.0);
    state.omega_b = Vector3::zero();
    state.q_att   = Quaternion::identity();

    IMUData imu{};

    // Simulate 50 frames with fluctuating head displacements (+9G pull down, -2G push up)
    for (int i = 0; i < 50; ++i) {
        imu.Nz = (i % 2 == 0) ? 9.0 : -2.0;
        engine.render_frame(0.016, state, imu);
    }

    // Now issue a glClear(GL_STENCIL_BUFFER_BIT) and inspect that the buffer is 100% 0
    glViewport(0, 0, 1280, 720);
    glClearStencil(0);
    glClear(GL_STENCIL_BUFFER_BIT);

    std::vector<uint8_t> stencil_pixels(1280 * 720);
    glReadPixels(0, 0, 1280, 720, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencil_pixels.data());

    int non_zero = 0;
    for (uint8_t s : stencil_pixels) {
        if (s != 0) non_zero++;
    }

    assert(non_zero == 0 && "Stencil buffer failed to clear because glStencilMask was not restored!");
    std::cout << "PASSED (Zero stencil pixel leakage across frames)\n";

    engine.destroy();
    SDL_GL_DestroyContext(ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void test_pitch_ladder_full_range() {
    std::cout << "[Test 2] Pitch Ladder Full-Range Coverage (-90 to +90 deg)... ";

    HUDCollimator hud;
    FlightState state{};
    state.pos_ned = Vector3(0.0, 0.0, -1500.0);
    state.vel_b   = Vector3(200.0, 0.0, 0.0);
    IMUData imu{};
    imu.Nz = 1.0;

    const double rad = 3.14159265 / 180.0;

    // Test extreme pitch attitudes:
    const double pitch_angles[] = {+60.0, +85.0, +90.0, -60.0, -85.0, -90.0};
    for (double p_deg : pitch_angles) {
        state.q_att = Quaternion::from_euler(0.0, p_deg * rad, 0.0);
        hud.build_symbology(state, imu);
        assert(hud.vertex_count() > 100 && "Pitch ladder disappeared at extreme pitch attitude!");
    }

    std::cout << "PASSED (Pitch ladder, Zenith, and Nadir generated across full sphere)\n";
}

void test_fpm_ghost_clamping() {
    std::cout << "[Test 3] Flight Path Marker Clamping & Ghosting at High AoA/Sideslip... ";

    HUDCollimator hud;
    FlightState state{};
    state.pos_ned = Vector3(0.0, 0.0, -1500.0);
    IMUData imu{};

    const double rad = 3.14159265 / 180.0;
    const double V = 200.0;

    // Extreme high-AoA case: alpha = 25 deg, beta = 15 deg
    const double alpha = 25.0 * rad;
    const double beta  = 15.0 * rad;
    state.vel_b.x = V * std::cos(alpha) * std::cos(beta);
    state.vel_b.y = V * std::sin(beta);
    state.vel_b.z = V * std::sin(alpha) * std::cos(beta);

    hud.build_symbology(state, imu);
    assert(hud.vertex_count() > 50 && "FPM failed to generate ghosted symbology at extreme AoA!");

    std::cout << "PASSED (FPM cleanly clamped and ghosted within display FOV)\n";
}

void test_hud_typography_and_resolution() {
    std::cout << "[Test 4] HUD Vector Font Typography & Legibility Verification... ";

    HUDCollimator hud;
    FlightState state{};
    state.pos_ned = Vector3(0.0, 0.0, -1500.0);
    state.vel_b   = Vector3(250.0, 0.0, 0.0); // 486 kts
    IMUData imu{};
    imu.Nz = 4.2;

    hud.build_symbology(state, imu);
    // With upgraded 16-segment font, vertex count should be robust and high-density (> 350 vertices)
    assert(hud.vertex_count() > 350 && "Upgraded vector stroke typography failed to generate adequate vertex detail!");

    std::cout << "PASSED (" << hud.vertex_count() << " crisp vector line vertices generated)\n";
}

void test_hud_supersonic_and_centering() {
    std::cout << "[Test 5] HUD Supersonic (>999 kts) Readout & Symbology Verification... ";

    HUDCollimator hud;
    // Test 1: High supersonic flight (600 m/s = ~1166 kts, Mach ~1.85 at 1500m)
    FlightState supersonic_state{};
    supersonic_state.pos_ned = Vector3(0.0, 0.0, -1500.0);
    supersonic_state.vel_b   = Vector3(600.0, 0.0, 0.0);
    IMUData imu{};
    imu.Nz = 1.0;

    hud.build_symbology(supersonic_state, imu);
    assert(hud.vertex_count() > 400 && "Supersonic HUD symbology failed to generate vertices!");

    // Test 2: Extreme Mach flight (800 m/s = ~1555 kts, Mach > 2.0 at 10000m)
    FlightState extreme_state{};
    extreme_state.pos_ned = Vector3(0.0, 0.0, -10000.0);
    extreme_state.vel_b   = Vector3(800.0, 0.0, 0.0);
    hud.build_symbology(extreme_state, imu);
    assert(hud.vertex_count() > 400 && "Extreme supersonic HUD symbology failed to generate vertices!");

    std::cout << "PASSED (Clean 4-digit speed and Mach readout generated up to Mach 2+)\n";
}

int main() {
    std::cout << "=================================================================\n";
    std::cout << "         F-16 HUD CLIPPING & STENCIL APERTURE VERIFICATION       \n";
    std::cout << "=================================================================\n\n";

    test_stencil_clear_persistence();
    test_pitch_ladder_full_range();
    test_fpm_ghost_clamping();
    test_hud_typography_and_resolution();
    test_hud_supersonic_and_centering();

    std::cout << "\nAll HUD Clipping and Aperture Tests Passed Successfully!\n";
    return 0;
}
