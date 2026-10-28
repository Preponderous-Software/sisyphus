#include "bridge/geometry.h"

#include <cmath>

#include "sim/hill.h"

namespace sisyphus::bridge {

namespace H = sisyphus::hill;

namespace {

Vec3 cross(Vec3 a, Vec3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }

Vec3 normalize(Vec3 v) {
    const float l = length(v);
    return l > 0 ? v * (1.0f / l) : Vec3{0, 1, 0};
}

// A small hash, for the variation in colours and where stones lie.
float hash01(uint32_t a, uint32_t b = 0) {
    uint32_t h = a * 0x9E3779B1u ^ (b + 0x7F4A7C15u) * 0x85EBCA77u;
    h ^= h >> 15;
    h *= 0x2C1B3C6Du;
    h ^= h >> 12;
    h *= 0x297A2D39u;
    h ^= h >> 15;
    return static_cast<float>(h & 0xFFFFFF) / 16777216.0f;
}

Rgb mix(Rgb a, Rgb b, float t) { return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t}; }
Rgb scale(Rgb c, float k) { return {c.r * k, c.g * k, c.b * k}; }
float smooth(float e0, float e1, float x) {
    const float t = clampf((x - e0) / (e1 - e0), 0, 1);
    return t * t * (3 - 2 * t);
}

// The hill's palette (linear RGB): a dry Mediterranean slope.
constexpr Rgb kPath{0.40f, 0.33f, 0.24f};
constexpr Rgb kTerrace{0.52f, 0.47f, 0.38f};
constexpr Rgb kGrass{0.20f, 0.22f, 0.08f};
constexpr Rgb kScrub{0.14f, 0.15f, 0.07f};
constexpr Rgb kRock{0.30f, 0.27f, 0.24f};
constexpr Rgb kMeadow{0.12f, 0.20f, 0.06f};
constexpr Rgb kWall{0.70f, 0.66f, 0.56f};
constexpr Rgb kWallShade{0.58f, 0.54f, 0.45f};
constexpr Rgb kRoof{0.40f, 0.14f, 0.07f};
constexpr Rgb kRoofDark{0.24f, 0.10f, 0.06f};
constexpr Rgb kWood{0.20f, 0.12f, 0.06f};
constexpr Rgb kCypress{0.04f, 0.10f, 0.04f};
constexpr Rgb kStone{0.36f, 0.33f, 0.29f};
constexpr Rgb kGold{0.95f, 0.66f, 0.12f};
constexpr Rgb kFurled{0.30f, 0.28f, 0.25f};
constexpr Rgb kWindowDark{0.06f, 0.06f, 0.07f};

// A house in the valley.
struct House {
    float x, z, w, d, h;
};

std::vector<House> houses() {
    std::vector<House> out;
    // Rows either side of the road that runs south from the trailhead.
    for (int i = 0; i < 18; ++i) {
        const float side = i % 2 ? 1.0f : -1.0f;
        const float row = static_cast<float>(i / 2);
        const float x = side * (7.0f + 9.0f * hash01(static_cast<uint32_t>(i), 1) + (i % 4 < 2 ? 0 : 9.0f));
        const float z = 16.0f + row * 3.6f + 2.0f * hash01(static_cast<uint32_t>(i), 2);
        out.push_back({x, z, 3.2f + 1.6f * hash01(static_cast<uint32_t>(i), 3), 3.0f + 1.2f * hash01(static_cast<uint32_t>(i), 4),
                       2.6f + 1.4f * hash01(static_cast<uint32_t>(i), 5)});
    }
    return out;
}

void house(MeshData& m, const House& h, int index) {
    const float x0 = h.x - h.w / 2, x1 = h.x + h.w / 2, z0 = h.z - h.d / 2, z1 = h.z + h.d / 2;
    const float base = H::groundHeight(h.x, h.z) - 0.3f;
    const float top = base + 0.3f + h.h;
    m.box({x0, base, z0}, {x1, top, z1}, index % 3 ? kWall : kWallShade);
    // A hipped-ish roof: a gable along x.
    const float o = 0.3f, ridge = top + 1.1f, zm = h.z;
    const Rgb roof = index % 4 == 3 ? kRoofDark : kRoof;
    m.quad({x1 + o, top - 0.1f, z0 - o}, {x0 - o, top - 0.1f, z0 - o}, {x0 - o, ridge, zm}, {x1 + o, ridge, zm}, roof);
    m.quad({x0 - o, top - 0.1f, z1 + o}, {x1 + o, top - 0.1f, z1 + o}, {x1 + o, ridge, zm}, {x0 - o, ridge, zm}, roof);
    m.triangle({x0, top, z0}, {x0, top, z1}, {x0, ridge - 0.05f, zm}, kWallShade);
    m.triangle({x1, top, z1}, {x1, top, z0}, {x1, ridge - 0.05f, zm}, kWallShade);
    // Dark window panes on the side facing the hill (they light as users come).
    const float y = base + 0.3f + h.h * 0.55f;
    for (int k = 0; k < 3; ++k) {
        const float wx = x0 + h.w * (0.2f + 0.3f * static_cast<float>(k));
        m.quad({wx + 0.3f, y - 0.35f, z0 - 0.02f}, {wx - 0.3f, y - 0.35f, z0 - 0.02f}, {wx - 0.3f, y + 0.35f, z0 - 0.02f},
               {wx + 0.3f, y + 0.35f, z0 - 0.02f}, kWindowDark);
    }
}

void cypress(MeshData& m, float x, float z, float tall) {
    const float g = H::groundHeight(x, z);
    m.box({x - 0.12f, g - 0.2f, z - 0.12f}, {x + 0.12f, g + 0.8f, z + 0.12f}, kWood);
    for (int k = 0; k < 4; ++k) {
        const float r = 0.75f - 0.15f * static_cast<float>(k), y0 = g + 0.6f + tall * 0.24f * static_cast<float>(k);
        m.box({x - r, y0, z - r}, {x + r, y0 + tall * 0.3f, z + r}, kCypress);
    }
}

// The icosphere the boulder is cut from: 1280 faces, each a unit triangle.
const std::vector<Vec3>& sphereFaces() {
    static const std::vector<Vec3> faces = [] {
        const float t = (1.0f + std::sqrt(5.0f)) / 2.0f;
        std::vector<Vec3> v = {{-1, t, 0}, {1, t, 0}, {-1, -t, 0}, {1, -t, 0}, {0, -1, t}, {0, 1, t},
                               {0, -1, -t}, {0, 1, -t}, {t, 0, -1}, {t, 0, 1}, {-t, 0, -1}, {-t, 0, 1}};
        const int idx[20][3] = {{0, 11, 5}, {0, 5, 1},  {0, 1, 7},   {0, 7, 10}, {0, 10, 11}, {1, 5, 9}, {5, 11, 4},
                                {11, 10, 2}, {10, 7, 6}, {7, 1, 8},  {3, 9, 4},  {3, 4, 2},   {3, 2, 6}, {3, 6, 8},
                                {3, 8, 9},  {4, 9, 5},  {2, 4, 11}, {6, 2, 10}, {8, 6, 7},   {9, 8, 1}};
        std::vector<Vec3> tri;
        for (const auto& f : idx)
            for (int k : f) tri.push_back(normalize(v[static_cast<size_t>(k)]));
        for (int level = 0; level < 3; ++level) {
            std::vector<Vec3> next;
            for (size_t i = 0; i < tri.size(); i += 3) {
                const Vec3 a = tri[i], b = tri[i + 1], c = tri[i + 2];
                const Vec3 ab = normalize(a + b), bc = normalize(b + c), ca = normalize(c + a);
                for (Vec3 p : {a, ab, ca, ab, b, bc, ca, bc, c, ab, bc, ca}) next.push_back(p);
            }
            tri.swap(next);
        }
        return tri;
    }();
    return faces;
}

constexpr Rgb kHues[8] = {{0.20f, 0.28f, 0.38f}, {0.16f, 0.32f, 0.30f}, {0.45f, 0.34f, 0.16f}, {0.32f, 0.22f, 0.32f},
                          {0.28f, 0.34f, 0.22f}, {0.46f, 0.40f, 0.30f}, {0.42f, 0.22f, 0.16f}, {0.30f, 0.32f, 0.36f}};
constexpr Rgb kRust{0.26f, 0.12f, 0.05f};
constexpr Rgb kDead{0.045f, 0.04f, 0.035f};
constexpr Rgb kSeam{0.08f, 0.07f, 0.06f};

// Rotates `p` about the x axis by `a` (positive tips +y towards -z), around `pivot`.
Vec3 tipped(Vec3 p, Vec3 pivot, float a) {
    const float c = std::cos(a), s = std::sin(a);
    const Vec3 r = p - pivot;
    return pivot + Vec3{r.x, r.y * c + r.z * s, -r.y * s + r.z * c};
}

}  // namespace

