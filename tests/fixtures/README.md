# Save fixtures

Real saves written by deployed builds of Sisyphus. `tests/test_saves.cpp` loads each one by name and checks it
exactly (its fingerprint, version and module count), so a change that stops a player's existing save from loading
fails the tests. A fixture is never edited or regenerated once it is committed: a new save format gets new fixtures
of its own.

Every save format needs a `v<N>-*.save` fixture, including one that never reached a tag: every push to `main` deploys
the browser build, so players can hold saves in formats no release wrote, and the release's `fixtures-complete` check
refuses a candidate without them.

| File | Format | Written by | State |
|---|---|---|---|
| `v1-first-climb.save` | 1 | 0.1.0, tag `0.1.0` (`fefce4aa08b91c7231c3fea5b254f14d5419c3b1`) | tick 2773, on the way up after the third release, 0.3, with 9 modules (fingerprint `aa131e98cf4f4146`) |
| `v1-after-summit.save` | 1 | 0.1.0, tag `0.1.0` (`fefce4aa08b91c7231c3fea5b254f14d5419c3b1`) | tick 9001, back at the trailhead after 1.0 shipped and the boulder rolled down, 13 modules (fingerprint `b0a2e0bdfaae817f`, the pinned playthrough's end) |
| `v2-after-summit.save` | 2 | `main` at `8fdcf8d09908fe915666a241833ef68bb4e5e3eb` (#11) | tick 9001, back at the trailhead after 1.0 shipped, 13 modules (fingerprint `b4a941683f19d955`, the pinned playthrough's end) |
| `v3-after-summit.save` | 3 | `main` at `f1d2bc1b2d5ad3e4d816c8a2095491abf3fa97c9` (#12) | tick 9001, back at the trailhead after 1.0 shipped, 13 modules (fingerprint `b4a941683f19d955`, the pinned playthrough's end) |
| `v4-after-summit.save` | 4 | 0.2.0, tag `0.2.0` (`57e2f819aaffe5a1b9de9f2a2505f2e25d558609`) | tick 8640, back at the trailhead after 1.0 shipped, its cairn raised, 13 modules, Sisyphus part spent (fingerprint `a3efaf36e40ffa97`, the pinned playthrough's end) |
| `v4-second-climb.save` | 4 | 0.2.0, tag `0.2.0` (`57e2f819aaffe5a1b9de9f2a2505f2e25d558609`) | tick 14879, on the way up again after the tenth release, 1.3, with 16 modules and one cairn (fingerprint `63e8b136888800c6`) |
| `v5-after-summit.save` | 5 | `main` at `7375908820dd52949cb2526f8c211e2a08615386` (#19) | tick 8640, back at the trailhead after 1.0 shipped, its cairn raised, 13 modules (fingerprint `01ebcfd568c044c2`, the pinned playthrough's end) |
| `v5-second-climb.save` | 5 | `main` at `7375908820dd52949cb2526f8c211e2a08615386` (#19) | tick 13998, on the way up again after the tenth release, 1.3, with 16 modules and one cairn (fingerprint `ae1ef90f4735fd71`) |
| `v6-after-summit.save` | 6 | `main` at `6f15708963d1f4bce5b243cbb89056c10feb8e16` (#20) | tick 8640, back at the trailhead after 1.0 shipped, its cairn raised, 13 modules (fingerprint `01ebcfd568c044c2`, the pinned playthrough's end) |
| `v6-second-climb.save` | 6 | `main` at `6f15708963d1f4bce5b243cbb89056c10feb8e16` (#20) | tick 13998, on the way up again after the tenth release, 1.3, with 16 modules and one cairn (fingerprint `ae1ef90f4735fd71`) |
| `v7-after-summit.save` | 7 | 0.3.0, tag `0.3.0` (`6954b5d6edea823f8e51e0ead2bf791e477c6bb5`) | tick 8527, back at the trailhead after 1.0 shipped, its cairn raised, 13 modules (fingerprint `d84c7b4cb80b963b`, the pinned playthrough's end) |
| `v7-second-climb.save` | 7 | 0.3.0, tag `0.3.0` (`6954b5d6edea823f8e51e0ead2bf791e477c6bb5`) | tick 14191, on the way up again after the tenth release, 1.3, with 16 modules and one cairn (fingerprint `cacdbd88b81446d3`) |

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

The format-4 fixtures were made the same way on 2026-10-28, with the tool as it is now (it names the files by the
save format and writes the second climb from format 4) built against the checkout of the `0.2.0` tag's `src/`:

```bash
git worktree add ../at-0.2.0 0.2.0
g++ -std=c++17 -O2 -ffp-contract=off -I../at-0.2.0/src tools/make_save_fixtures.cpp \
    ../at-0.2.0/src/sim/*.cpp ../at-0.2.0/src/save/*.cpp -o make_save_fixtures
./make_save_fixtures tests/fixtures
```

```
3c76701340aaab41379a69b6948ce0bba1de8879555b3d90376ea2766636e084  v4-after-summit.save
5ae75b6b172d7cbc55d2041b9af431e006c764cb9903ff275b8b7d88f474b897  v4-second-climb.save
```

The format-7 fixtures were made on 2026-10-28 the same way, with the tool and `src/` both from the `0.3.0` tag:

```bash
git worktree add ../at-0.3.0 0.3.0
g++ -std=c++17 -O2 -ffp-contract=off -I../at-0.3.0/src ../at-0.3.0/tools/make_save_fixtures.cpp \
    ../at-0.3.0/src/sim/*.cpp ../at-0.3.0/src/save/*.cpp -o make_save_fixtures
./make_save_fixtures tests/fixtures
```

```
456780d5e01ec5713da578d3463953a5722075a3e4188ec6ead8ec8e9d838e30  v7-after-summit.save
d434a9991d826324c0b07a282fb285f847f01148519cb8fe4cffe77c8a35f68e  v7-second-climb.save
```

Formats 2, 3, 5 and 6 never reached a tag. The
[`Save fixtures`](../../.github/workflows/save-fixtures.yml) workflow wrote theirs the same way on 2026-11-06, on a
runner (Ubuntu 24.04, GCC 13.3.0), with the tool built against the last commit on `main` at each format. It fails
unless the playthrough ends on that commit's own pinned fingerprint. The files here are the artifacts of run
[42180779434](https://github.com/Preponderous-Software/sisyphus/actions/runs/42180779434):

```
c128e9f6d007e1d38caba234fe5486e514b10eacaa757f73f4737fb0cbe02ae7  v2-after-summit.save
007cef219a38ca06a2615ea349d4cc55695ceef1e1922c0803c922458930cd3c  v3-after-summit.save
aa1732e0fe1ac4d823f1dd2decd3e1deff1bdbe8430b22fbc4bcbf5a6f2d8065  v5-after-summit.save
bee4a23e1c7b1d15d85238643c19e7184d4d789ed8b9d32e8314c838d842cd8d  v5-second-climb.save
8a79319f4e7d440c3bcb1b8095e2b7043d8e811d14ab4a34131d05a98bb62bac  v6-after-summit.save
afdfac5bcdabe33612ee99242b2c2b7df3dde68807236c4a44e6a3d375626301  v6-second-climb.save
```
