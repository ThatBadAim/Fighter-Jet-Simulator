#pragma once

#include <SDL3/SDL.h>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <iostream>
#include "fastjet/graphics/cockpit_telemetry.hpp"

namespace fastjet::audio {

/// @brief Represents an authentic audio sample loaded from a .wav file
struct AudioSample {
    std::vector<float> pcm;    // Interleaved stereo 44.1kHz float32 samples
    bool loaded        = false;
    double playhead    = 0.0;
    bool is_playing    = false;
    bool loop          = true;
    float current_gain = 0.0f;
    float current_rate = 1.0f; // Resampling playback speed ratio (pitch)

    size_t frame_count() const noexcept {
        return pcm.size() / 2;
    }
};

/// @brief Authentic sample-based cockpit audio engine using SDL3
/// Replaces procedural synthetic math with real recorded .wav audio samples:
/// - F-16 interior turbofan spool loop (engine_spool.wav)
/// - F110 afterburner combustion rumble (afterburner.wav)
/// - High-speed canopy boundary-layer wind rush (wind_rush.wav)
/// - Touchdown tire screech chirp (touchdown.wav)
/// - Landing gear hydraulic transit clunk (gear_transit.wav)
/// - Cockpit aural warning tones / Betty (over_g.wav, stall.wav, pull_up.wav)
///
/// Radar warning receiver tones are synthesised, because in the jet they are
/// synthesised too: the RWR drives the headset with electronic tones, not
/// recordings. New emitter: three short chirps. Lock: a steady pulsed beep.
/// Missile launch: a fast two-tone warble that holds while the missile is guided.
///
/// NOTE: If sound files are not installed in assets/sounds/, the recorded
/// channels stay silent (no procedural substitutes for engine or airflow).
///
/// Threading: mixing runs on SDL's audio thread, in the stream's get
/// callback, so playback never depends on the frame rate: a long frame
/// (loading, a dragged window) cannot starve the device and crackle. The
/// main thread only sets playback parameters, holding the stream lock that
/// SDL also holds around the callback.
class AudioEngine {
public:
    static constexpr int SAMPLE_RATE = 44100;
    static constexpr int CHANNELS    = 2;

private:
    SDL_AudioDeviceID device_id_ = 0;
    SDL_AudioStream* stream_     = nullptr;
    bool initialized_            = false;
    bool enabled_                = true;

    // Authentic audio sample slots
    AudioSample sfx_spool_;        // Looped engine spool
    AudioSample sfx_afterburner_;  // Looped afterburner roar
    AudioSample sfx_wind_;         // Looped canopy wind rush
    AudioSample sfx_touchdown_;    // One-shot tire touchdown chirp
    AudioSample sfx_gear_;         // One-shot gear transit clunk
    AudioSample sfx_over_g_;       // Over-G aural warning
    AudioSample sfx_stall_;        // Stall warning horn
    AudioSample sfx_pullup_;       // Betty "Pull Up" alert

    // Mix bus gains: continuous engine/airflow loops vs. alerts and one-shots
    float engine_bus_gain_ = 1.0f;
    float alerts_bus_gain_ = 1.0f;

    // Mix buffer for the audio thread, allocated once at init.
    std::vector<float> mix_buffer_;
    static constexpr int kMixChunkFrames = 1024;

    // RWR tone generator (audio thread reads, main thread sets under the stream lock)
    enum class RwrTone : uint8_t { NONE, LOCK, LAUNCH };
    RwrTone rwr_tone_ = RwrTone::NONE;
    int rwr_chirps_left_ = 0;     // New-emitter chirps still to play
    double rwr_clock_ = 0.0;      // Seconds into the current pattern
    double rwr_phase_ = 0.0;      // Oscillator phase [cycles]
    float rwr_env_ = 0.0f;        // Click-free on/off envelope
    int prev_rwr_level_ = 0;

    // State transition tracking
    bool prev_on_ground_     = false;
    bool prev_gear_deployed_ = true;
    bool prev_over_g_        = false;
    bool prev_stall_         = false;

public:
    AudioEngine() = default;

    ~AudioEngine() {
        destroy();
    }

