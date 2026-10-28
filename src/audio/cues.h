// What the hill sounds like, worked out from the world: the steady sounds
// (the wind, the boulder grinding as it rolls, Sisyphus straining against it,
// footsteps, crickets at night, the town when you are near it) and the
// one-off sounds of things happening (a release, an incident, a module going
// down, a patch). No device and no engine: the synth (synth.h) turns these
// into samples, and the tests can check them without either.
#pragma once

#include <vector>

#include "sim/world.h"

namespace sisyphus::audio {

// The steady sounds, each 0 (silent) to 1 (full).
struct Ambience {
    float wind = 0;      // always a little, more higher up
    float grind = 0;     // the boulder rolling, by its speed
    float strain = 0;    // pushing, by how heavy it is
    float crickets = 0;  // at night
    float town = 0;      // near the town, by how many users it has
    float alarm = 0;     // a module is down
    float steps = 0;     // footsteps per second
};

// One-off sounds.
enum class Cue : int {
    Release,    // a terrace reached: a release ships
    Summit,     // the next major version ships
    Fall,       // the platform's end of life: it starts to roll back
    Incident,   // something happened to a module
    Advisory,   // ... a security advisory, which is worse
    Down,       // a module rotted through
    Patched,    // a module patched
    Retired,    // a module retired
    Slip,       // too heavy: it starts to slide back
    Count
};

// `listener` is where the ears are; `stepsPerSecond` is how fast he walks.
Ambience ambience(const World& w, Vec3 listener, float stepsPerSecond, float sunUp);

// The one-off sounds for the change from `before` to `after` (one frame).
std::vector<Cue> cues(const World& before, const World& after);

}  // namespace sisyphus::audio
