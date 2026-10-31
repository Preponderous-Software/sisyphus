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
// The weather turns. Showers come and go: rain makes the path slick, so the
// boulder is slower to push and quicker to roll back. The seasons turn every
// three days; in winter the rain falls as snow, and certificates expire
// twice as often.
//
// At the summit there is a choice: let it roll back with every module
// aboard, or hold R to rewrite it, and start the next major version light,
// with a few fresh modules and most of the users gone.
//
// Each time the boulder rolls back to the trailhead, a cairn is raised there
// for the major version that just shipped, recording how its climb went.
// Over a long game they pile up beside the path.
//
// Now and then, at a release, someone comes up from the town to help for a
// while: they patch what they can reach while the boulder rests, then move
// on, and the module they knew best starts to drift (the bus factor).
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
constexpr float kRewriteSeconds = 3.0f;   // holding R at the summit chooses a rewrite
constexpr float kRewriteKeeps = 0.4f;     // the users who follow a rewrite

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

// One major version's climb, raised as a cairn at the trailhead.
struct Cairn {
    int32_t major = 0;  // the version shipped: 1 for 1.0
    int64_t tick = 0;   // when the boulder came back down
    float users = 0;
    int32_t modules = 0;
    // Totals when it was raised; the climb's own are the difference from the cairn before.
    int32_t patched = 0, retired = 0, refactored = 0, breakdowns = 0;
    int32_t helpers = 0;  // people who came up to help on this climb (save format 5)
    bool rewrite = false;  // it was rewritten at the summit (save format 6)
};
constexpr int kMaxCairns = 24;  // the oldest goes when there are more

// Someone helping for a while (save format 5).
constexpr float kHelperStays = 150.0f;     // seconds
constexpr float kHelperSlower = 1.5f;      // they patch this much slower than Sisyphus
constexpr float kHelperChance = 0.35f;     // at each release, when nobody is helping
constexpr int kHelperNames = 16;
struct Helper {
    int32_t name = -1;  // into the names (helperName); -1: nobody is helping
    float x = 0, z = 0;
    float secondsLeft = 0;
    int32_t module = -1;  // what they are working on
    float work = 0;
    std::array<int32_t, kMaxModules> patches{};  // what they patched, to know what they knew best
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
    bool blocked = false;      // this tick, walked into it without being lined up to push (shown, never saved)
    Work work;
    int32_t incidents = 0, patched = 0, retired = 0, breakdowns = 0;
    int32_t lastIncident = -1;  // the module the newest incident hit
    Issue lastIssue = Issue::None;
    float satSeconds = 0;  // time spent sitting, looking at the valley
    int32_t refactored = 0;  // modules folded into the ones they use (save format 3)
    int32_t refactoredInto = -1;  // the module the newest refactor kept, until the next
    std::array<Cairn, kMaxCairns> cairns{};  // oldest first (save format 4)
    int32_t cairnCount = 0;
    Helper helper;
    int32_t helpersThisClimb = 0;
    int32_t busFactor = -1;  // the module the last helper knew best, left drifting
    bool rewriteChosen = false;  // at the summit R was held: it starts again light (save format 6)
    float rewriteSeconds = 0;    // how long R has been held at the summit
    float rain = 0;              // 0 dry .. 1 pouring (save format 7)
    float rainLeft = 0;          // seconds of the shower left; 0 when dry
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
constexpr float kSpentStrength = 0.7f;
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
// Which way the boulder is from where he faces, in radians: 0 dead ahead,
// positive to his right, up to pi either way behind him.
float boulderBearing(const World& w);
// The module that module i uses: the nearest older active module on the
// stone, or -1 for the oldest.
int dependency(const World& w, int i);
// Module i uses a module that is down, and rots fast for it.
bool cascading(const World& w, int i);
// How fast module i is rotting now, in health per second.
float rotRate(const World& w, int i);
// Seconds of work to patch the module as it is now.
float patchSeconds(const Module& m);
// A helper's name.
const char* helperName(int i);
// Where the helper is going: beside the boulder, on the side of the path
// they came up (the side away from Sisyphus when they arrived).
Vec3 helperSpot(const World& w);
// Where cairn k stands, on the ground at the trailhead.
Vec3 cairnBase(int k);
// The cairn the player stands at, or -1.
int cairnHere(const World& w);
// Cairn k's climb in words.
std::string cairnStory(const World& w, int k);
// Whether module i can be refactored into the module it uses now.
bool canRefactor(const World& w, int i);
// "0.3", or "1.0" once the summit is reached.
std::string versionText(const World& w);
// The version the next terrace ships.
std::string nextVersionText(const World& w);
int terraceOfBoulder(const World& w);
bool boulderResting(const World& w);
// The season, from the day: each lasts kSeasonDays days.
enum class Season { Spring, Summer, Autumn, Winter };
constexpr int kSeasonDays = 3;
Season season(const World& w);
const char* seasonName(Season s);
// Rain or snow falling now, and whether it is snow.
inline bool snowing(const World& w) { return season(w) == Season::Winter; }
// The time of day, 0..1: 0 sunrise, 0.375 noon, 0.75 sunset (the night is
// short), and the length of a whole day.
constexpr float kDaySeconds = 720.0f;
float timeOfDay(const World& w);

const char* moduleName(int i);
const char* issueName(Issue issue);
// What the issue is, in a few words for the HUD.
const char* issueLine(Issue issue);

}  // namespace sisyphus