    /// @brief Initialize SDL3 audio playback device and load authentic .wav assets
    bool init() {
        if (initialized_) return true;

        if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
            return false;
        }

        SDL_AudioSpec spec{};
        spec.format   = SDL_AUDIO_F32;
        spec.channels = CHANNELS;
        spec.freq     = SAMPLE_RATE;

        device_id_ = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
        if (!device_id_) {
            return false;
        }

        stream_ = SDL_CreateAudioStream(&spec, &spec);
        if (!stream_) {
            SDL_CloseAudioDevice(device_id_);
            device_id_ = 0;
            return false;
        }

        if (!SDL_BindAudioStream(device_id_, stream_)) {
            SDL_DestroyAudioStream(stream_);
            SDL_CloseAudioDevice(device_id_);
            stream_ = nullptr;
            device_id_ = 0;
            return false;
        }

        // Load authentic audio samples from disk if present. Done before the
        // mixer starts, so the audio thread never sees a half-loaded sample.
        load_sample("assets/sounds/engine_spool.wav", sfx_spool_, true);
        load_sample("assets/sounds/afterburner.wav", sfx_afterburner_, true);
        load_sample("assets/sounds/wind_rush.wav", sfx_wind_, true);
        load_sample("assets/sounds/touchdown.wav", sfx_touchdown_, false);
        load_sample("assets/sounds/gear_transit.wav", sfx_gear_, false);
        load_sample("assets/sounds/over_g.wav", sfx_over_g_, false);
        load_sample("assets/sounds/stall.wav", sfx_stall_, false);
        load_sample("assets/sounds/pull_up.wav", sfx_pullup_, false);

        int loaded_count = (sfx_spool_.loaded ? 1 : 0) +
                           (sfx_afterburner_.loaded ? 1 : 0) +
                           (sfx_wind_.loaded ? 1 : 0) +
                           (sfx_touchdown_.loaded ? 1 : 0) +
                           (sfx_gear_.loaded ? 1 : 0) +
                           (sfx_over_g_.loaded ? 1 : 0) +
                           (sfx_stall_.loaded ? 1 : 0) +
                           (sfx_pullup_.loaded ? 1 : 0);

        if (loaded_count > 0) {
            std::cout << "[AUDIO] Loaded " << loaded_count << " authentic .wav sample assets.\n";
        } else {
            std::cout << "[AUDIO] No .wav audio samples in assets/sounds/ - Audio muted cleanly.\n";
        }

        mix_buffer_.assign(static_cast<size_t>(kMixChunkFrames) * CHANNELS, 0.0f);
        initialized_ = true;
        // The mixer always runs: the RWR tones need no sample files. With
        // nothing playing it only writes silence.
        SDL_SetAudioStreamGetCallback(stream_, &AudioEngine::feed, this);
        SDL_ResumeAudioDevice(device_id_);
        return true;
    }

    void destroy() noexcept {
        if (stream_) {
            SDL_DestroyAudioStream(stream_);
            stream_ = nullptr;
        }
        if (device_id_) {
            SDL_CloseAudioDevice(device_id_);
            device_id_ = 0;
        }
        initialized_ = false;
    }

    void set_enabled(bool enabled) noexcept {
        StreamLock lock(stream_);
        enabled_ = enabled;
    }

    /// @brief Final linear gains per mix bus (master volume and mute already
    /// folded in by the caller). Takes effect on the next mixed chunk.
    void set_bus_gains(float engine_bus, float alerts_bus) noexcept {
        StreamLock lock(stream_);
        engine_bus_gain_ = std::clamp(engine_bus, 0.0f, 1.0f);
        alerts_bus_gain_ = std::clamp(alerts_bus, 0.0f, 1.0f);
    }

    bool is_initialized() const noexcept { return initialized_; }

