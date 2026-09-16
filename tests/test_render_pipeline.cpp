#include "fastjet/graphics/render_engine.hpp"
#include <SDL3/SDL.h>
#include <iostream>
#include <chrono>
#include <cassert>

using namespace fastjet::graphics;
using namespace fastjet::math;
using namespace fastjet::fdm;
using namespace fastjet::flcs;

void test_full_render_pipeline() {
    std::cout << "[Test] Render Engine: Multi-Pass Graphics Pipeline (Headless OpenGL)... ";

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        assert(false);
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window* window = SDL_CreateWindow("Fast Jet Sim Render Test", 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    assert(window != nullptr);

    SDL_GLContext ctx = SDL_GL_CreateContext(window);
    assert(ctx != nullptr);

    RenderEngine engine;
    bool ok = engine.init(1280, 720, 60.0f);
    assert(ok && "RenderEngine failed to initialize all graphics passes!");

    FlightState state{};
    state.pos_ned = Vector3(1500.0, 0.0, -1000.0); // 1,000 m altitude over runway
    state.vel_b   = Vector3(250.0, 0.0, 0.0);
    state.q_att   = Quaternion::from_euler(0.1, 0.05, 0.0); // Slight bank and pitch

    IMUData imu{};
    imu.Nz = 4.5; // +4.5 G pull
    imu.Ny = 0.5;
    imu.Nx = 0.2;

    // Run 100 simulation frames to verify temporal stability and measure rendering latency
    const int NUM_FRAMES = 100;
    const auto t_start = std::chrono::high_resolution_clock::now();

    for (int frame = 0; frame < NUM_FRAMES; ++frame) {
        engine.render_frame(0.005, state, imu);
        assert(glGetError() == GL_NO_ERROR && "OpenGL error detected during frame rendering!");
    }

    const auto t_end = std::chrono::high_resolution_clock::now();
    const double elapsed_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
    const double frame_latency_ms = elapsed_ms / NUM_FRAMES;

    std::cout << "PASSED (" << NUM_FRAMES << " frames rendered, Zero GL Errors, "
              << "Latency: " << frame_latency_ms << " ms/frame, "
              << "FPS: " << 1000.0 / frame_latency_ms << ")\n";

    engine.destroy();
    SDL_GL_DestroyContext(ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main() {
    std::cout << "=== Integrated First-Person Graphics & Avionics Pipeline Verification ===\n";
    test_full_render_pipeline();
    std::cout << "All Render Pipeline tests passed successfully!\n\n";
    return 0;
}
