#include "save/save.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <type_traits>

#include "save/sha256.h"
#include "sim/hill.h"

namespace fs = std::filesystem;

namespace sisyphus {

namespace {

constexpr char kMagic[8] = {'S', 'I', 'S', 'Y', 'P', 'H', 'U', 'S'};

// Writes little-endian.
struct Writer {
    std::vector<uint8_t> out;
    void raw(uint64_t v, int bytes) {
        for (int i = 0; i < bytes; ++i) out.push_back(static_cast<uint8_t>(v >> (8 * i)));
    }
    template <class T>
    void value(const T& v) {
        if constexpr (std::is_same_v<T, bool>) {
            raw(v ? 1 : 0, 1);
        } else if constexpr (std::is_same_v<T, float>) {
            uint32_t bits;
            std::memcpy(&bits, &v, 4);
            raw(bits, 4);
        } else if constexpr (std::is_enum_v<T>) {
            raw(static_cast<uint64_t>(static_cast<int64_t>(v)), 4);
        } else {
            raw(static_cast<uint64_t>(v), static_cast<int>(sizeof(T)));
        }
    }
};

// Reads little-endian, refusing to read past the end.
struct Reader {
    const uint8_t* p;
    size_t n, at = 0;
    bool ok = true;
    uint64_t raw(int bytes) {
        if (!ok || n - at < static_cast<size_t>(bytes)) {
            ok = false;
            return 0;
        }
        uint64_t v = 0;
        for (int i = 0; i < bytes; ++i) v |= static_cast<uint64_t>(p[at + i]) << (8 * i);
        at += static_cast<size_t>(bytes);
        return v;
    }
    template <class T>
    void value(T& v) {
        if constexpr (std::is_same_v<T, bool>) {
            const uint64_t b = raw(1);
            if (b > 1) ok = false;
            v = b != 0;
        } else if constexpr (std::is_same_v<T, float>) {
            const auto bits = static_cast<uint32_t>(raw(4));
            std::memcpy(&v, &bits, 4);
        } else if constexpr (std::is_enum_v<T>) {
            v = static_cast<T>(static_cast<int32_t>(raw(4)));
        } else {
            v = static_cast<T>(raw(static_cast<int>(sizeof(T))));
        }
    }
};

// One description of the save's fields, used both ways. `version` is the
// format being read; writing is always kSaveVersion.
template <class IO>
struct Fields {
    IO& io;
    uint32_t version = kSaveVersion;

