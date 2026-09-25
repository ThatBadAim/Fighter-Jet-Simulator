#pragma once

#include "fastjet/sim/world.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace fastjet::sim {

/// @brief One search return ("brick"): where and when the beam painted a jet.
/// Angles are in the stabilised scan frame: azimuth from the ownship's heading
/// (positive right), elevation from the local horizon.
struct RadarHit {
    int target{-1};
    int frame{0};         ///< Scan frame that painted it
    double az_rad{0.0};
    double el_rad{0.0};
    double range_m{0.0};
    double alt_m{0.0};
};

/**
 * @brief The pilot's air-to-air radar, as an APG-68-class set in combined
 * radar mode: range-while-search (RWS) and single-target track (STT).
 *
 * RWS sweeps a roll- and pitch-stabilised beam through a 4-bar, +/-60 deg
 * raster at 65 deg/s, so a full frame takes about 7.4 s. A jet is painted
 * once per bar that the beam crosses it, and only if the radar could see it
 * then: inside detection range, inside the gimbal and out of the doppler
 * notch (the same SensorLimits test the missiles and the AI's radar use).
 * Each paint leaves a brick where the jet was; bricks age out after a few
 * frames, so a moving jet leaves a short trail of history. What RWS cannot
 * tell the pilot is heading, speed or height.
 *
 * Designating a brick commands the jet's FireControlRadar into STT on that
 * target, which is what the target's RWR hears as a lock. STT follows the
 * target continuously, coasts on memory through a notch and, unlike the AI's
 * radar, does not reacquire by itself when the lock breaks: the scope drops
 * back to RWS and the pilot must find the target again.
 */
class RadarScope {
public:
    enum class Mode : uint8_t { OFF, RWS, STT };

    static constexpr std::array<double, 4> kRangeScalesNm{10.0, 20.0, 40.0, 80.0};
    static constexpr int DEFAULT_RANGE_INDEX = 1;
    static constexpr double DEG = M_PI / 180.0;
    static constexpr double SCAN_RATE_RAD_S = 65.0 * DEG;
    static constexpr double AZ_LIMIT_RAD = 60.0 * DEG;
    static constexpr int BARS = 4;
    static constexpr double BAR_SPACING_RAD = 2.2 * DEG;
    static constexpr double BEAM_HALF_RAD = 1.75 * DEG; ///< Half the 3.5 deg pencil beam
    static constexpr double ELEV_LIMIT_RAD = 55.0 * DEG; ///< Scan centre, keeping the top bar inside 60 deg
    static constexpr double ELEV_SLEW_RAD_S = 8.0 * DEG;
    /// Search detection range against a fighter, as a multiple of the lock-on range.
    static constexpr double DETECTION_RANGE_FACTOR = 1.25;
    static constexpr int HISTORY_FRAMES = 3;
    static constexpr int kMaxHits = 32;
    static constexpr double ACQUIRE_S = 0.1;      ///< Designation to STT with the jet in the beam
    static constexpr double LOCK_TIMEOUT_S = 0.5; ///< A designation that finds nothing gives up

    // Scan state
    Mode mode{Mode::OFF};
    int range_index{DEFAULT_RANGE_INDEX};
    double ant_az{-AZ_LIMIT_RAD};
    double ant_el{0.0};
    double el_centre{0.0};   ///< Pilot's antenna elevation (scan centre)
    int bar{0};
    int sweep_dir{1};
    int frame{0};
    std::array<RadarHit, kMaxHits> hits{};
    int hit_count{0};
    bool lock_failed_cue{false}; ///< The last designation found nothing to lock

    [[nodiscard]] bool fitted() const noexcept { return spec_.fitted; }
    [[nodiscard]] double range_scale_m() const noexcept {
        return kRangeScalesNm[static_cast<size_t>(range_index)] * 1852.0;
    }
    [[nodiscard]] double detection_range_m() const noexcept { return spec_.track_range_m * DETECTION_RANGE_FACTOR; }
    [[nodiscard]] int own() const noexcept { return own_; }

    /// @brief Heading of the scan frame: the ownship's nose, projected level.
    [[nodiscard]] static double scan_heading(const fdm::FlightState& s) noexcept {
        const math::Vector3 nose = s.q_att.rotate_body_to_ned(math::Vector3(1.0, 0.0, 0.0));
        return std::atan2(nose.y, nose.x);
    }

    /// @brief Azimuth (from the scan heading, +right) and elevation (from the
    /// horizon) of a point, in the stabilised frame.
    static void stabilised_angles(const fdm::FlightState& own, const math::Vector3& at, double& az,
                                  double& el) noexcept {
        const math::Vector3 r = at - own.pos_ned;
        az = wrap_pi(std::atan2(r.y, r.x) - scan_heading(own));
        el = std::atan2(-r.z, std::hypot(r.x, r.y));
    }

    [[nodiscard]] static double wrap_pi(double a) noexcept {
        while (a > M_PI) a -= 2.0 * M_PI;
        while (a < -M_PI) a += 2.0 * M_PI;
        return a;
    }

