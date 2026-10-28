#include "audio/cues.h"

#include <cmath>

#include "sim/hill.h"

namespace sisyphus::audio {

namespace {
float clamp01(float v) { return v < 0 ? 0 : v > 1 ? 1 : v; }
}  // namespace

Ambience ambience(const World& w, Vec3 listener, float stepsPerSecond, float sunUp) {
    Ambience a;
    const float s = hill::sOf(listener.z);
    a.wind = 0.15f + 0.45f * clamp01(s / hill::kTopS);
    // The boulder: how fast it rolls, heard less the further away it is.
    const Vec3 c = boulderCentre(w);
    const float dx = c.x - listener.x, dz = c.z - listener.z;
    const float near = clamp01(1.0f - std::sqrt(dx * dx + dz * dz) / 60.0f);
    a.grind = clamp01(std::fabs(w.boulder.v) / 4.0f) * near;
    if (w.pushing) a.strain = clamp01(load(w));
    a.crickets = clamp01((0.05f - sunUp) / 0.15f);
    a.town = clamp01(1.0f - (s + 30.0f) / 50.0f) * clamp01(w.users / 300.0f);
    a.alarm = outage(w) ? 1.0f : 0.0f;
    a.steps = w.player.sitting ? 0 : stepsPerSecond;
    return a;
}

std::vector<Cue> cues(const World& before, const World& after) {
    std::vector<Cue> out;
    if (after.releases > before.releases) out.push_back(after.summitSeconds >= 0 ? Cue::Summit : Cue::Release);
    if (after.falling && !before.falling) out.push_back(Cue::Fall);
    if (after.incidents > before.incidents)
        out.push_back(after.lastIssue == Issue::Advisory ? Cue::Advisory : Cue::Incident);
    if (after.breakdowns > before.breakdowns) out.push_back(Cue::Down);
    if (after.patched > before.patched) out.push_back(Cue::Patched);
    if (after.retired > before.retired) out.push_back(Cue::Retired);
    if (after.slipping && !before.slipping) out.push_back(Cue::Slip);
    return out;
}

}  // namespace sisyphus::audio
