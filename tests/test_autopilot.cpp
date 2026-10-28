// The autopilot plays from the trailhead to 1.0 at the summit and back down,
// and ends on the pinned fingerprint (the same on every compiler).
#include <cinttypes>
#include <cstdio>

#include "check.h"
#include "sim/autopilot.h"
#include "sim/hill.h"

using namespace sisyphus;

int main() {
    World w = newWorld(1);
    Autopilot a;
    bool patched = false, pushed = false;
    const int64_t limit = 60LL * 60 * kTickRate;
    for (int64_t t = 0; t < limit && !playthroughDone(w); ++t) {
        tick(w, a.next(w));
        pushed = pushed || w.pushing;
        patched = patched || w.patched > 0;
    }
    std::printf("ends at tick %" PRId64 " (%.1f min): fingerprint %016" PRIx64 "\n", w.tick,
                static_cast<double>(w.tick) / kTickRate / 60.0, fingerprint(w));
    CHECK(playthroughDone(w));
    CHECK(pushed);
    CHECK(patched);
    CHECK(w.major == 1);
    CHECK(w.releases == hill::kSummit);
    CHECK(fingerprint(w) == kPlaythroughFingerprint);
    TEST_MAIN_END();
}