    /// @brief Elevation of bar @p b (bar 0 is the top one).
    [[nodiscard]] double bar_elevation(int b) const noexcept {
        return el_centre + (0.5 * (BARS - 1) - b) * BAR_SPACING_RAD;
    }
    /// @brief Upper and lower edges of the scanned elevation block.
    [[nodiscard]] double coverage_top_rad() const noexcept { return bar_elevation(0) + BEAM_HALF_RAD; }
    [[nodiscard]] double coverage_bottom_rad() const noexcept { return bar_elevation(BARS - 1) - BEAM_HALF_RAD; }

    // ---------------------------------------------------------------------
    // Pilot controls
    // ---------------------------------------------------------------------

    void range_up() noexcept { range_index = std::min(range_index + 1, static_cast<int>(kRangeScalesNm.size()) - 1); }
    void range_down() noexcept { range_index = std::max(range_index - 1, 0); }

    /// @brief Antenna elevation knob, held: @p dir +1 up, -1 down.
    void slew_elevation(double dir, double dt) noexcept {
        el_centre = std::clamp(el_centre + dir * ELEV_SLEW_RAD_S * dt, -ELEV_LIMIT_RAD, ELEV_LIMIT_RAD);
    }

    /// @brief The brick under the acquisition cursor. With no cursor slew, the
    /// cursor rests on the nearest of the newest bricks.
    [[nodiscard]] const RadarHit* cursor_hit() const noexcept {
        const RadarHit* best = nullptr;
        for (int i = 0; i < hit_count; ++i) {
            const RadarHit& h = hits[static_cast<size_t>(i)];
            if (h.range_m > range_scale_m()) continue;
            if (!best || h.frame > best->frame || (h.frame == best->frame && h.range_m < best->range_m)) best = &h;
        }
        return best;
    }

    /// @brief TMS forward: lock the brick under the cursor. If the jet that
    /// left it is still inside the radar's limits the radar locks it after
    /// ACQUIRE_S; otherwise the designation times out and the scope stays in RWS.
    bool designate(World& w) noexcept {
        if (!fitted() || mode != Mode::RWS || pending_ > 0.0) return false;
        const RadarHit* h = cursor_hit();
        if (!h) return false;
        FireControlRadar& fcr = w.radars[static_cast<size_t>(own_)];
        fcr.enable(h->target, ACQUIRE_S);
        fcr.auto_reacquire = false;
        pending_ = LOCK_TIMEOUT_S;
        lock_failed_cue = false;
        return true;
    }

    /// @brief TMS aft: drop the lock and return to search.
    void undesignate(World& w) noexcept {
        if (!fitted()) return;
        w.radars[static_cast<size_t>(own_)].switch_off();
        pending_ = 0.0;
        if (mode == Mode::STT) enter_rws();
    }

    /// @brief STT (or a lock in progress): the target the radar is on.
    [[nodiscard]] int locked_target(const World& w) const noexcept {
        if (mode != Mode::STT) return -1;
        return w.radars[static_cast<size_t>(own_)].target;
    }

    // ---------------------------------------------------------------------
    // Stepping
    // ---------------------------------------------------------------------

    /// @brief Advance the antenna and paint what the beam crosses. Call after
    /// each World::step. Resets itself when the world or the airframe changes.
    void step(double dt, World& w, int own_index) noexcept {
        sync(w, own_index);
        if (!fitted() || mode == Mode::OFF) return;
        const Aircraft& self = w.aircraft[static_cast<size_t>(own_)];
        FireControlRadar& fcr = w.radars[static_cast<size_t>(own_)];

        if (pending_ > 0.0) {
            if (fcr.locked()) {
                pending_ = 0.0;
                mode = Mode::STT;
                hit_count = 0;
            } else if ((pending_ -= dt) <= 0.0) {
                pending_ = 0.0;
                fcr.switch_off();
                lock_failed_cue = true;
            }
        }

        if (mode == Mode::STT) {
            if (!fcr.locked()) {
                // The lock broke (memory ran out, chaff, gimbal): back to search.
                fcr.switch_off();
                enter_rws();
                return;
            }
            stabilised_angles(self.state, fcr.track_pos, ant_az, ant_el);
            ant_az = std::clamp(ant_az, -AZ_LIMIT_RAD, AZ_LIMIT_RAD);
            auto_range((fcr.track_pos - self.position()).norm());
            return;
        }

        // RWS raster: sweep in azimuth, step a bar at each edge.
        ant_az += sweep_dir * SCAN_RATE_RAD_S * dt;
        if (std::abs(ant_az) >= AZ_LIMIT_RAD) {
            ant_az = std::clamp(ant_az, -AZ_LIMIT_RAD, AZ_LIMIT_RAD);
            sweep_dir = -sweep_dir;
            if (++bar >= BARS) {
                bar = 0;
                ++frame;
                age_out();
            }
            in_beam_.fill(false);
        }
        ant_el = bar_elevation(bar);
        paint(w, self);
    }

