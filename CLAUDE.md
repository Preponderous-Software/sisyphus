# CLAUDE.md

Guidance for Claude Code sessions in this repository.

## Project overview

Sisyphus is a first-person game about software upkeep, written in C++17 on the
[Strata](https://github.com/Preponderous-Software/strata) engine. The boulder is the software. Each terrace you push it
to ships a release, each release adds a module, every module rots, and at the summit it all rolls back down. It runs
on the desktop and, through Emscripten and WebGL 2, in the browser at https://sisyphus.play.danielstephenson.dev.
Every push to `main` redeploys the browser version. See README.md for the game and ARCHITECTURE.md for how it is
built.

## Build and test

The game needs CMake 3.24 or newer; Strata is fetched at the pinned `SISYPHUS_STRATA_REF`. The rules and tests alone need no engine.

```sh
cmake -S . -B build-sim -DSISYPHUS_WITH_STRATA=OFF -DSISYPHUS_WARNINGS_AS_ERRORS=ON && cmake --build build-sim -j
ctest --test-dir build-sim --output-on-failure          # the anchor: run this before every PR
cmake -S . -B build -DSISYPHUS_WARNINGS_AS_ERRORS=ON && cmake --build build -j --target sisyphus
LIBGL_ALWAYS_SOFTWARE=1 GALLIUM_DRIVER=llvmpipe ./build/sisyphus --headless --frames frames   # headless playthrough
```

docs/WEB.md covers the browser build and its Playwright smoke test. CI runs everything: on Linux, the rules and tests
and the game's headless playthrough; on Windows, the build and tests with MSVC; and the browser build with its smoke
test on desktop and phone profiles.

## Conventions that matter

- **Determinism is load-bearing.** `src/sim` must give bit-identical results on GCC, Clang, MSVC and Emscripten:
  - No libm trigonometry: use `detSin`, `detCos` and `detAtan2` from `src/sim/math.h`.
  - No fused multiply-adds: the sim is built with `-ffp-contract=off` or `/fp:precise`.
  - Randomness comes only from the World's splitmix64 state.
  - No unordered containers or wall-clock time in the rules.
- **The pinned fingerprint.** `kPlaythroughFingerprint` in `src/sim/autopilot.h` pins where the autopilot's first
  major version ends. Any change to the rules, the hill or the autopilot moves it.
  - Update it in the same PR, and say in the PR why it moved. `test_autopilot` and `sisyphus --headless` both print
    the new value.
  - Never update it to paper over a change you can't explain.
- **Saves belong to the players.** Changing `World` means changing `Fields::world` and `plausible` in
  `src/save/save.cpp`.
  - If the byte layout changes, bump `kSaveVersion` and keep older versions loading: a save that loaded before must
    still load.
  - `test_saves` must keep refusing every flipped byte.
- **Engine-free layers stay engine-free.** `src/sim` and `src/bridge/geometry.*` do not include Strata. Only
  `src/bridge/scene.*`, `src/play` and `app/` do.
- **Strata traps:**
  - `strata::cameraLookAt` takes the field of view in degrees.
  - Mesh transforms are column-major.
  - Strata's text is an 8x8 font, so a glyph is as wide as its pixel size.
- **The browser page** (`web/shell.html`) talks to the game only through the `sisyphus_*` functions exported in
  `app/web_main.cpp`, which are also listed in `CMakeLists.txt`. Add a new one in all three places, and keep the smoke
  test passing.
- **Writing.** Use plain, concrete English in docs, comments and in-game text. The game's one quotation is Camus's
  line, with its attribution. Keep all other writing original.

## Pull requests

One coherent change per PR, squash-merged once every check is green. Commit messages and PR bodies end with the
attribution lines the session provides.