    template <class T>
    void v(T& x) { io.value(x); }
    void world(World& w) {
        v(w.seed);
        v(w.rng);
        v(w.tick);
        v(w.player.x);
        v(w.player.z);
        v(w.player.yaw);
        v(w.player.pitch);
        v(w.player.sitting);
        v(w.boulder.s);
        v(w.boulder.v);
        v(w.boulder.roll);
        for (Module& m : w.modules) {
            v(m.active);
            v(m.health);
            v(m.issue);
            v(m.born);
        }
        v(w.added);
        v(w.major);
        v(w.best);
        v(w.releases);
        v(w.users);
        v(w.summitSeconds);
        v(w.falling);
        v(w.pushing);
        v(w.pushingBack);
        v(w.slipping);
        v(w.work.module);
        v(w.work.seconds);
        v(w.work.retiring);
        v(w.incidents);
        v(w.patched);
        v(w.retired);
        v(w.breakdowns);
        v(w.lastIncident);
        v(w.lastIssue);
        v(w.satSeconds);
        if (version >= 2) v(w.player.stamina);
        if (version >= 3) {
            v(w.work.refactoring);
            v(w.refactored);
            v(w.refactoredInto);
        }
        if (version >= 4) {
            v(w.cairnCount);
            for (Cairn& c : w.cairns) {
                v(c.major);
                v(c.tick);
                v(c.users);
                v(c.modules);
                v(c.patched);
                v(c.retired);
                v(c.refactored);
                v(c.breakdowns);
                if (version >= 5) v(c.helpers);
            }
        }
        if (version >= 5) {
            Helper& h = w.helper;
            v(h.name);
            v(h.x);
            v(h.z);
            v(h.secondsLeft);
            v(h.module);
            v(h.work);
            for (int32_t& n : h.patches) v(n);
            v(w.helpersThisClimb);
            v(w.busFactor);
        }
    }
};

// What a decoded world must look like before the rules may run on it.
bool plausible(const World& w, std::string* error) {
    auto fail = [error](const char* why) {
        if (error) *error = why;
        return false;
    };
    const float floats[] = {w.player.x, w.player.z, w.player.yaw, w.player.pitch, w.boulder.s, w.boulder.v,
                            w.boulder.roll, w.users, w.summitSeconds, w.work.seconds, w.satSeconds,
                            w.player.stamina};
    for (float f : floats)
        if (!std::isfinite(f)) return fail("a number in the save is not finite");
    if (w.tick < 0) return fail("the clock is impossible");
    if (w.player.stamina < 0 || w.player.stamina > 1) return fail("Sisyphus's stamina is out of range");
    if (std::fabs(w.player.x) > hill::kWalkHalf + 1 || hill::sOf(w.player.z) < hill::kValleyS ||
        hill::sOf(w.player.z) > hill::kTopS + 1)
        return fail("Sisyphus is off the hill");
    if (w.boulder.s < hill::kRestS - 0.01f || w.boulder.s > hill::terraceStart(hill::kSummit) + 8.01f)
        return fail("the boulder is off the path");
    if (w.added < kStartModules || w.added > kMaxModules) return fail("an impossible number of modules");
    int active = 0;
    for (int i = 0; i < kMaxModules; ++i) {
        const Module& m = w.modules[static_cast<size_t>(i)];
        const int issue = static_cast<int>(m.issue);
        if (issue < 0 || issue >= kIssueKinds) return fail("an unknown kind of issue");
        if (!(m.health >= 0 && m.health <= 1)) return fail("a module's health is out of range");
        if (m.active && i >= w.added) return fail("a module that was never added");
        active += m.active ? 1 : 0;
    }
    if (active < kMinModules) return fail("too few modules");
    if (w.major < 0 || w.best < 0 || w.best > hill::kSummit || w.releases < 0 || w.users < 0)
        return fail("the release history is impossible");
    if (w.work.module < -1 || w.work.module >= kMaxModules || w.lastIncident < -1 || w.lastIncident >= kMaxModules)
        return fail("work on a module that does not exist");
    const int last = static_cast<int>(w.lastIssue);
    if (last < 0 || last >= kIssueKinds) return fail("an unknown kind of issue");
    if (w.falling && w.summitSeconds >= 0) return fail("both at the summit and rolling down");
    if (w.work.retiring && w.work.refactoring) return fail("retiring and refactoring at once");
    if (w.refactored < 0 || w.refactoredInto < -1 || w.refactoredInto >= kMaxModules)
        return fail("the refactoring history is impossible");
    if (w.cairnCount < 0 || w.cairnCount > kMaxCairns) return fail("an impossible number of cairns");
    for (int k = 0; k < w.cairnCount; ++k) {
        const Cairn& c = w.cairns[static_cast<size_t>(k)];
        const Cairn* before = k > 0 ? &w.cairns[static_cast<size_t>(k - 1)] : nullptr;
        if (!std::isfinite(c.users) || c.users < 0 || c.major < 1 || c.major > w.major || c.tick < 0 || c.tick > w.tick ||
            c.modules < kMinModules || c.modules > kMaxModules || c.patched < 0 || c.retired < 0 || c.refactored < 0 ||
            c.breakdowns < 0 || (before && (c.tick < before->tick || c.major <= before->major)))
            return fail("a cairn's record is impossible");
        if (c.helpers < 0) return fail("a cairn's record is impossible");
    }
    const Helper& h = w.helper;
    if (h.name < -1 || h.name >= kHelperNames || w.helpersThisClimb < 0 || w.busFactor < -1 || w.busFactor >= kMaxModules)
        return fail("an impossible helper");
    if (h.name >= 0) {
        if (!std::isfinite(h.x) || !std::isfinite(h.z) || !std::isfinite(h.secondsLeft) || !std::isfinite(h.work) ||
            h.secondsLeft < 0 || h.secondsLeft > kHelperStays || h.work < 0 || h.module < -1 || h.module >= kMaxModules ||
            std::fabs(h.x) > hill::kWalkHalf + 1 || hill::sOf(h.z) < hill::kValleyS - 1 || hill::sOf(h.z) > hill::kTopS + 10)
            return fail("an impossible helper");
        for (int32_t n : h.patches)
            if (n < 0) return fail("an impossible helper");
    }
    return true;
}

bool readFile(const std::string& path, std::vector<uint8_t>& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    out.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    return true;
}

}  // namespace

std::vector<uint8_t> encodeSave(const World& w) {
    Writer payload;
    Fields<Writer> fields{payload};
    fields.world(const_cast<World&>(w));  // the Writer only reads

    Writer file;
    file.out.insert(file.out.end(), kMagic, kMagic + 8);
    file.value(kSaveVersion);
    file.value(static_cast<uint64_t>(payload.out.size()));
    file.out.insert(file.out.end(), payload.out.begin(), payload.out.end());
    const std::array<uint8_t, 32> digest = sha256(payload.out.data(), payload.out.size());
    file.out.insert(file.out.end(), digest.begin(), digest.end());
    return file.out;
}

bool decodeSave(const std::vector<uint8_t>& bytes, World& out, std::string* error) {
    auto fail = [error](const std::string& why) {
        if (error) *error = why;
        return false;
    };
    if (bytes.size() < 8 + 4 + 8 + 32 || std::memcmp(bytes.data(), kMagic, 8) != 0) return fail("not an Sisyphus save");
    Reader head{bytes.data(), bytes.size()};
    head.at = 8;
    uint32_t version = 0;
    uint64_t length = 0;
    head.value(version);
    head.value(length);
    if (version < 1 || version > kSaveVersion)
        return fail("saved by a different version of Sisyphus (format " + std::to_string(version) + ")");
    if (length != bytes.size() - 8 - 4 - 8 - 32) return fail("the save is truncated or has extra bytes");
    const uint8_t* payload = bytes.data() + 20;
    const std::array<uint8_t, 32> digest = sha256(payload, static_cast<size_t>(length));
    if (std::memcmp(digest.data(), payload + length, 32) != 0) return fail("the save is damaged (its checksum does not match)");

    Reader r{payload, static_cast<size_t>(length)};
    Fields<Reader> fields{r, version};
    World w;
    fields.world(w);
    if (!r.ok || r.at != r.n) return fail("the save's contents do not parse");
    if (!plausible(w, error)) return false;
    out = std::move(w);
    return true;
}

std::string savePath(const std::string& slot) { return (fs::path(slot) / "sisyphus.save").string(); }
std::string previousSavePath(const std::string& slot) { return (fs::path(slot) / "sisyphus.save.prev").string(); }

bool writeSave(const std::string& slot, const World& w, std::string* error) {
    std::error_code ec;
    fs::create_directories(slot, ec);
    const std::vector<uint8_t> bytes = encodeSave(w);
    const fs::path tmp = fs::path(slot) / "sisyphus.save.tmp";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        f.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        f.flush();
        if (!f) {
            if (error) *error = "could not write " + tmp.string();
            return false;
        }
    }
    // Keep what was there as the previous save, then move the new one in. A
    // damaged save never replaces a good previous one: it is discarded instead.
    const fs::path cur = savePath(slot), prev = previousSavePath(slot);
    if (fs::exists(cur, ec)) {
        std::vector<uint8_t> old;
        World check;
        if (readFile(cur.string(), old) && decodeSave(old, check, nullptr)) {
            fs::remove(prev, ec);
            fs::rename(cur, prev, ec);
            if (ec) {
                if (error) *error = "could not keep the previous save: " + ec.message();
                return false;
            }
        } else {
            fs::remove(cur, ec);
        }
    }
    fs::rename(tmp, cur, ec);
    if (ec) {
        if (error) *error = "could not move the new save into place: " + ec.message();
        return false;
    }
    return true;
}

