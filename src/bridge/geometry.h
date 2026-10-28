// Triangle meshes for the hill, the town and the boulder, built from the
// layout in sim/hill.h and the world. No engine here: Scene (bridge/scene.h)
// uploads what this builds, and tests check it without a GPU.
#pragma once

#include <cstdint>
#include <vector>

#include "sim/math.h"
#include "sim/world.h"

namespace sisyphus::bridge {

// Linear RGB.
struct Rgb {
    float r = 1, g = 1, b = 1;
};

// An indexed triangle list with a normal and a colour per vertex. Faces do not
// share vertices, so shading is flat. Triangles wind counter-clockwise seen
// from the side their normal points to (Strata culls back faces).
struct MeshData {
    std::vector<float> positions, normals, colors;
    std::vector<uint32_t> indices;

    size_t vertexCount() const { return positions.size() / 3; }
    bool empty() const { return indices.empty(); }

    void quad(Vec3 a, Vec3 b, Vec3 c, Vec3 d, Rgb color);
    void triangle(Vec3 a, Vec3 b, Vec3 c, Rgb color);
    void box(Vec3 lo, Vec3 hi, Rgb color);
    // A box from corner `o` along edges ex, ey, ez (a right-handed set).
    void orientedBox(Vec3 o, Vec3 ex, Vec3 ey, Vec3 ez, Rgb color);
};

// The terrain grid for Strata's terrain tile, sampled from hill::groundHeight.
struct TerrainData {
    int samplesX = 0, samplesZ = 0;
    float origin[3] = {0, 0, 0};
    float cell = 1;
    std::vector<float> heights;  // samplesX * samplesZ, row by row along +Z
    std::vector<float> colors;   // linear RGB triples
};
TerrainData buildTerrain();

// The town in the valley, the stones along the path, and a marker at each
// terrace: a post with a pennant, raised and coloured once that terrace has
// shipped this major version (`best`), and a cairn at the summit.
MeshData buildTown();
MeshData buildMarkers(int best);
// The cairns at the trailhead, one for each major version: taller for a
// bigger codebase. `cairnsKey` says what they were built from.
MeshData buildCairns(const World& w);
uint64_t cairnsKey(const World& w);

// The boulder, a unit sphere around the origin: each face belongs to the
// nearest active module and is coloured by its health and its issue; the
// scene scales, rolls and places it. `key` says what it was built from, so
// the scene rebuilds it only when that changes.
MeshData buildBoulder(const World& w);
uint64_t boulderKey(const World& w);

// Sisyphus, seen from outside: standing at the origin facing -z, about 1.8 m
// tall; `pushing` leans him into the boulder with his arms out.
MeshData buildAvatar(bool pushing);

// The colour a module shows: its own hue when fresh, rust as it rots, nearly
// black once it is down; an issue tints it.
Rgb moduleColor(int i, const Module& m);
Rgb issueColor(Issue issue);

// Windows in the town that light up as users come (one per kUsersPerWindow).
constexpr int kUsersPerWindow = 8;
std::vector<Vec3> townWindows();

}  // namespace sisyphus::bridge
