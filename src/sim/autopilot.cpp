#include "sim/autopilot.h"

#include "sim/hill.h"

namespace sisyphus {

namespace {

constexpr float kStandOff = 0.55f;  // from the boulder's surface, to work on it
constexpr float kNeedy = 0.8f;      // patch modules below this health before pushing on
constexpr float kPushBelow = 0.7f; // start a push only when the load is under this
constexpr float kSitBelow = 0.25f;  // sit down to rest below this stamina ...
constexpr float kRested = 0.95f;    // ... until this
constexpr float kRestBeforePush = 0.5f;

struct Reach {
    bool ok = false;
    float x = 0, z = 0;  // where to stand
    Vec3 at;             // the module's centre on the surface
};

// Where to stand to patch module i, if it can be reached from the ground.
Reach reachFor(const World& w, int i) {
    Reach r;
    const Vec3 d = rolled(w, moduleDirection(i));
    if (d.y < -0.45f) return r;  // underneath
    const Vec3 c = boulderCentre(w);
    const float R = boulderRadius(w);
    float hx = d.x, hz = d.z;
    float h = std::sqrt(hx * hx + hz * hz);
    if (h < 0.25f) {  // near the top: from the downhill side
        hx = 0;
        hz = 1;
        h = 1;
    }
    r.x = c.x + hx / h * (R + kStandOff);
    r.z = c.z + hz / h * (R + kStandOff);
    if (r.x < -hill::kWalkHalf || r.x > hill::kWalkHalf) return r;
    r.at = c + d * R;
    const Vec3 eye{r.x, hill::groundHeight(r.x, r.z) + kEyeHeight, r.z};
    const Vec3 to = r.at - eye;
    const float len = length(to);
    if (len > kReach - 0.15f || len < 1e-3f) return r;
    if (moduleOnSight(w, eye, to * (1.0f / len)) != i) return r;
    r.ok = true;
    return r;
}

float yawTo(float dx, float dz) { return detAtan2(dx, -dz); }

// Whether a module that needs work is out of reach (underneath, or on top of
// a big boulder).
bool needyOutOfReach(const World& w) {
    for (int i = 0; i < kMaxModules; ++i) {
        const Module& m = w.modules[static_cast<size_t>(i)];
        if (m.active && m.health < kNeedy && !reachFor(w, i).ok) return true;
    }
    return false;
}

// More is down than one pair of hands can bring back: time to cut features.
bool overwhelmed(const World& w, float load) {
    const int down = brokenModules(w);
    return activeModules(w) > kMinModules && (down >= 3 || (down >= 2 && load >= 1));
}

// A module that is down and can be reached, or -1.
int downInReach(const World& w) {
    for (int i = 0; i < kMaxModules; ++i) {
        const Module& m = w.modules[static_cast<size_t>(i)];
        if (m.active && m.health <= 0 && reachFor(w, i).ok) return i;
    }
    return -1;
}

}  // namespace

Input Autopilot::walkTo(const World& w, float x, float z, float within) {
    Input in;
    const Player& p = w.player;
    in.yaw = p.yaw;
    in.pitch = 0;
    float tx = x, tz = z;
    // Go round the boulder rather than into it: if the straight line passes
    // through it, head for a point beside it first.
    const Vec3 c = boulderCentre(w);
    const float R = boulderRadius(w) + 0.9f;
    const float dx = tx - p.x, dz = tz - p.z;
    const float len = std::sqrt(dx * dx + dz * dz);
    if (len < within) return in;
    const float ux = dx / len, uz = dz / len;
    const float along = (c.x - p.x) * ux + (c.z - p.z) * uz;
    if (along > 0 && along < len) {
        const float px = p.x + ux * along - c.x, pz = p.z + uz * along - c.z;
        const float off = std::sqrt(px * px + pz * pz);
        if (off < R) {
            // The side of the boulder the line passes on (or the right, dead centre).
            float sx = px, sz = pz;
            if (off < 1e-3f) {
                sx = -uz;
                sz = ux;
            } else {
                sx /= off;
                sz /= off;
            }
            tx = c.x + sx * (R + 0.4f);
            tz = c.z + sz * (R + 0.4f);
        }
    }
    in.yaw = yawTo(tx - p.x, tz - p.z);
    // Slowing for the last step, so it stops where it meant to.
    const float step = 2.2f / kTickRate;
    in.forward = len < step ? len / step : 1;
    in.hurry = len > 8;
    return in;
}

Input Autopilot::next(const World& w) {
    const Player& p = w.player;
    if (p.sitting) {
        Input in;
        in.yaw = p.yaw;
        // Resting while the boulder rests; up again when rested, or if it moves.
        if (p.stamina < kRested && boulderResting(w)) {
            doing_ = "resting";
            return in;
        }
        in.sit = true;  // stand up
        return in;
    }
    const Vec3 c = boulderCentre(w);
    const float R = boulderRadius(w);

    // Rolling back down: keep out of its way and follow it.
    if (w.falling) {
        doing_ = "following it down";
        pushingOn_ = false;
        return walkTo(w, 6.0f, c.z - 4.0f > hill::zOf(hill::kRestS) + 6.0f ? hill::zOf(hill::kRestS) + 6.0f : c.z - 4.0f, 0.5f);
    }

    // The module most in need that can be reached, and whether any needy one cannot.
    int worst = -1;
    float worstHealth = 2;
    Reach worstReach;
    for (int i = 0; i < kMaxModules; ++i) {
        const Module& m = w.modules[static_cast<size_t>(i)];
        if (!m.active) continue;
        float need = m.health;
        if (m.issue == Issue::Advisory) need -= 0.2f;  // it spreads
        if (m.issue != Issue::None) need -= 0.1f;
        if (need >= worstHealth || w.tick < skipUntil_[static_cast<size_t>(i)]) continue;
        const Reach r = reachFor(w, i);
        if (!r.ok) continue;
        worst = i;
        worstHealth = need;
        worstReach = r;
    }

    // A push ends at the next terrace: stop there and look after it.
    if (w.releases != releasesSeen_) {
        releasesSeen_ = w.releases;
        pushingOn_ = false;
    }
    const bool resting = boulderResting(w) || w.summitSeconds >= 0;
    const float L = load(w);
    if (resting) pushingOn_ = false;
    // Spent, with the boulder settled and at most one module down: sit down for a while.
    const auto sitDown = [&] {
        doing_ = "sitting down to rest";
        Input in;
        in.yaw = p.yaw;
        in.sit = true;
        return in;
    };
    if (boulderResting(w) && p.stamina < kSitBelow && brokenModules(w) <= 1) return sitDown();
    bool push = false, patch = false, turn = false;
    if (w.summitSeconds >= 0) patch = worst >= 0;  // nothing to push at the top
    else if (!resting) push = pushingOn_ && (L < 1 || hill::level(w.boulder.s));
    else if (overwhelmed(w, L) && downInReach(w) >= 0) {}  // more down than it can patch: cut one (below)
    else if (worst >= 0 && worstHealth < kNeedy) patch = true;
    else if (L < kPushBelow) push = true;
    else if (needyOutOfReach(w)) turn = true;  // roll it to bring them round
    else if (worst >= 0 && worstHealth < 0.98f) patch = true;
    else push = L < 1;
    // About to push on from a terrace half spent: rest first.
    if (push && boulderResting(w) && p.stamina < kRestBeforePush) return sitDown();

    // At the summit there is nothing to push, and R would be a rewrite,
    // which the autopilot never chooses: it waits.
    if (w.summitSeconds >= 0 && !patch) {
        doing_ = "at the summit";
        Input still;
        still.yaw = p.yaw;
        return still;
    }
    if (!push && !patch && !turn && !resting) {
        // Slipping back to the terrace below: wait beside it.
        doing_ = "letting it settle";
        pushingOn_ = false;
        if (hill::level(w.boulder.s)) {
            Input still;
            still.yaw = p.yaw;
            return still;
        }
        return walkTo(w, R + 2.5f, c.z + 1.0f, 0.5f);
    }

    if (patch) {
        const Module& m = w.modules[static_cast<size_t>(worst)];
        doing_ = std::string(m.issue == Issue::None ? "tending " : "patching ") + moduleName(worst);
        const float dx = worstReach.x - p.x, dz = worstReach.z - p.z;
        const Vec3 eye = eyePosition(w);
        const Vec3 to = worstReach.at - eye;
        // From where it actually stands, the line of sight must land on this module.
        const bool onTarget = moduleOnSight(w, eye, to * (1.0f / length(to))) == worst;
        if (!onTarget && dx * dx + dz * dz > 0.03f * 0.03f) return walkTo(w, worstReach.x, worstReach.z, 0.02f);
        if (!onTarget) {
            skipUntil_[static_cast<size_t>(worst)] = w.tick + 10 * kTickRate;  // try another for a while
            Input still;
            still.yaw = p.yaw;
            return still;
        }
        Input in;
        in.yaw = yawTo(to.x, to.z);
        in.pitch = detAtan2(to.y, std::sqrt(to.x * to.x + to.z * to.z));
        in.work = true;
        return in;
    }

    if (turn) {
        // Turning it on the terrace: back towards the terrace's start from
        // above, or on from below, whichever has room.
        doing_ = "turning it";
        const bool back = w.boulder.s - hill::terraceStart(terraceOfBoulder(w)) > hill::kFlat / 2;
        const float rz = p.z - c.z, ax = p.x < 0 ? -p.x : p.x;
        const bool placed = back ? rz < -0.4f * R && rz > -(R + 1.0f) : rz > 0.4f * R && rz < R + 1.0f;
        if (placed && ax < 0.5f * R) {
            Input in;
            in.yaw = back ? kPi : 0;
            in.forward = 1;
            in.strafe = clampf(back ? p.x : -p.x, -0.5f, 0.5f);
            return in;
        }
        return walkTo(w, 0, back ? c.z - R - 0.6f : c.z + R + 0.6f, 0.2f);
    }

    if (!push) {
        // Too heavy, and nothing in reach to patch, or overwhelmed: cut a feature.
        doing_ = "retiring a module";
        int victim = -1;
        float lowest = 2;
        Reach vr;
        if (overwhelmed(w, L)) {  // only one that is down
            victim = downInReach(w);
            if (victim >= 0) vr = reachFor(w, victim);
        }
        for (int i = 0; i < kMaxModules && victim < 0; ++i) {
            const Module& m = w.modules[static_cast<size_t>(i)];
            if (!m.active || m.health >= lowest) continue;
            const Reach r = reachFor(w, i);
            if (!r.ok) continue;
            victim = i;
            lowest = m.health;
            vr = r;
        }
        if (victim < 0) return walkTo(w, 0, c.z + R + 0.6f, 0.3f);
        const float dx = vr.x - p.x, dz = vr.z - p.z;
        if (dx * dx + dz * dz > 0.3f * 0.3f) return walkTo(w, vr.x, vr.z, 0.25f);
        Input in;
        const Vec3 to = vr.at - eyePosition(w);
        in.yaw = yawTo(to.x, to.z);
        in.pitch = detAtan2(to.y, std::sqrt(to.x * to.x + to.z * to.z));
        in.retire = true;
        return in;
    }

    // Pushing: from straight below it, facing up the hill.
    doing_ = "pushing";
    const float rz = p.z - c.z;
    const float ax = p.x < 0 ? -p.x : p.x;
    if (rz > 0.4f * R && ax < 0.5f * R && rz < R + 1.0f) {
        pushingOn_ = true;
        Input in;
        in.yaw = 0;
        in.pitch = 0;
        in.forward = 1;
        in.strafe = clampf(-p.x, -0.5f, 0.5f);
        return in;
    }
    return walkTo(w, 0, c.z + R + 0.6f, 0.2f);
}

bool playthroughDone(const World& w) { return w.major >= 1 && !w.falling; }

}  // namespace sisyphus
