#include "../include/fastjet/ui/settings_manager.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace fastjet::ui;
namespace fs = std::filesystem;

namespace {

fs::path scratch_dir() {
    const fs::path dir = fs::temp_directory_path() / "fastjet_test_user_settings";
    fs::remove_all(dir);
    fs::create_directories(dir);
    return dir;
}

void write_file(const fs::path& p, const std::string& text) {
    std::ofstream out(p, std::ios::binary);
    out << text;
}

} // namespace

void test_json_parser() {
    std::cout << "[Test] JSON: escapes, unicode, malformed input... " << std::flush;
    const auto v = json::parse(R"({"a": "x\"y\\z\n", "u": "\u00e9\ud83d\ude80", "n": -1.5e2, "arr": [true, false, null]})");
    assert(v && v->is_object());
    assert(v->find("a")->as_string("") == "x\"y\\z\n");
    assert(v->find("u")->as_string("") == "\xC3\xA9\xF0\x9F\x9A\x80"); // e-acute + rocket, UTF-8
    assert(v->find("n")->as_number(0.0) == -150.0);
    assert(v->find("arr")->array()->size() == 3);

    std::string err;
    assert(!json::parse("{\"a\": 1,}", &err) && !err.empty());      // trailing comma
    assert(!json::parse("{\"a\": 1} x"));                            // trailing garbage
    assert(!json::parse("[1, 2"));                                   // unterminated
    assert(!json::parse("\"\\ud800\""));                             // lone surrogate
    assert(!json::parse(std::string(200, '[') + std::string(200, ']'))); // nesting limit

    // Writer output parses back to the same document.
    const auto again = json::parse(json::serialize(*v));
    assert(again && again->find("a")->as_string("") == "x\"y\\z\n");
    std::cout << "PASS\n";
}

void test_defaults_round_trip() {
    std::cout << "[Test] Settings: defaults survive a JSON round trip... " << std::flush;
    UserSettings s;
    s.graphics.display_mode = DisplayMode::BORDERLESS;
    s.graphics.frame_rate_cap = FrameRateCap::FPS_60;
    s.audio.master = 35;
    s.audio.muted = true;
    s.controls.invert_pitch = true;
    s.controls.assign(InputAction::LANDING_GEAR, 1, scancode::PAGE_DOWN); // steals from THROTTLE_DOWN
    s.accessibility.colorblind = ColorblindMode::TRITANOPIA;
    const auto doc = json::parse(json::serialize(s.to_json()));
    assert(doc);
    assert(UserSettings::from_json(*doc) == s);
    std::cout << "PASS\n";
}

void test_partial_and_invalid_values() {
    std::cout << "[Test] Settings: partial files keep defaults, bad values are clamped... " << std::flush;
    const auto doc = json::parse(R"({
        "audio": {"master": 250, "alerts": "loud"},
        "graphics": {"display_mode": "hologram", "vsync": false},
        "accessibility": {"ui_scale": 10},
        "controls": {"key_bindings": {"pitch_down": [41, 26], "roll_left": [9999, 0]}}
    })");
    assert(doc);
    const UserSettings s = UserSettings::from_json(*doc);
    const UserSettings d;
    assert(s.audio.master == limits::kVolume.max);          // clamped
    assert(s.audio.alerts == d.audio.alerts);                // wrong type -> default
    assert(s.audio.engine == d.audio.engine);                // missing -> default
    assert(s.graphics.display_mode == d.graphics.display_mode); // unknown token -> default
    assert(!s.graphics.vsync);                               // valid value kept
    assert(s.accessibility.ui_scale == limits::kUiScale.min);
    assert(s.controls.binding(InputAction::PITCH_DOWN).primary == scancode::NONE); // Esc is reserved
    assert(s.controls.binding(InputAction::PITCH_DOWN).secondary == 26);
    assert(s.controls.binding(InputAction::ROLL_LEFT).primary == scancode::NONE);  // out of range
    std::cout << "PASS\n";
}

