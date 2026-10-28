// The hill: a valley with a town at its foot, and a path that climbs north
// (towards -z) in terraces. Each terrace is a release; the last is the
// summit. Between terraces the path is a steady slope, where a boulder left
// alone rolls back down. No engine here: the rules and the meshes both read
// it.
#pragma once

namespace sisyphus::hill {

// Terrace 0 is the trailhead, where every major version starts; terrace
// kSummit is the top.
constexpr int kTerraces = 8;
constexpr int kSummit = kTerraces - 1;
constexpr float kSpan = 36.0f;  // metres along the path from one terrace's start to the next
constexpr float kFlat = 10.0f;  // the flat part of each terrace
constexpr float kRise = 9.0f;   // metres climbed between terraces
constexpr float kPathHalf = 7.0f;  // the path's half width; beyond it the hillside rises
// The summit's far edge (beyond it, a drop), and the valley's south edge.
constexpr float kTopS = kSummit * kSpan + 16.0f;
constexpr float kValleyS = -48.0f;
// Where the player can walk.
constexpr float kWalkHalf = 22.0f;
// Where the boulder rests at the trailhead.
constexpr float kRestS = 3.0f;

// Distance uphill along the path for a world z (the path climbs towards -z).
inline float sOf(float z) { return -z; }
inline float zOf(float s) { return -s; }

// The path's height at `s` metres along it (0 at the trailhead).
float pathHeight(float s);
// The ground's height anywhere: the path, the hillside either side of it,
// the valley floor.
float groundHeight(float x, float z);
// The terrace whose flat part `s` is on, or -1 on a slope (or in the valley).
int terraceAt(float s);
// Where terrace i's flat part begins.
inline float terraceStart(int i) { return static_cast<float>(i) * kSpan; }
// Whether `s` is on level ground (a terrace, or the valley), where a boulder stays put.
bool level(float s);

}  // namespace sisyphus::hill
