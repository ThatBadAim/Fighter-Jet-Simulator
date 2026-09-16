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
/// NOTE: If sound files are not installed in assets/sounds/, the engine remains
/// completely silent with zero CPU overhead (no harsh procedural buzz or chiptune beeps).
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

        SDL_ResumeAudioDevice(device_id_);

        // Load authentic audio samples from disk if present
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

        initialized_ = true;
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
        enabled_ = enabled;
    }

    bool is_initialized() const noexcept { return initialized_; }

    /// @brief Update audio playback and mix active sample channels into stream
    void update(const graphics::AvionicsTelemetry& tel,
                double dynamic_pressure,
                double airspeed,
                [[maybe_unused]] double frame_dt) {
        if (!initialized_ || !enabled_) return;

        // If no authentic samples are installed on disk, run completely silent without queuing buffer
        const bool any_sample_loaded = (sfx_spool_.loaded || sfx_afterburner_.loaded ||
                                        sfx_wind_.loaded || sfx_touchdown_.loaded ||
                                        sfx_gear_.loaded || sfx_over_g_.loaded ||
                                        sfx_stall_.loaded || sfx_pullup_.loaded);
        if (!any_sample_loaded) {
            return;
        }

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

        // Keep 40-80ms of audio queued in the stream
        constexpr int MIN_QUEUE_BYTES = static_cast<int>(SAMPLE_RATE * CHANNELS * sizeof(float) * 0.04);
        constexpr int CHUNK_SAMPLES   = 1024;

        int queued = SDL_GetAudioStreamQueued(stream_);
        while (queued < MIN_QUEUE_BYTES) {
            std::vector<float> buffer(CHUNK_SAMPLES * CHANNELS, 0.0f);
            mix_samples(buffer.data(), CHUNK_SAMPLES);
            SDL_PutAudioStreamData(stream_, buffer.data(), static_cast<int>(buffer.size() * sizeof(float)));
            queued = SDL_GetAudioStreamQueued(stream_);
        }
    }

private:
    void play_one_shot(AudioSample& s, float gain) noexcept {
        if (!s.loaded) return;
        s.playhead = 0.0;
        s.current_gain = gain;
        s.current_rate = 1.0f;
        s.is_playing = true;
    }

    /// @brief Mix active audio sample channels into output buffer with linear interpolation
    void mix_samples(float* out, int num_samples) noexcept {
        AudioSample* channels[] = {
            &sfx_spool_, &sfx_afterburner_, &sfx_wind_,
            &sfx_touchdown_, &sfx_gear_, &sfx_over_g_, &sfx_stall_, &sfx_pullup_
        };

        for (AudioSample* s : channels) {
            if (!s->loaded || !s->is_playing || s->pcm.empty()) continue;

            const size_t total_frames = s->frame_count();
            if (total_frames < 2) continue;

            const float gain = s->current_gain;
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
