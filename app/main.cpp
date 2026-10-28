// sisyphus: the game on the Strata engine.
//
//   sisyphus [--slot DIR] [--new] [--seed N] [--width W --height H]
//   sisyphus --autopilot [--speed N] [--width W --height H]
//   sisyphus --headless [--frames DIR] [--width W --height H]
//   sisyphus --version
//
// Controls: mouse to look, W A S D to walk (walk into the boulder from below
// to push it up the hill; on a terrace, from above to roll it back and turn
// it), Shift to hurry, hold E on a module to patch it, hold R to retire it,
// Q to sit and look at the view, V for his eyes (or back over his shoulder), M for sound, F5 to save, F9 to
// load the last save, F1 for help, Esc to free the mouse (Esc again quits).
//
// The game continues from its save in DIR (by default %APPDATA%\Sisyphus\default
// on Windows, ~/.local/share/sisyphus/default elsewhere), saves every minute
// and on quit, and keeps the save before last; --new starts over.
//
// --autopilot lets the autopilot play (at N times speed), watched from
// behind. --headless runs the autopilot's playthrough (from the trailhead to
// 1.0 at the summit and back down) without a window, writes a screenshot at
// each moment worth seeing if DIR is given, and fails unless it ends on the
// pinned fingerprint.
#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "bridge/scene.h"
#include "play/session.h"
#include "play/view.h"
#include "save/save.h"
#include "sim/autopilot.h"
#include "sim/hill.h"
#include "sim/world.h"
#include "strata/strata.hpp"

using namespace sisyphus;

namespace {

struct Args {
    uint64_t seed = 1;
    bool headless = false;
    bool autopilot = false;
    int speed = 1;
    std::string slot;
    bool fresh = false;
    std::string framesDir;
    int width = 1280, height = 720;
};

bool parseArgs(int argc, char** argv, Args& a) {
    for (int i = 1; i < argc; ++i) {
        const std::string s = argv[i];
        auto next = [&](const char* what) -> const char* {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "sisyphus: %s needs a value\n", what);
                std::exit(2);
            }
            return argv[++i];
        };
        if (s == "--seed") a.seed = std::strtoull(next("--seed"), nullptr, 10);
        else if (s == "--headless") a.headless = true;
        else if (s == "--autopilot") a.autopilot = true;
        else if (s == "--slot") a.slot = next("--slot");
        else if (s == "--new") a.fresh = true;
        else if (s == "--speed") a.speed = std::atoi(next("--speed"));
        else if (s == "--frames") a.framesDir = next("--frames");
        else if (s == "--width") a.width = std::atoi(next("--width"));
        else if (s == "--height") a.height = std::atoi(next("--height"));
        else if (s == "--version") {
            std::printf("sisyphus %s\n", SISYPHUS_VERSION);
            std::exit(0);
        } else if (s == "--help" || s == "-h") return false;
        else {
            std::fprintf(stderr, "sisyphus: unknown argument '%s'\n", s.c_str());
            return false;
        }
    }
    if (a.speed < 1 || a.speed > 120) {
        std::fprintf(stderr, "sisyphus: --speed is 1 to 120\n");
        return false;
    }
    return true;
}

void usage() {
    std::puts("usage: sisyphus [--slot DIR] [--new] [--seed N] [--width W --height H]\n"
              "       sisyphus --autopilot [--speed N] [--width W --height H]\n"
              "       sisyphus --headless [--frames DIR] [--width W --height H]\n"
              "       sisyphus --version");
}

// ---- the autopilot's playthrough, headless

// A camera at `target` + `offset`, looking at `target`.
play::Camera lookingAt(Vec3 target, Vec3 offset) {
    play::Camera c;
    c.eye = target + offset;
    const Vec3 d = target - c.eye;
    c.yaw = std::atan2(d.x, -d.z);
    c.pitch = std::atan2(d.y, std::sqrt(d.x * d.x + d.z * d.z));
    return c;
}

struct Shooter {
    strata::Engine& engine;
    bridge::Scene& scene;
    const std::string& dir;
    int count = 0;

    void shoot(const World& w, const char* name, const play::Camera& cam, bool outside, bool hud, const std::string& message = "",
               const std::string& quote = "") {
        std::printf("  shot %-22s v%-4s s=%6.1f modules %2d health %3.0f%%\n", name, versionText(w).c_str(), w.boulder.s,
                    activeModules(w), health(w) * 100);
        if (dir.empty()) return;
        scene.update(w, outside);
        play::Overlay o;
        o.help = false;
        o.noText = !hud && quote.empty();
        o.message = message;
        o.quote = quote;
        play::render(engine, w, cam, outside, o);
        engine.present();
        char file[160];
        std::snprintf(file, sizeof file, "/%02d-%s.png", ++count, name);
        engine.saveScreenshotPng(dir + file);
    }
};

