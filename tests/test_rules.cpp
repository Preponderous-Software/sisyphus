// The rules, one at a time: pushing, releases, rolling back, rot and
// incidents, patching, retiring, the summit and the fall.
#include <cmath>

#include "check.h"
#include "sim/hill.h"
#include "sim/world.h"

using namespace sisyphus;
namespace H = sisyphus::hill;

namespace {

// A world with no incidents for a while: the rng is consumed, but nothing
// strikes unless the test makes it.
World quiet() {
    World w = newWorld(7);
    return w;
}

// Stands Sisyphus just below the boulder, facing up the hill.
void standBelow(World& w) {
    w.player.x = 0;
    w.player.z = H::zOf(w.boulder.s) + boulderRadius(w) + 0.5f;
    w.player.yaw = 0;
}

Input pushIn() {
    Input in;
    in.forward = 1;
    return in;
}

void run(World& w, const Input& in, float seconds) {
    for (int i = 0; i < static_cast<int>(seconds * kTickRate); ++i) tick(w, in);
}

void heal(World& w) {
    for (Module& m : w.modules) {
        m.health = 1;
        m.issue = Issue::None;
    }
}

// Aims at module i from where Sisyphus stands beside it; returns false if it cannot.
bool aimAt(World& w, int i) {
    const Vec3 d = rolled(w, moduleDirection(i));
    const Vec3 c = boulderCentre(w);
    const float R = boulderRadius(w);
    float hx = d.x, hz = d.z;
    const float h = std::sqrt(hx * hx + hz * hz);
    if (h < 0.3f || d.y < -0.4f) return false;
    w.player.x = c.x + hx / h * (R + 0.6f);
    w.player.z = c.z + hz / h * (R + 0.6f);
    const Vec3 to = (c + d * R) - eyePosition(w);
    w.player.yaw = std::atan2(to.x, -to.z);
    w.player.pitch = std::atan2(to.y, std::sqrt(to.x * to.x + to.z * to.z));
    return aimedModule(w) == i;
}

}  // namespace

