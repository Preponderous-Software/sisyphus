// What the player sees each frame: the sky at the hour, the camera (at
// Sisyphus's eyes, or behind him), the glow of modules in trouble, the town's
// lit windows, and the HUD.
#pragma once

#include <string>
#include <vector>

#include "sim/world.h"
#include "strata/strata.hpp"

namespace sisyphus::play {

// How high the sun is for the world's time of day, -1..1 (below 0 is night).
float sunHeight(const World& w);

// `lightweight`: fewer, smaller shadow cascades, for phones and the browser.
void setLighting(strata::Engine& engine, bool lightweight = false);

struct Camera {
    Vec3 eye;
    float yaw = 0, pitch = 0;
};
// At his eyes, looking where he looks.
Camera cameraAtEyes(const World& w);
// Behind and above him, looking past him (the autopilot's view, and screenshots).
Camera cameraBehind(const World& w);
// Straight behind him, looking where he looks: the view V switches to from
// his eyes.
Camera cameraFollow(const World& w);
// Off to his side, looking across at him and the boulder (the tour's shots).
Camera cameraBeside(const World& w, float side = 1.0f, float back = 2.0f);

struct HudLine {
    std::string text;
    bool heading = false;
    bool alarm = false;
};

// The HUD's left column: the version, the users, the codebase's health and
// weight, how far to the next release, and the open issues, worst first.
std::vector<HudLine> hudLines(const World& w);
// The line under the crosshair: the module looked at and what holding E (or
// R) would do to it, or what pushing is doing; empty if nothing.
std::string prompt(const World& w);
// "[#####.....]" for a value in 0..1.
std::string bar(float value, int width);

struct Overlay {
    bool help = true;            // the controls along the bottom
    bool noText = false;         // a narrow browser page shows the words itself
    std::string message;         // the newest news, above the middle
    std::string quote;           // while sitting: words over the view
    bool aim = true;             // the crosshair (outside him, it marks where he aims)
    bool saves = true;           // the game saves (F5 and F9 do something)
};

// Draws one frame (not presented) from `cam`.
void render(strata::Engine& engine, const World& w, const Camera& cam, bool outside, const Overlay& o);

// The HUD's text size: 1 is 16-pixel text.
void setTextScale(float scale);

// `text` split at its newlines, and each line wrapped at word boundaries.
std::vector<std::string> wrapped(const std::string& text, size_t columns);

}  // namespace sisyphus::play