    /// @brief Update audio playback and mix active sample channels into stream
    void update(const graphics::AvionicsTelemetry& tel,
                double dynamic_pressure,
                double airspeed,
                [[maybe_unused]] double frame_dt) {
        if (!initialized_ || !enabled_) return;

        update_rwr(tel);

        // If no authentic samples are installed on disk, run completely silent without queuing buffer
        const bool any_sample_loaded = (sfx_spool_.loaded || sfx_afterburner_.loaded ||
                                        sfx_wind_.loaded || sfx_touchdown_.loaded ||
                                        sfx_gear_.loaded || sfx_over_g_.loaded ||
                                        sfx_stall_.loaded || sfx_pullup_.loaded);
        if (!any_sample_loaded) {
            return;
        }

        // The audio thread reads these parameters while mixing.
        StreamLock lock(stream_);

        // If aircraft is crashed, silence propulsion and aerodynamic rush loops
        if (tel.is_crashed) {
            sfx_spool_.is_playing = false;
            sfx_afterburner_.is_playing = false;
            sfx_wind_.is_playing = false;
        } else {
            // 1. Spool sample parameters
            if (sfx_spool_.loaded) {
                sfx_spool_.is_playing = true;
                const double rpm_norm = std::clamp((tel.engine_rpm_pct - 60.0) / 40.0, 0.0, 1.0);
                sfx_spool_.current_rate = static_cast<float>(0.75 + 0.50 * rpm_norm); // Pitch tracks RPM
                sfx_spool_.current_gain = static_cast<float>(0.30 + 0.45 * rpm_norm); // Volume tracks RPM
            }

            // 2. Afterburner sample parameters
            if (sfx_afterburner_.loaded) {
                const double ab_frac = std::clamp((tel.throttle_input - 0.85) / 0.15, 0.0, 1.0);
                sfx_afterburner_.is_playing = (ab_frac > 0.01);
                sfx_afterburner_.current_gain = static_cast<float>(ab_frac * 0.75);
                sfx_afterburner_.current_rate = 1.0f;
            }

            // 3. Canopy aerodynamic wind rush sample parameters
            if (sfx_wind_.loaded) {
                const double q_norm = std::clamp(dynamic_pressure / 60000.0, 0.0, 1.0);
                sfx_wind_.is_playing = (q_norm > 0.02);
                sfx_wind_.current_gain = static_cast<float>(q_norm * q_norm * 0.60);
                sfx_wind_.current_rate = static_cast<float>(0.85 + 0.35 * q_norm);
            }
        }

        // 4. One-shot transitions
        // Touchdown tire screech
        if (tel.on_ground && !prev_on_ground_ && airspeed > 25.0) {
            play_one_shot(sfx_touchdown_, 0.70f);
        }
        prev_on_ground_ = tel.on_ground;

        // Gear handle transition
        if (tel.gear_deployed != prev_gear_deployed_) {
            play_one_shot(sfx_gear_, 0.60f);
            prev_gear_deployed_ = tel.gear_deployed;
        }

        // Over-G alert tone
        if (tel.over_g_alert && !prev_over_g_) {
            play_one_shot(sfx_over_g_, 0.80f);
        }
        prev_over_g_ = tel.over_g_alert;

        // Stall / High-AoA alert horn
        if (tel.high_aoa_alert && !prev_stall_) {
            play_one_shot(sfx_stall_, 0.75f);
        }
        prev_stall_ = tel.high_aoa_alert;
    }

private:
    /// @brief Pick the RWR tone from the combat telemetry. The highest threat
    /// wins; a new emitter (clear -> anything) starts the three-chirp alert.
    void update_rwr(const graphics::AvionicsTelemetry& tel) noexcept {
        const int level = (tel.combat.active && tel.combat.rwr_active && !tel.is_crashed) ? tel.combat.rwr_level : 0;
        StreamLock lock(stream_);
        const RwrTone want = level >= 3 ? RwrTone::LAUNCH : level == 2 ? RwrTone::LOCK : RwrTone::NONE;
        if (want != rwr_tone_) {
            rwr_tone_ = want;
            rwr_clock_ = 0.0;
        }
        if (level > 0 && prev_rwr_level_ == 0) {
            rwr_chirps_left_ = 3;
            if (rwr_tone_ == RwrTone::NONE) rwr_clock_ = 0.0;
        }
        if (level == 0) rwr_chirps_left_ = 0;
        prev_rwr_level_ = level;
    }

