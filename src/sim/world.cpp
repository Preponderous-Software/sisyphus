#include "sim/world.h"

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
    int total = 0;
    for (int k : kIssueWeight) total += k;
    int r = randomBelow(w.rng, total);
    for (int k = 1; k < kIssueKinds; ++k) {
        if (r < kIssueWeight[k]) return static_cast<Issue>(k);
        r -= kIssueWeight[k];
    }
    return Issue::Drift;
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

void rot(World& w) {
    for (Module& m : w.modules) {
        if (!m.active) continue;
        const float was = m.health;
        m.health = clamp01(m.health - rotRate(w, m) * kDt);
        if (was > 0 && m.health <= 0) ++w.breakdowns;
    }
    const int n = activeModules(w);
    const float p = static_cast<float>(n) * kIncidentRate * (1.0f + 0.1f * static_cast<float>(w.major)) * kDt;
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
        if (L < 1) target = kPushSpeed * (flat ? kFlatEase : 1.0f) * (1 - L) * f;
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
        b.v -= kRollAccel * kDt;
        if (b.v < -kMaxRoll) b.v = -kMaxRoll;
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
        }
    }
    b.roll = wrapAngle(b.roll + (b.s - before) / R);
}

void movePlayer(World& w, const Input& in) {
    Player& p = w.player;
    if (p.sitting) return;
    float f = in.forward, s = in.strafe;
    const float len2 = f * f + s * s;
    if (len2 > 1) {
        const float l = std::sqrt(len2);
        f /= l;
        s /= l;
    }
    const float speed = (in.hurry ? kHurrySpeed : kWalkSpeed) * kDt;
    const float sy = detSin(p.yaw), cy = detCos(p.yaw);
    float x = p.x + (sy * f + cy * s) * speed;
    float z = p.z + (-cy * f + sy * s) * speed;
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

void doWork(World& w, const Input& in) {
    const bool wants = (in.work || in.retire) && !w.player.sitting && !w.pushing;
    Work& k = w.work;
    if (!wants) {  // letting go starts the work over
        k = Work{};
        return;
    }
    const int i = aimedModule(w);
    if (i < 0) return;
    // A module as good as new needs nothing (it can still be retired).
    if (!in.retire && w.modules[static_cast<size_t>(i)].health >= 0.98f) {
        k = Work{};
        return;
    }
    if (k.module != i || k.retiring != in.retire) {
        k.module = i;
        k.seconds = 0;
        k.retiring = in.retire;
    }
    k.seconds += kDt;
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
        if (t == hill::kSummit) w.summitSeconds = 0;
    }
    if (w.summitSeconds >= 0) {
        w.summitSeconds += kDt;
        if (w.summitSeconds >= kSummitPause) {
            // The platform it runs on reaches its end of life.
            w.summitSeconds = -1;
            w.falling = true;
            w.boulder.v = -0.5f;
        }
    }

    movePlayer(w, in);
    doWork(w, in);
    rot(w);
    if (outage(w)) w.users *= 1.0f - 0.01f * kDt;  // a module is down: users leave
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

float rotRate(const World& w, const Module& m) {
    // Everything ages, faster in later major versions; older modules rot
    // faster (legacy); an issue makes it fast.
    const float age = 1.0f + 0.1f * static_cast<float>(w.major);
    const float years = static_cast<float>(w.releases - m.born);
    return kBaseRot * age * (1.0f + 0.04f * (years < 10 ? years : 10)) + kIssueRot[static_cast<int>(m.issue)];
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
    return h;
}

}  // namespace sisyphus
