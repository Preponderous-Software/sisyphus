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
    // A push from a terrace carries it up the slope to the next one, even
    // started a little off-centre and not quite square to the hill: he stays
    // on it rather than sliding round it (a playtest lost the boulder this way).
    // Turned well off the hill too (a second playtest slid off this way).
    for (float yaw : {-0.6f, -0.15f, 0.15f, 0.35f, 0.6f}) {
        const float off = yaw < 0 ? -0.5f : yaw < 0.3f ? 0.3f : 0.6f;
        World w = quiet();
        heal(w);
        w.best = 1;
        w.boulder.s = H::terraceStart(1) + 2.0f;
        standBelow(w);
        w.player.x = off;
        Input in = pushIn();
        in.yaw = yaw;
        for (int t = 0; t < 90 * kTickRate && w.best < 2; ++t) {
            heal(w);
            tick(w, in);
        }
        CHECK(w.best == 2);
    }
    // Letting go of E pauses the work; holding it again goes on from there, and
    // work of another kind starts over.
    {
        World w = quiet();
        int target = -1;
        for (int i = 0; i < kStartModules && target < 0; ++i)
            if (aimAt(w, i)) target = i;
        if (target >= 0) {
            w.modules[static_cast<size_t>(target)].health = 0.4f;
            Input in;
            in.yaw = w.player.yaw;
            in.pitch = w.player.pitch;
            in.work = true;
            run(w, in, 1);
            CHECK(w.work.module == target && w.work.seconds > 0.9f);
            in.work = false;
            const float done = w.work.seconds;
            run(w, in, 1);
            CHECK(w.work.module == target && w.work.seconds == done);
            in.work = true;
            tick(w, in);
            CHECK(w.work.module == target && w.work.seconds > done);
            in.work = false;
            in.retire = true;
            tick(w, in);
            CHECK(w.work.module == target && w.work.retiring && w.work.seconds < 0.1f);
            in.retire = false;
            // And a module as good as new has nothing to patch.
            w.modules[static_cast<size_t>(target)].health = 1;
            in.work = true;
            run(w, in, 3);
            CHECK(w.patched == 0);
        }
        CHECK(target >= 0);
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
    // Refactoring: hold F and the module folds into the one it uses, which
    // comes out new. No users are lost. The oldest module cannot be refactored.
    {
        World w = quiet();
        int target = -1;
        for (int i = 1; i < kStartModules && target < 0; ++i)
            if (aimAt(w, i) && canRefactor(w, i)) target = i;
        CHECK(target >= 0);
        if (target >= 0) {
            const int into = dependency(w, target);
            w.modules[static_cast<size_t>(into)].health = 0.3f;
            const float users = w.users;
            Input in;
            in.yaw = w.player.yaw;
            in.pitch = w.player.pitch;
            in.refactor = true;
            run(w, in, kRefactorSeconds - 1);
            CHECK(w.modules[static_cast<size_t>(target)].active);  // not yet
            run(w, in, 1.2f);
            CHECK(!w.modules[static_cast<size_t>(target)].active);
            CHECK(w.refactored == 1 && w.refactoredInto == into);
            const Module& kept = w.modules[static_cast<size_t>(into)];
            CHECK(kept.active && kept.health > 0.99f && kept.born == w.releases);
            CHECK(kept.issue == Issue::None || kept.issue == Issue::Flaky);
            CHECK(w.users == users);
            CHECK(activeModules(w) == kStartModules - 1);
        }
        CHECK(!canRefactor(w, 0));
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
        // ... and a cairn for 1.0 stands at the trailhead, telling its climb.
        CHECK(w.cairnCount == 1);
        CHECK(w.cairns[0].helpers >= 0 && w.helpersThisClimb == 0);
        CHECK(w.cairns[0].major == 1 && w.cairns[0].modules == modules && w.cairns[0].tick == w.tick);
        CHECK(cairnStory(w, 0).find("The cairn for 1.0") == 0);
        const Vec3 b = cairnBase(0);
        w.player.x = b.x;
        w.player.z = b.z + 0.5f;
        CHECK(cairnHere(w) == 0);
        // The oldest goes when there are too many.
        World many = w;
        for (int k = 1; k < kMaxCairns + 3; ++k) {
            many.cairns[static_cast<size_t>(many.cairnCount - 1)].major = 0;  // see that the oldest are the ones dropped
            many.major = k + 1;
            many.falling = true;
            many.boulder.s = H::kRestS + 0.01f;
            many.boulder.v = -1;
            for (int t = 0; t < 10 && many.falling; ++t) tick(many, Input{});
        }
        CHECK(many.cairnCount == kMaxCairns);
        CHECK(many.cairns[static_cast<size_t>(kMaxCairns - 1)].major == many.major);
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
    // Stamina: pushing wears him out, a spent Sisyphus is weaker, and
    // sitting down restores him.
    {
        World w = quiet();
        heal(w);
        standBelow(w);
        CHECK(strength(w) == 1);
        run(w, pushIn(), 10);
        CHECK(w.player.stamina < 0.99f);
        w.player.stamina = 0;
        CHECK(tired(w));
        CHECK(strength(w) == kSpentStrength);
        Input sit;
        sit.sit = true;
        tick(w, sit);
        CHECK(w.player.sitting);
        run(w, Input{}, 21);
        CHECK(w.player.stamina == 1);
        CHECK(!tired(w));
    }
    // Weather: rain slows the push and speeds the roll back; a shower sets in,
    // passes and clears; the seasons turn every few days, winter last.
    {
        auto pushedFor = [](float rain) {
            World w = quiet();
            heal(w);
            w.best = 1;
            w.boulder.s = H::terraceStart(1) + H::kFlat + 2.0f;  // on the slope
            standBelow(w);
            for (int t = 0; t < 4 * kTickRate; ++t) {
                heal(w);
                w.rain = rain;
                w.rainLeft = rain > 0 ? 100.0f : 0.0f;
                tick(w, pushIn());
            }
            return w.boulder.s;
        };
        const float start = H::terraceStart(1) + H::kFlat + 2.0f;
        CHECK(pushedFor(1) - start < 0.8f * (pushedFor(0) - start));
        auto rolledFor = [](float rain) {
            World w = quiet();
            heal(w);
            w.best = 2;
            w.boulder.s = H::terraceStart(1) + H::kFlat + 20.0f;
            w.player.x = 6;
            for (int t = 0; t < 2 * kTickRate; ++t) {
                w.rain = rain;
                w.rainLeft = rain > 0 ? 100.0f : 0.0f;
                tick(w, Input{});
            }
            return w.boulder.v;
        };
        CHECK(rolledFor(1) < rolledFor(0) - 0.5f);  // faster down the slope
        World w = quiet();
        w.rainLeft = 20;
        run(w, Input{}, 12);
        CHECK(w.rain == 1.0f);
        run(w, Input{}, 9);
        CHECK(w.rainLeft == 0 && w.rain < 1.0f);
        run(w, Input{}, 11);
        CHECK(w.rain == 0.0f);
        World s = quiet();
        CHECK(season(s) == Season::Spring);
        s.tick = static_cast<int64_t>(kDaySeconds * kTickRate) * kSeasonDays * 3;
        CHECK(season(s) == Season::Winter && snowing(s));
        s.tick = static_cast<int64_t>(kDaySeconds * kTickRate) * kSeasonDays * 4;
        CHECK(season(s) == Season::Spring);
    }
    // The rewrite: holding R at the summit (not retiring a module there)
    // chooses it; the moment at the top lasts while he decides; back at the
    // trailhead it starts again light, and the cairn says so.
    {
        World w = quiet();
        heal(w);
        for (int i = kStartModules; i < kStartModules + 6; ++i) {
            w.modules[static_cast<size_t>(i)].active = true;
            w.modules[static_cast<size_t>(i)].health = 0.4f;
        }
        w.added = kStartModules + 6;
        w.best = H::kSummit;
        w.boulder.s = H::terraceStart(H::kSummit) + 2.0f;
        w.summitSeconds = kSummitPause - 1.0f;  // a second left
        w.users = 1000;
        const int before = activeModules(w);
        Input hold;
        hold.retire = true;
        hold.yaw = w.player.yaw;
        hold.pitch = w.player.pitch;
        run(w, hold, 2.0f);
        CHECK(w.summitSeconds >= 0);  // still at the top: deciding
        CHECK(!w.rewriteChosen);
        CHECK(activeModules(w) == before);  // R did not retire a module here
        run(w, hold, 1.2f);
        CHECK(w.rewriteChosen);
        for (int t = 0; t < 120 * kTickRate && (w.summitSeconds >= 0 || w.falling); ++t) tick(w, Input{});
        CHECK(!w.falling && w.major == 1);
        CHECK(activeModules(w) == kStartModules);
        CHECK(health(w) > 0.999f && openIssues(w) == 0);  // fresh (rot has had one tick)
        CHECK(w.users < 1000 * kRewriteKeeps + 1);
        CHECK(w.cairnCount == 1 && w.cairns[0].rewrite && w.cairns[0].modules == before);
        CHECK(!w.rewriteChosen && w.rewriteSeconds == 0);
        CHECK(cairnStory(w, 0).find("rewritten") != std::string::npos);
    }
    // Let go at the top, and it rolls with everything aboard.
    {
        World w = quiet();
        heal(w);
        w.best = H::kSummit;
        w.boulder.s = H::terraceStart(H::kSummit) + 2.0f;
        w.summitSeconds = 0;
        Input hold;
        hold.retire = true;
        run(w, hold, 2.0f);
        run(w, Input{}, kSummitPause);
        CHECK(w.falling && !w.rewriteChosen);
    }
    // Helpers: someone comes up, patches what they can reach while the
    // boulder rests, and on leaving the module they knew best drifts.
    {
        World w = quiet();
        heal(w);
        w.helper.name = 3;
        w.helper.secondsLeft = 40;
        w.helper.x = -2;
        w.helper.z = hill::zOf(hill::kValleyS + 4.0f);
        w.player.x = 6;  // well clear of them
        int target = -1;
        for (int i = 0; i < kStartModules; ++i)
            if (rolled(w, moduleDirection(i)).y > 0.2f) target = i;
        CHECK(target >= 0);
        w.modules[static_cast<size_t>(target)].health = 0.5f;
        for (int t = 0; t < 39 * kTickRate; ++t) {
            for (int i = 0; i < kMaxModules; ++i)
                if (i != target) w.modules[static_cast<size_t>(i)].health = 1;
            tick(w, Input{});
        }
        CHECK(w.helper.name == 3);
        CHECK(w.modules[static_cast<size_t>(target)].health > 0.95f);  // patched (and aged a little since)
        CHECK(w.helper.patches[static_cast<size_t>(target)] >= 1);
        // Their 40 s count only beside the boulder, not the walk up from the
        // valley: they are still there now, and move on a little later.
        int64_t waited = 0;
        while (w.helper.name != -1 && waited < 120 * kTickRate) {
            tick(w, Input{});
            ++waited;
        }
        CHECK(waited > kTickRate);
        CHECK(w.helper.name == -1);  // moved on
        CHECK(w.busFactor == target);
        CHECK(w.modules[static_cast<size_t>(target)].issue == Issue::Drift);
    }
    // Walking straight into it without being lined up to push, he stops
    // against it instead of sliding off round it (a playtest walked on up the
    // path past it), and is told why.
    {
        World w = quiet();
        heal(w);
        w.best = 1;
        w.boulder.s = H::terraceStart(1) + 4.0f;
        const float R = boulderRadius(w), bz = H::zOf(w.boulder.s);
        w.player.x = R + 1.0f;
        w.player.z = bz;
        Input in;
        in.forward = 1;
        in.yaw = -kPi / 2;  // facing -x, at its side
        run(w, in, 3);
        CHECK(w.blocked);
        CHECK(w.player.x > R);  // still on that side
        CHECK(std::fabs(w.player.z - bz) < 0.3f);
        CHECK(w.boulder.s == H::terraceStart(1) + 4.0f);
    }
    // Dependencies: every module but the oldest uses an older one, and a
    // module that is down makes the ones that use it rot fast.
    {
        World w = quiet();
        heal(w);
        CHECK(dependency(w, 0) == -1);
        for (int i = 1; i < kStartModules; ++i) {
            const int d = dependency(w, i);
            CHECK(d >= 0 && d < i);
        }
        const int user = kStartModules - 1, used = dependency(w, user);
        const float before = rotRate(w, user);
        CHECK(!cascading(w, user));
        w.modules[static_cast<size_t>(used)].health = 0;
        CHECK(cascading(w, user));
        CHECK(rotRate(w, user) > before + 0.005f);
        // Patching the cause stops it.
        w.modules[static_cast<size_t>(used)].health = 1;
        CHECK(!cascading(w, user));
        CHECK(rotRate(w, user) == before);
        // Retiring what it used: it uses the next nearest older module.
        w.modules[static_cast<size_t>(used)].active = false;
        const int next = dependency(w, user);
        CHECK(next != used && next < user);
    }
    TEST_MAIN_END();
}
