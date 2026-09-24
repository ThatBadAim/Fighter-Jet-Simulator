#pragma once

#include "fastjet/graphics/gl_common.hpp"
#include <array>
#include <cstdio>
#include <string>
#include <vector>

namespace fastjet::graphics {

/// @brief Per-pass GPU timing from timestamp queries.
///
/// Each frame records a timestamp at every mark(); the time between two marks
/// is charged to the earlier one's pass. Queries are kept in a ring several
/// frames deep and read back only once the GPU reports them available, so
/// profiling never stalls the pipeline. Disabled, every call returns at once.
class GpuProfiler {
public:
    static constexpr int kMaxMarks = 24;
    static constexpr int kFramesInFlight = 4;

    GpuProfiler() = default;
    ~GpuProfiler() { destroy(); }
    GpuProfiler(const GpuProfiler&) = delete;
    GpuProfiler& operator=(const GpuProfiler&) = delete;

    void set_enabled(bool on) {
        if (on && !queries_created_) {
            for (auto& f : frames_) glGenQueries(kMaxMarks, f.queries.data());
            queries_created_ = true;
        }
        enabled_ = on;
    }

    [[nodiscard]] bool enabled() const noexcept { return enabled_; }

    void destroy() noexcept {
        if (queries_created_) {
            for (auto& f : frames_) glDeleteQueries(kMaxMarks, f.queries.data());
            queries_created_ = false;
        }
        enabled_ = false;
    }

    /// @brief Starts a frame, harvesting any earlier frame the GPU has finished.
    void begin_frame() {
        if (!enabled_) return;
        cur_ = (cur_ + 1) % kFramesInFlight;
        Frame& f = frames_[static_cast<size_t>(cur_)];
        if (f.count > 0) harvest(f);
        f.count = 0;
    }

    /// @brief Closes the previous pass and opens `name`.
    void mark(const char* name) {
        if (!enabled_) return;
        Frame& f = frames_[static_cast<size_t>(cur_)];
        if (f.count >= kMaxMarks) return;
        glQueryCounter(f.queries[static_cast<size_t>(f.count)], GL_TIMESTAMP);
        f.names[static_cast<size_t>(f.count)] = name;
        ++f.count;
    }

    /// @brief Closes the last pass of the frame.
    void end_frame() { mark(nullptr); }

    /// @brief Mean milliseconds per pass since the last report, one line each.
    [[nodiscard]] std::string report() {
        std::string out;
        char line[96];
        double total = 0.0;
        for (const Pass& p : passes_) {
            if (p.samples == 0) continue;
            const double mean = p.total_ms / p.samples;
            total += mean;
            std::snprintf(line, sizeof(line), "    gpu %-14s %6.2f ms\n", p.name.c_str(), mean);
            out += line;
        }
        std::snprintf(line, sizeof(line), "    gpu %-14s %6.2f ms\n", "TOTAL", total);
        out += line;
        for (Pass& p : passes_) { p.total_ms = 0.0; p.samples = 0; }
        return out;
    }

private:
    struct Frame {
        std::array<GLuint, kMaxMarks> queries{};
        std::array<const char*, kMaxMarks> names{};
        int count = 0;
    };
    struct Pass {
        std::string name;
        double total_ms = 0.0;
        int samples = 0;
    };

    void harvest(Frame& f) {
        GLint ready = 0;
        glGetQueryObjectiv(f.queries[static_cast<size_t>(f.count - 1)], GL_QUERY_RESULT_AVAILABLE, &ready);
        if (!ready) return;  // still in flight: drop this sample rather than wait
        GLuint64 prev = 0;
        for (int i = 0; i < f.count; ++i) {
            GLuint64 t = 0;
            glGetQueryObjectui64v(f.queries[static_cast<size_t>(i)], GL_QUERY_RESULT, &t);
            if (i > 0 && f.names[static_cast<size_t>(i - 1)]) {
                Pass& p = pass(f.names[static_cast<size_t>(i - 1)]);
                p.total_ms += static_cast<double>(t - prev) / 1.0e6;
                ++p.samples;
            }
            prev = t;
        }
    }

    Pass& pass(const char* name) {
        for (Pass& p : passes_) {
            if (p.name == name) return p;
        }
        passes_.push_back(Pass{name});
        return passes_.back();
    }

    std::array<Frame, kFramesInFlight> frames_{};
    std::vector<Pass> passes_;
    int cur_ = 0;
    bool enabled_ = false;
    bool queries_created_ = false;
};

} // namespace fastjet::graphics
