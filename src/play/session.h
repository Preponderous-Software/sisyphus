// A game being played: the world and its save slot, the scene, the input,
// the news, and the autosave, one frame at a time. The desktop game
// (app/main.cpp) drives it from its own loop; the browser game
// (app/web_main.cpp) from the browser's.
#pragma once

#include <functional>
#include <string>

#include "audio/device.h"
#include "bridge/scene.h"
#include "play/view.h"
#include "sim/autopilot.h"
#include "sim/world.h"
#include "strata/strata.hpp"

namespace sisyphus::play {

struct SessionOptions {
    uint64_t seed = 1;
    std::string slot;        // the save slot directory; empty = no saves
    bool fresh = false;      // start a new game even if there is a save
    bool autopilot = false;  // the autopilot plays, watched from behind
    int speed = 1;           // game seconds per second of real time
    // In a browser the page owns the pointer lock and the canvas size; Esc
    // never quits.
    bool browser = false;
    bool sound = true;       // open the sound card (it runs silent if there is none)
    // Called after every save written (the browser flushes it to IndexedDB).
    std::function<void()> afterSave;
};

class Session {
public:
    Session(strata::Engine& engine, const SessionOptions& options);

    void handle(const StrataEvent& e);
    // Advances by `dt` seconds of real time and draws a frame (not presented).
    void frame(double dt);
    bool running() const { return running_; }

    bool save(const char* note);
    void load();
    void restart();

    // The page's input (the browser): look by a pixel delta; walk with a
    // stick, -1..1 each, beyond 0.95 hurries; hold work (patch), retire or refactor;
    // press a key by its SDL scancode.
    void look(float dx, float dy);
    void setStick(float forward, float strafe);
    void setHeld(bool work, bool retire, bool refactor);
    void press(int scancode);
    void setCaptured(bool on) { captured_ = on; }
    void setHelp(bool on) { help_ = on; }
    void setCompactHud(bool on) { compact_ = on; }
    // Sound on or off (M); it fades rather than stops. `announce` says so on screen.
    void setMuted(bool muted, bool announce = true);
    bool muted() const { return sound_.muted(); }

    const World& world() const { return w_; }
    bool captured() const { return captured_; }
    bool help() const { return help_; }
    bool outside() const { return outside_; }
    bool history() const { return history_; }
    const std::string& message() const { return message_; }
    // While sitting: the words over the view.
    std::string quote() const;

private:
    void capture(bool on);
    void onKeyDown(int scancode, bool repeat);
    void step();
    void say(const std::string& text, double seconds = 5);
    void noticeChanges();
    void adopt(const World& w);
    void listen(double dt);  // the sounds of this frame's change, and of the place

    strata::Engine& engine_;
    SessionOptions options_;
    World w_;
    audio::Sound sound_;
    World seen_;   // the world as last reported, for the news
    World heard_;  // the world as it last sounded, for this frame's cues
    bridge::Scene scene_;
    Autopilot pilot_;
    static constexpr int kMaxScancode = 512;
    bool keys_[kMaxScancode] = {};
    bool running_ = true, captured_ = false, help_ = true, compact_ = false, outside_ = false;
    float yaw_ = 0, pitch_ = 0;
    float stickForward_ = 0, stickStrafe_ = 0;
    float steps_ = 0;  // footsteps a second, from the last ticks
    bool history_ = false;  // Tab: the history screen
    bool letGo_ = false;  // a retire or refactor finished: R and F wait to be let go
    bool heldWork_ = false, heldRetire_ = false, heldRefactor_ = false;
    bool sitOnce_ = false;
    double pending_ = 0;
    double clock_ = 0, lastSave_ = 0;
    std::string message_;
    double messageUntil_ = 0;
};

}  // namespace sisyphus::play