    /// @brief Synthesise the RWR tone into @p out (audio thread).
    void mix_rwr(float* out, int num_samples) noexcept {
        constexpr double DT = 1.0 / SAMPLE_RATE;
        constexpr float GAIN = 0.22f;
        const bool chirping = rwr_tone_ == RwrTone::NONE && rwr_chirps_left_ > 0;
        if (rwr_tone_ == RwrTone::NONE && !chirping && rwr_env_ <= 0.0f) return;
        for (int i = 0; i < num_samples; ++i) {
            double freq = 0.0;
            bool on = false;
            switch (rwr_tone_) {
                case RwrTone::LAUNCH:
                    // Two-tone warble, 12 alternations a second, continuous.
                    freq = std::fmod(rwr_clock_ * 12.0, 1.0) < 0.5 ? 1350.0 : 1800.0;
                    on = true;
                    break;
                case RwrTone::LOCK:
                    // Steady pulsed tone: beep-beep-beep, 5 a second.
                    freq = 1000.0;
                    on = std::fmod(rwr_clock_ * 5.0, 1.0) < 0.5;
                    break;
                case RwrTone::NONE:
                    if (rwr_chirps_left_ > 0) {
                        // New emitter: three 70 ms chirps, 50 ms apart.
                        const double cycle = std::fmod(rwr_clock_, 0.12);
                        freq = 1200.0;
                        on = cycle < 0.07;
                        if (rwr_clock_ >= 0.12 * 3) rwr_chirps_left_ = 0;
                    }
                    break;
            }
            // 3 ms attack/release: no clicks at the pulse edges.
            const float target = on ? 1.0f : 0.0f;
            rwr_env_ += (target - rwr_env_) * 0.0075f;
            if (!on && rwr_env_ < 1e-4f) rwr_env_ = 0.0f;
            if (freq > 0.0) rwr_phase_ = std::fmod(rwr_phase_ + freq * DT, 1.0);
            const double ph = 2.0 * M_PI * rwr_phase_;
            // A little third harmonic gives the hard edge of an electronic tone.
            const float v = static_cast<float>(std::sin(ph) + 0.3 * std::sin(3.0 * ph)) * rwr_env_ * GAIN *
                            alerts_bus_gain_;
            out[i * 2 + 0] = std::clamp(out[i * 2 + 0] + v, -1.0f, 1.0f);
            out[i * 2 + 1] = std::clamp(out[i * 2 + 1] + v, -1.0f, 1.0f);
            rwr_clock_ += DT;
        }
    }

    /// Scoped SDL stream lock (no-op without a stream).
    struct StreamLock {
        SDL_AudioStream* s;
        explicit StreamLock(SDL_AudioStream* stream) : s(stream) { if (s) SDL_LockAudioStream(s); }
        ~StreamLock() { if (s) SDL_UnlockAudioStream(s); }
        StreamLock(const StreamLock&) = delete;
        StreamLock& operator=(const StreamLock&) = delete;
    };

    /// Audio-thread callback: the device wants `additional` more bytes.
    /// SDL holds the stream lock for the duration.
    static void SDLCALL feed(void* userdata, SDL_AudioStream* stream, int additional, int /*total*/) {
        auto* self = static_cast<AudioEngine*>(userdata);
        constexpr int kFrameBytes = CHANNELS * static_cast<int>(sizeof(float));
        int frames = (additional + kFrameBytes - 1) / kFrameBytes;
        while (frames > 0) {
            const int n = std::min(frames, kMixChunkFrames);
            float* buf = self->mix_buffer_.data();
            std::fill(buf, buf + static_cast<size_t>(n) * CHANNELS, 0.0f);
            if (self->enabled_) self->mix_samples(buf, n);
            SDL_PutAudioStreamData(stream, buf, n * kFrameBytes);
            frames -= n;
        }
    }

    void play_one_shot(AudioSample& s, float gain) noexcept {
        if (!s.loaded) return;
        s.playhead = 0.0;
        s.current_gain = gain;
        s.current_rate = 1.0f;
        s.is_playing = true;
    }

