// The sounds: each change in the world makes the cue it should, the steady
// sounds follow the place, and the synth's output is finite and in range.
#include <cmath>

#include "audio/cues.h"
#include "audio/synth.h"
#include "check.h"
#include "sim/hill.h"

using namespace sisyphus;
using namespace sisyphus::audio;

namespace {
bool has(const std::vector<Cue>& cs, Cue c) {
    for (Cue x : cs)
        if (x == c) return true;
    return false;
}
}  // namespace

int main() {
    const World a = newWorld(1);
    CHECK(cues(a, a).empty());
    {
        World b = a;
        b.releases = 1;
        CHECK(has(cues(a, b), Cue::Release));
        b.summitSeconds = 0;
        CHECK(has(cues(a, b), Cue::Summit));
    }
    {
        World b = a;
        b.incidents = 1;
        b.lastIssue = Issue::Flaky;
        CHECK(has(cues(a, b), Cue::Incident));
        b.lastIssue = Issue::Advisory;
        CHECK(has(cues(a, b), Cue::Advisory));
    }
    {
        World b = a;
        b.breakdowns = 1;
        b.patched = 1;
        b.retired = 1;
        b.falling = true;
        b.slipping = true;
        const std::vector<Cue> cs = cues(a, b);
        CHECK(has(cs, Cue::Down) && has(cs, Cue::Patched) && has(cs, Cue::Retired) && has(cs, Cue::Fall) && has(cs, Cue::Slip));
    }
    // The steady sounds: windier at the top, crickets only at night, the town
    // heard from the valley, an alarm only during an outage.
    {
        World w = newWorld(1);
        const Ambience low = ambience(w, {0, 1.6f, 20}, 0, 0.5f);
        const Ambience high = ambience(w, {0, 70, hill::zOf(250)}, 0, 0.5f);
        CHECK(high.wind > low.wind);
        CHECK(low.crickets == 0 && ambience(w, {0, 1.6f, 20}, 0, -0.5f).crickets > 0.9f);
        w.users = 300;
        CHECK(ambience(w, {0, 1.6f, 20}, 0, 0.5f).town > ambience(w, {0, 70, hill::zOf(250)}, 0, 0.5f).town);
        CHECK(low.alarm == 0);
        w.modules[0].health = 0;
        CHECK(ambience(w, {0, 1.6f, 20}, 0, 0.5f).alarm == 1);
        w.boulder.v = 3;
        CHECK(ambience(w, {0, 1.6f, 0}, 0, 0.5f).grind > 0.5f);
    }
    // Every cue has a sound; everything the synth makes is finite and in range.
    Synth s;
    for (int c = 0; c < static_cast<int>(Cue::Count); ++c) CHECK(!s.sound(static_cast<Cue>(c)).empty());
    Ambience loud;
    loud.wind = loud.grind = loud.strain = loud.crickets = loud.town = loud.alarm = 1;
    loud.steps = 3;
    s.setAmbience(loud);
    for (int c = 0; c < static_cast<int>(Cue::Count); ++c) s.play(static_cast<Cue>(c));
    std::vector<float> out(kRate * 2);
    s.render(out.data(), static_cast<int>(out.size()));
    bool ok = true;
    float peak = 0;
    for (float v : out) {
        ok = ok && std::isfinite(v) && v >= -1 && v <= 1;
        peak = std::fmax(peak, std::fabs(v));
    }
    CHECK(ok);
    CHECK(peak > 0.05f);
    TEST_MAIN_END();
}