int main() {
    // A new world: six modules at the trailhead, version 0.0.
    {
        const World w = newWorld(1);
        CHECK(activeModules(w) == kStartModules);
        CHECK(w.boulder.s == H::kRestS);
        CHECK(versionText(w) == "0.0");
        CHECK(nextVersionText(w) == "0.1");
        CHECK(!outage(w));
        CHECK(load(w) < 0.5f);
    }
    // Pushing from below moves it uphill and rolls it; walking into it from
    // the side does not push.
    {
        World w = quiet();
        standBelow(w);
        run(w, pushIn(), 3);
        CHECK(w.boulder.s > H::kRestS + 1.5f);
        CHECK(w.boulder.roll != 0);
        World side = quiet();
        side.player.x = boulderRadius(side) + 0.6f;
        side.player.z = H::zOf(side.boulder.s);
        side.player.yaw = -kPi / 2;
        Input in = pushIn();
        in.yaw = -kPi / 2;
        run(side, in, 2);
        CHECK(side.boulder.s == H::kRestS);
    }
    // Reaching the next terrace ships a release, which adds a module and users.
    {
        World w = quiet();
        standBelow(w);
        const float users = w.users;
        for (int t = 0; t < 120 * kTickRate && w.best < 1; ++t) {
            heal(w);
            tick(w, pushIn());
        }
        CHECK(w.best == 1);
        CHECK(w.releases == 1);
        CHECK(activeModules(w) == kStartModules + 1);
        CHECK(w.users > users);
        CHECK(versionText(w) == "0.1");
    }
    // Left on a slope, it rolls back down to the terrace below and stops there.
    {
        World w = quiet();
        w.boulder.s = H::terraceStart(1) + H::kFlat + 10.0f;
        w.player.x = 15;
        run(w, Input{}, 20);
        CHECK(H::terraceAt(w.boulder.s) == 1);
        CHECK(w.boulder.v == 0);
    }
    // Too heavy (rotten through), it slides back even while pushed on a slope;
    // on level ground it can still be inched along.
    {
        World w = quiet();
        for (Module& m : w.modules)
            if (m.active) m.health = 0;
        CHECK(load(w) >= 1);
        w.boulder.s = H::terraceStart(1) + H::kFlat + 10.0f;
        standBelow(w);
        const float s0 = w.boulder.s;
        tick(w, pushIn());
        run(w, pushIn(), 1);
        CHECK(w.slipping);
        CHECK(w.boulder.s < s0);
        World flat = quiet();
        for (Module& m : flat.modules)
            if (m.active) m.health = 0.05f;
        standBelow(flat);
        run(flat, pushIn(), 2);
        CHECK(flat.boulder.s > H::kRestS + 0.2f);
    }
    // On a terrace it can be pushed back from above, to turn it.
    {
        World w = quiet();
        w.boulder.s = H::terraceStart(2) + 6.0f;
        w.player.x = 0;
        w.player.z = H::zOf(w.boulder.s) - boulderRadius(w) - 0.5f;
        Input in = pushIn();
        in.yaw = kPi;
        const float s0 = w.boulder.s;
        run(w, in, 2);
        CHECK(w.pushingBack);
        CHECK(w.boulder.s < s0 - 0.3f);
        // Kept at it, it stops at the terrace's edge instead of going over.
        run(w, in, 15);
        CHECK(w.boulder.s >= H::terraceStart(2) + 0.29f);
        CHECK(H::terraceAt(w.boulder.s) == 2);
    }
    // Rot: every module ages, an incident makes one rot fast, and a module
    // rotted through is down, and users leave.
    {
        World w = quiet();
        w.modules[1].issue = Issue::Advisory;
        run(w, Input{}, 10);
        CHECK(w.modules[1].health < w.modules[2].health);
        CHECK(w.modules[2].health < 1);
        w.modules[1].health = 0.001f;
        const float users = w.users;
        run(w, Input{}, 2);
        CHECK(outage(w));
        CHECK(w.breakdowns >= 1);
        CHECK(w.users < users);
        // Incidents come on their own.
        World busy = quiet();
        run(busy, Input{}, 300);
        CHECK(busy.incidents > 0);
        CHECK(openIssues(busy) > 0 || busy.breakdowns > 0);
    }
    // An advisory left to rot spreads to its neighbour.
    {
        World w = quiet();
        w.modules[0].issue = Issue::Advisory;
        w.modules[0].health = 0.4f;
        for (int t = 0; t < 120 * kTickRate && openIssues(w) < 2; ++t) {
            w.modules[0].health = 0.4f;
            tick(w, Input{});
        }
        int advisories = 0;
        for (const Module& m : w.modules) advisories += m.active && m.issue == Issue::Advisory ? 1 : 0;
        CHECK(advisories >= 2);
    }
    // Patching: hold E on a module for long enough and it is as new.
    {
        World w = quiet();
        int target = -1;
        for (int i = 0; i < kStartModules && target < 0; ++i)
            if (aimAt(w, i)) target = i;
        CHECK(target >= 0);
        if (target >= 0) {
            w.modules[static_cast<size_t>(target)].health = 0.3f;
            w.modules[static_cast<size_t>(target)].issue = Issue::Drift;
            Input in;
            in.yaw = w.player.yaw;
            in.pitch = w.player.pitch;
            in.work = true;
            const float need = patchSeconds(w.modules[static_cast<size_t>(target)]);
            run(w, in, need * 0.5f);
            CHECK(w.modules[static_cast<size_t>(target)].issue == Issue::Drift);
            CHECK(w.work.module == target);
            run(w, in, need * 0.6f + 0.5f);
            CHECK(w.modules[static_cast<size_t>(target)].health > 0.99f);
            CHECK(w.modules[static_cast<size_t>(target)].issue == Issue::None);
            CHECK(w.patched == 1);
        }
    }
    // Retiring: hold R and the module goes, down to a minimum.
    {
        World w = quiet();
        int target = -1;
        for (int i = 0; i < kStartModules && target < 0; ++i)
            if (aimAt(w, i)) target = i;
        CHECK(target >= 0);
        if (target >= 0) {
            Input in;
            in.yaw = w.player.yaw;
            in.pitch = w.player.pitch;
            in.retire = true;
            run(w, in, kRetireSeconds + 0.2f);
            CHECK(!w.modules[static_cast<size_t>(target)].active);
            CHECK(w.retired == 1);
            CHECK(activeModules(w) == kStartModules - 1);
        }
        World few = quiet();
        for (int i = kMinModules; i < kMaxModules; ++i) few.modules[static_cast<size_t>(i)].active = false;
        bool any = false;
        for (int i = 0; i < kMinModules; ++i)
            if (aimAt(few, i)) {
                any = true;
                Input in;
                in.yaw = few.player.yaw;
                in.pitch = few.player.pitch;
                in.retire = true;
                run(few, in, kRetireSeconds + 1);
                break;
            }
        CHECK(any);
        CHECK(activeModules(few) == kMinModules);
    }
    // The summit ships the next major version; then it rolls all the way back
    // to the trailhead, every module still aboard, and the next major begins.
    {
        World w = quiet();
        heal(w);
        w.best = H::kSummit - 1;
        w.boulder.s = H::terraceStart(H::kSummit) - 1.0f;
        standBelow(w);
        for (int t = 0; t < 20 * kTickRate && w.summitSeconds < 0; ++t) {
            heal(w);
            tick(w, pushIn());
        }
        CHECK(w.summitSeconds >= 0);
        CHECK(versionText(w) == "1.0");
        const int modules = activeModules(w);
        w.player.x = 15;
        run(w, Input{}, kSummitPause + 0.5f);
        CHECK(w.falling);
        for (int t = 0; t < 120 * kTickRate && w.falling; ++t) tick(w, Input{});
        CHECK(!w.falling);
        CHECK(w.boulder.s == H::kRestS);
        CHECK(w.major == 1);
        CHECK(w.best == 0);
        CHECK(versionText(w) == "1.0");
        CHECK(nextVersionText(w) == "1.1");
        CHECK(activeModules(w) == modules);
    }
    // Sitting: he stays put, and the time is counted.
    {
        World w = quiet();
        Input sit;
        sit.sit = true;
        tick(w, sit);
        CHECK(w.player.sitting);
        const float x = w.player.x, z = w.player.z;
        run(w, pushIn(), 2);
        CHECK(w.player.x == x && w.player.z == z);
        CHECK(w.satSeconds > 1.9f);
        tick(w, sit);
        CHECK(!w.player.sitting);
    }
    TEST_MAIN_END();
}