void test_binding_conflicts() {
    std::cout << "[Test] Settings: a key is never bound to two actions... " << std::flush;
    ControlSettings c;
    const auto displaced = c.assign(InputAction::PITCH_DOWN, 0, scancode::A); // A was RUDDER_LEFT
    assert(displaced && *displaced == InputAction::RUDDER_LEFT);
    assert(c.binding(InputAction::PITCH_DOWN).primary == scancode::A);
    assert(c.binding(InputAction::RUDDER_LEFT).primary == scancode::NONE);

    // Moving a key between the two slots of one action is not a conflict.
    const auto none = c.assign(InputAction::PITCH_DOWN, 1, scancode::A);
    assert(!none);
    assert(c.binding(InputAction::PITCH_DOWN).primary == scancode::NONE);
    assert(c.binding(InputAction::PITCH_DOWN).secondary == scancode::A);
    std::cout << "PASS\n";
}

void test_storage_states() {
    std::cout << "[Test] Storage: missing, corrupt and atomic save... " << std::flush;
    const fs::path dir = scratch_dir();
    const fs::path file = dir / "nested" / "settings.json";

    SettingsStorage storage(file);
    auto r = storage.load();
    assert(r.status == SettingsStorage::LoadStatus::NOT_FOUND);
    assert(r.settings == UserSettings{});

    UserSettings s;
    s.audio.engine = 40;
    const auto saved = storage.save(s);
    assert(saved.ok);
    assert(fs::exists(file));
    assert(!fs::exists(dir / "nested" / "settings.json.tmp")); // temp file renamed away
    r = storage.load();
    assert(r.status == SettingsStorage::LoadStatus::LOADED);
    assert(r.settings == s);

    write_file(file, "{ this is not json");
    r = storage.load();
    assert(r.status == SettingsStorage::LoadStatus::CORRUPT);
    assert(r.settings == UserSettings{});
    assert(fs::exists(dir / "nested" / "settings.json.corrupt")); // user's file preserved

    assert(SettingsStorage{}.load().status == SettingsStorage::LoadStatus::MEMORY_ONLY);
    fs::remove_all(dir);
    std::cout << "PASS\n";
}

void test_manager_lifecycle() {
    std::cout << "[Test] Manager: preview, revert, apply, section reset, observers... " << std::flush;
    const fs::path dir = scratch_dir();
    const fs::path file = dir / "settings.json";
    SettingsManager m{SettingsStorage(file)};

    int previews = 0, applies = 0, reverts = 0, loads = 0;
    const auto id = m.subscribe([&](SettingsEvent e, const SettingsManager&) {
        if (e == SettingsEvent::PREVIEW) ++previews;
        if (e == SettingsEvent::APPLIED) ++applies;
        if (e == SettingsEvent::REVERTED) ++reverts;
        if (e == SettingsEvent::LOADED) ++loads;
    });
    m.load();
    assert(loads == 1 && !m.is_dirty());

    m.edit([](UserSettings& s) { s.audio.master = 10; });
    assert(previews == 1 && m.is_dirty());
    assert(m.committed().audio.master == UserSettings{}.audio.master);

    m.edit([](UserSettings& s) { s.audio.master = 10; }); // no change -> no event
    assert(previews == 1);

    m.revert();
    assert(reverts == 1 && !m.is_dirty() && m.pending().audio.master == UserSettings{}.audio.master);

    m.edit([](UserSettings& s) {
        s.audio.master = 10;
        s.graphics.vsync = false;
    });
    const auto r = m.apply();
    assert(r.ok && applies == 1 && !m.is_dirty());

    // Section reset touches only that category and is itself unapplied.
    m.reset_to_defaults(SettingsSection::AUDIO);
    assert(m.pending().audio == AudioSettings{});
    assert(!m.pending().graphics.vsync);
    assert(m.is_dirty());

    // What was applied is on disk.
    SettingsManager reloaded{SettingsStorage(file)};
    reloaded.load();
    assert(reloaded.committed().audio.master == 10);
    assert(!reloaded.committed().graphics.vsync);

    m.unsubscribe(id);
    m.revert();
    assert(reverts == 1); // unsubscribed listener no longer called
    fs::remove_all(dir);
    std::cout << "PASS\n";
}

int main() {
    std::cout << "=========================================================\n";
    std::cout << "  User Settings Model & Persistence Tests\n";
    std::cout << "=========================================================\n";
    test_json_parser();
    test_defaults_round_trip();
    test_partial_and_invalid_values();
    test_binding_conflicts();
    test_storage_states();
    test_manager_lifecycle();
    std::cout << "All user settings tests passed.\n";
    return 0;
}
