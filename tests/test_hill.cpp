// The hill's shape: terraces level, slopes between them, the hillside rising
// either side of the path, the valley flat.
#include "check.h"
#include "sim/hill.h"

using namespace sisyphus;
namespace H = sisyphus::hill;

int main() {
    // Each terrace is level, at its own height, and higher than the last.
    for (int i = 0; i < H::kTerraces; ++i) {
        const float s = H::terraceStart(i);
        CHECK(H::terraceAt(s + 0.5f) == i);
        CHECK(H::terraceAt(s + H::kFlat - 0.5f) == i);
        CHECK(H::level(s + 1.0f));
        CHECK(H::pathHeight(s + 1.0f) == H::pathHeight(s + H::kFlat - 1.0f));
        CHECK(H::pathHeight(s + 1.0f) == static_cast<float>(i) * H::kRise);
    }
    // Between terraces, a steady slope uphill.
    for (int i = 0; i < H::kSummit; ++i) {
        const float s = H::terraceStart(i) + H::kFlat + 5.0f;
        CHECK(H::terraceAt(s) == -1);
        CHECK(!H::level(s));
        CHECK(H::pathHeight(s + 1.0f) > H::pathHeight(s));
    }
    // The summit runs on to its far edge.
    CHECK(H::terraceAt(H::kTopS - 1.0f) == H::kSummit);
    // The valley is level, and the hillside rises from the path's edges.
    CHECK(H::level(-10.0f));
    CHECK(H::groundHeight(0, 20.0f) == 0);
    const float z = H::zOf(60.0f);
    CHECK(H::groundHeight(H::kPathHalf + 5.0f, z) > H::groundHeight(0, z) + 1.0f);
    CHECK(H::groundHeight(-H::kPathHalf - 5.0f, z) == H::groundHeight(H::kPathHalf + 5.0f, z));
    CHECK(H::groundHeight(3.0f, z) == H::pathHeight(60.0f));
    TEST_MAIN_END();
}