void MeshData::triangle(Vec3 a, Vec3 b, Vec3 c, Rgb color) {
    const Vec3 n = normalize(cross(b - a, c - a));
    const auto base = static_cast<uint32_t>(vertexCount());
    for (Vec3 p : {a, b, c}) {
        positions.insert(positions.end(), {p.x, p.y, p.z});
        normals.insert(normals.end(), {n.x, n.y, n.z});
        colors.insert(colors.end(), {color.r, color.g, color.b});
    }
    indices.insert(indices.end(), {base, base + 1, base + 2});
}

void MeshData::quad(Vec3 a, Vec3 b, Vec3 c, Vec3 d, Rgb color) {
    const Vec3 n = normalize(cross(b - a, c - a));
    const auto base = static_cast<uint32_t>(vertexCount());
    for (Vec3 p : {a, b, c, d}) {
        positions.insert(positions.end(), {p.x, p.y, p.z});
        normals.insert(normals.end(), {n.x, n.y, n.z});
        colors.insert(colors.end(), {color.r, color.g, color.b});
    }
    indices.insert(indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
}

void MeshData::box(Vec3 lo, Vec3 hi, Rgb color) {
    orientedBox(lo, {hi.x - lo.x, 0, 0}, {0, hi.y - lo.y, 0}, {0, 0, hi.z - lo.z}, color);
}

void MeshData::orientedBox(Vec3 o, Vec3 ex, Vec3 ey, Vec3 ez, Rgb color) {
    auto p = [&](int a, int b, int c) {
        return o + ex * static_cast<float>(a) + ey * static_cast<float>(b) + ez * static_cast<float>(c);
    };
    quad(p(0, 0, 1), p(1, 0, 1), p(1, 1, 1), p(0, 1, 1), color);
    quad(p(1, 0, 0), p(0, 0, 0), p(0, 1, 0), p(1, 1, 0), color);
    quad(p(1, 0, 1), p(1, 0, 0), p(1, 1, 0), p(1, 1, 1), color);
    quad(p(0, 0, 0), p(0, 0, 1), p(0, 1, 1), p(0, 1, 0), color);
    quad(p(0, 1, 1), p(1, 1, 1), p(1, 1, 0), p(0, 1, 0), color);
    quad(p(0, 0, 0), p(1, 0, 0), p(1, 0, 1), p(0, 0, 1), color);
}

TerrainData buildTerrain() {
    TerrainData t;
    t.cell = 1.0f;
    const float x0 = -56.0f, z0 = H::zOf(H::kTopS + 34.0f), z1 = H::zOf(H::kValleyS - 12.0f);
    t.samplesX = 113;
    t.samplesZ = static_cast<int>(z1 - z0) + 1;
    t.origin[0] = x0;
    t.origin[1] = 0;
    t.origin[2] = z0;
    t.heights.resize(static_cast<size_t>(t.samplesX * t.samplesZ));
    t.colors.resize(t.heights.size() * 3);
    for (int iz = 0; iz < t.samplesZ; ++iz)
        for (int ix = 0; ix < t.samplesX; ++ix) {
            const float x = x0 + static_cast<float>(ix), z = z0 + static_cast<float>(iz);
            const float s = H::sOf(z);
            const float h = H::groundHeight(x, z);
            const float ax = std::fabs(x);
            const float n = hash01(static_cast<uint32_t>(ix), static_cast<uint32_t>(iz));
            Rgb c;
            if (s < -4) {
                c = mix(kMeadow, kGrass, 0.3f * n);
                if (ax < 2.0f) c = kPath;  // the road into town
            } else {
                const float above = h - H::pathHeight(s);
                c = mix(kGrass, kScrub, n);
                c = mix(c, kRock, smooth(1.5f, 7.0f, above) * (0.6f + 0.4f * n));
                const float edge = smooth(H::kPathHalf - 1.5f, H::kPathHalf + 0.5f, ax);
                const Rgb path = H::terraceAt(s) >= 0 ? kTerrace : kPath;
                c = mix(path, c, edge);
            }
            c = scale(c, 0.9f + 0.2f * n);
            const size_t k = static_cast<size_t>(iz * t.samplesX + ix);
            t.heights[k] = h;
            t.colors[3 * k] = c.r;
            t.colors[3 * k + 1] = c.g;
            t.colors[3 * k + 2] = c.b;
        }
    return t;
}

MeshData buildTown() {
    MeshData m;
    const std::vector<House> hs = houses();
    for (size_t i = 0; i < hs.size(); ++i) house(m, hs[i], static_cast<int>(i));
    for (int i = 0; i < 14; ++i) {
        const float side = i % 2 ? 1.0f : -1.0f;
        cypress(m, side * (4.0f + 26.0f * hash01(static_cast<uint32_t>(i), 9)), 6.0f + 30.0f * hash01(static_cast<uint32_t>(i), 10),
                3.0f + 2.0f * hash01(static_cast<uint32_t>(i), 11));
    }
    // Stones scattered over the hillside, and kerbs at each terrace's edge.
    for (int i = 0; i < 260; ++i) {
        const float s = -4.0f + (H::kTopS + 4.0f) * hash01(static_cast<uint32_t>(i), 20);
        const float side = hash01(static_cast<uint32_t>(i), 21) < 0.5f ? -1.0f : 1.0f;
        const float x = side * (H::kPathHalf + 1.0f + 22.0f * hash01(static_cast<uint32_t>(i), 22));
        const float z = H::zOf(s), g = H::groundHeight(x, z);
        const float r = 0.3f + 0.9f * hash01(static_cast<uint32_t>(i), 23) * hash01(static_cast<uint32_t>(i), 24);
        m.box({x - r, g - 0.3f, z - r * 0.8f}, {x + r, g + r * 0.9f, z + r * 0.8f}, scale(kStone, 0.8f + 0.4f * hash01(static_cast<uint32_t>(i), 25)));
    }
    for (int i = 1; i < H::kTerraces; ++i) {
        const float s0 = H::terraceStart(i) + H::kFlat;  // the lip, where the next slope begins
        const float g = H::pathHeight(s0 - 0.5f);
        for (float side : {-1.0f, 1.0f}) {
            const float x = side * (H::kPathHalf - 0.6f);
            m.box({x - 0.5f, g - 0.2f, H::zOf(s0 - 0.2f) - 0.4f}, {x + 0.5f, g + 0.35f, H::zOf(s0 - 0.2f) + 0.4f}, kStone);
        }
    }
    return m;
}

MeshData buildCairns(const World& w) {
    MeshData m;
    for (int k = 0; k < w.cairnCount; ++k) {
        const Cairn& c = w.cairns[static_cast<size_t>(k)];
        const Vec3 b = cairnBase(k);
        const int stones = 3 + c.modules / 8;
        float y = b.y - 0.1f;
        for (int i = 0; i < stones; ++i) {
            const float r = 0.5f - 0.32f * static_cast<float>(i) / static_cast<float>(stones);
            const float h = 0.22f + 0.08f * hash01(static_cast<uint32_t>(c.major * 16 + i), 41);
            const float ox = 0.06f * (hash01(static_cast<uint32_t>(c.major * 16 + i), 42) - 0.5f);
            const float oz = 0.06f * (hash01(static_cast<uint32_t>(c.major * 16 + i), 43) - 0.5f);
            m.box({b.x + ox - r, y, b.z + oz - r}, {b.x + ox + r, y + h, b.z + oz + r},
                  scale(kStone, 0.8f + 0.4f * hash01(static_cast<uint32_t>(c.major * 16 + i), 44)));
            y += h;
        }
    }
    return m;
}

uint64_t cairnsKey(const World& w) {
    return w.cairnCount == 0 ? 0 : static_cast<uint64_t>(w.cairnCount) << 32 | static_cast<uint32_t>(w.cairns[static_cast<size_t>(w.cairnCount - 1)].major);
}

MeshData buildMarkers(int best) {
    MeshData m;
    for (int i = 1; i < H::kSummit; ++i) {
        const float s = H::terraceStart(i) + H::kFlat / 2, x = -H::kPathHalf + 0.9f, z = H::zOf(s);
        const float g = H::pathHeight(s);
        m.box({x - 0.08f, g - 0.2f, z - 0.08f}, {x + 0.08f, g + 2.6f, z + 0.08f}, kWood);
        if (i <= best) m.orientedBox({x + 0.08f, g + 1.8f, z - 0.02f}, {1.1f, -0.2f, 0}, {0, 0.7f, 0}, {0, 0, 0.04f}, kGold);
        else m.box({x - 0.12f, g + 1.6f, z - 0.12f}, {x + 0.12f, g + 2.5f, z + 0.12f}, kFurled);
    }
    // The summit's cairn, which grows a pennant at each major release.
    const float s = H::terraceStart(H::kSummit) + 12.0f, x = 3.5f, z = H::zOf(s), g = H::pathHeight(s);
    for (int k = 0; k < 5; ++k) {
        const float r = 1.0f - 0.17f * static_cast<float>(k);
        m.box({x - r, g - 0.2f + 0.45f * static_cast<float>(k), z - r}, {x + r, g + 0.25f + 0.45f * static_cast<float>(k), z + r},
              scale(kStone, 0.9f + 0.05f * static_cast<float>(k)));
    }
    m.box({x - 0.06f, g + 2.0f, z - 0.06f}, {x + 0.06f, g + 4.4f, z + 0.06f}, kWood);
    if (best >= H::kSummit) m.orientedBox({x + 0.06f, g + 3.5f, z - 0.02f}, {1.4f, -0.25f, 0}, {0, 0.85f, 0}, {0, 0, 0.04f}, kGold);
    return m;
}

Rgb issueColor(Issue issue) {
    switch (issue) {
        case Issue::Drift: return {0.95f, 0.55f, 0.10f};
        case Issue::Flaky: return {0.60f, 0.32f, 0.95f};
        case Issue::Deprecated: return {0.55f, 0.55f, 0.50f};
        case Issue::Advisory: return {1.00f, 0.12f, 0.08f};
        case Issue::Certificate: return {0.95f, 0.85f, 0.20f};
        default: return {1, 1, 1};
    }
}

Rgb moduleColor(int i, const Module& m) {
    const Rgb fresh = kHues[i % 8];
    if (m.health <= 0) return kDead;
    Rgb c = mix(kRust, fresh, smooth(0.0f, 0.85f, m.health));
    if (m.issue != Issue::None) c = mix(c, issueColor(m.issue), 0.3f);
    return c;
}

uint64_t boulderKey(const World& w) {
    uint64_t h = 1469598103934665603ull;
    for (int i = 0; i < kMaxModules; ++i) {
        const Module& m = w.modules[static_cast<size_t>(i)];
        const uint64_t v = m.active ? 1 + static_cast<uint64_t>(m.health * 16.0f) * 8 + static_cast<uint64_t>(m.issue) * 256 : 0;
        h = (h ^ v) * 1099511628211ull;
    }
    return h;
}

MeshData buildBoulder(const World& w) {
    MeshData m;
    const std::vector<Vec3>& f = sphereFaces();
    for (size_t i = 0; i < f.size(); i += 3) {
        const Vec3 centre = normalize(f[i] + f[i + 1] + f[i + 2]);
        // The nearest module, and how near the next one is: faces on the
        // border between two modules are the seams.
        int best = -1;
        float d1 = -2, d2 = -2;
        for (int k = 0; k < kMaxModules; ++k) {
            if (!w.modules[static_cast<size_t>(k)].active) continue;
            const float d = dot(centre, moduleDirection(k));
            if (d > d1) {
                d2 = d1;
                d1 = d;
                best = k;
            } else if (d > d2) d2 = d;
        }
        if (best < 0) continue;
        const float n = hash01(static_cast<uint32_t>(i), 77);
        Rgb c = d1 - d2 < 0.025f ? kSeam : scale(moduleColor(best, w.modules[static_cast<size_t>(best)]), 0.88f + 0.24f * n);
        m.triangle(f[i], f[i + 1], f[i + 2], c);
    }
    return m;
}

MeshData buildAvatar(bool pushing) {
    MeshData m;
    constexpr Rgb kSkin{0.42f, 0.25f, 0.15f}, kTunic{0.62f, 0.57f, 0.47f}, kBelt{0.18f, 0.10f, 0.05f}, kHair{0.06f, 0.05f, 0.04f};
    // Legs, a little apart; the back leg braced when pushing.
    const float stride = pushing ? 0.35f : 0.0f;
    m.box({-0.22f, 0, -0.08f - stride}, {-0.05f, 0.85f, 0.10f - stride}, kSkin);
    m.box({0.05f, 0, -0.08f + stride}, {0.22f, 0.85f, 0.10f + stride}, kSkin);
    // The body, from the hips up, leaning into the boulder.
    const float lean = pushing ? 0.55f : 0.05f;
    const Vec3 hip{0, 0.85f, 0};
    auto part = [&](Vec3 lo, Vec3 hi, Rgb c) {
        const Vec3 o = tipped(lo, hip, lean);
        const Vec3 ex = tipped({hi.x, lo.y, lo.z}, hip, lean) - o;
        const Vec3 ey = tipped({lo.x, hi.y, lo.z}, hip, lean) - o;
        const Vec3 ez = tipped({lo.x, lo.y, hi.z}, hip, lean) - o;
        m.orientedBox(o, ex, ey, ez, c);
    };
    part({-0.26f, 0.80f, -0.14f}, {0.26f, 1.42f, 0.14f}, kTunic);
    part({-0.27f, 0.84f, -0.15f}, {0.27f, 0.92f, 0.15f}, kBelt);
    part({-0.11f, 1.45f, -0.12f}, {0.11f, 1.72f, 0.11f}, kSkin);
    part({-0.12f, 1.66f, -0.11f}, {0.12f, 1.78f, 0.13f}, kHair);
    if (pushing) {
        // Arms straight out ahead, hands on the stone.
        part({-0.30f, 1.25f, -0.62f}, {-0.18f, 1.37f, -0.05f}, kSkin);
        part({0.18f, 1.25f, -0.62f}, {0.30f, 1.37f, -0.05f}, kSkin);
    } else {
        part({-0.36f, 0.85f, -0.06f}, {-0.26f, 1.40f, 0.06f}, kSkin);
        part({0.26f, 0.85f, -0.06f}, {0.36f, 1.40f, 0.06f}, kSkin);
    }
    return m;
}

std::vector<Vec3> townWindows() {
    std::vector<Vec3> out;
    const std::vector<House> hs = houses();
    for (const House& h : hs) {
        const float x0 = h.x - h.w / 2, z0 = h.z - h.d / 2;
        const float y = H::groundHeight(h.x, h.z) + h.h * 0.55f;
        for (int k = 0; k < 3; ++k) out.push_back({x0 + h.w * (0.2f + 0.3f * static_cast<float>(k)), y, z0 - 0.06f});
    }
    return out;
}

}  // namespace sisyphus::bridge
