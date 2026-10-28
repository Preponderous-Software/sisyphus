// The world and its rules: Sisyphus, the boulder, and the modules it is made
// of. The boulder is the software. Pushing it up to the next terrace ships a
// release, and every release adds a module, so the boulder grows. Every
// module rots: slowly on its own, and fast once something happens to it (a
// dependency drifts, a test turns flaky, an advisory lands). Rot makes the
// boulder heavy; a module rotted through is down, and its users leave. At the
// summit the next major version ships, and the platform under it reaches its
// end of life: the boulder rolls back to the trailhead, every module still
// aboard, and the next major version begins.
//
// Modules depend on one another: each uses the nearest older module on the
// stone. A module that is down makes everything that uses it rot fast, so
// one outage spreads unless its cause is patched first.
//
// A module can be refactored into the module it uses: holding F on it for a
// while folds the two into one, fresh, with one less thing to keep. It is
// the slow middle way between patching and retiring, costs no users, and
// can break a test on the way.
//
// Sisyphus tires. Pushing wears him out, the heavier the boulder the faster,
// and so does working on modules; tired, he pushes and works slowly.
// Standing still he gets his breath back a little; sitting down to look at
// the valley, he rests properly.
//
// Fixed-step and deterministic: the same seed and inputs give the same
// world on every compiler (no libm trigonometry; see math.h).
#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "sim/math.h"

namespace sisyphus {

constexpr int kTickRate = 30;
constexpr int kMaxModules = 48;
constexpr int kStartModules = 6;
constexpr int kMinModules = 3;  // fewer cannot be retired: something has to be left to run
constexpr float kEyeHeight = 1.6f, kSitEyeHeight = 1.0f;
constexpr float kReach = 3.0f;           // how far away a module can be patched, from the eye
constexpr float kSummitPause = 8.0f;     // seconds at the summit before it rolls
constexpr float kRetireSeconds = 3.0f;   // holding R on a module retires it
constexpr float kRefactorSeconds = 10.0f; // holding F on a module refactors it into the one it uses

// What has gone wrong with a module, if anything.
enum class Issue : uint8_t { None, Drift, Flaky, Deprecated, Advisory, Certificate };
constexpr int kIssueKinds = 6;

struct Module {
    bool active = false;
    float health = 1;  // 1 fresh .. 0 down
    Issue issue = Issue::None;
    int32_t born = 0;  // the release it shipped in (0: the first commit)
};

struct Player {
    float x = 0, z = 6;
    float yaw = 0, pitch = 0;  // yaw 0 looks up the hill (-z)
    bool sitting = false;
    float stamina = 1;  // 1 rested .. 0 spent (save format 2)
};

struct Boulder {
    float s = 3;     // metres along the path (see hill.h)
    float v = 0;     // metres per second uphill
    float roll = 0;  // radians it has turned about x, rolling uphill
};

// Work on one module: patching it, or (holding R) retiring it, or (holding
// F) refactoring it into the module it uses.
struct Work {
    int32_t module = -1;
    float seconds = 0;
    bool retiring = false;
    bool refactoring = false;  // save format 3
};

struct World {
    uint64_t seed = 1;
    uint64_t rng = 0;
    int64_t tick = 0;
    Player player;
    Boulder boulder;
    std::array<Module, kMaxModules> modules{};
    int32_t added = 0;     // modules ever added (the next new one takes slot `added`)
    int32_t major = 0;     // the major version being climbed towards (0 until 1.0 ships)
    int32_t best = 0;      // the highest terrace reached this major version
    int32_t releases = 0;  // every release ever shipped
    float users = 0;
    float summitSeconds = -1;  // seconds since the summit was reached, until it rolls; -1 not there
    bool falling = false;      // rolling back to the trailhead after a major release
    bool pushing = false;      // this tick, Sisyphus pushed
    bool pushingBack = false;  // ... from the uphill side, rolling it back (level ground only)
    bool slipping = false;     // pushed, but too heavy: it slides back
    Work work;
    int32_t incidents = 0, patched = 0, retired = 0, breakdowns = 0;
    int32_t lastIncident = -1;  // the module the newest incident hit
    Issue lastIssue = Issue::None;
    float satSeconds = 0;  // time spent sitting, looking at the valley
    int32_t refactored = 0;  // modules folded into the ones they use (save format 3)
    int32_t refactoredInto = -1;  // the module the newest refactor kept, until the next
};

// One tick's input. `work`, `retire` and `refactor` are held; `sit` toggles.
struct Input {
    float forward = 0, strafe = 0;  // -1..1
    bool hurry = false;
    float yaw = 0, pitch = 0;
    bool work = false;
    bool retire = false;
    bool refactor = false;
    bool sit = false;
};

World newWorld(uint64_t seed);
void tick(World& w, const Input& in);
uint64_t fingerprint(const World& w);

// ---- queries

int activeModules(const World& w);
float boulderRadius(const World& w);
Vec3 boulderCentre(const World& w);
// Mean health of the active modules, 0..1.
float health(const World& w);
int brokenModules(const World& w);  // health 0: down
int openIssues(const World& w);
inline bool outage(const World& w) { return brokenModules(w) > 0; }
// How heavy the boulder is to push: below 1 it moves, at 1 or more it will not.
float load(const World& w);
// How much of his strength Sisyphus has: 1 while he has some stamina left,
// down to kSpentStrength when it is gone. Pushing and working go at this rate.
constexpr float kTired = 0.4f;  // below this stamina he is tired
constexpr float kSpentStrength = 0.55f;
float strength(const World& w);
inline bool tired(const World& w) { return w.player.stamina < kTired; }
// The module's centre on the unrotated boulder (a unit vector), and where it
// is now the boulder has rolled.
Vec3 moduleDirection(int i);
Vec3 rolled(const World& w, Vec3 local);
Vec3 unrolled(const World& w, Vec3 world);
// The nearest active module to a unit direction on the unrotated boulder.
int moduleAt(const World& w, Vec3 local);
// The module under a line of sight from `eye` along unit `dir`, within reach;
// -1 if none. `hit` gets where the line meets the boulder.
int moduleOnSight(const World& w, Vec3 eye, Vec3 dir, Vec3* hit = nullptr);
Vec3 eyePosition(const World& w);
// The module the player is looking at, within reach, or -1.
int aimedModule(const World& w);
// The module that module i uses: the nearest older active module on the
// stone, or -1 for the oldest.
int dependency(const World& w, int i);
// Module i uses a module that is down, and rots fast for it.
bool cascading(const World& w, int i);
// How fast module i is rotting now, in health per second.
float rotRate(const World& w, int i);
// Seconds of work to patch the module as it is now.
float patchSeconds(const Module& m);
// Whether module i can be refactored into the module it uses now.
bool canRefactor(const World& w, int i);
// "0.3", or "1.0" once the summit is reached.
std::string versionText(const World& w);
// The version the next terrace ships.
std::string nextVersionText(const World& w);
int terraceOfBoulder(const World& w);
bool boulderResting(const World& w);
// The time of day, 0..1: 0 sunrise, 0.375 noon, 0.75 sunset (the night is
// short), and the length of a whole day.
constexpr float kDaySeconds = 720.0f;
float timeOfDay(const World& w);

const char* moduleName(int i);
const char* issueName(Issue issue);
// What the issue is, in a few words for the HUD.
const char* issueLine(Issue issue);

}  // namespace sisyphus