    /// @brief Mix active audio sample channels into output buffer with linear interpolation
    void mix_samples(float* out, int num_samples) noexcept {
        struct Channel { AudioSample* sample; float bus_gain; };
        const Channel channels[] = {
            {&sfx_spool_, engine_bus_gain_}, {&sfx_afterburner_, engine_bus_gain_}, {&sfx_wind_, engine_bus_gain_},
            {&sfx_touchdown_, alerts_bus_gain_}, {&sfx_gear_, alerts_bus_gain_}, {&sfx_over_g_, alerts_bus_gain_},
            {&sfx_stall_, alerts_bus_gain_}, {&sfx_pullup_, alerts_bus_gain_}
        };

        for (const Channel& ch : channels) {
            AudioSample* s = ch.sample;
            if (!s->loaded || !s->is_playing || s->pcm.empty()) continue;

            const size_t total_frames = s->frame_count();
            if (total_frames < 2) continue;

            const float gain = s->current_gain * ch.bus_gain;
            const double rate = s->current_rate;

            for (int i = 0; i < num_samples; ++i) {
                const size_t f0 = static_cast<size_t>(s->playhead);
                const size_t f1 = (f0 + 1 < total_frames) ? (f0 + 1) : (s->loop ? 0 : f0);
                const float frac = static_cast<float>(s->playhead - static_cast<double>(f0));

                const float l0 = s->pcm[f0 * 2 + 0];
                const float r0 = s->pcm[f0 * 2 + 1];
                const float l1 = s->pcm[f1 * 2 + 0];
                const float r1 = s->pcm[f1 * 2 + 1];

                const float l_sample = (l0 + (l1 - l0) * frac) * gain;
                const float r_sample = (r0 + (r1 - r0) * frac) * gain;

                out[i * 2 + 0] = std::clamp(out[i * 2 + 0] + l_sample, -1.0f, 1.0f);
                out[i * 2 + 1] = std::clamp(out[i * 2 + 1] + r_sample, -1.0f, 1.0f);

                s->playhead += rate;
                if (s->playhead >= static_cast<double>(total_frames)) {
                    if (s->loop) {
                        s->playhead = std::fmod(s->playhead, static_cast<double>(total_frames));
                    } else {
                        s->is_playing = false;
                        s->playhead = 0.0;
                        break;
                    }
                }
            }
        }
        mix_rwr(out, num_samples);
    }

    /// @brief Load a .wav file from disk and convert to target 44.1kHz stereo Float32 format
    bool load_sample(const std::string& filepath, AudioSample& sample, bool loop) {
        sample.loaded = false;
        sample.loop = loop;
        sample.pcm.clear();

        SDL_AudioSpec src_spec{};
        Uint8* wav_buf = nullptr;
        Uint32 wav_len = 0;

        if (!SDL_LoadWAV(filepath.c_str(), &src_spec, &wav_buf, &wav_len)) {
            return false; // File not present on disk
        }

        SDL_AudioSpec dst_spec{};
        dst_spec.format   = SDL_AUDIO_F32;
        dst_spec.channels = CHANNELS;
        dst_spec.freq     = SAMPLE_RATE;

        // Use SDL3 AudioStream for automatic resampling, channel remixing, and format conversion
        SDL_AudioStream* conv_stream = SDL_CreateAudioStream(&src_spec, &dst_spec);
        if (!conv_stream) {
            SDL_free(wav_buf);
            return false;
        }

        if (!SDL_PutAudioStreamData(conv_stream, wav_buf, static_cast<int>(wav_len)) ||
            !SDL_FlushAudioStream(conv_stream)) {
            SDL_DestroyAudioStream(conv_stream);
            SDL_free(wav_buf);
            return false;
        }
        SDL_free(wav_buf);

        const int available_bytes = SDL_GetAudioStreamAvailable(conv_stream);
        if (available_bytes > 0) {
            const size_t num_floats = static_cast<size_t>(available_bytes) / sizeof(float);
            sample.pcm.resize(num_floats);
            SDL_GetAudioStreamData(conv_stream, sample.pcm.data(), available_bytes);
            sample.loaded = true;
        }

        SDL_DestroyAudioStream(conv_stream);
        return sample.loaded;
    }
};

} // namespace fastjet::audio
