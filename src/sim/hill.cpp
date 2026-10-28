#include "sim/hill.h"

namespace sisyphus::hill {

float pathHeight(float s) {
    if (s <= 0) return 0;
    const float top = static_cast<float>(kSummit) * kRise;
    if (s >= terraceStart(kSummit)) {
        if (s <= kTopS) return top;
        return top - (s - kTopS) * 1.6f;  // the far side drops away
    }
    const int i = static_cast<int>(s / kSpan);
    const float local = s - terraceStart(i);
    const float base = static_cast<float>(i) * kRise;
    if (local < kFlat) return base;
    return base + kRise * (local - kFlat) / (kSpan - kFlat);
}

float groundHeight(float x, float z) {
    const float s = sOf(z);
    const float path = pathHeight(s < kValleyS ? kValleyS : s);
    const float ax = x < 0 ? -x : x;
    if (ax <= kPathHalf) return path;
    // The hillside rises either side of the path, more steeply higher up, and
    // opens out into the valley below the trailhead.
    float open = (s + 8.0f) / 12.0f;
    open = open < 0 ? 0 : open > 1 ? 1 : open;
    // It levels off into a ridge some way above the path.
    const float d = ax - kPathHalf;
    const float rise = 0.22f * d + 0.025f * d * d;
    const float ridge = 10.0f + 0.03f * (s > 0 ? s : 0);
    return path + open * (rise < ridge ? rise : ridge + 0.1f * (rise - ridge) / (1.0f + 0.02f * (rise - ridge)));
}

int terraceAt(float s) {
    if (s < 0) return -1;
    if (s >= terraceStart(kSummit)) return kSummit;
    const int i = static_cast<int>(s / kSpan);
    return s - terraceStart(i) < kFlat ? i : -1;
}

bool level(float s) { return s < 0 || terraceAt(s) >= 0; }

}  // namespace sisyphus::hill