int runPlaythrough(strata::Engine& engine, const Args& args) {
    World w = newWorld(args.seed);
    bridge::Scene scene(engine);
    Autopilot pilot;
    Shooter shot{engine, scene, args.framesDir};
    shot.shoot(w, "trailhead", play::cameraOverShoulder(w), true, true,
               "The boulder is your software. Push it up to the next terrace to ship a release.");
    bool pushed = false, patched = false, summit = false, fell = false, night = false;
    int releases = 0;
    const int64_t limit = 60LL * 60 * kTickRate;
    for (int64_t t = 0; t < limit && !playthroughDone(w); ++t) {
        tick(w, pilot.next(w));
        if (!pushed && w.pushing && w.boulder.s > 14) {
            pushed = true;
            shot.shoot(w, "pushing", play::cameraBeside(w, 1.0f, 1.0f), true, false);
            shot.shoot(w, "pushing-behind", play::cameraBehind(w), true, true);
        }
        if (!patched && w.work.module >= 0 && w.work.seconds > 0.6f && !w.work.retiring) {
            patched = true;
            shot.shoot(w, "patching", play::cameraAtEyes(w), false, true);
        }
        if (w.releases > releases) {
            releases = w.releases;
            if (w.summitSeconds < 0) {
                const std::string name = "release-" + versionText(w);
                shot.shoot(w, name.c_str(), play::cameraBeside(w, w.releases % 2 ? -1.0f : 1.0f, 3.0f), true, false);
            }
        }
        const float tod = timeOfDay(w);
        if (!night && tod > 0.8f && tod < 0.95f) {
            night = true;
            shot.shoot(w, "night", play::cameraBehind(w), true, true);
        }
        if (!summit && w.summitSeconds > 2.0f) {
            summit = true;
            shot.shoot(w, "summit", play::cameraBeside(w, 1.0f, -2.0f), true, false);
            World look = w;
            look.player.yaw = kPi;
            look.player.pitch = -0.2f;
            shot.shoot(look, "summit-view", play::cameraAtEyes(look), false, true, versionText(w) + " shipped, at the summit.");
        }
        if (!fell && w.falling && w.boulder.s < hill::terraceStart(4)) {
            fell = true;
            shot.shoot(w, "rolling-back", lookingAt(boulderCentre(w), {-9.0f, 3.0f, 9.0f}), true, false,
                       "The platform it runs on reached its end of life.");
        }
    }
    // Back at the trailhead: sit, and look at the town.
    World sit = w;
    sit.player.x = 2.5f;
    sit.player.z = hill::zOf(hill::kRestS) + 4.0f;
    sit.player.sitting = true;
    sit.player.yaw = kPi;
    sit.player.pitch = -0.1f;
    shot.shoot(sit, "sitting", play::cameraAtEyes(sit), false, false, "",
               "\"Il faut imaginer Sisyphe heureux.\"\n(One must imagine Sisyphus happy.)\n-- Albert Camus");
    const uint64_t fp = fingerprint(w);
    std::printf("playthrough: %s in %.1f min, %d releases, %d incidents, %d patched, %d down, %d retired, users %.0f\n",
                versionText(w).c_str(), static_cast<double>(w.tick) / kTickRate / 60.0, w.releases, w.incidents, w.patched,
                w.breakdowns, w.retired, static_cast<double>(w.users));
    std::printf("fingerprint %016" PRIx64 " (pinned %016" PRIx64 "), %zu meshes\n", fp, kPlaythroughFingerprint,
                scene.meshCount());
    if (!playthroughDone(w) || fp != kPlaythroughFingerprint) {
        std::fprintf(stderr, "sisyphus: the playthrough did not end on the pinned fingerprint\n");
        return 1;
    }
    return 0;
}

// ---- playing in a window

int runGame(strata::Engine& engine, const Args& args) {
    play::SessionOptions o;
    o.seed = args.seed;
    o.autopilot = args.autopilot;
    o.speed = args.speed;
    // Watching the autopilot never touches the player's save, and neither does
    // a test harness driving the game (Strata's STRATA_DRIVE).
    const bool driven = engine.driven();
    if (!args.autopilot && !driven) o.slot = args.slot.empty() ? defaultSlot() : args.slot;
    o.fresh = args.fresh;
    // A driven run is silent: its clock may be stepped, and synthesized sound
    // would drift against it.
    o.sound = !driven;
    if (driven) std::puts("sisyphus: driven by a test harness; this game does not save and is silent");
    play::Session session(engine, o);
    while (session.running()) {
        StrataEvent e;
        while (engine.pollEvent(e)) session.handle(e);
        session.frame(engine.frameSeconds());
        engine.present();
    }
    session.save(nullptr);
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    Args args;
    if (!parseArgs(argc, argv, args)) {
        usage();
        return 2;
    }
    try {
        strata::EngineDesc desc;
        desc.surface = args.headless ? STRATA_SURFACE_HEADLESS : STRATA_SURFACE_WINDOW;
        desc.width = args.width;
        desc.height = args.height;
        desc.flags = args.headless ? 0u : static_cast<uint32_t>(STRATA_FLAG_VSYNC);
        desc.title = "Sisyphus";
        auto engine = strata::Engine::create(desc);
        play::setLighting(*engine);
        if (args.headless) return runPlaythrough(*engine, args);
        return runGame(*engine, args);
    } catch (const strata::Error& e) {
        std::fprintf(stderr, "sisyphus: %s\n", e.what());
        return 1;
    }
}