    /// @brief Follow the world the ownship is in. A new fight, a reset or a
    /// different jet gives a cold scope; the pilot's range and elevation
    /// settings are kept. step() calls it; call it directly before reading
    /// the scope or designating when no step has run yet.
    void sync(const World& w, int own_index) noexcept {
        const aircraft::AircraftType type = w.aircraft[static_cast<size_t>(own_index)].type;
        const bool restarted = &w != world_ || w.time < last_time_ || own_index != own_;
        if (!configured_ || restarted || type != type_) {
            world_ = &w;
            own_ = own_index;
            type_ = type;
            spec_ = radar_spec(type);
            configured_ = true;
            pending_ = 0.0;
            lock_failed_cue = false;
            mode = spec_.fitted ? Mode::RWS : Mode::OFF;
            enter_rws_scan();
        }
        last_time_ = w.time;
    }

private:
    RadarSpec spec_{};
    aircraft::AircraftType type_{aircraft::AircraftType::F16_FIGHTING_FALCON};
    bool configured_{false};
    const World* world_{nullptr};
    double last_time_{-1.0};
    int own_{0};
    double pending_{0.0};
    std::array<bool, World::kMaxAircraft> in_beam_{};
    uint32_t rng_{0x9E3779B9u};

    void enter_rws() noexcept {
        mode = Mode::RWS;
        enter_rws_scan();
    }

    void enter_rws_scan() noexcept {
        hit_count = 0;
        frame = 0;
        bar = 0;
        sweep_dir = 1;
        ant_az = -AZ_LIMIT_RAD;
        in_beam_.fill(false);
    }

    [[nodiscard]] double uniform() noexcept {
        rng_ ^= rng_ << 13;
        rng_ ^= rng_ >> 17;
        rng_ ^= rng_ << 5;
        return static_cast<double>(rng_) / 4294967296.0;
    }

    /// @brief Single-look detection probability: certain inside 75% of the
    /// detection range, fading to nothing at its edge.
    [[nodiscard]] double p_detect(double range) const noexcept {
        const double rd = detection_range_m();
        return std::clamp((rd - range) / (0.25 * rd), 0.0, 1.0);
    }

    void paint(const World& w, const Aircraft& self) noexcept {
        const double hdg = scan_heading(self.state);
        const math::Vector3 beam(std::cos(ant_el) * std::cos(hdg + ant_az), std::cos(ant_el) * std::sin(hdg + ant_az),
                                 -std::sin(ant_el));
        const math::Vector3 nose = self.state.q_att.rotate_body_to_ned(math::Vector3(1.0, 0.0, 0.0));
        SensorLimits lim;
        lim.max_range_m = detection_range_m();
        lim.gimbal_rad = spec_.gimbal_deg * DEG;
        lim.notch_mps = spec_.notch_mps;
        for (int j = 0; j < w.count; ++j) {
            if (j == own_) continue;
            const Aircraft& t = w.aircraft[static_cast<size_t>(j)];
            const math::Vector3 los = t.position() - self.position();
            const bool inside = !t.crashed && AirCombatGeometry::angle_between(beam, los) < BEAM_HALF_RAD;
            const bool entered = inside && !in_beam_[static_cast<size_t>(j)];
            in_beam_[static_cast<size_t>(j)] = inside;
            if (!entered) continue;
            if (lim.check(self.position(), nose, t.position(), t.velocity()) != TrackCheck::OK) continue;
            if (uniform() >= p_detect(los.norm())) continue;
            RadarHit h;
            h.target = j;
            h.frame = frame;
            stabilised_angles(self.state, t.position(), h.az_rad, h.el_rad);
            h.range_m = los.norm();
            h.alt_m = t.state.altitude();
            add_hit(h);
        }
    }

    /// @brief One brick per jet per frame: overlapping bars repaint it in place.
    void add_hit(const RadarHit& h) noexcept {
        for (int i = 0; i < hit_count; ++i) {
            RadarHit& o = hits[static_cast<size_t>(i)];
            if (o.target == h.target && o.frame == h.frame) {
                o = h;
                return;
            }
        }
        if (hit_count < kMaxHits) {
            hits[static_cast<size_t>(hit_count++)] = h;
            return;
        }
        // Full: the oldest brick goes.
        int oldest = 0;
        for (int i = 1; i < hit_count; ++i) {
            if (hits[static_cast<size_t>(i)].frame < hits[static_cast<size_t>(oldest)].frame) oldest = i;
        }
        hits[static_cast<size_t>(oldest)] = h;
    }

    void age_out() noexcept {
        int n = 0;
        for (int i = 0; i < hit_count; ++i) {
            if (frame - hits[static_cast<size_t>(i)].frame < HISTORY_FRAMES) {
                hits[static_cast<size_t>(n++)] = hits[static_cast<size_t>(i)];
            }
        }
        hit_count = n;
    }

    /// @brief STT keeps the target between 40% and 95% of the scale.
    void auto_range(double range_m) noexcept {
        if (range_m > 0.95 * range_scale_m()) range_up();
        else if (range_index > 0 && range_m < 0.40 * range_scale_m() &&
                 range_m < 0.85 * kRangeScalesNm[static_cast<size_t>(range_index - 1)] * 1852.0) {
            range_down();
        }
    }
};

} // namespace fastjet::sim
