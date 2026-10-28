// Sisyphus in a browser: the same game on Strata's WebGL 2 backend (Emscripten).
//
// The same rules, scene, HUD and session (src/play) as the desktop game, with
// these differences:
//
//  - The page owns the pointer lock (a click on the canvas takes it, Esc
//    gives it back) and turns mouse movement into sisyphus_look(); there is
//    no quit.
//  - On a touch screen the page draws a stick, a look pad, and buttons held
//    to patch or retire (sisyphus_hold) or pressed to sit and change camera.
//  - On a narrow page the canvas shows no words: the page shows the HUD, the
//    crosshair and the prompt as text (sisyphus_status()).
//  - Saves are the desktop format (src/save), written to
//    /sisyphus-save/default, which the page mounts on IndexedDB (IDBFS) and
//    loads before main() runs. Every save is flushed to IndexedDB at once.
//    They stay in that browser: they are never sent anywhere.
#include <cinttypes>
#include <cstdio>
#include <memory>
#include <string>

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include "play/session.h"
#include "play/view.h"
#include "strata/strata.hpp"

using namespace sisyphus;

namespace {

struct Game {
    std::unique_ptr<strata::Engine> engine;
    std::unique_ptr<play::Session> session;
    double last = 0;
    std::string statusJson;
};

constexpr const char* kSlot = "/sisyphus-save/default";

Game* g = nullptr;

void frame(void*) {
    StrataEvent e;
    while (g->engine->pollEvent(e)) g->session->handle(e);
    const double now = emscripten_get_now() / 1000.0;
    g->session->frame(now - g->last);
    g->last = now;
    g->engine->present();
}

void jsonString(std::string& out, const std::string& s) {
    out += '"';
    for (char ch : s) {
        if (ch == '"' || ch == '\\') {
            out += '\\';
            out += ch;
        } else if (ch == '\n') {
            out += "\\n";
        } else if (static_cast<unsigned char>(ch) < 0x20) {
            out += ' ';
        } else {
            out += ch;
        }
    }
    out += '"';
}

void jsonBool(std::string& out, const char* key, bool v) {
    out += ",\"";
    out += key;
    out += v ? "\":true" : "\":false";
}

}  // namespace

// ---- Called by the page (web/shell.html) ------------------------------------
extern "C" {

EMSCRIPTEN_KEEPALIVE void sisyphus_look(double dx, double dy) {
    if (g) g->session->look(static_cast<float>(dx), static_cast<float>(dy));
}
EMSCRIPTEN_KEEPALIVE void sisyphus_stick(double forward, double strafe) {
    if (g) g->session->setStick(static_cast<float>(forward), static_cast<float>(strafe));
}
// The page's Patch and Retire buttons, held down or let go.
EMSCRIPTEN_KEEPALIVE void sisyphus_hold(int work, int retire) {
    if (g) g->session->setHeld(work != 0, retire != 0);
}
// A key press from an on-screen button (an SDL scancode, as the keyboard sends).
EMSCRIPTEN_KEEPALIVE void sisyphus_press(int scancode) {
    if (g) g->session->press(scancode);
}
EMSCRIPTEN_KEEPALIVE void sisyphus_captured(int on) {
    if (g) g->session->setCaptured(on != 0);
}
// Saves now (the page calls this when it is hidden); 1 on success.
EMSCRIPTEN_KEEPALIVE int sisyphus_save() { return g && g->session->save("Saved.") ? 1 : 0; }
EMSCRIPTEN_KEEPALIVE void sisyphus_restart() {
    if (g) g->session->restart();
}
EMSCRIPTEN_KEEPALIVE void sisyphus_help(int on) {
    if (g) g->session->setHelp(on != 0);
}
// The canvas changed size: draw at w x h (the page caps the device pixel
// ratio), with the HUD's words on the page instead when `compact`.
EMSCRIPTEN_KEEPALIVE void sisyphus_resize(double w, double h, int compact) {
    if (!g || w < 1 || h < 1) return;
    g->engine->resize(static_cast<int32_t>(w), static_cast<int32_t>(h));
    g->session->setCompactHud(compact != 0);
}
// The game's state for the page, as JSON.
EMSCRIPTEN_KEEPALIVE const char* sisyphus_status() {
    if (!g) return "{}";
    const play::Session& s = *g->session;
    const World& w = s.world();
    std::string o = "{\"version\":";
    jsonString(o, versionText(w));
    char buf[256];
    std::snprintf(buf, sizeof buf,
                  ",\"tick\":%" PRId64 ",\"x\":%.3f,\"z\":%.3f,\"yaw\":%.3f,\"boulder\":%.3f,\"modules\":%d,\"releases\":%d,"
                  "\"patched\":%d,\"users\":%d,\"load\":%.3f,\"aimed\":%d",
                  w.tick, static_cast<double>(w.player.x), static_cast<double>(w.player.z),
                  static_cast<double>(w.player.yaw), static_cast<double>(w.boulder.s), activeModules(w), w.releases,
                  w.patched, static_cast<int>(w.users), static_cast<double>(load(w)), aimedModule(w));
    o += buf;
    jsonBool(o, "captured", s.captured());
    jsonBool(o, "help", s.help());
    jsonBool(o, "outside", s.outside());
    jsonBool(o, "sitting", w.player.sitting);
    jsonBool(o, "pushing", w.pushing);
    o += ",\"message\":";
    jsonString(o, s.message());
    o += ",\"quote\":";
    jsonString(o, s.quote());
    o += ",\"prompt\":";
    jsonString(o, play::prompt(w));
    o += ",\"hud\":[";
    bool first = true;
    for (const play::HudLine& l : play::hudLines(w)) {
        if (!first) o += ',';
        first = false;
        o += "{\"heading\":";
        o += l.heading ? "true" : "false";
        o += ",\"alarm\":";
        o += l.alarm ? "true" : "false";
        o += ",\"text\":";
        jsonString(o, l.text);
        o += '}';
    }
    o += "]}";
    g->statusJson = std::move(o);
    return g->statusJson.c_str();
}

}  // extern "C"

int main() {
    static Game game;
    g = &game;

    double cw = 0, ch = 0;
    emscripten_get_element_css_size("#canvas", &cw, &ch);
    if (cw < 1 || ch < 1) {
        cw = 1280;
        ch = 720;
    }
    strata::EngineDesc desc;
    desc.surface = STRATA_SURFACE_WINDOW;
    desc.width = static_cast<int32_t>(cw);
    desc.height = static_cast<int32_t>(ch);
    desc.title = "Sisyphus";
    try {
        g->engine = strata::Engine::create(desc);
    } catch (const std::exception& ex) {
        std::fprintf(stderr, "sisyphus: cannot start the renderer: %s\n", ex.what());
        std::string js = "if (Module.onRendererFailed) Module.onRendererFailed(";
        jsonString(js, ex.what());
        js += ")";
        emscripten_run_script(js.c_str());
        return 1;
    }
    play::setLighting(*g->engine, true);

    play::SessionOptions o;
    o.browser = true;
    o.slot = kSlot;
    o.afterSave = [] { emscripten_run_script("if (Module.flushSaves) Module.flushSaves()"); };
    g->session = std::make_unique<play::Session>(*g->engine, o);

    g->last = emscripten_get_now() / 1000.0;
    emscripten_run_script("if (Module.onGameReady) Module.onGameReady()");
    emscripten_set_main_loop_arg(frame, nullptr, 0, false);
    return 0;
}