LoadResult loadSave(const std::string& slot, World& out) {
    LoadResult r;
    std::vector<uint8_t> bytes;
    std::string why;
    const bool haveCurrent = readFile(savePath(slot), bytes);
    if (haveCurrent && decodeSave(bytes, out, &why)) {
        r.ok = true;
        r.message = "Loaded your save.";
        return r;
    }
    std::string whyPrev;
    if (readFile(previousSavePath(slot), bytes) && decodeSave(bytes, out, &whyPrev)) {
        r.ok = true;
        r.fromPrevious = true;
        r.message = haveCurrent ? "Your latest save was refused (" + why + "). Loaded the one before it."
                                : "Loaded your previous save.";
        return r;
    }
    r.message = haveCurrent ? "Your save was refused (" + why + ") and there is no good earlier save."
                            : "No save to load.";
    return r;
}

bool hasSave(const std::string& slot) {
    std::error_code ec;
    return fs::exists(savePath(slot), ec) || fs::exists(previousSavePath(slot), ec);
}

std::string defaultSlot() {
#ifdef _WIN32
    char* appData = nullptr;
    size_t len = 0;
    std::string base = ".";
    if (_dupenv_s(&appData, &len, "APPDATA") == 0 && appData) base = appData;
    std::free(appData);
    return (fs::path(base) / "Sisyphus" / "default").string();
#else
    const char* xdg = std::getenv("XDG_DATA_HOME");
    if (xdg && *xdg) return (fs::path(xdg) / "sisyphus" / "default").string();
    const char* home = std::getenv("HOME");
    return (fs::path(home ? home : ".") / ".local" / "share" / "sisyphus" / "default").string();
#endif
}

}  // namespace sisyphus
