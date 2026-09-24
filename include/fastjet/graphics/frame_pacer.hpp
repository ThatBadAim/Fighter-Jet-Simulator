#pragma once

#include "fastjet/graphics/gl_common.hpp"
#include <array>
#include <cstdint>

namespace fastjet::graphics {

/// @brief Bounds how many frames the CPU may queue ahead of the GPU.
///
/// Without a bound, a GPU-limited loop runs far ahead until the driver's
/// queue fills, then blocks for a long stretch: frames go out in bursts, the
/// measured frame time swings between well under a millisecond and the
/// clamp, and input is sampled long before it is shown. A fence after each
/// swap, waited on kMaxFramesInFlight frames later, keeps the loop in step
/// with what the GPU actually delivers, so frame times are steady and input
/// is fresh. Two frames in flight still lets CPU work overlap the GPU.
class FramePacer {
public:
    static constexpr int kMaxFramesInFlight = 2;

    FramePacer() = default;
    ~FramePacer() { destroy(); }
    FramePacer(const FramePacer&) = delete;
    FramePacer& operator=(const FramePacer&) = delete;

    /// @brief Call at the top of the frame, before input is sampled: waits
    /// until the GPU has finished the frame kMaxFramesInFlight back.
    void wait() {
        GLsync& f = fences_[static_cast<size_t>(slot_)];
        if (!f) return;
        // Flush on the first wait so the fence is certain to be submitted.
        GLbitfield flags = GL_SYNC_FLUSH_COMMANDS_BIT;
        for (;;) {
            const GLenum r = glClientWaitSync(f, flags, kWaitStepNs);
            if (r == GL_ALREADY_SIGNALED || r == GL_CONDITION_SATISFIED || r == GL_WAIT_FAILED) break;
            flags = 0;
        }
        glDeleteSync(f);
        f = nullptr;
    }

    /// @brief Call right after the buffer swap.
    void frame_submitted() {
        GLsync& f = fences_[static_cast<size_t>(slot_)];
        if (f) glDeleteSync(f);
        f = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        // Hand the frame to the GPU now. Not every swap path flushes (an
        // unmapped window does not), and a frame left in the command buffer
        // only reaches the GPU with the next one, so pairs of frames finish
        // together and pacing alternates between long and zero waits.
        glFlush();
        slot_ = (slot_ + 1) % kMaxFramesInFlight;
    }

    void destroy() noexcept {
        for (GLsync& f : fences_) {
            if (f) glDeleteSync(f);
            f = nullptr;
        }
    }

private:
    static constexpr std::uint64_t kWaitStepNs = 100'000'000;  // re-check every 100 ms

    std::array<GLsync, kMaxFramesInFlight> fences_{};
    int slot_ = 0;
};

} // namespace fastjet::graphics
