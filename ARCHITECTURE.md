# Architecture

```
src/sim      the rules: the hill, the boulder, the modules and their rot, the autopilot   (no engine)
src/bridge   geometry.*: the hill, town, boulder and Sisyphus as triangle meshes          (no engine)
             scene.*: uploads them to Strata and keeps them in step with the world
src/play     view.*: sky, lights, camera, HUD; session.*: input, news, autosave
src/save     the World as a checksummed file, the previous save kept
app/         main.cpp (desktop: play, watch the autopilot, headless playthrough), web_main.cpp (browser)
web/         the page around the browser build, and its smoke test
```

## The rules (`src/sim`)

- **Fixed step, deterministic.** `tick(World&, const Input&)` advances 1/30 s. The same seed and inputs give the same
  world, bit for bit, on every compiler. The sim is compiled without fused multiply-adds (`-ffp-contract=off`,
  `/fp:precise`) and uses `detSin`/`detCos`/`detAtan2` from `math.h` rather than libm's trigonometry. Randomness is
  splitmix64 (`rng.h`), and its state is part of the World and of the save.
- **The hill** (`hill.*`) is a path climbing towards -z in eight terraces 36 m apart. Each terrace has a 10 m level
  part and then a 26 m slope that rises 9 m. Terrace 0 is the trailhead, terrace 7 the summit. The hillside either
  side rises into ridges. Below the trailhead is the valley and its town.
- **The boulder** is a position along the path (`s`), a speed and a roll angle. Its radius grows with the number of
  active modules. Pushing (close behind it, facing up the hill, walking forward) drives it towards
  `kPushSpeed * (1 - load)`, which is easier on level ground. Left on a slope it rolls back until a terrace stops it.
  At `load >= 1` it slides back even while pushed, though on level ground it can always be inched along. From
  above, on level ground, it can be pushed back to turn it.
- **Modules** sit at fixed points on the sphere: a Fibonacci sphere visited 29 points at a time, so a young boulder's
  few modules are spread out. Each face of the boulder belongs to the nearest active module. A module has a health
  (1 fresh, 0 down), an issue, and the release it shipped in. It rots slowly on its own, faster when it is old, and
  fast with an issue. Incidents arrive at a rate that grows with the number of modules and the major version. An
  advisory left to rot spreads to its nearest neighbour.
- **Weight**: `load = 0.25 + 0.01 * modules + 0.8 * (1 - mean health) + 0.15 * modules down`.
- **Releases**: the first time the boulder reaches each terrace in a major version, a release ships. It adds users
  and a new module (until all 48 have shipped). The summit ships the next major. After `kSummitPause` seconds the
  boulder rolls all the way back to the trailhead, and the major version goes up.
- **Work**: holding E on the module under the crosshair (within `kReach`) patches it in `patchSeconds`. Holding R
  retires it, down to `kMinModules`.
- **The autopilot** (`autopilot.*`) plays by simple rules. On a terrace it patches the worst module it can reach.
  It pushes on when the load is low, and rolls the boulder to bring modules out from underneath. When the boulder
  is too heavy and nothing can be patched, it retires a module. `test_autopilot` and the headless playthrough pin
  where its first major version ends (`kPlaythroughFingerprint`). A change to the rules or the autopilot moves the
  fingerprint: update it in the same change, and say why.

## Rendering (`src/bridge`, `src/play`)

The terrain is one Strata terrain tile sampled from `hill::groundHeight`. The town, the stones and the terrace markers
are static meshes. The markers are rebuilt when a release ships.

The boulder is a 1280-face icosphere built around the origin and placed with `strata_set_mesh_transform`: scaled to
its radius, rolled about x, and set on the path. It is rebuilt only when a module's colour changes: its health in
sixteenths, its issue, or a module added or retired (`boulderKey`). Modules in trouble glow as soft points that pulse
faster the worse they get. They are also point lights, so they can be worked on at night. The town's windows light
up as users come.

The sun is placed by the time of day: up for three quarters of a 12-minute day, behind you as you climb. The HUD is
Strata's 8x8 text.

## Saves (`src/save`)

One file per slot. It starts with the magic `SISYPHUS`, then the format version, the payload length, the payload
(every World field, little-endian) and a SHA-256 of the payload. The checksum is checked before any field is read.
Every read is bounds-checked, and a decoded world must be plausible (finite numbers, the boulder on the path, the
modules consistent) before the rules may run on it. Writes go to a temporary file that is renamed into place. The
save it replaces is kept as the previous save, and is loaded instead if the current one is damaged.
`test_saves` flips every byte of a save and expects each one to be refused. The format is Kindling's and Atman's,
with Sisyphus's World.

## The browser

The same session runs in the browser (`app/web_main.cpp`) on Strata's WebGL 2 backend; see [docs/WEB.md](docs/WEB.md).
