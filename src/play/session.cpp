#include "play/session.h"

#include <cmath>

#include "audio/cues.h"
#include "save/save.h"
#include "sim/hill.h"

namespace sisyphus::play {

namespace {

// SDL scancodes.
constexpr int kM = 16;
constexpr int kW = 26, kA = 4, kS = 22, kD = 7, kE = 8, kF = 9, kR = 21, kQ = 20, kV = 25, kLShift = 225, kRShift = 229,
              kEscape = 41, kF1 = 58, kF5 = 62, kF9 = 66;
constexpr float kLookPerPixel = 0.0025f;
constexpr double kAutosaveSeconds = 60;
constexpr float kHurryStick = 0.95f;
constexpr const char* kGreeting =
    "The boulder is your software. Push it up to the next terrace to ship a release (W, into it).\n"
    "Every release adds a module, and every module rots. Hold E on one to patch it.";

// Said while sitting, one after another; the last stays.
constexpr const char* kQuotes[] = {
    "The boulder is still. For a moment, so are you.",
    "Below, the town's windows. Somebody is using what you keep.",
    "Nothing you ship stays shipped. The rot is not a failure; it is the weather.",
    "There is no top. There is the next terrace, and the one after.",
    "\"Il faut imaginer Sisyphe heureux.\"\n(One must imagine Sisyphus happy.)\n-- Albert Camus",
};
constexpr int kQuoteCount = sizeof kQuotes / sizeof kQuotes[0];
constexpr float kQuoteSeconds = 7.0f;

}  // namespace

Session::Session(strata::Engine& engine, const SessionOptions& options)
    : engine_(engine), options_(options), w_(newWorld(options.seed)), scene_(engine) {
    if (options_.sound) sound_.start();
    World start = w_;
    outside_ = false;  // his eyes; V for behind him
    if (!options_.slot.empty() && !options_.fresh && hasSave(options_.slot)) {
        World loaded;
        const LoadResult r = loadSave(options_.slot, loaded);
        if (r.ok) start = loaded;
        say(r.ok ? r.message : r.message + " A new game was started.");
    } else if (!options_.autopilot) {
        say(kGreeting, 14);
    }
    adopt(start);
    if (!options_.browser) capture(true);
}

void Session::adopt(const World& w) {
    w_ = w;
    seen_ = w;
    heard_ = w;  // a loaded or new world is not a change to be heard
    yaw_ = w_.player.yaw;
    pitch_ = w_.player.pitch;
    pilot_ = Autopilot{};
    pending_ = 0;
}

void Session::say(const std::string& text, double seconds) {
    message_ = text;
    messageUntil_ = clock_ + seconds;
}

std::string Session::quote() const {
    if (!w_.player.sitting) return "";
    // Counted over all the time he has sat: each sitting goes on from the
    // words the last one reached.
    const int k = static_cast<int>(w_.satSeconds / kQuoteSeconds);
    return kQuotes[k < kQuoteCount ? k : kQuoteCount - 1];
}

void Session::noticeChanges() {
    const World& a = seen_;
    const World& b = w_;
    if (b.releases > a.releases) {
        std::string s = "Shipped " + versionText(b) + ".";
        if (b.added > a.added) s += " New module: " + std::string(moduleName(b.added - 1)) + ".";
        if (b.summitSeconds >= 0) s = versionText(b) + " shipped, at the summit. Look down at the town.";
        say(s, 7);
    } else if (b.falling && !a.falling) {
        say("The platform it runs on reached its end of life.\nThe boulder rolls. Every module goes with it.", 9);
    } else if (!b.falling && a.falling) {
        say("Back at the trailhead. " + std::to_string(b.major) + ".x starts here, with all the old code aboard.\n"
            "Q to sit a while, if you like.", 10);
    } else if (b.breakdowns > a.breakdowns) {
        for (int i = 0; i < kMaxModules; ++i)
            if (b.modules[static_cast<size_t>(i)].active && b.modules[static_cast<size_t>(i)].health <= 0 &&
                a.modules[static_cast<size_t>(i)].health > 0) {
                say(std::string(moduleName(i)) + " is down. Users are leaving.", 6);
                break;
            }
    } else if (b.incidents > a.incidents && b.lastIncident >= 0) {
        say(std::string(moduleName(b.lastIncident)) + ": " + issueName(b.lastIssue) + ".", 4);
    } else if (b.retired > a.retired) {
        say("Retired. One less thing to keep, and a few users miss it.", 5);
    } else if (b.refactored > a.refactored && b.refactoredInto >= 0) {
        const std::string into = moduleName(b.refactoredInto);
        say(b.modules[static_cast<size_t>(b.refactoredInto)].issue == Issue::Flaky
                ? "Refactored into " + into + ". One less thing to keep, but it broke a test."
                : "Refactored into " + into + ". One less thing to keep, and what is left is new.",
            5);
    } else if (b.boulder.v < -1.0f && a.boulder.v >= -1.0f && !b.falling && !b.pushing) {
        say("It is rolling back down! Let go only on a terrace: it stops at the one below.", 6);
    } else if (b.slipping && !a.slipping) {
        say("Too heavy. It slides back.", 4);
    }
    seen_ = w_;
}

bool Session::save(const char* note) {
    if (options_.slot.empty()) return false;
    std::string why;
    if (!writeSave(options_.slot, w_, &why)) {
        say("Could not save: " + why);
        return false;
    }
    lastSave_ = clock_;
    if (note) say(note, 3);
    if (options_.afterSave) options_.afterSave();
    return true;
}

void Session::load() {
    if (options_.slot.empty()) return;
    World loaded;
    const LoadResult r = loadSave(options_.slot, loaded);
    if (r.ok) adopt(loaded);
    say(r.message);
}

void Session::restart() {
    adopt(newWorld(options_.seed + static_cast<uint64_t>(w_.tick)));
    say(kGreeting, 14);
}

void Session::setMuted(bool muted, bool announce) {
    sound_.setMuted(muted);
    if (announce) say(muted ? "Sound off (M turns it on)" : "Sound on", 3);
}

void Session::listen(double dt) {
    for (audio::Cue c : audio::cues(heard_, w_)) sound_.play(c);
    // His pace from the rules' own ticks, not from frames: a frame that
    // drew no new tick, or two, must not make him stop or sprint.
    (void)dt;
    if (w_.tick != heard_.tick) {
        const float dx = w_.player.x - heard_.player.x, dz = w_.player.z - heard_.player.z;
        const float seconds = static_cast<float>(w_.tick - heard_.tick) / kTickRate;
        const float speed = seconds > 0 ? std::sqrt(dx * dx + dz * dz) / seconds : 0.0f;
        steps_ = speed > 0.3f && speed < 6.0f ? speed / 0.75f : 0.0f;  // a stride is about 0.75 m
    }
    sound_.setAmbience(audio::ambience(w_, eyePosition(w_), steps_, sunHeight(w_)));
    heard_ = w_;
}

void Session::capture(bool on) {
    if (!options_.browser) engine_.setRelativeMouseMode(on);
    captured_ = on;
}

void Session::look(float dx, float dy) {
    yaw_ += dx * kLookPerPixel;
    pitch_ = clampf(pitch_ - dy * kLookPerPixel, -1.45f, 1.45f);
}

void Session::setStick(float forward, float strafe) {
    stickForward_ = clampf(forward, -1.0f, 1.0f);
    stickStrafe_ = clampf(strafe, -1.0f, 1.0f);
}

void Session::setHeld(bool work, bool retire, bool refactor) {
    heldWork_ = work;
    heldRetire_ = retire;
    heldRefactor_ = refactor;
}

void Session::press(int scancode) { onKeyDown(scancode, false); }

void Session::onKeyDown(int scancode, bool repeat) {
    if (scancode == kEscape) {
        if (options_.browser) return;
        if (captured_) capture(false);
        else running_ = false;
        return;
    }
    if (repeat) return;
    if (scancode == kF1) help_ = !help_;
    else if (scancode == kF5) {
        if (options_.slot.empty()) say("This game does not save (the autopilot, or a test harness, is playing).");
        else save("Saved.");
    } else if (scancode == kF9) load();
    else if (scancode == kV) outside_ = !outside_;
    else if (scancode == kM) setMuted(!sound_.muted());
    else if (scancode == kQ && !options_.autopilot) {
        sitOnce_ = true;
        // Sitting down, he turns to the view.
        if (!w_.player.sitting) {
            yaw_ = kPi;
            pitch_ = -0.12f;
        }
    }
}

void Session::handle(const StrataEvent& e) {
    switch (e.type) {
        case STRATA_EVENT_QUIT: running_ = false; break;
        case STRATA_EVENT_KEY_DOWN:
            if (e.scancode >= 0 && e.scancode < kMaxScancode) keys_[e.scancode] = true;
            onKeyDown(e.scancode, e.repeat != 0);
            break;
        case STRATA_EVENT_KEY_UP:
            if (e.scancode >= 0 && e.scancode < kMaxScancode) keys_[e.scancode] = false;
            break;
        case STRATA_EVENT_MOUSE_BUTTON_DOWN:
            if (!captured_ && !options_.browser) capture(true);
            break;
        case STRATA_EVENT_MOUSE_MOTION:
            if (captured_ && !options_.browser) look(e.dx, e.dy);
            break;
        case STRATA_EVENT_RESIZED:
            if (!options_.browser) engine_.resize(e.width, e.height);
            break;
        default: break;
    }
}

void Session::step() {
    if (options_.autopilot) {
        sitOnce_ = false;
        tick(w_, pilot_.next(w_));
        return;
    }
    Input in;
    const float f = (keys_[kW] ? 1.0f : 0.0f) - (keys_[kS] ? 1.0f : 0.0f) + stickForward_;
    const float s = (keys_[kD] ? 1.0f : 0.0f) - (keys_[kA] ? 1.0f : 0.0f) + stickStrafe_;
    in.forward = clampf(f, -1.0f, 1.0f);
    in.strafe = clampf(s, -1.0f, 1.0f);
    in.hurry = keys_[kLShift] || keys_[kRShift] || stickForward_ * stickForward_ + stickStrafe_ * stickStrafe_ > kHurryStick * kHurryStick;
    in.work = keys_[kE] || heldWork_;
    in.retire = keys_[kR] || heldRetire_;
    in.refactor = keys_[kF] || heldRefactor_;
    // A retire or refactor that finished does not run on into the next
    // module under the crosshair: let go first.
    if (letGo_) {
        if (!in.retire && !in.refactor) letGo_ = false;
        in.retire = in.refactor = false;
    }
    in.sit = sitOnce_;
    sitOnce_ = false;
    // Getting up by walking off.
    if (w_.player.sitting && (in.forward != 0 || in.strafe != 0)) in.sit = true;
    in.yaw = yaw_;
    in.pitch = pitch_;
    const int32_t cut = w_.retired + w_.refactored;
    tick(w_, in);
    if (w_.retired + w_.refactored != cut) letGo_ = true;
}

void Session::frame(double dt) {
    clock_ += dt;
    if (!message_.empty() && clock_ > messageUntil_) message_.clear();
    if (!options_.slot.empty() && clock_ - lastSave_ > kAutosaveSeconds) save(nullptr);
    const double speed = options_.speed;
    pending_ += dt * speed;
    if (pending_ > 0.25 * speed) pending_ = 0.25 * speed;  // after a stall, do not try to catch up
    while (pending_ >= 1.0 / kTickRate) {
        pending_ -= 1.0 / kTickRate;
        step();
    }
    if (options_.autopilot) {
        yaw_ = w_.player.yaw;
        pitch_ = w_.player.pitch;
    }
    listen(dt);
    noticeChanges();
    const bool sitting = w_.player.sitting;
    const bool out = outside_ && !sitting;
    scene_.update(w_, out);
    Overlay o;
    o.help = help_ && !compact_ && !sitting;
    o.noText = compact_;
    o.message = message_;
    o.quote = quote();
    o.aim = !options_.autopilot;
    o.saves = !options_.slot.empty();
    const Camera cam = !out ? cameraAtEyes(w_) : options_.autopilot ? cameraBehind(w_) : cameraFollow(w_);
    render(engine_, w_, cam, out, o);
}

}  // namespace sisyphus::play
