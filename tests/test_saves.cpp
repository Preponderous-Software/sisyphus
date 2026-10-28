// Saves: a world survives the round trip exactly; a damaged, truncated or
// foreign file is refused; a damaged save falls back to the previous one.
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include "check.h"
#include "save/save.h"
#include "sim/autopilot.h"

using namespace sisyphus;
namespace fs = std::filesystem;

namespace {
World played(int seconds) {
    World w = newWorld(5);
    Autopilot a;
    for (int t = 0; t < seconds * kTickRate; ++t) tick(w, a.next(w));
    return w;
}
}  // namespace

int main(int argc, char** argv) {
    const World w = played(200);
    const std::vector<uint8_t> bytes = encodeSave(w);
    World back;
    std::string why;
    CHECK(decodeSave(bytes, back, &why));
    CHECK(fingerprint(back) == fingerprint(w));
    // ... and plays on the same.
    {
        World a = w, b = back;
        Autopilot pa, pb;
        for (int t = 0; t < 60 * kTickRate; ++t) {
            tick(a, pa.next(a));
            tick(b, pb.next(b));
        }
        CHECK(fingerprint(a) == fingerprint(b));
    }
    // Every single flipped byte is refused.
    int accepted = 0;
    for (size_t i = 0; i < bytes.size(); ++i) {
        std::vector<uint8_t> bad = bytes;
        bad[i] ^= 0x5A;
        World x;
        if (decodeSave(bad, x, nullptr)) ++accepted;
    }
    CHECK(accepted == 0);
    // Truncated, extended, or not a save at all.
    {
        World x;
        std::vector<uint8_t> shorter(bytes.begin(), bytes.end() - 1);
        CHECK(!decodeSave(shorter, x, nullptr));
        std::vector<uint8_t> longer = bytes;
        longer.push_back(0);
        CHECK(!decodeSave(longer, x, nullptr));
        CHECK(!decodeSave(std::vector<uint8_t>(100, 7), x, &why));
    }
    // An impossible world, even with a good checksum, is refused.
    {
        World odd = w;
        odd.boulder.s = 9999;
        World x;
        CHECK(!decodeSave(encodeSave(odd), x, &why));
    }
    // Files: the save before last is kept, and loaded when the latest is damaged.
    const std::string slot = (fs::path(argc > 1 ? argv[1] : ".") / "test-slot").string();
    fs::remove_all(slot);
    const World first = played(30);
    CHECK(writeSave(slot, first, &why));
    CHECK(writeSave(slot, w, &why));
    CHECK(hasSave(slot));
    World loaded;
    LoadResult r = loadSave(slot, loaded);
    CHECK(r.ok && !r.fromPrevious);
    CHECK(fingerprint(loaded) == fingerprint(w));
    {
        std::fstream f(savePath(slot), std::ios::in | std::ios::out | std::ios::binary);
        f.seekp(40);
        f.put('\x7f');
    }
    r = loadSave(slot, loaded);
    CHECK(r.ok && r.fromPrevious);
    CHECK(fingerprint(loaded) == fingerprint(first));
    // A damaged save never replaces a good previous one.
    CHECK(writeSave(slot, w, &why));
    r = loadSave(slot, loaded);
    CHECK(r.ok && !r.fromPrevious && fingerprint(loaded) == fingerprint(w));
    fs::remove_all(slot);

    // Saves written by released builds still load, exactly: the fixtures were
    // written by 0.1.0 (tools/make_save_fixtures.cpp; tests/fixtures/README.md).
    // A fixture is never edited; a new save format gets fixtures of its own.
    struct Fixture {
        const char* name;
        uint64_t fingerprint;
        const char* version;
        int modules;
    };
    const Fixture fixtures[] = {
        {"v1-first-climb.save", 0xaa131e98cf4f4146ull, "0.3", 9},
        {"v1-after-summit.save", 0xb0a2e0bdfaae817full, "1.0", 13},
    };
    for (const Fixture& f : fixtures) {
        std::ifstream in(fs::path(SISYPHUS_FIXTURES) / f.name, std::ios::binary);
        const std::vector<uint8_t> saved((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        World x;
        const bool ok = decodeSave(saved, x, &why);
        if (!ok) std::fprintf(stderr, "%s: %s\n", f.name, why.c_str());
        CHECK(ok);
        CHECK(fingerprint(x) == f.fingerprint);
        CHECK(versionText(x) == f.version);
        CHECK(activeModules(x) == f.modules);
        // ... and the game plays on from it.
        Autopilot a;
        for (int t = 0; t < 30 * kTickRate; ++t) tick(x, a.next(x));
        CHECK(x.tick > 0);
    }
    TEST_MAIN_END();
}
