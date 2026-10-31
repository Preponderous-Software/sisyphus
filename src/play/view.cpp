#include "play/view.h"

#include <algorithm>
#include <cmath>

#include "bridge/geometry.h"
#include "sim/hill.h"

namespace sisyphus::play {

namespace {

constexpr float kFovY = 65.0f;  // degrees
constexpr float kMargin = 16.0f;
float gTextScale = 1.0f;
float gTextSize = 16.0f, gLine = 20.0f;

StrataCamera lookFrom(Vec3 eye, float yaw, float pitch, float aspect) {
    const Vec3 d = lookDirection(yaw, pitch);
    const float e[3] = {eye.x, eye.y, eye.z};
    const float target[3] = {eye.x + d.x, eye.y + d.y, eye.z + d.z};
    const float up[3] = {0, 1, 0};
    return strata::cameraLookAt(e, target, up, kFovY, aspect, 0.05f, 600.0f);
}

// Where the sun is: up for the first three quarters of the day, from the
// east over the south (behind you as you climb) to the west; below the
// horizon for the last quarter. Returns how far up it is, -1..1.
void sunDirection(const World& w, float to[3]) {
    const float u = timeOfDay(w);
    const float a = u < 0.75f ? u / 0.75f * kPi : kPi + (u - 0.75f) / 0.25f * kPi;
    to[0] = std::cos(a);
    to[1] = 0.9f * std::sin(a);
    to[2] = 0.45f;
    const float l = std::sqrt(to[0] * to[0] + to[1] * to[1] + to[2] * to[2]);
    for (int i = 0; i < 3; ++i) to[i] /= l;
}

float setSky(strata::Engine& engine, const World& w) {
    float to[3];
    sunDirection(w, to);
    engine.setSunDirection(to);
    return to[1];
}

float nightness(float sunUp) { return clampf((0.08f - sunUp) / 0.2f, 0.0f, 1.0f); }

void setLights(strata::Engine& engine, const World& w, float night) {
    std::vector<StrataPointLight> lights;
    auto add = [&](Vec3 at, float range, bridge::Rgb c, float intensity) {
        StrataPointLight l{};
        l.position[0] = at.x;
        l.position[1] = at.y;
        l.position[2] = at.z;
        l.range = range;
        l.color[0] = c.r;
        l.color[1] = c.g;
        l.color[2] = c.b;
        l.intensity = intensity;
        lights.push_back(l);
    };
    // Modules in trouble glow, enough to work by at night.
    const Vec3 c = boulderCentre(w);
    const float R = boulderRadius(w);
    for (int i = 0; i < kMaxModules && lights.size() < 12; ++i) {
        const Module& m = w.modules[static_cast<size_t>(i)];
        if (!m.active || m.issue == Issue::None) continue;
        const Vec3 at = c + rolled(w, moduleDirection(i)) * (R + 0.4f);
        add(at, 4.0f + R, bridge::issueColor(m.issue), 0.6f + 1.2f * night);
    }
    if (night > 0) {
        // A faint fill from the night sky, so the hill keeps its shape, and
        // the town's glow.
        const Player& p = w.player;
        add({p.x, hill::groundHeight(p.x, p.z) + 30.0f, p.z + 10.0f}, 90.0f, {0.55f, 0.65f, 1.0f}, 0.9f * night);
        add({0, 6.0f, 26.0f}, 40.0f, {1.0f, 0.7f, 0.4f}, 1.5f * night * clampf(w.users / 200.0f, 0.2f, 1.0f));
    }
    engine.setPointLights(lights.empty() ? nullptr : lights.data(), lights.size());
}

}  // namespace

std::vector<StrataPoint> glowPoints(const World& w, float night, Vec3 eye) {
    std::vector<StrataPoint> pts;
    // Rain, or snow, falling round the eye: streaks that fall fast, or flakes
    // that drift down. Where each one is comes from the clock, not the rules.
    if (w.rain > 0.02f) {
        const bool snow = snowing(w);
        const float clock = static_cast<float>(w.tick) / kTickRate;
        const int n = static_cast<int>(360 * w.rain);
        for (int i = 0; i < n; ++i) {
            const auto hash = [i](uint32_t salt) {
                uint32_t h = static_cast<uint32_t>(i) * 2654435761u ^ salt * 2246822519u;
                h ^= h >> 15;
                h *= 2246822519u;
                h ^= h >> 13;
                return static_cast<float>(h & 0xffffff) / 16777216.0f;
            };
            const float fall = snow ? 1.2f : 9.0f, height = 9.0f;
            const float y = height - std::fmod(clock * fall * (0.8f + 0.4f * hash(3)) + hash(4) * height, height);
            const float drift = snow ? 0.4f * std::sin(clock * 0.9f + hash(5) * 6.28f) : 0.0f;
            // A flake is one point; a raindrop is a short streak of them.
            const int streak = snow ? 1 : 5;
            for (int k = 0; k < streak; ++k) {
                StrataPoint p{};
                p.position[0] = eye.x + (hash(1) - 0.5f) * 16.0f + drift;
                p.position[1] = eye.y - 3.0f + y + 0.06f * static_cast<float>(k);
                p.position[2] = eye.z + (hash(2) - 0.5f) * 16.0f;
                p.world_diameter = snow ? 0.06f : 0.03f;
                p.size_pixels = snow ? 2.5f : 2.0f;
                p.color[0] = snow ? 0.95f : 0.72f;
                p.color[1] = snow ? 0.96f : 0.78f;
                p.color[2] = snow ? 1.00f : 0.90f;
                p.color[3] = (snow ? 0.85f : 0.55f) * w.rain;
                p.flags = STRATA_POINT_SOFT;
                pts.push_back(p);
            }
        }
    }
    const Vec3 c = boulderCentre(w);
    const float R = boulderRadius(w);
    const float t = static_cast<float>(w.tick) / kTickRate;
    for (int i = 0; i < kMaxModules; ++i) {
        const Module& m = w.modules[static_cast<size_t>(i)];
        if (!m.active || (m.issue == Issue::None && m.health > 0)) continue;
        const Vec3 d = rolled(w, moduleDirection(i));
        const Vec3 at = c + d * (R + 0.05f);
        // Faster pulses the worse it gets; a module that is down glows dull red, steady.
        const bridge::Rgb col = m.health <= 0 ? bridge::Rgb{0.6f, 0.05f, 0.02f} : bridge::issueColor(m.issue);
        const float beat = m.health <= 0 ? 0.6f : 0.5f + 0.5f * std::sin(t * (2.0f + 6.0f * (1.0f - m.health)));
        StrataPoint p{};
        p.position[0] = at.x;
        p.position[1] = at.y;
        p.position[2] = at.z;
        p.world_diameter = 0.6f * R;
        p.color[0] = col.r;
        p.color[1] = col.g;
        p.color[2] = col.b;
        // Brighter at night, but never past 1: Strata refuses the frame.
        p.color[3] = std::min(1.0f, (0.55f + 0.3f * night) * beat + 0.25f);
        p.flags = STRATA_POINT_SOFT;
        pts.push_back(p);
    }
    // Who uses whom: from a module that is down, red links to everything
    // that uses it; from the module you look at, faint links to its own.
    const int aimed = w.player.sitting ? -1 : aimedModule(w);
    for (int i = 0; i < kMaxModules; ++i) {
        if (!w.modules[static_cast<size_t>(i)].active) continue;
        const int d = dependency(w, i);
        if (d < 0) continue;
        const bool down = w.modules[static_cast<size_t>(d)].health <= 0;
        if (!down && i != aimed && d != aimed) continue;
        const Vec3 from = moduleDirection(i), to = moduleDirection(d);
        for (int k = 1; k < 12; ++k) {
            const float f = static_cast<float>(k) / 12.0f;
            const Vec3 mid = from * (1 - f) + to * f;
            if (length(mid) < 1e-3f) continue;
            const Vec3 dir = rolled(w, mid * (1.0f / length(mid)));
            const Vec3 at = c + dir * (R + 0.04f);
            StrataPoint p{};
            p.position[0] = at.x;
            p.position[1] = at.y;
            p.position[2] = at.z;
            p.world_diameter = 0.07f * R;
            p.color[0] = down ? 1.0f : 0.95f;
            p.color[1] = down ? 0.25f : 0.92f;
            p.color[2] = down ? 0.15f : 0.85f;
            p.color[3] = down ? 0.9f : 0.55f;
            p.flags = STRATA_POINT_SOFT;
            pts.push_back(p);
        }
    }
    // The town's windows: one lit for every few users.
    static const std::vector<Vec3> windows = bridge::townWindows();
    const size_t lit = std::min(windows.size(), static_cast<size_t>(w.users / bridge::kUsersPerWindow));
    for (size_t i = 0; i < lit; ++i) {
        StrataPoint p{};
        p.position[0] = windows[i].x;
        p.position[1] = windows[i].y;
        p.position[2] = windows[i].z;
        p.world_diameter = 0.7f;
        p.size_pixels = 1.5f;
        p.color[0] = 1.0f;
        p.color[1] = 0.72f;
        p.color[2] = 0.35f;
        p.color[3] = 0.35f + 0.6f * night;
        p.flags = STRATA_POINT_SOFT;
        pts.push_back(p);
    }
    return pts;
}

std::string badPoint(const std::vector<StrataPoint>& pts) {
    for (size_t i = 0; i < pts.size(); ++i) {
        const float* c = pts[i].color;
        for (int k = 0; k < 4; ++k)
            if (!std::isfinite(c[k]) || c[k] < 0)
                return "point " + std::to_string(i) + ": colour " + std::to_string(k) + " is " + std::to_string(c[k]);
        if (c[3] > 1) return "point " + std::to_string(i) + ": alpha " + std::to_string(c[3]) + " is above 1";
    }
    return "";
}

namespace {

void drawGlows(strata::Engine& engine, const World& w, float night, Vec3 eye) {
    const std::vector<StrataPoint> pts = glowPoints(w, night, eye);
    if (!pts.empty()) engine.submitPoints(pts.data(), pts.size());
}

void text(std::vector<StrataText>& out, std::vector<std::string>& keep, float x, float y, const std::string& s,
          const float color[4], float size = 0) {
    keep.push_back(s);
    StrataText t{};
    t.x = x;
    t.y = y;
    t.pixel_size = size > 0 ? size : gTextSize;
    t.flags = STRATA_TEXT_SHADOW;
    for (int i = 0; i < 4; ++i) t.color[i] = color[i];
    out.push_back(t);
}

bool toScreen(const StrataCamera& cam, Vec3 p, int32_t width, int32_t height, float& x, float& y) {
    float v[4], c[4];
    const float in[4] = {p.x, p.y, p.z, 1};
    for (int r = 0; r < 4; ++r) v[r] = cam.view[r] * in[0] + cam.view[4 + r] * in[1] + cam.view[8 + r] * in[2] + cam.view[12 + r] * in[3];
    for (int r = 0; r < 4; ++r)
        c[r] = cam.projection[r] * v[0] + cam.projection[4 + r] * v[1] + cam.projection[8 + r] * v[2] + cam.projection[12 + r] * v[3];
    if (c[3] <= 0.1f) return false;
    x = (c[0] / c[3] * 0.5f + 0.5f) * static_cast<float>(width);
    y = (0.5f - c[1] / c[3] * 0.5f) * static_cast<float>(height);
    return x > 0 && x < static_cast<float>(width) && y > 0 && y < static_cast<float>(height);
}

// Whether the boulder stands between the eye and a point.
bool behindBoulder(const World& w, Vec3 eye, Vec3 p) {
    const Vec3 c = boulderCentre(w);
    const float R = boulderRadius(w);
    Vec3 dir = p - eye;
    const float len = length(dir);
    dir = dir * (1.0f / len);
    float t = 0;
    return sightPasses(eye, dir, c, R, t) && t < len;
}

std::string percent(float v) { return std::to_string(static_cast<int>(std::lround(v * 100))) + "%"; }

std::string padded(std::string s, size_t n) {
    if (s.size() < n) s.append(n - s.size(), ' ');
    return s;
}

}  // namespace

float sunHeight(const World& w) {
    float to[3];
    sunDirection(w, to);
    return to[1];
}

void setLighting(strata::Engine& engine, bool lightweight) {
    StrataLighting light = strata::lightingDefaults(STRATA_LIGHTING_SUN);
    light.cascade_count = lightweight ? 3 : 4;
    light.shadow_map_size = lightweight ? 1024 : 2048;
    light.shadow_distance = 90.0f;
    light.fog = 1;
    engine.setLighting(light);
}

Camera cameraAtEyes(const World& w) { return {eyePosition(w), w.player.yaw, w.player.pitch}; }

Camera cameraBehind(const World& w) {
    const Player& p = w.player;
    const float sy = detSin(p.yaw), cy = detCos(p.yaw);
    Camera c;
    c.eye = {p.x - sy * 4.5f, 0, p.z + cy * 4.5f};
    c.eye.y = std::max(hill::groundHeight(p.x, p.z) + 2.6f, hill::groundHeight(c.eye.x, c.eye.z) + 1.0f);
    const Vec3 at{p.x + sy * 2.0f, hill::groundHeight(p.x, p.z) + 1.3f, p.z - cy * 2.0f};
    const Vec3 d = at - c.eye;
    c.yaw = std::atan2(d.x, -d.z);
    c.pitch = std::atan2(d.y, std::sqrt(d.x * d.x + d.z * d.z));
    return c;
}

Camera cameraFollow(const World& w) {
    const Player& p = w.player;
    const Vec3 dir = lookDirection(p.yaw, p.pitch);
    Camera c;
    c.eye = eyePosition(w) - dir * 3.8f + Vec3{0, 0.9f, 0};
    c.eye.y = std::max(c.eye.y, hill::groundHeight(c.eye.x, c.eye.z) + 0.6f);
    c.yaw = p.yaw;
    c.pitch = p.pitch;
    return c;
}

Camera cameraBeside(const World& w, float side, float back) {
    const Player& p = w.player;
    const Vec3 b = boulderCentre(w);
    const Vec3 mid{(p.x + b.x) / 2, (hill::groundHeight(p.x, p.z) + b.y) / 2 + 0.3f, (p.z + b.z) / 2};
    Camera c;
    const float dist = 6.0f + 2.0f * boulderRadius(w);
    c.eye = {mid.x + side * dist, 0, mid.z + back};
    c.eye.y = std::max(mid.y + 1.2f, hill::groundHeight(c.eye.x, c.eye.z) + 1.4f);
    const Vec3 d = mid - c.eye;
    c.yaw = std::atan2(d.x, -d.z);
    c.pitch = std::atan2(d.y, std::sqrt(d.x * d.x + d.z * d.z));
    return c;
}

std::string bar(float value, int width) {
    const int n = static_cast<int>(std::lround(clampf(value, 0, 1) * static_cast<float>(width)));
    return "[" + std::string(static_cast<size_t>(n), '#') + std::string(static_cast<size_t>(width - n), '.') + "]";
}

// Where module i is on the stone, as he sees it: on top, underneath, the
// side facing him, the far side, or to his left or right.
std::string whereOn(const World& w, int i) {
    const Vec3 n = rolled(w, moduleDirection(i));
    if (n.y > 0.7f) return "on top";
    if (n.y < -0.55f) return "under: roll it";
    const Vec3 c = boulderCentre(w);
    float tx = w.player.x - c.x, tz = w.player.z - c.z;
    const float tl = std::sqrt(tx * tx + tz * tz);
    if (tl < 1e-3f) return "";
    tx /= tl;
    tz /= tl;
    const float nl = std::sqrt(n.x * n.x + n.z * n.z);
    const float facing = (n.x * tx + n.z * tz) / (nl > 1e-3f ? nl : 1.0f);
    if (facing > 0.5f) return "facing you";
    if (facing < -0.5f) return "far side";
    // Seen from him, looking at the boulder (along -t): his right is (-tz, tx).
    return n.x * -tz + n.z * tx > 0 ? "right" : "left";
}

// Which way up the hill is, from where he faces, when it is not ahead.
std::string uphillWay(const World& w) {
    float turn = -w.player.yaw;  // uphill is yaw 0
    while (turn > kPi) turn -= 2 * kPi;
    while (turn < -kPi) turn += 2 * kPi;
    if (std::fabs(turn) < 0.6f) return "";
    return std::fabs(turn) > 2.5f ? " (behind you)" : turn > 0 ? " (to your right)" : " (to your left)";
}

namespace {
// Which way the boulder is, from where he faces.
const char* boulderWay(const World& w) {
    const float turn = boulderBearing(w);
    return std::fabs(turn) < 0.5f ? "ahead" : std::fabs(turn) > 2.6f ? "behind you" : turn > 0 ? "to your right" : "to your left";
}
}  // namespace

std::vector<HudLine> hudLines(const World& w) {
    std::vector<HudLine> out;
    out.push_back({"SISYPHUS   version " + versionText(w) + "   " + seasonName(season(w)), true});
    if (w.rain > 0.2f) out.push_back({snowing(w) ? "Snow: the path is slick" : "Rain: the path is slick", false, false, true});
    const int users = static_cast<int>(w.users);
    if (outage(w)) {
        int down = -1;
        for (int i = 0; i < kMaxModules && down < 0; ++i)
            if (w.modules[static_cast<size_t>(i)].active && w.modules[static_cast<size_t>(i)].health <= 0) down = i;
        out.push_back({"Users " + std::to_string(users) + "   OUTAGE: " + moduleName(down) + " is down (" + whereOn(w, down) + ")",
                       false, true});
    }
    else out.push_back({"Users " + std::to_string(users)});
    out.push_back({"Modules " + std::to_string(activeModules(w)) + "   health " + bar(health(w), 10) + " " + percent(health(w))});
    const float L = load(w);
    out.push_back({"Weight  " + bar(L, 10) + (L >= 1 ? "  too heavy to push uphill" : L >= 0.7f ? "  heavy" : ""), false, L >= 1});
    // What it is made of, once it starts to tell: rot, and above all what is down.
    if (L >= 0.7f) {
        const int down = brokenModules(w);
        out.push_back({down > 0 ? "  Down modules weigh most: patch or retire"
                                : "  Rot weighs: patch the oldest",
                       false, false});
    }
    if (L >= 0.7f) out.push_back({"  (rot weighs it down: patch, retire or refactor)", false, false, true});
    if (w.player.stamina < 0.9f)
        out.push_back({"Stamina " + bar(w.player.stamina, 10) + (tired(w) ? "  tired: rest (Q) on a terrace" : ""), false, tired(w), !tired(w)});
    if (w.summitSeconds >= 0) out.push_back({"At the summit: " + versionText(w) + " ships."});
    else if (w.falling) out.push_back({w.rewriteChosen ? "It rolls down to be rewritten. Follow it down." : "It rolls. Follow it down."});
    else if (w.boulder.v < -0.5f && !w.pushing && !hill::level(w.boulder.s))
        out.push_back({"The boulder is rolling back down!", false, true});
    else {
        const int next = w.best + 1;
        const float d = hill::terraceStart(next) - w.boulder.s;
        out.push_back({"Next " + nextVersionText(w) + (next >= hill::kSummit ? " (the summit)" : "") + ": " +
                       (d < 1 ? std::string("under 1") : std::to_string(static_cast<int>(std::ceil(d)))) + " m uphill" + uphillWay(w)});
    }
    // Where the boulder is, once it is out of reach.
    {
        const Vec3 c = boulderCentre(w);
        const float dx = c.x - w.player.x, dz = c.z - w.player.z;
        const float d = std::sqrt(dx * dx + dz * dz);
        if (d > boulderRadius(w) + 4.0f)
            out.push_back({"The boulder: " + std::to_string(static_cast<int>(d)) + " m " + boulderWay(w)});
    }
    // Whoever is helping.
    if (w.helper.name >= 0) {
        const Helper& h = w.helper;
        const int left = static_cast<int>(std::ceil(h.secondsLeft));
        const std::string clock = std::to_string(left / 60) + ":" + (left % 60 < 10 ? "0" : "") + std::to_string(left % 60);
        const Vec3 to = helperSpot(w);
        const float dx = to.x - h.x, dz = to.z - h.z;
        const std::string doing = dx * dx + dz * dz > 0.1f        ? "on the way up"
                                  : h.module >= 0                 ? std::string("patching ") + moduleName(h.module)
                                  : boulderResting(w)             ? "nothing they can reach"
                                                                  : "waiting for it to settle";
        out.push_back({std::string(helperName(h.name)) + " is helping: " + doing + ", " + clock + " left", false, false, false});
    }
    out.push_back({"", false, false, true});
    // The open issues, worst first.
    std::vector<int> open;
    for (int i = 0; i < kMaxModules; ++i) {
        const Module& m = w.modules[static_cast<size_t>(i)];
        if (m.active && (m.issue != Issue::None || m.health <= 0.5f || cascading(w, i))) open.push_back(i);
    }
    std::sort(open.begin(), open.end(), [&](int a, int b) {
        const float ha = w.modules[static_cast<size_t>(a)].health, hb = w.modules[static_cast<size_t>(b)].health;
        return ha != hb ? ha < hb : a < b;
    });
    out.push_back({open.empty() ? "No open issues. It will not last." : "OPEN ISSUES " + std::to_string(open.size()), true});
    const size_t shown = std::min<size_t>(open.size(), 6);
    for (size_t k = 0; k < shown; ++k) {
        const int i = open[k];
        const Module& m = w.modules[static_cast<size_t>(i)];
        static const char* const kShort[kIssueKinds] = {"", "drift", "flaky test", "deprecated", "advisory", "certificate"};
        const std::string what = m.health <= 0               ? "DOWN"
                                 : m.issue != Issue::None      ? kShort[static_cast<int>(m.issue)]
                                 : cascading(w, i)             ? std::string("uses ") + moduleName(dependency(w, i))
                                                               : "rotting";
        // How fast it is going: percent a minute.
        const int perMinute = static_cast<int>(std::lround(rotRate(w, i) * 6000.0f));
        const std::string pace = m.health <= 0 ? "" : "  -" + std::to_string(perMinute) + "%/min";
        out.push_back({padded(moduleName(i), 13) + padded(what, 12) + padded(percent(m.health), 5) + padded(pace, 11) + whereOn(w, i),
                       false, m.health <= 0});
    }
    if (open.size() > shown) out.push_back({"... and " + std::to_string(open.size() - shown) + " more"});
    return out;
}

namespace {
std::string column(const std::string& s, size_t width) {
    return s.size() >= width ? s + " " : std::string(width - s.size(), ' ') + s;
}
}  // namespace

std::vector<std::string> historyLines(const World& w) {
    std::vector<std::string> out;
    out.push_back("HISTORY   (Tab to close)");
    out.push_back("");
    struct Row {
        std::string version;
        int64_t minutes;
        int modules, users, patched, retired, refactored, down, helpers;
    };
    // One layout for the header and the rows: the version on the left, the
    // numbers right-aligned under their names.
    static const char* const kNames[] = {"min", "modules", "users", "patched", "retired", "refactored", "down", "helpers"};
    auto line = [](const std::string& first, const std::vector<std::string>& cells) {
        std::string l = first + std::string(first.size() < 9 ? 9 - first.size() : 1, ' ');
        for (size_t i = 0; i < cells.size(); ++i) l += column(cells[i], std::string(kNames[i]).size() + 2);
        return l;
    };
    out.push_back(line("version", {kNames, kNames + 8}));
    std::vector<Row> rows;
    Cairn before{};
    for (int k = 0; k < w.cairnCount; ++k) {
        const Cairn& c = w.cairns[static_cast<size_t>(k)];
        rows.push_back({std::to_string(c.major) + ".0" + (c.rewrite ? "*" : ""), (c.tick - before.tick) / (60 * kTickRate), c.modules,
                        static_cast<int>(c.users), c.patched - before.patched, c.retired - before.retired,
                        c.refactored - before.refactored, c.breakdowns - before.breakdowns, c.helpers});
        before = c;
    }
    // The climb under way, so far.
    rows.push_back({"now " + std::to_string(w.falling ? w.major + 1 : w.major) + ".x", (w.tick - before.tick) / (60 * kTickRate),
                    activeModules(w), static_cast<int>(w.users), w.patched - before.patched, w.retired - before.retired,
                    w.refactored - before.refactored, w.breakdowns - before.breakdowns, w.helpersThisClimb});
    for (const Row& r : rows)
        out.push_back(line(r.version, {std::to_string(r.minutes), std::to_string(r.modules), std::to_string(r.users),
                                       std::to_string(r.patched), std::to_string(r.retired), std::to_string(r.refactored),
                                       std::to_string(r.down), std::to_string(r.helpers)}));
    for (int k = 0; k < w.cairnCount; ++k)
        if (w.cairns[static_cast<size_t>(k)].rewrite) {
            out.push_back("* rewritten at the summit (modules and users at the end, before the rewrite)");
            break;
        }
    out.push_back("(min: minutes of the climb, from the trailhead to the summit and back down)");
    if (w.cairnCount == 0) {
        out.push_back("");
        out.push_back("No climb has finished yet.");
        out.push_back("Each one that does leaves a cairn at the trailhead.");
        return out;
    }
    // Bars, scaled to the biggest.
    auto bars = [&](const char* title, auto value) {
        out.push_back("");
        out.push_back(title);
        int most = 1;
        for (const Row& r : rows) most = std::max(most, value(r));
        for (const Row& r : rows) {
            const int n = static_cast<int>(std::lround(30.0 * value(r) / most));
            out.push_back(r.version + std::string(r.version.size() < 9 ? 9 - r.version.size() : 1, ' ') +
                          std::string(static_cast<size_t>(n), '#') + " " + std::to_string(value(r)));
        }
    };
    bars("Minutes per climb", [](const Row& r) { return static_cast<int>(r.minutes); });
    bars("Users at the end of each climb", [](const Row& r) { return r.users; });
    return out;
}

std::string prompt(const World& w) {
    if (w.player.sitting) return "";
    // The choice at the top.
    if (w.summitSeconds >= 0) {
        if (w.rewriteChosen) return "Rewrite chosen: it starts again light at the trailhead, and most users will not follow";
        if (w.rewriteSeconds > 0) return "Rewriting " + bar(w.rewriteSeconds / kRewriteSeconds, 12);
        return "It will roll back down with every module aboard.\nOr hold R to rewrite it: start again light, and lose most users";
    }
    if (w.pushing) {
        if (w.slipping) return "Too heavy: it slides back. Let it settle on a terrace, and patch the rot.";
        if (w.pushingBack) {
            const int t = hill::terraceAt(w.boulder.s);
            const float edge = t >= 0 ? hill::terraceStart(t) + 0.3f : hill::kRestS;
            if (w.boulder.s <= edge + 0.05f) return "At the terrace's edge: it rolls back no further. Push it on from below.";
            return "Rolling it back to turn it";
        }
        if (load(w) >= 1) return "Too heavy to push uphill; on level ground it only inches round";
        if (tired(w)) return hill::level(w.boulder.s) ? "Tired: he pushes slowly. Sit down (Q) to rest"
                                                      : "Tired: he pushes slowly. Push on to the terrace to rest: sit here and it rolls back";
        return "";
    }
    if (w.blocked) return "Not lined up to push: put the boulder in the middle of your view, and walk into it";
    const int i = aimedModule(w);
    if (i < 0 && cairnHere(w) >= 0) return cairnStory(w, cairnHere(w));
    if (i < 0) {
        // Close to it but not pushing: say how.
        const Vec3 c = boulderCentre(w);
        const float dx = c.x - w.player.x, dz = c.z - w.player.z;
        const float d = std::sqrt(dx * dx + dz * dz);
        if (d < boulderRadius(w) + 2.5f && !w.falling && w.summitSeconds < 0)
            return dz < 0 ? "Face the boulder and walk into it (W) to push it up the hill"
                          : "From up here, walking into it (W) rolls it back (on level ground)";
        // Walked on up without it: it slipped away, or was let go. Not said
        // while it is ahead of him, and said which way to turn.
        if (dz > 0 && d > boulderRadius(w) + 4.0f && !w.falling && w.summitSeconds < 0 &&
            std::fabs(boulderBearing(w)) >= 0.5f)
            return std::string("The boulder is down the hill, ") + boulderWay(w) + ": go back down to it";
        return "";
    }
    const Module& m = w.modules[static_cast<size_t>(i)];
    std::string s;
    // On a slope the boulder rolls back the moment he lets go of it.
    if (!hill::level(w.boulder.s) && !w.falling && w.summitSeconds < 0 && m.health < 0.98f)
        s += "On the slope it rolls back if you stop: patch on a terrace\n";
    s += std::string(moduleName(i)) + ": ";
    if (m.health <= 0) s += "DOWN";
    else if (m.issue != Issue::None) s += std::string(issueName(m.issue)) + ", " + issueLine(m.issue);
    else s += m.health >= 0.98f ? "healthy" : m.health > 0.9f ? "nearly new" : "rotting, no issue yet";
    s += " (" + percent(m.health) + ")";
    const int dep = dependency(w, i);
    if (dep >= 0) s += cascading(w, i) ? std::string(", and ") + moduleName(dep) + ", which it uses, is DOWN"
                                       : std::string(", uses ") + moduleName(dep);
    s += "\n";
    const bool working = w.work.module == i;
    // The work goes on from where it stopped when the key is held again.
    if (working && w.work.retiring) return s + "Retiring (hold R) " + bar(w.work.seconds / kRetireSeconds, 12);
    if (working && w.work.refactoring)
        return s + "Refactoring it into " + moduleName(dep) + " (hold F) " + bar(w.work.seconds / kRefactorSeconds, 12);
    if (working) s += "Patching (hold E) " + bar(w.work.seconds / patchSeconds(m), 12);
    else if (m.health >= 0.98f) s += "Nothing to patch";
    else s += "Hold E to patch";
    if (activeModules(w) > kMinModules) {
        s += "\n(hold R to retire it";
        if (canRefactor(w, i)) s += std::string(", F to refactor it into ") + moduleName(dep);
        s += ")";
    }
    return s;
}

void setTextScale(float scale) { gTextScale = scale; }

std::vector<std::string> wrapped(const std::string& text, size_t columns) {
    std::vector<std::string> out;
    size_t at = 0;
    while (at <= text.size()) {
        size_t end = text.find('\n', at);
        if (end == std::string::npos) end = text.size();
        std::string line;
        size_t i = at;
        while (i < end) {
            size_t sp = text.find(' ', i);
            if (sp == std::string::npos || sp > end) sp = end;
            const std::string word = text.substr(i, sp - i);
            if (!line.empty() && line.size() + 1 + word.size() > columns) {
                out.push_back(line);
                line.clear();
            }
            line += (line.empty() ? "" : " ") + word;
            i = sp + 1;
        }
        if (!line.empty() || end == at) out.push_back(line);
        if (end == text.size()) break;
        at = end + 1;
    }
    return out;
}

void render(strata::Engine& engine, const World& w, const Camera& cam, bool outside, const Overlay& o) {
    const float sunUp = setSky(engine, w);
    const float night = nightness(sunUp);
    setLights(engine, w, night);
    int32_t width = 0, height = 0;
    engine.drawableSize(width, height);
    const float fit = height < 600 ? 0.75f : 1.0f;
    gTextSize = std::round(16.0f * gTextScale * fit);
    gLine = std::round(20.0f * gTextScale * fit);
    const float aspect = height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
    const StrataCamera camera = lookFrom(cam.eye, cam.yaw, cam.pitch, aspect);
    engine.setCamera(camera);

    StrataFrameDesc fd{};
    fd.clear_flags = STRATA_CLEAR_COLOR | STRATA_CLEAR_DEPTH | STRATA_CLEAR_SKY;
    engine.beginFrame(fd);
    drawGlows(engine, w, night, cam.eye);

    std::vector<StrataText> texts;
    std::vector<std::string> keep;
    keep.reserve(48);
    const float ink[4] = {0.95f, 0.92f, 0.85f, 1.0f};
    const float gold[4] = {1.0f, 0.70f, 0.20f, 1.0f};
    const float alarm[4] = {1.0f, 0.35f, 0.25f, 1.0f};
    const auto columns = static_cast<size_t>(std::max(16.0f, (static_cast<float>(width) - 2 * kMargin) / gTextSize));
    auto centred = [&](const std::string& s, float row, const float* colour) {
        for (const std::string& line : wrapped(s, columns)) {
            text(texts, keep, static_cast<float>(width) / 2 - gTextSize * 0.5f * static_cast<float>(line.size()), row, line, colour);
            row += gLine;
        }
        return row;
    };

    float wordsEnd = 0;  // the bottom of the prompt and the message
    if (o.history) {
        // The history in place of everything else.
        if (!o.noText) {
            // Smaller text if the widest line would not fit the window.
            const std::vector<std::string> lines = historyLines(w);
            size_t widest = 1;
            for (const std::string& l : lines) widest = std::max(widest, l.size());
            const float size = gTextSize, line = gLine;
            gTextSize = std::min(gTextSize, std::floor((static_cast<float>(width) - 2 * kMargin) / static_cast<float>(widest)));
            gLine = std::round(gTextSize * 1.25f);
            float y = kMargin;
            for (const std::string& l : lines) {
                const bool head = !l.empty() && (l[0] == 'H' || l[0] == 'M' || l[0] == 'U' || l[0] == 'v');
                if (!l.empty()) text(texts, keep, kMargin, y, l, head ? gold : ink);
                y += gLine;
            }
            gTextSize = size;
            gLine = line;
        }
    } else if (w.player.sitting) {
        // Sitting: no HUD, only the view, and the words.
        if (!o.quote.empty()) centred(o.quote, static_cast<float>(height) * 0.25f, ink);
        // ... except that an outage still shows: nothing goes down unseen.
        if (outage(w)) {
            for (int i = 0; i < kMaxModules; ++i)
                if (w.modules[static_cast<size_t>(i)].active && w.modules[static_cast<size_t>(i)].health <= 0) {
                    centred(std::string("OUTAGE: ") + moduleName(i) + " is down", kMargin, alarm);
                    break;
                }
        }
        // ... and so does the boulder rolling away down a slope.
        if (w.boulder.v < -0.5f && !w.falling && !hill::level(w.boulder.s))
            centred("The boulder is rolling back down!", kMargin + gLine, alarm);
    } else {
        float y = kMargin;
        float hudRight = 0;  // the HUD's block, which labels keep out of
        std::vector<HudLine> hud = hudLines(w);
        // Short of room above the prompt: drop the optional lines first, so
        // the open issues always show.
        const auto rows = static_cast<size_t>(std::max(4.0f, (static_cast<float>(height) * 0.5f - gLine - y) / gLine));
        for (size_t k = hud.size(); k-- > 0 && hud.size() > rows;)
            if (hud[k].optional) hud.erase(hud.begin() + static_cast<long>(k));
        // The HUD stops above the prompt under the crosshair: a long list of
        // issues ends in "... and N more" rather than printing over it.
        const float hudLimit = static_cast<float>(height) * 0.5f - gLine;
        for (size_t li = 0; li < hud.size(); ++li) {
            const HudLine& l = hud[li];
            if (y + 2 * gLine > hudLimit && li + 1 < hud.size()) {
                const std::string more = "... and " + std::to_string(hud.size() - li) + " more lines";
                text(texts, keep, kMargin, y, more, ink);
                hudRight = std::max(hudRight, kMargin + gTextSize * static_cast<float>(more.size()));
                y += gLine;
                break;
            }
            if (!l.text.empty()) text(texts, keep, kMargin, y, l.text, l.alarm ? alarm : l.heading ? gold : ink);
            hudRight = std::max(hudRight, kMargin + gTextSize * static_cast<float>(l.text.size()));
            y += gLine;
        }
        const float hudBottom = y;
        auto overHud = [&](float x, float top) { return x < hudRight + gTextSize && top < hudBottom; };
        const std::string promptText = prompt(w);
        // The release numbers over the terraces ahead.
        for (int i = 1; i < hill::kTerraces; ++i) {
            const float s = hill::terraceStart(i) + hill::kFlat / 2;
            const Vec3 at{-hill::kPathHalf + 0.9f, hill::pathHeight(s) + 3.0f, hill::zOf(s)};
            const float dx = at.x - cam.eye.x, dz = at.z - cam.eye.z;
            const float d = std::sqrt(dx * dx + dz * dz);
            float sx = 0, sy = 0;
            if (d < 4.0f || d > 80.0f || !toScreen(camera, at, width, height, sx, sy)) continue;
            if (behindBoulder(w, cam.eye, at)) continue;
            if (!promptText.empty() && sy > static_cast<float>(height) * 0.45f &&
                std::fabs(sx - static_cast<float>(width) / 2) < static_cast<float>(width) * 0.4f)
                continue;  // not over the prompt
            const std::string v = i == hill::kSummit ? std::to_string(w.major + 1) + ".0"
                                                     : std::to_string(w.falling ? w.major + 1 : w.major) + "." + std::to_string(i);
            const bool shipped = w.falling || i <= w.best;
            const float a = clampf((80.0f - d) / 20.0f, 0, 1) * 0.9f;
            const float c[4] = {shipped ? gold[0] : ink[0], shipped ? gold[1] : ink[1], shipped ? gold[2] : ink[2], a};
            if (!overHud(sx - gTextSize, sy)) text(texts, keep, sx - gTextSize, sy, v, c);
        }
        // The versions over the cairns at the trailhead.
        for (int k = 0; k < w.cairnCount; ++k) {
            const Vec3 b = cairnBase(k);
            const Vec3 at{b.x, b.y + 2.0f, b.z};
            const float dx = at.x - cam.eye.x, dz = at.z - cam.eye.z;
            const float d = std::sqrt(dx * dx + dz * dz);
            float sx = 0, sy = 0;
            if (d < 1.0f || d > 30.0f || !toScreen(camera, at, width, height, sx, sy)) continue;
            if (behindBoulder(w, cam.eye, at)) continue;
            const std::string v = std::to_string(w.cairns[static_cast<size_t>(k)].major) + ".0";
            const float c[4] = {gold[0], gold[1], gold[2], clampf((30.0f - d) / 10.0f, 0, 1) * 0.85f};
            if (!overHud(sx - gTextSize, sy)) text(texts, keep, sx - gTextSize, sy, v, c);
        }
        // The names of the modules in trouble, on the stone where they are.
        {
            const Vec3 c = boulderCentre(w);
            const float R = boulderRadius(w);
            for (int i = 0; i < kMaxModules; ++i) {
                const Module& m = w.modules[static_cast<size_t>(i)];
                if (!m.active || (m.issue == Issue::None && m.health > 0.5f)) continue;
                const Vec3 n = rolled(w, moduleDirection(i));
                const Vec3 at = c + n * (R + 0.1f);
                const Vec3 toEye = cam.eye - at;
                const float d = length(toEye);
                if (d > 30.0f || dot(n, toEye) <= 0) continue;  // far, or on the far side
                float sx = 0, sy = 0;
                if (!toScreen(camera, at, width, height, sx, sy)) continue;
                const bridge::Rgb col = m.health <= 0 ? bridge::Rgb{1.0f, 0.3f, 0.25f} : bridge::issueColor(m.issue);
                const float c4[4] = {col.r, col.g, col.b, clampf((30.0f - d) / 8.0f, 0, 1)};
                const std::string name = moduleName(i);
                const float left = sx - gTextSize * 0.5f * static_cast<float>(name.size());
                if (!overHud(left, sy - gTextSize)) text(texts, keep, left, sy - gTextSize, name, c4);
            }
        }
        if (o.aim && !outside) text(texts, keep, static_cast<float>(width) / 2 - gTextSize / 2, static_cast<float>(height) / 2 - gTextSize / 2, "+", ink);
        if (o.aim && outside) {
            // From behind him, the crosshair marks where his own line of sight lands.
            const Vec3 eye = eyePosition(w), dir = lookDirection(w.player.yaw, w.player.pitch);
            Vec3 at;
            if (moduleOnSight(w, eye, dir, &at) < 0) at = eye + dir * 8.0f;
            float sx = 0, sy = 0;
            if (toScreen(camera, at, width, height, sx, sy)) text(texts, keep, sx - gTextSize / 2, sy - gTextSize / 2, "+", ink);
        }
        // The prompt, then the message, below the crosshair (lower from behind
        // him, where the crosshair sits low), moved up if they would run off
        // the bottom of a small window.
        const std::string here = prompt(w);
        const size_t lines = (here.empty() ? 0 : wrapped(here, columns).size()) +
                             (o.message.empty() ? 0 : 1 + wrapped(o.message, columns).size());
        float py = static_cast<float>(height) * (outside ? 0.6f : 0.5f) + 2 * gLine;
        py = std::min(py, static_cast<float>(height) - kMargin - gLine * static_cast<float>(lines));
        if (!here.empty()) py = centred(here, py, w.slipping ? alarm : ink) + gLine;
        if (!o.message.empty()) py = centred(o.message, py, gold);
        wordsEnd = py;
    }
    if (o.help) {
        std::vector<std::string> items = {"W A S D walk / push", "Shift hurry", "Mouse look", "E hold: patch",
                                          "R hold: retire", "F hold: refactor", "Q sit", "V camera", "Tab history", "M sound"};
        if (o.saves) items.insert(items.end(), {"F5 save", "F9 load"});
        items.insert(items.end(), {"F1 hide help", "Esc free mouse"});
        std::vector<std::string> lines(1);
        for (const std::string& item : items) {
            if (!lines.back().empty() && lines.back().size() + 3 + item.size() > columns) lines.emplace_back();
            lines.back() += (lines.back().empty() ? "" : "   ") + item;
        }
        float bottom = static_cast<float>(height) - kMargin - gTextSize;
        // The controls give way to the words when a small window has no room for both.
        if (bottom - gLine * static_cast<float>(lines.size() - 1) < wordsEnd) lines.clear();
        for (size_t i = lines.size(); i-- > 0;) {
            const float dim[4] = {ink[0], ink[1], ink[2], 0.8f};
            text(texts, keep, kMargin, bottom, lines[i], dim);
            bottom -= gLine;
        }
    }
    for (size_t i = 0; i < texts.size(); ++i) texts[i].utf8 = keep[i].c_str();
    if (!o.noText && !texts.empty()) engine.submitText(texts.data(), texts.size());
    engine.endFrame();
}

}  // namespace sisyphus::play
