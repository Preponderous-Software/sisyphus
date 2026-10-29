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
    CHECK(w.player.stamina < 1 && back.player.stamina == w.player.stamina);  // format 2
    {
        World r = w;  // format 3
        r.refactored = 4;
        r.refactoredInto = 2;
        World rb;
        CHECK(decodeSave(encodeSave(r), rb, &why));
        CHECK(rb.refactored == 4 && rb.refactoredInto == 2 && fingerprint(rb) == fingerprint(r));
    }
    {
        World c = played(400);  // format 4: the cairns
        CHECK(c.cairnCount >= 1);
        World cb;
        CHECK(decodeSave(encodeSave(c), cb, &why));
        CHECK(cb.cairnCount == c.cairnCount && cb.cairns[0].major == c.cairns[0].major && fingerprint(cb) == fingerprint(c));
        World odd = c;
        odd.cairnCount = kMaxCairns + 1;
        CHECK(!decodeSave(encodeSave(odd), cb, &why));
        World hp = c;  // format 5: a helper
        hp.helper.name = 5;
        hp.helper.x = 1;
        hp.helper.z = c.player.z;
        hp.helper.secondsLeft = 60;
        hp.helper.patches[2] = 3;
        hp.helpersThisClimb = 1;
        CHECK(decodeSave(encodeSave(hp), cb, &why));
        CHECK(cb.helper.name == 5 && cb.helper.patches[2] == 3 && fingerprint(cb) == fingerprint(hp));
        hp.helper.name = kHelperNames;
        CHECK(!decodeSave(encodeSave(hp), cb, &why));
        odd = c;
        odd.cairns[0].tick = c.tick + 1;  // from the future
        CHECK(!decodeSave(encodeSave(odd), cb, &why));
    }
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
        World both = w;
        both.work.retiring = both.work.refactoring = true;
        CHECK(!decodeSave(encodeSave(both), x, &why));
        World spent = w;
        spent.player.stamina = 1.5f;
        CHECK(!decodeSave(encodeSave(spent), x, &why));
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
    // written by 0.1.0 and 0.2.0 (tools/make_save_fixtures.cpp; tests/fixtures/README.md).
    // A fixture is never edited; a new save format gets fixtures of its own.
    struct Fixture {
        const char* name;
        uint64_t fingerprint;
        const char* version;
        int modules;
        int cairns;  // format 1 had none, and loads rested
    };
    const Fixture fixtures[] = {
        {"v1-first-climb.save", 0xaa131e98cf4f4146ull, "0.3", 9, 0},
        {"v1-after-summit.save", 0xb0a2e0bdfaae817full, "1.0", 13, 0},
        {"v4-after-summit.save", 0xa3efaf36e40ffa97ull, "1.0", 13, 1},
        {"v4-second-climb.save", 0x63e8b136888800c6ull, "1.3", 16, 1},
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
        CHECK(x.cairnCount == f.cairns);
        if (f.cairns == 0) CHECK(x.player.stamina == 1);  // format 1 had no stamina: he loads rested
        else CHECK(x.player.stamina < 1);
        // ... and the game plays on from it.
        Autopilot a;
        for (int t = 0; t < 30 * kTickRate; ++t) tick(x, a.next(x));
        CHECK(x.tick > 0);
    }
    TEST_MAIN_END();
}
