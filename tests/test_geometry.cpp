// The meshes: well-formed, finite, and the boulder painted by its modules.
#include <cmath>

#include "bridge/geometry.h"
#include "check.h"
#include "sim/hill.h"

using namespace sisyphus;
using namespace sisyphus::bridge;

namespace {
bool wellFormed(const MeshData& m) {
    if (m.positions.size() != m.normals.size() || m.positions.size() != m.colors.size()) return false;
    if (m.positions.size() % 3 || m.indices.size() % 3) return false;
    for (uint32_t i : m.indices)
        if (i >= m.vertexCount()) return false;
    for (float f : m.positions)
        if (!std::isfinite(f)) return false;
    for (float c : m.colors)
        if (!(c >= 0 && c <= 1)) return false;
    return true;
}
}  // namespace

int main() {
    const TerrainData t = buildTerrain();
    CHECK(t.samplesX >= 2 && t.samplesX <= 1025 && t.samplesZ >= 2 && t.samplesZ <= 1025);
    CHECK(t.heights.size() == static_cast<size_t>(t.samplesX * t.samplesZ));
    CHECK(t.colors.size() == t.heights.size() * 3);
    // It covers the whole walkable hill, valley to summit.
    CHECK(t.origin[2] <= hill::zOf(hill::kTopS));
    CHECK(t.origin[2] + static_cast<float>(t.samplesZ - 1) * t.cell >= hill::zOf(hill::kValleyS));
    CHECK(t.origin[0] <= -hill::kWalkHalf && t.origin[0] + static_cast<float>(t.samplesX - 1) * t.cell >= hill::kWalkHalf);

    CHECK(wellFormed(buildTown()));
    CHECK(!buildTown().empty());
    CHECK(wellFormed(buildMarkers(0)));
    CHECK(buildMarkers(3).indices.size() > buildMarkers(0).indices.size() - 1);
    CHECK(wellFormed(buildAvatar(false)) && wellFormed(buildAvatar(true)));

    World w = newWorld(1);
    const MeshData b = buildBoulder(w);
    CHECK(wellFormed(b));
    CHECK(b.indices.size() == 1280 * 3);
    // Every vertex on the unit sphere.
    for (size_t i = 0; i < b.positions.size(); i += 3) {
        const float r = std::sqrt(b.positions[i] * b.positions[i] + b.positions[i + 1] * b.positions[i + 1] +
                                  b.positions[i + 2] * b.positions[i + 2]);
        CHECK(std::fabs(r - 1) < 1e-4f);
    }
    // Rot changes the boulder's colours, and its key.
    const uint64_t fresh = boulderKey(w);
    w.modules[0].health = 0;
    CHECK(boulderKey(w) != fresh);
    const MeshData rotten = buildBoulder(w);
    CHECK(rotten.colors != b.colors);
    // A module's colour darkens as it rots.
    Module m;
    m.active = true;
    const Rgb a = moduleColor(0, m);
    m.health = 0.2f;
    const Rgb c = moduleColor(0, m);
    CHECK(c.r + c.g + c.b != a.r + a.g + a.b);
    CHECK(townWindows().size() >= 30);
    TEST_MAIN_END();
}
