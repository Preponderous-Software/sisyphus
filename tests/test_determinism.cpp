// The same seed and inputs give the same world; a different seed does not.
#include "check.h"
#include "sim/autopilot.h"

using namespace sisyphus;

namespace {
uint64_t play(uint64_t seed, int seconds) {
    World w = newWorld(seed);
    Autopilot a;
    for (int t = 0; t < seconds * kTickRate; ++t) tick(w, a.next(w));
    return fingerprint(w);
}
}  // namespace

int main() {
    CHECK(play(1, 120) == play(1, 120));
    CHECK(play(1, 120) != play(2, 120));
    CHECK(fingerprint(newWorld(3)) == fingerprint(newWorld(3)));
    TEST_MAIN_END();
}
