# Save fixtures

Real saves written by released builds of Sisyphus. `tests/test_saves.cpp` loads each one by name and checks it
exactly (its fingerprint, version and module count), so a change that stops a player's existing save from loading
fails the tests. A fixture is never edited or regenerated once it is committed: a new save format gets new fixtures
of its own.

| File | Format | Written by | State |
|---|---|---|---|
| `v1-first-climb.save` | 1 | 0.1.0, tag `0.1.0` (`fefce4aa08b91c7231c3fea5b254f14d5419c3b1`) | tick 2773, on the way up after the third release, 0.3, with 9 modules (fingerprint `aa131e98cf4f4146`) |
| `v1-after-summit.save` | 1 | 0.1.0, tag `0.1.0` (`fefce4aa08b91c7231c3fea5b254f14d5419c3b1`) | tick 9001, back at the trailhead after 1.0 shipped and the boulder rolled down, 13 modules (fingerprint `b0a2e0bdfaae817f`, the pinned playthrough's end) |

## How they were made

[`tools/make_save_fixtures.cpp`](../../tools/make_save_fixtures.cpp) plays the autopilot on seed 1 and saves through
the game's own `writeSave`, the call the game makes on F5, on quit and every minute. It was built against the
checkout of the `0.1.0` tag and run on 2026-10-28 (Ubuntu 20.04, GCC 9.4):

```bash
git checkout 0.1.0
g++ -std=c++17 -O2 -ffp-contract=off -Isrc tools/make_save_fixtures.cpp \
    src/sim/*.cpp src/save/*.cpp -o make_save_fixtures
./make_save_fixtures tests/fixtures
```

SHA-256 of the files as written:

```
5653d65e9afbfb8c88e48d3b45f73120f5ab6b6f99634e63ed47a4e8e32ad749  v1-first-climb.save
5e04cd4c3f927293f7a21d0fe4a79e2bc0a6ea62867b6d58468458ff8f6e57d5  v1-after-summit.save
```
