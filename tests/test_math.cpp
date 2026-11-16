// The rules' own math: sine, cosine and atan2 built from + - * / (so every
// compiler gets the same bits) stay close to the real functions, the look
// direction points where the comments say, and the box and sight tests treat
// their edges as the header describes.
#include <cmath>

#include "check.h"
#include "sim/math.h"

using namespace sisyphus;

namespace {

bool within(double a, double b, double tolerance) { return std::fabs(a - b) <= tolerance; }

}  // namespace

int main() {
    // detSin and detCos against the C library, in double, across [-pi, pi] and
    // then several turns either way. The header claims about 1e-7 in [-pi, pi].
    int sinMisses = 0, cosMisses = 0;
    for (int i = 0; i <= 4000; ++i) {
        const float x = static_cast<float>(-3.14159265358979 + 6.28318530717958 * i / 4000.0);
        if (!within(detSin(x), std::sin(static_cast<double>(x)), 1e-6)) ++sinMisses;
        if (!within(detCos(x), std::cos(static_cast<double>(x)), 1e-6)) ++cosMisses;
    }
    CHECK(sinMisses == 0);
    CHECK(cosMisses == 0);
    sinMisses = cosMisses = 0;
    for (int i = 0; i <= 4000; ++i) {
        const float x = static_cast<float>(-20.0 + 40.0 * i / 4000.0);
        if (!within(detSin(x), std::sin(static_cast<double>(x)), 1e-5)) ++sinMisses;
        if (!within(detCos(x), std::cos(static_cast<double>(x)), 1e-5)) ++cosMisses;
    }
    CHECK(sinMisses == 0);
    CHECK(cosMisses == 0);
    CHECK(detSin(0) == 0);
    CHECK(within(detSin(kPi / 2), 1, 1e-6));
    CHECK(within(detSin(-kPi / 2), -1, 1e-6));
    CHECK(within(detCos(0), 1, 1e-6));
    CHECK(within(detCos(kPi), -1, 1e-6));

    // detAtan2 against the C library all the way round, at small, unit and
    // large radii. The header claims about 1e-5 radians.
    int atanMisses = 0;
    const double radii[] = {0.001, 1.0, 1000.0};
    for (int i = 0; i < 3600; ++i) {
        const double a = 6.28318530717958 * i / 3600.0;
        for (double r : radii) {
            const float y = static_cast<float>(r * std::sin(a)), x = static_cast<float>(r * std::cos(a));
            if (!within(detAtan2(y, x), std::atan2(static_cast<double>(y), static_cast<double>(x)), 1e-5)) ++atanMisses;
        }
    }
    CHECK(atanMisses == 0);
    // The axes, and the origin, which has no direction and gives 0.
    CHECK(within(detAtan2(0, 1), 0, 1e-6));
    CHECK(within(detAtan2(1, 0), kPi / 2, 1e-6));
    CHECK(within(detAtan2(-1, 0), -kPi / 2, 1e-6));
    CHECK(within(detAtan2(0, -1), kPi, 1e-6));
    CHECK(detAtan2(0, 0) == 0);

    // The look direction: yaw 0 faces -z (north), positive yaw turns right
    // (towards +x), positive pitch looks up, and the result is a unit vector.
    const Vec3 north = lookDirection(0, 0);
    CHECK(within(north.x, 0, 1e-6) && within(north.y, 0, 1e-6) && within(north.z, -1, 1e-6));
    const Vec3 right = lookDirection(kPi / 2, 0);
    CHECK(within(right.x, 1, 1e-6) && within(right.y, 0, 1e-6) && within(right.z, 0, 1e-6));
    const Vec3 up = lookDirection(0, kPi / 2);
    CHECK(within(up.y, 1, 1e-6));
    CHECK(within(length(lookDirection(0.7f, -0.4f)), 1, 1e-6));

    // A line of sight passes a point within the radius ahead of the eye, and
    // reports how far along it does; not one behind the eye, nor one too far off
    // the line.
    const Vec3 eye{0, 0, 0}, dir{0, 0, -1};
    float t = 0;
    CHECK(sightPasses(eye, dir, {0.5f, 0, -4}, 1.0f, t));
    CHECK(within(t, 4, 1e-6));
    CHECK(!sightPasses(eye, dir, {0, 0, 4}, 1.0f, t));
    CHECK(!sightPasses(eye, dir, {1.5f, 0, -4}, 1.0f, t));

    // Boxes include their edges; clampf holds a value to its range.
    const Box2 area{-1, -2, 1, 2};
    CHECK(area.contains(0, 0));
    CHECK(area.contains(1, 2));
    CHECK(area.contains(-1, -2));
    CHECK(!area.contains(1.01f, 0));
    CHECK(!area.contains(0, -2.01f));
    const Box3 target{{0, 0, 0}, {1, 1, 1}};
    CHECK(target.contains({1, 1, 1}));
    CHECK(target.contains({0.5f, 0.5f, 0.5f}));
    CHECK(!target.contains({0.5f, 1.01f, 0.5f}));
    CHECK(clampf(5, 0, 1) == 1);
    CHECK(clampf(-5, 0, 1) == 0);
    CHECK(clampf(0.25f, 0, 1) == 0.25f);
    TEST_MAIN_END();
}
