// Writes save fixtures in tests/fixtures/ for the current save format by
// playing the autopilot on seed 1 with the current rules and saving through
// the game's own writeSave (the same call the game makes on F5, on quit and
// every minute). Nothing is written by hand. See tests/fixtures/README.md.
//
// Format 1 (0.1.0):
//   v1-first-climb.save   on the way up, after the third release (0.3)
//   v1-after-summit.save  back at the trailhead after 1.0, the next major begun
// Format 4 (0.2.0), with stamina and cairns:
//   v4-after-summit.save  back at the trailhead after 1.0, its cairn raised
//   v4-second-climb.save  on the way up again, after the tenth release
// Format 7 (0.3.0), with helpers, rewrites and weather: the same two, as v7-.
#include <cinttypes>
#include <cstdio>
#include <filesystem>
#include <string>

#include "save/save.h"
#include "sim/autopilot.h"
#include "sim/world.h"

using namespace sisyphus;
namespace fs = std::filesystem;

namespace {

bool save(const World& w, const fs::path& out, const fs::path& scratch) {
    std::error_code ec;
    fs::remove_all(scratch, ec);
    std::string why;
    if (!writeSave(scratch.string(), w, &why)) {
        std::fprintf(stderr, "writeSave failed: %s\n", why.c_str());
        return false;
    }
    fs::copy_file(savePath(scratch.string()), out, fs::copy_options::overwrite_existing, ec);
    if (ec) {
        std::fprintf(stderr, "copy to %s failed: %s\n", out.string().c_str(), ec.message().c_str());
        return false;
    }
    std::printf("%s  tick %" PRId64 "  version %s  modules %d  fingerprint %016" PRIx64 "\n", out.filename().string().c_str(),
                w.tick, versionText(w).c_str(), activeModules(w), fingerprint(w));
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: make_save_fixtures <tests/fixtures directory>\n");
        return 2;
    }
    const fs::path dir = argv[1];
    const fs::path scratch = fs::temp_directory_path() / "sisyphus_make_save_fixtures";
    const int64_t limit = 60LL * 60 * kTickRate;
    const std::string v = "v" + std::to_string(kSaveVersion) + "-";
    World w = newWorld(1);
    Autopilot pilot;
    if (kSaveVersion == 1) {
        while (w.releases < 3 && w.tick < limit) tick(w, pilot.next(w));
        if (w.releases < 3 || !save(w, dir / (v + "first-climb.save"), scratch)) return 1;
    }
    while (!playthroughDone(w) && w.tick < limit) tick(w, pilot.next(w));
    if (!playthroughDone(w) || !save(w, dir / (v + "after-summit.save"), scratch)) return 1;
    if (kSaveVersion >= 4) {
        while (w.releases < 10 && w.tick < limit) tick(w, pilot.next(w));
        if (w.releases < 10 || !save(w, dir / (v + "second-climb.save"), scratch)) return 1;
    }
    std::error_code ec;
    fs::remove_all(scratch, ec);
    return 0;
}
