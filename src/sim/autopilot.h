// A player made of rules, for screenshots, CI and watching: it patches the
// worst module it can reach while the boulder rests on a terrace, pushes on
// when the boulder is light enough, retires a module when it is too heavy
// and nothing within reach can be patched, and walks down after it when it
// rolls back to the trailhead.
#pragma once

#include <cstdint>
#include <string>

#include "sim/world.h"

namespace sisyphus {

class Autopilot {
public:
    Input next(const World& w);
    // What it is doing, in a few words ("pushing", "patching auth").
    const std::string& doing() const { return doing_; }

private:
    Input walkTo(const World& w, float x, float z, float within);
    bool pushingOn_ = false;  // started a push from a terrace; keep going to the next
    int releasesSeen_ = 0;    // a push ends when it ships a release
    std::string doing_;
};

// The autopilot's first major version: from the trailhead to the summit and
// back down to the trailhead again (1.0 shipped, the platform gone).
bool playthroughDone(const World& w);

// The world's fingerprint at the end of the playthrough from seed 1. Any
// change to the rules or the autopilot moves it: update it in the same change.
constexpr uint64_t kPlaythroughFingerprint = 0x974ce19b32f6b911ull;

}  // namespace sisyphus
