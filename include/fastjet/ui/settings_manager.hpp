#pragma once

#include "fastjet/ui/user_settings.hpp"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

/// @file
/// @brief Settings persistence and the observable settings store.
///
/// SettingsStorage reads and writes the JSON file. SettingsManager holds two
/// copies of the preferences:
///   - committed: what is saved on disk and applied to the whole application;
///   - pending:   what the settings screen is editing.
/// Views edit `pending`; "Apply" commits and saves it, "Revert" discards it.
/// Subsystems subscribe to change notifications instead of being called by
/// the menu, which keeps the UI decoupled from audio, video and input code.

namespace fastjet::ui {

// ---------------------------------------------------------------------------
// Storage
// ---------------------------------------------------------------------------

class SettingsStorage {
public:
    enum class LoadStatus : uint8_t {
        LOADED,      ///< File read and parsed
        NOT_FOUND,   ///< No file yet: defaults in use (first launch)
        CORRUPT,     ///< Unreadable JSON: defaults in use, original kept as *.corrupt
        MEMORY_ONLY, ///< No path configured (tests, headless runs)
    };

    struct LoadResult {
        UserSettings settings{};
        LoadStatus status = LoadStatus::MEMORY_ONLY;
        std::string message;
    };

    struct SaveResult {
        bool ok = true;
        std::string message;
    };

    /// @param path JSON file location; empty keeps settings in memory only.
    explicit SettingsStorage(std::filesystem::path path = {}) : path_(std::move(path)) {}

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }
    [[nodiscard]] bool is_persistent() const noexcept { return !path_.empty(); }

    [[nodiscard]] LoadResult load() const {
        LoadResult r;
        if (!is_persistent()) return r;

        std::ifstream in(path_, std::ios::binary);
        if (!in) {
            r.status = LoadStatus::NOT_FOUND;
            r.message = "no settings file at " + path_.string() + "; using defaults";
            return r;
        }
        std::stringstream buf;
        buf << in.rdbuf();

        std::string error;
        const std::optional<json::Value> doc = json::parse(buf.str(), &error);
        if (!doc || !doc->is_object()) {
            // Keep the unreadable file for the user rather than silently
            // overwriting it on the next save.
            std::error_code ec;
            std::filesystem::path backup = path_;
            backup += ".corrupt";
            std::filesystem::copy_file(path_, backup, std::filesystem::copy_options::overwrite_existing, ec);
            r.status = LoadStatus::CORRUPT;
            r.message = "settings file is not valid JSON (" + (doc ? std::string("root is not an object") : error) +
                        "); using defaults, original saved as " + backup.string();
            return r;
        }
        r.settings = UserSettings::from_json(*doc);
        r.status = LoadStatus::LOADED;
        r.message = "loaded " + path_.string();
        return r;
    }

    /// @brief Writes atomically: serialise to a sibling temp file, then rename
    /// over the target so a crash mid-write never leaves a truncated file.
    [[nodiscard]] SaveResult save(const UserSettings& settings) const {
        if (!is_persistent()) return {true, "memory only"};

        std::error_code ec;
        if (path_.has_parent_path()) {
            std::filesystem::create_directories(path_.parent_path(), ec);
            if (ec) return {false, "cannot create " + path_.parent_path().string() + ": " + ec.message()};
        }

        std::filesystem::path tmp = path_;
        tmp += ".tmp";
        {
            std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
            if (!out) return {false, "cannot open " + tmp.string() + " for writing"};
            out << json::serialize(settings.to_json());
            out.flush();
            if (!out) return {false, "write to " + tmp.string() + " failed"};
        }
        std::filesystem::rename(tmp, path_, ec);
        if (ec) {
            std::filesystem::remove(tmp, ec);
            return {false, "cannot replace " + path_.string()};
        }
        return {true, "saved " + path_.string()};
    }

private:
    std::filesystem::path path_;
};

// ---------------------------------------------------------------------------
// Observable manager
// ---------------------------------------------------------------------------

enum class SettingsEvent : uint8_t {
    LOADED,   ///< Initial load finished (pending == committed)
    PREVIEW,  ///< Pending edited; preview-safe subsystems may follow it live
    APPLIED,  ///< Pending committed and saved; apply everything
    REVERTED, ///< Pending discarded back to committed
};

class SettingsManager {
public:
    using Listener = std::function<void(SettingsEvent, const SettingsManager&)>;
    using SubscriptionId = uint32_t;

    explicit SettingsManager(SettingsStorage storage = SettingsStorage{}) : storage_(std::move(storage)) {}

    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    /// @brief Loads from storage (defaults if absent or corrupt) and notifies LOADED.
    SettingsStorage::LoadResult load() {
        SettingsStorage::LoadResult r = storage_.load();
        committed_ = r.settings;
        pending_ = r.settings;
        notify(SettingsEvent::LOADED);
        return r;
    }

    [[nodiscard]] const UserSettings& committed() const noexcept { return committed_; }
    [[nodiscard]] const UserSettings& pending() const noexcept { return pending_; }
    [[nodiscard]] bool is_dirty() const noexcept { return pending_ != committed_; }
    [[nodiscard]] const SettingsStorage& storage() const noexcept { return storage_; }

    /// @brief Mutates the pending copy through `fn`, validates it, and emits
    /// PREVIEW if anything actually changed.
    template <typename Fn>
    void edit(Fn&& fn) {
        UserSettings next = pending_;
        std::forward<Fn>(fn)(next);
        next.sanitize();
        if (next == pending_) return;
        pending_ = std::move(next);
        notify(SettingsEvent::PREVIEW);
    }

    /// @brief Commits pending, writes it to disk, and emits APPLIED.
    ///
    /// The in-memory commit happens even if the write fails, so the session
    /// behaves as the user asked; the result reports the disk error.
    SettingsStorage::SaveResult apply() {
        committed_ = pending_;
        SettingsStorage::SaveResult r = storage_.save(committed_);
        notify(SettingsEvent::APPLIED);
        return r;
    }

    /// @brief Discards unapplied edits.
    void revert() {
        if (!is_dirty()) return;
        pending_ = committed_;
        notify(SettingsEvent::REVERTED);
    }

    /// @brief Resets one category of the pending copy to defaults (not saved until apply()).
    void reset_to_defaults(SettingsSection section) {
        edit([section](UserSettings& s) { s.reset_section(section); });
    }

    SubscriptionId subscribe(Listener listener) {
        const SubscriptionId id = ++next_id_;
        listeners_.emplace_back(id, std::move(listener));
        return id;
    }

    void unsubscribe(SubscriptionId id) {
        std::erase_if(listeners_, [id](const auto& entry) { return entry.first == id; });
    }

private:
    SettingsStorage storage_;
    UserSettings committed_{};
    UserSettings pending_{};
    std::vector<std::pair<SubscriptionId, Listener>> listeners_;
    SubscriptionId next_id_ = 0;

    void notify(SettingsEvent event) {
        // Copy so a listener may subscribe/unsubscribe during dispatch.
        const auto snapshot = listeners_;
        for (const auto& [id, fn] : snapshot) {
            if (fn) fn(event, *this);
        }
    }
};

} // namespace fastjet::ui
