#include "sim/world.h"

#include <algorithm>
#include <cstring>

#include "sim/hill.h"
#include "sim/rng.h"

namespace sisyphus {

namespace {

constexpr float kDt = 1.0f / kTickRate;
constexpr float kWalkSpeed = 2.2f, kHurrySpeed = 3.6f;  // metres per second
constexpr float kPushSpeed = 1.7f;    // an unrotted boulder, pushed up a slope
constexpr float kFlatEase = 1.35f;    // pushing along a terrace is easier
constexpr float kRollAccel = 2.2f;    // a boulder left on a slope speeds downhill
constexpr float kMaxRoll = 8.0f;
constexpr float kFallAccel = 3.0f, kMaxFall = 9.0f;
constexpr float kSlideBack = 0.35f;   // too heavy to push: it slides back this fast
constexpr float kInch = 0.25f;        // on level ground it can always be inched along
constexpr float kBody = 0.45f;        // Sisyphus's radius, for bumping into the boulder
constexpr float kContact = 0.75f;     // close enough to push

// Rot, per second. Everything ages slowly; an issue makes it fast.
constexpr float kBaseRot = 1.0f / 1200.0f;
constexpr float kIssueRot[kIssueKinds] = {0, 1.0f / 200, 1.0f / 240, 1.0f / 180, 1.0f / 90, 1.0f / 110};
// Incidents a second, per active module (more modules, more trouble).
constexpr float kIncidentRate = 1.0f / 240.0f;
// How likely each kind of incident is (weights, in Issue order after None).
constexpr int kIssueWeight[kIssueKinds] = {0, 30, 25, 20, 15, 10};
constexpr float kSpreadBelow = 0.5f;    // an advisory spreads once its module is this rotten
constexpr float kSpreadRate = 1.0f / 10.0f;
constexpr float kCascadeRot = 1.0f / 150.0f;  // using a module that is down
constexpr float kRefactorBreaks = 0.25f;      // the chance a refactor breaks a test
// Showers: they start this often when it is dry, last a minute or two, and
// take ten seconds to set in and to clear.
constexpr float kShowerRate = 1.0f / 400.0f;
constexpr float kShowerMin = 60.0f, kShowerSpread = 90.0f, kRainFade = 10.0f;
constexpr float kRainSlows = 0.3f, kRainSpeedsRoll = 0.4f;
// Stamina, per second: pushing a light boulder takes it from full to spent
// in about six minutes, a heavy one in under four; working, in ten.
constexpr float kPushTire = 1.0f / 240.0f;  // times (0.4 + 0.6 * load)
constexpr float kWorkTire = 1.0f / 600.0f;
constexpr float kHurryTire = 1.0f / 400.0f;
constexpr float kCatchBreath = 1.0f / 300.0f;  // standing, or walking
constexpr float kRest = 1.0f / 20.0f;          // sitting

const char* const kNames[kMaxModules] = {
    "core",          "auth",         "parser",        "router",       "storage",       "ui",
    "search",        "billing",      "cache",         "mailer",       "scheduler",     "uploads",
    "sessions",      "i18n",         "metrics",       "logging",      "notifications", "admin",
    "migrations",    "feature-flags","webhooks",      "export",       "import",        "rate-limit",
    "sso",           "audit-log",    "backups",       "image-resize", "pdf-render",    "websockets",
    "graphql",       "theming",      "onboarding",    "recommender",  "analytics",     "dark-mode",
    "offline-sync",  "chat",         "captcha",       "geo-lookup",   "legacy-soap",   "plugin-api",
    "markdown",      "timezones",    "accessibility", "ab-testing",   "crash-reports", "string-utils",
};

float clamp01(float v) { return v < 0 ? 0 : v > 1 ? 1 : v; }

float wrapAngle(float a) {
    while (a > kPi) a -= 2 * kPi;
    while (a < -kPi) a += 2 * kPi;
    return a;
}

void release(World& w, int terrace) {
    w.best = terrace;
    ++w.releases;
    w.users += 30.0f * static_cast<float>(w.major + 1) + 10.0f * static_cast<float>(terrace);
    // Every release ships a feature, and every feature is a module to keep.
    if (w.added < kMaxModules) {
        Module& m = w.modules[static_cast<size_t>(w.added++)];
        m = Module{};
        m.active = true;
        m.born = w.releases;
    }
}

Issue pickIssue(World& w) {
    // In winter certificates expire twice as often.
    int weight[kIssueKinds];
    for (int k = 0; k < kIssueKinds; ++k) weight[k] = kIssueWeight[k];
    if (season(w) == Season::Winter) weight[static_cast<int>(Issue::Certificate)] *= 2;
    int total = 0;
    for (int k : weight) total += k;
    int r = randomBelow(w.rng, total);
    for (int k = 1; k < kIssueKinds; ++k) {
        if (r < weight[k]) return static_cast<Issue>(k);
        r -= weight[k];
    }
    return Issue::Drift;
}

void weather(World& w) {
    if (w.rainLeft > 0) {
        w.rainLeft = std::max(0.0f, w.rainLeft - kDt);
        w.rain = std::min(1.0f, w.rain + kDt / kRainFade);
    } else if (w.rain > 0) {
        w.rain = std::max(0.0f, w.rain - kDt / kRainFade);
    } else if (randomUnit(w.rng) < kShowerRate * kDt) {
        w.rainLeft = kShowerMin + kShowerSpread * randomUnit(w.rng);
    }
}

void strike(World& w, int i, Issue issue) {
    w.modules[static_cast<size_t>(i)].issue = issue;
    ++w.incidents;
    w.lastIncident = i;
    w.lastIssue = issue;
}

// The nearest other active module, for an advisory to spread to.
int neighbour(const World& w, int i) {
    const Vec3 a = moduleDirection(i);
    int best = -1;
    float bestDot = -2;
    for (int j = 0; j < kMaxModules; ++j) {
        if (j == i || !w.modules[static_cast<size_t>(j)].active) continue;
        const float d = dot(a, moduleDirection(j));
        if (d > bestDot) {
            bestDot = d;
            best = j;
        }
    }
    return best;
}

void tire(World& w, const Input& in) {
    Player& p = w.player;
    float change;
    if (p.sitting) change = kRest;
    else if (w.pushing) change = -kPushTire * (0.4f + 0.6f * clampf(load(w), 0, 1));
    else if (w.work.module >= 0) change = -kWorkTire;
    else if (in.hurry && (in.forward != 0 || in.strafe != 0)) change = -kHurryTire;
    else change = kCatchBreath;
    p.stamina = clamp01(p.stamina + change * kDt);
}

void rot(World& w) {
    // Every rate first, so a module going down this tick does not speed up
    // the rest until the next one.
    std::array<float, kMaxModules> rate{};
    for (int i = 0; i < kMaxModules; ++i)
        if (w.modules[static_cast<size_t>(i)].active) rate[static_cast<size_t>(i)] = rotRate(w, i);
    for (int i = 0; i < kMaxModules; ++i) {
        Module& m = w.modules[static_cast<size_t>(i)];
        if (!m.active) continue;
        const float was = m.health;
        m.health = clamp01(m.health - rate[static_cast<size_t>(i)] * kDt);
        if (was > 0 && m.health <= 0) ++w.breakdowns;
    }
    const int n = activeModules(w);
    // Trouble comes more slowly while much is already open, so one pair of
    // hands is pressed, not buried.
    const float p = static_cast<float>(n) * kIncidentRate * (1.0f + 0.1f * static_cast<float>(w.major)) /
                    (1.0f + 0.25f * static_cast<float>(openIssues(w))) * kDt;
    if (randomUnit(w.rng) < p) {
        for (int tries = 0; tries < 8; ++tries) {
            const int i = randomBelow(w.rng, kMaxModules);
            const Module& m = w.modules[static_cast<size_t>(i)];
            if (!m.active || m.issue != Issue::None) continue;
            strike(w, i, pickIssue(w));
            break;
        }
    }
    for (int i = 0; i < kMaxModules; ++i) {
        const Module& m = w.modules[static_cast<size_t>(i)];
        if (!m.active || m.issue != Issue::Advisory || m.health >= kSpreadBelow) continue;
        if (randomUnit(w.rng) >= kSpreadRate * kDt) continue;
        const int j = neighbour(w, i);
        if (j >= 0 && w.modules[static_cast<size_t>(j)].issue == Issue::None) strike(w, j, Issue::Advisory);
    }
}

const char* const kHelperNameList[kHelperNames] = {"Ana", "Ben",  "Chen", "Dara", "Eli",  "Fatima", "Gus",  "Hana",
                                                   "Ivan", "Jo",  "Kofi", "Lena", "Maya", "Noor",   "Omar", "Priya"};

void arrive(World& w) {
    Helper& h = w.helper;
    if (h.name >= 0 || randomUnit(w.rng) >= kHelperChance) return;
    h = Helper{};
    h.name = randomBelow(w.rng, kHelperNames);
    h.secondsLeft = kHelperStays;
    h.x = w.player.x >= 0 ? -2.0f : 2.0f;  // on the other side of the path from him
    h.z = hill::zOf(hill::kValleyS + 4.0f);  // up from the town
    ++w.helpersThisClimb;
}

void help(World& w) {
    Helper& h = w.helper;
    if (h.name < 0) return;
    // Over to the boulder's side, at a walk.
    const Vec3 to = helperSpot(w);
    const float dx = to.x - h.x, dz = to.z - h.z;
    const float d = std::sqrt(dx * dx + dz * dz);
    const float step = kWalkSpeed * kDt;
    if (d > step) {
        h.x += dx / d * step;
        h.z += dz / d * step;
    } else {
        h.x = to.x;
        h.z = to.z;
    }
    // Their time counts only beside the boulder, not on the walk up from
    // the town or while catching up with it.
    if (d < 2.0f) h.secondsLeft -= kDt;
    if (h.secondsLeft <= 0) {
        // Moving on: what they knew best starts to drift.
        int best = -1;
        for (int i = 0; i < kMaxModules; ++i)
            if (w.modules[static_cast<size_t>(i)].active && h.patches[static_cast<size_t>(i)] > 0 &&
                (best < 0 || h.patches[static_cast<size_t>(i)] > h.patches[static_cast<size_t>(best)]))
                best = i;
        w.busFactor = best;
        if (best >= 0 && w.modules[static_cast<size_t>(best)].issue == Issue::None) strike(w, best, Issue::Drift);
        h = Helper{};
        return;
    }
    // Working only beside a boulder at rest, on what they can reach: the
    // worst module not underneath it.
    if (d > 0.3f || !(boulderResting(w) || w.summitSeconds >= 0)) {
        h.module = -1;
        h.work = 0;
        return;
    }
    int worst = -1;
    for (int i = 0; i < kMaxModules; ++i) {
        const Module& m = w.modules[static_cast<size_t>(i)];
        if (!m.active || m.health >= 0.98f || rolled(w, moduleDirection(i)).y < -0.3f) continue;
        if (i == w.work.module) continue;  // Sisyphus has that one
        if (worst < 0 || m.health < w.modules[static_cast<size_t>(worst)].health) worst = i;
    }
    if (worst != h.module) {
        h.module = worst;
        h.work = 0;
    }
    if (worst < 0) return;
    h.work += kDt;
    Module& m = w.modules[static_cast<size_t>(worst)];
    if (h.work < patchSeconds(m) * kHelperSlower) return;
    m.health = 1;
    m.issue = Issue::None;
    ++h.patches[static_cast<size_t>(worst)];
    h.module = -1;
    h.work = 0;
}

void raiseCairn(World& w) {
    if (w.cairnCount == kMaxCairns) {
        for (int k = 1; k < kMaxCairns; ++k) w.cairns[static_cast<size_t>(k - 1)] = w.cairns[static_cast<size_t>(k)];
        --w.cairnCount;
    }
    Cairn& c = w.cairns[static_cast<size_t>(w.cairnCount++)];
    c.major = w.major;
    c.tick = w.tick;
    c.users = w.users;
    c.modules = activeModules(w);
    c.patched = w.patched;
    c.retired = w.retired;
    c.refactored = w.refactored;
    c.breakdowns = w.breakdowns;
    c.helpers = w.helpersThisClimb;
    c.rewrite = w.rewriteChosen;
    w.helpersThisClimb = 0;
}

// Starting again light: the oldest few modules, written fresh; the rest gone,
// and most of the users with them.
void rewrite(World& w) {
    int kept = 0;
    for (Module& m : w.modules) {
        if (!m.active) continue;
        if (kept < kStartModules) {
            ++kept;
            m.health = 1;
            m.issue = Issue::None;
            m.born = w.releases;
        } else {
            m = Module{};
        }
    }
    w.users *= kRewriteKeeps;
    w.work = Work{};
    w.rewriteChosen = false;
    w.rewriteSeconds = 0;
}

void moveBoulder(World& w, const Input& in) {
    Boulder& b = w.boulder;
    const float R = boulderRadius(w);
    const bool flat = hill::level(b.s);
    w.slipping = false;
    if (w.falling) {
        b.v -= kFallAccel * kDt;
        if (b.v < -kMaxFall) b.v = -kMaxFall;
    } else if (w.summitSeconds >= 0) {
        b.v = 0;  // the moment at the top
    } else if (w.pushing) {
        const float L = load(w);
        float target;
        const float f = in.forward < 1 ? in.forward : 1;
        if (L < 1) target = kPushSpeed * (flat ? kFlatEase : 1.0f) * (1 - L) * f * strength(w) * (1 - kRainSlows * w.rain);
        else target = flat ? 0 : -kSlideBack;
        if (flat && target < kInch * f) target = kInch * f;
        if (w.pushingBack) target = -target;
        w.slipping = !flat && L >= 1;
        const float k = 6.0f * kDt;
        b.v += (target - b.v) * k;
    } else if (flat) {
        b.v *= 0.8f;
        if (b.v > -0.02f && b.v < 0.02f) b.v = 0;
    } else {
        // A slick path lets it go faster.
        b.v -= kRollAccel * (1 + kRainSpeedsRoll * w.rain) * kDt;
        if (b.v < -kMaxRoll * (1 + kRainSpeedsRoll * w.rain)) b.v = -kMaxRoll * (1 + kRainSpeedsRoll * w.rain);
    }
    const float before = b.s;
    b.s += b.v * kDt;
    // Rolled back to turn it, it stops at the terrace's edge rather than going over.
    if (w.pushingBack) {
        const int t = hill::terraceAt(before);
        const float edge = t >= 0 ? hill::terraceStart(t) + 0.3f : hill::kRestS;
        if (b.s < edge) {
            b.s = before > edge ? edge : before;
            b.v = 0;
        }
    }
    const float top = hill::terraceStart(hill::kSummit) + 8.0f;
    if (b.s > top) {
        b.s = top;
        b.v = 0;
    }
    if (b.s < hill::kRestS && (w.falling || b.v < 0)) {
        b.s = hill::kRestS;
        b.v = 0;
        if (w.falling) {
            // Back at the trailhead: the next major version starts here, with
            // every module still aboard.
            w.falling = false;
            ++w.major;
            w.best = 0;
            raiseCairn(w);
            if (w.rewriteChosen) rewrite(w);
        }
    }
    b.roll = wrapAngle(b.roll + (b.s - before) / R);
}

void movePlayer(World& w, const Input& in) {
    Player& p = w.player;
    w.blocked = false;
    if (p.sitting) return;
    float f = in.forward, s = in.strafe;
    const float len2 = f * f + s * s;
    if (len2 > 1) {
        const float l = std::sqrt(len2);
        f /= l;
        s /= l;
    }
    const bool hurry = in.hurry && w.player.stamina > 0.15f;  // too tired to hurry
    const float speed = (hurry ? kHurrySpeed : kWalkSpeed) * kDt;
    const float sy = detSin(p.yaw), cy = detCos(p.yaw);
    float x = p.x + (sy * f + cy * s) * speed;
    float z = p.z + (-cy * f + sy * s) * speed;
    // Pushing, he leans straight into it: no drifting off round its side
    // however he faces, and no stepping sideways (squaring up does the rest).
    if (w.pushing) {
        x = p.x;
        z = p.z + (w.pushingBack ? 1.0f : -1.0f) * (f > 0 ? f : 0) * speed;
    }
    // The boulder is in the way.
    const float R = boulderRadius(w);
    const float bz = hill::zOf(w.boulder.s);
    float rx = x, rz = z - bz;
    float d = std::sqrt(rx * rx + rz * rz);
    const float clear = R + kBody;
    if (d < clear) {
        if (d < 1e-4f) {
            rx = 0;
            rz = 1;
            d = 1;
        }
        // Walking straight into it without pushing (not lined up), he stops
        // against it rather than sliding off round its side and on up the
        // path; walking past it at a slant still slides round.
        const float mx = x - p.x, mz = z - p.z, ml = std::sqrt(mx * mx + mz * mz);
        const float into = ml > 1e-5f ? -(mx * rx + mz * rz) / (ml * d) : 0.0f;
        w.blocked = !w.pushing && !w.falling && into > 0.7f;
        if (w.blocked) {
            rx = p.x;
            rz = p.z - bz;
            d = std::sqrt(rx * rx + rz * rz);
            if (d < 1e-4f) {
                rx = 0;
                rz = 1;
                d = 1;
            }
        }
        x = rx / d * clear;
        z = bz + rz / d * clear;
        // Rolling down onto you, it knocks you aside rather than carrying you.
        if ((w.falling || w.boulder.v < -1.0f) && rz > 0) x += (x >= 0 ? 1.0f : -1.0f) * 0.15f;
    }
    x = clampf(x, -hill::kWalkHalf, hill::kWalkHalf);
    z = clampf(z, hill::zOf(hill::kTopS - 1.0f), hill::zOf(hill::kValleyS + 2.0f));
    p.x = x;
    p.z = z;
}

void doWork(World& w, const Input& in_) {
    // At the summit R is the rewrite, not retiring a module.
    Input in = in_;
    if (w.summitSeconds >= 0) in.retire = false;
    const bool wants = (in.work || in.retire || in.refactor) && !w.player.sitting && !w.pushing;
    Work& k = w.work;
    if (!wants) {  // letting go starts the work over
        k = Work{};
        return;
    }
    const int i = aimedModule(w);
    if (i < 0) return;
    // R wins over F, and F over E.
    const bool retiring = in.retire, refactoring = !retiring && in.refactor;
    // A module as good as new needs nothing (it can still be retired or refactored).
    if (!retiring && !refactoring && w.modules[static_cast<size_t>(i)].health >= 0.98f) {
        k = Work{};
        return;
    }
    if (refactoring && !canRefactor(w, i)) {
        k = Work{};
        return;
    }
    if (k.module != i || k.retiring != retiring || k.refactoring != refactoring) {
        k.module = i;
        k.seconds = 0;
        k.retiring = retiring;
        k.refactoring = refactoring;
    }
    k.seconds += kDt * strength(w);
    Module& m = w.modules[static_cast<size_t>(i)];
    if (k.retiring) {
        if (activeModules(w) <= kMinModules || k.seconds < kRetireSeconds) return;
        // Cutting a feature: one less thing to keep, and some users miss it.
        m.active = false;
        m.issue = Issue::None;
        ++w.retired;
        w.users *= 0.93f;
        k = Work{};
        return;
    }
    if (k.refactoring) {
        if (k.seconds < kRefactorSeconds) return;
        // Folded into the module it uses: one less thing to keep, and the
        // code that is left is new. Sometimes it breaks a test.
        const int into = dependency(w, i);
        Module& kept = w.modules[static_cast<size_t>(into)];
        m.active = false;
        m.issue = Issue::None;
        kept.health = 1;
        kept.issue = Issue::None;
        kept.born = w.releases;
        ++w.refactored;
        w.refactoredInto = into;
        if (randomUnit(w.rng) < kRefactorBreaks) strike(w, into, Issue::Flaky);
        k = Work{};
        return;
    }
    if (k.seconds < patchSeconds(m)) return;
    m.health = 1;
    m.issue = Issue::None;
    ++w.patched;
    k = Work{};
}

}  // namespace

World newWorld(uint64_t seed) {
    World w;
    w.seed = seed;
    w.rng = seed * 0x2545F4914F6CDD1Dull + 0x1234567ull;
    for (int i = 0; i < kStartModules; ++i) w.modules[static_cast<size_t>(i)].active = true;
    w.added = kStartModules;
    w.boulder.s = hill::kRestS;
    w.player.z = hill::zOf(hill::kRestS) + 4.0f;
    w.users = 10;
    return w;
}

void tick(World& w, const Input& in) {
    ++w.tick;
    Player& p = w.player;
    if (in.sit) p.sitting = !p.sitting;
    if (p.sitting) w.satSeconds += kDt;
    p.yaw = wrapAngle(in.yaw);
    p.pitch = clampf(in.pitch, -1.45f, 1.45f);

    // Pushing: close to the boulder, walking into it, either from below
    // facing up the hill, or (on level ground, to turn it) from above facing
    // down it. Facing the boulder roughly is enough: he squares up to it as
    // he pushes.
    const float R = boulderRadius(w);
    const float rx = p.x, rz = p.z - hill::zOf(w.boulder.s);
    const float d = std::sqrt(rx * rx + rz * rz);
    const float lookX = detSin(p.yaw), lookZ = -detCos(p.yaw);
    const float atIt = d > 1e-4f ? -(lookX * rx + lookZ * rz) / d : 1;  // 1 looking straight at its centre
    const bool free = !p.sitting && !w.falling && w.summitSeconds < 0 && in.forward > 0.3f && d <= R + kContact &&
                      (rx < 0 ? -rx : rx) < 0.9f * R && atIt > 0.6f;
    const float facing = detCos(p.yaw);  // 1 up the hill, -1 down it
    w.pushing = free && rz > 0.2f * R && facing > 0.3f;
    w.pushingBack = free && !w.pushing && rz < -0.2f * R && facing < -0.3f && hill::level(w.boulder.s);
    w.pushing = w.pushing || w.pushingBack;
    if (w.pushing) {
        // Squaring up: drawn towards the line through its centre.
        const float pull = 0.6f * kDt;
        p.x += rx > pull ? -pull : rx < -pull ? pull : -rx;
    }

    moveBoulder(w, in);

    // Releases: the first time the boulder reaches each terrace this major version.
    const int t = hill::terraceAt(w.boulder.s);
    if (!w.falling && w.summitSeconds < 0 && t > w.best) {
        release(w, t);
        arrive(w);
        if (t == hill::kSummit) w.summitSeconds = 0;
    }
    if (w.summitSeconds >= 0) {
        w.summitSeconds += kDt;
        // Holding R at the top chooses a rewrite; the moment lasts while he decides.
        const bool deciding = in.retire && !p.sitting && !w.rewriteChosen;
        if (deciding) {
            w.rewriteSeconds += kDt;
            if (w.rewriteSeconds >= kRewriteSeconds) w.rewriteChosen = true;
        } else if (!w.rewriteChosen) {
            w.rewriteSeconds = 0;
        }
        if (w.summitSeconds >= kSummitPause && !(deciding && w.rewriteSeconds > 0)) {
            // The platform it runs on reaches its end of life.
            w.summitSeconds = -1;
            w.falling = true;
            w.boulder.v = -0.5f;
        }
    }

    movePlayer(w, in);
    doWork(w, in);
    tire(w, in);
    weather(w);
    help(w);
    // While it rolls back down after the summit there is nothing he can do:
    // time out from rot until it comes to rest.
    if (!w.falling) rot(w);
    if (outage(w) && !w.falling) w.users *= 1.0f - 0.01f * kDt;  // a module is down: users leave
}

// ---- queries

int activeModules(const World& w) {
    int n = 0;
    for (const Module& m : w.modules) n += m.active ? 1 : 0;
    return n;
}

float boulderRadius(const World& w) { return 1.1f + 0.035f * static_cast<float>(activeModules(w)); }

Vec3 boulderCentre(const World& w) {
    const float R = boulderRadius(w);
    return {0, hill::pathHeight(w.boulder.s) + R, hill::zOf(w.boulder.s)};
}

float health(const World& w) {
    float sum = 0;
    int n = 0;
    for (const Module& m : w.modules)
        if (m.active) {
            sum += m.health;
            ++n;
        }
    return n ? sum / static_cast<float>(n) : 1;
}

int brokenModules(const World& w) {
    int n = 0;
    for (const Module& m : w.modules) n += m.active && m.health <= 0 ? 1 : 0;
    return n;
}

int openIssues(const World& w) {
    int n = 0;
    for (const Module& m : w.modules) n += m.active && m.issue != Issue::None ? 1 : 0;
    return n;
}

float strength(const World& w) {
    const float s = w.player.stamina;
    return s >= kTired ? 1.0f : kSpentStrength + (1 - kSpentStrength) * s / kTired;
}

float load(const World& w) {
    return 0.25f + 0.01f * static_cast<float>(activeModules(w)) + 0.7f * (1 - health(w)) +
           0.15f * static_cast<float>(brokenModules(w));
}

Vec3 moduleDirection(int i) {
    // Spread over the sphere in the order modules are added, so a young
    // boulder's few modules are not all at one pole: a Fibonacci sphere,
    // visited 29 points at a time.
    const int k = (i * 29) % kMaxModules;
    const float y = 1.0f - 2.0f * (static_cast<float>(k) + 0.5f) / static_cast<float>(kMaxModules);
    const float r = std::sqrt(1.0f - y * y);
    const float phi = static_cast<float>(k) * 2.39996323f;
    return {detCos(phi) * r, y, detSin(phi) * r};
}

// Rolling uphill turns the top of the boulder towards -z.
Vec3 rolled(const World& w, Vec3 v) {
    const float c = detCos(w.boulder.roll), s = detSin(w.boulder.roll);
    return {v.x, v.y * c + v.z * s, -v.y * s + v.z * c};
}

Vec3 unrolled(const World& w, Vec3 v) {
    const float c = detCos(w.boulder.roll), s = detSin(w.boulder.roll);
    return {v.x, v.y * c - v.z * s, v.y * s + v.z * c};
}

int moduleAt(const World& w, Vec3 local) {
    int best = -1;
    float bestDot = -2;
    for (int i = 0; i < kMaxModules; ++i) {
        if (!w.modules[static_cast<size_t>(i)].active) continue;
        const float d = dot(local, moduleDirection(i));
        if (d > bestDot) {
            bestDot = d;
            best = i;
        }
    }
    return best;
}

int moduleOnSight(const World& w, Vec3 eye, Vec3 dir, Vec3* hit) {
    const Vec3 c = boulderCentre(w);
    const float R = boulderRadius(w);
    const Vec3 oc = eye - c;
    const float b = dot(oc, dir);
    const float disc = b * b - (dot(oc, oc) - R * R);
    if (disc < 0) return -1;
    const float t = -b - std::sqrt(disc);
    if (t < 0 || t > kReach) return -1;
    const Vec3 at = eye + dir * t;
    if (hit) *hit = at;
    return moduleAt(w, unrolled(w, (at - c) * (1.0f / R)));
}

Vec3 eyePosition(const World& w) {
    const Player& p = w.player;
    return {p.x, hill::groundHeight(p.x, p.z) + (p.sitting ? kSitEyeHeight : kEyeHeight), p.z};
}

int aimedModule(const World& w) {
    return moduleOnSight(w, eyePosition(w), lookDirection(w.player.yaw, w.player.pitch));
}

int dependency(const World& w, int i) {
    // Older means added earlier, and modules take slots in the order they
    // are added. Ties go to the lower slot, the same on every compiler.
    const Vec3 a = moduleDirection(i);
    int best = -1;
    float bestDot = -2;
    for (int j = 0; j < i; ++j) {
        if (!w.modules[static_cast<size_t>(j)].active) continue;
        const float d = dot(a, moduleDirection(j));
        if (d > bestDot) {
            bestDot = d;
            best = j;
        }
    }
    return best;
}

bool cascading(const World& w, int i) {
    const int d = dependency(w, i);
    return d >= 0 && w.modules[static_cast<size_t>(d)].health <= 0;
}

float rotRate(const World& w, int i) {
    // Everything ages, faster in later major versions; older modules rot
    // faster (legacy); an issue makes it fast, and so does using a module
    // that is down.
    const Module& m = w.modules[static_cast<size_t>(i)];
    const float age = 1.0f + 0.1f * static_cast<float>(w.major);
    const float years = static_cast<float>(w.releases - m.born);
    return kBaseRot * age * (1.0f + 0.04f * (years < 10 ? years : 10)) + kIssueRot[static_cast<int>(m.issue)] +
           (cascading(w, i) ? kCascadeRot : 0.0f);
}

Vec3 cairnBase(int k) {
    // Beside the path on the trailhead terrace, both sides turn about, in
    // rows along it; the boulder rests in the middle.
    const float side = k % 2 == 0 ? 1.0f : -1.0f;
    const int n = k / 2;
    const float s = 0.8f + 1.7f * static_cast<float>(n % 6);
    const float x = side * (hill::kPathHalf - 0.8f - 1.5f * static_cast<float>(n / 6));
    const float z = hill::zOf(s);
    return {x, hill::groundHeight(x, z), z};
}

int cairnHere(const World& w) {
    for (int k = 0; k < w.cairnCount; ++k) {
        const Vec3 b = cairnBase(k);
        const float dx = b.x - w.player.x, dz = b.z - w.player.z;
        if (dx * dx + dz * dz < 1.6f * 1.6f) return k;
    }
    return -1;
}

std::string cairnStory(const World& w, int k) {
    const Cairn& c = w.cairns[static_cast<size_t>(k)];
    const Cairn before = k > 0 ? w.cairns[static_cast<size_t>(k - 1)] : Cairn{};
    const int64_t minutes = (c.tick - before.tick) / (60 * kTickRate);
    return "The cairn for " + std::to_string(c.major) + ".0: " + std::to_string(minutes) + " min to the summit and back, " +
           std::to_string(c.modules) + " modules, " + std::to_string(static_cast<int>(c.users)) + " users.\n" +
           std::to_string(c.patched - before.patched) + " patched, " + std::to_string(c.retired - before.retired) +
           " retired, " + std::to_string(c.refactored - before.refactored) + " refactored, " +
           std::to_string(c.breakdowns - before.breakdowns) + " went down." +
           (c.helpers == 0 ? "" : c.helpers == 1 ? " One person came up to help." : " " + std::to_string(c.helpers) + " people came up to help.") +
           (c.rewrite ? " Then it was rewritten from scratch." : "");
}

const char* helperName(int i) { return i >= 0 && i < kHelperNames ? kHelperNameList[i] : "?"; }

Vec3 helperSpot(const World& w) {
    const Vec3 c = boulderCentre(w);
    const float side = w.helper.x >= 0 ? 1.0f : -1.0f;  // the side they came up on
    const float x = clampf(side * (boulderRadius(w) + 0.6f), -hill::kWalkHalf, hill::kWalkHalf);
    return {x, hill::groundHeight(x, c.z), c.z};
}

bool canRefactor(const World& w, int i) {
    return w.modules[static_cast<size_t>(i)].active && activeModules(w) > kMinModules && dependency(w, i) >= 0;
}

float patchSeconds(const Module& m) {
    return 1.2f + 2.6f * (1 - m.health) + (m.issue == Issue::Advisory ? 1.0f : 0.0f);
}

std::string versionText(const World& w) {
    if (w.summitSeconds >= 0 || w.falling) return std::to_string(w.major + 1) + ".0";
    return std::to_string(w.major) + "." + std::to_string(w.best);
}

std::string nextVersionText(const World& w) {
    if (w.best + 1 >= hill::kSummit) return std::to_string(w.major + 1) + ".0";
    return std::to_string(w.major) + "." + std::to_string(w.best + 1);
}

int terraceOfBoulder(const World& w) { return hill::terraceAt(w.boulder.s); }

bool boulderResting(const World& w) { return !w.falling && w.boulder.v == 0 && hill::level(w.boulder.s); }

Season season(const World& w) {
    const int64_t day = w.tick / static_cast<int64_t>(kDaySeconds * kTickRate);
    return static_cast<Season>((day / kSeasonDays) % 4);
}

const char* seasonName(Season s) {
    switch (s) {
        case Season::Spring: return "spring";
        case Season::Summer: return "summer";
        case Season::Autumn: return "autumn";
        default: return "winter";
    }
}

float timeOfDay(const World& w) {
    // The first morning starts a little after sunrise.
    const float t = static_cast<float>(w.tick % static_cast<int64_t>(kDaySeconds * kTickRate)) / (kDaySeconds * kTickRate);
    const float d = t + 0.08f;
    return d >= 1 ? d - 1 : d;
}

const char* moduleName(int i) { return i >= 0 && i < kMaxModules ? kNames[i] : "?"; }

const char* issueName(Issue issue) {
    switch (issue) {
        case Issue::Drift: return "dependency drift";
        case Issue::Flaky: return "flaky test";
        case Issue::Deprecated: return "deprecated API";
        case Issue::Advisory: return "security advisory";
        case Issue::Certificate: return "expiring certificate";
        default: return "";
    }
}

const char* issueLine(Issue issue) {
    switch (issue) {
        case Issue::Drift: return "a dependency fell three majors behind";
        case Issue::Flaky: return "a test fails one run in five";
        case Issue::Deprecated: return "a call it makes is going away";
        case Issue::Advisory: return "a vulnerability, and it spreads";
        case Issue::Certificate: return "a certificate is about to expire";
        default: return "";
    }
}

uint64_t fingerprint(const World& w) {
    uint64_t h = 1469598103934665603ull;
    auto mix = [&](const void* p, size_t n) {
        const auto* b = static_cast<const uint8_t*>(p);
        for (size_t i = 0; i < n; ++i) {
            h ^= b[i];
            h *= 1099511628211ull;
        }
    };
    auto f = [&](float v) { mix(&v, sizeof v); };
    auto i = [&](int64_t v) { mix(&v, sizeof v); };
    i(static_cast<int64_t>(w.rng));
    i(w.tick);
    f(w.player.x);
    f(w.player.z);
    f(w.boulder.s);
    f(w.boulder.v);
    f(w.boulder.roll);
    for (const Module& m : w.modules) {
        i(m.active);
        f(m.health);
        i(static_cast<int64_t>(m.issue));
    }
    i(w.added);
    i(w.major);
    i(w.best);
    i(w.releases);
    f(w.users);
    i(w.incidents);
    i(w.patched);
    i(w.retired);
    i(w.breakdowns);
    i(w.pushingBack);
    // Fields added after save format 1 count only when they differ from
    // their defaults, so a format-1 world's fingerprint is what it was.
    if (w.player.stamina != 1) f(w.player.stamina);
    if (w.refactored != 0) i(w.refactored);
    if (w.work.refactoring) i(1);
    if (w.cairnCount != 0) {
        i(w.cairnCount);
        for (int k = 0; k < w.cairnCount; ++k) {
            const Cairn& c = w.cairns[static_cast<size_t>(k)];
            i(c.major);
            i(c.tick);
            f(c.users);
            i(c.modules);
            i(c.patched);
            i(c.retired);
            i(c.refactored);
            i(c.breakdowns);
            if (c.helpers != 0) i(c.helpers);
            if (c.rewrite) i(1);
        }
    }
    if (w.helper.name >= 0) {
        i(w.helper.name);
        f(w.helper.x);
        f(w.helper.z);
        f(w.helper.secondsLeft);
        i(w.helper.module);
        f(w.helper.work);
        for (int32_t n : w.helper.patches) i(n);
    }
    if (w.helpersThisClimb != 0) i(w.helpersThisClimb);
    if (w.busFactor != -1) i(w.busFactor);
    if (w.rewriteChosen) i(2);
    if (w.rewriteSeconds != 0) f(w.rewriteSeconds);
    if (w.rain != 0) f(w.rain);
    if (w.rainLeft != 0) f(w.rainLeft);
    return h;
}

}  // namespace sisyphus
