#include "fastjet/graphics/flight_instruments.hpp"
#include <SDL3/SDL.h>
#include <iostream>
#include <cassert>

using namespace fastjet::graphics;
using namespace fastjet::math;
using namespace fastjet::fdm;

void test_fbo_and_instruments_gl() {
    std::cout << "[Test] Flight Instruments: FBO Creation & Render-to-Texture... ";

    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_Window* window = SDL_CreateWindow("Instruments Test", 640, 480, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    assert(window != nullptr);
    SDL_GLContext ctx = SDL_GL_CreateContext(window);
    assert(ctx != nullptr);

    FlightInstruments inst;
    bool ok = inst.init();
    assert(ok && "FlightInstruments failed to initialize FBO or shaders!");

    assert(inst.fbo_id() != 0);
    assert(inst.texture_id() != 0);

    // 1. Test Low AoA condition (alpha = 8 deg) -> bottom chevron
    FlightState state{};
    state.pos_ned = Vector3(0.0, 0.0, -3000.0);
    const double speed = 250.0;
    const double alpha_low = 8.0 * (3.14159265 / 180.0);
    state.vel_b = Vector3(speed * std::cos(alpha_low), 0.0, speed * std::sin(alpha_low));
    inst.update_and_render(state);
    assert(glGetError() == GL_NO_ERROR);

    // 2. Test On-Speed condition (alpha = 13 deg) -> center donut
    const double alpha_onspeed = 13.0 * (3.14159265 / 180.0);
    state.vel_b = Vector3(speed * std::cos(alpha_onspeed), 0.0, speed * std::sin(alpha_onspeed));
    inst.update_and_render(state);
    assert(glGetError() == GL_NO_ERROR);

    // 3. Test High AoA condition (alpha = 18 deg) -> top chevron
    const double alpha_high = 18.0 * (3.14159265 / 180.0);
    state.vel_b = Vector3(speed * std::cos(alpha_high), 0.0, speed * std::sin(alpha_high));
    inst.update_and_render(state);
    assert(glGetError() == GL_NO_ERROR);

    // 4. Test texture binding
    inst.bind_texture(GL_TEXTURE0);
    assert(glGetError() == GL_NO_ERROR);

    inst.destroy();
    SDL_GL_DestroyContext(ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << "PASSED (FBO complete, off-screen rendering and texture binding verified)\n";
}

int main() {
    std::cout << "=== Flight Instruments (Render-to-Texture) Verification ===\n";
    test_fbo_and_instruments_gl();
    std::cout << "All Flight Instruments tests passed successfully!\n\n";
    return 0;
}
