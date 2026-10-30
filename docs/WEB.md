# The browser build

The same game, compiled with Emscripten onto Strata's WebGL 2 backend. The page is `web/shell.html`, and the game is
`app/web_main.cpp` driving the same `play::Session` as the desktop.

## Building

You need emsdk 6.0.11 (Strata's pin) and CMake 3.24 or newer.

```sh
source /path/to/emsdk/emsdk_env.sh
emcmake cmake -S . -B build-web -DCMAKE_BUILD_TYPE=Release -DSISYPHUS_WITH_STRATA=OFF -DSISYPHUS_WEB=ON
cmake --build build-web -j
ctest --test-dir build-web          # the test suite as wasm, under Node: the same pinned fingerprint
mkdir -p web-site && cp build-web/sisyphus.html web-site/index.html && cp build-web/sisyphus.js build-web/sisyphus.wasm web-site/
python3 -m http.server 8080 --directory web-site
```

## How it differs from the desktop

| | Desktop | Browser |
|---|---|---|
| Mouse look | the game takes the mouse | the page owns the pointer lock (click to play, Esc frees it) and calls `sisyphus_look` |
| Touch | | a stick on the left half, look on the right half; **Patch**, **Retire** and **Refactor** are held (`sisyphus_hold`), **Sit**, **Camera** and **History** pressed (`sisyphus_press`), **Sound** toggled (`sisyphus_sound`) |
| HUD | drawn on the canvas | on a page narrower than 760 CSS pixels, the page draws the HUD, the prompt and the words itself, from `sisyphus_status()` |
| Saves | `~/.local/share/sisyphus/default` or `%APPDATA%\Sisyphus\default` | `/sisyphus-save/default` on IndexedDB (IDBFS), loaded before `main()`, flushed after every save and when the page is hidden |
| Quit | Esc twice | none |

Saves made in a browser stay in that browser on that device. They are never sent anywhere.

## Smoke test

`web/smoke/smoke.cjs` runs the built page in headless Chromium (Playwright) twice, on a desktop profile and on an
iPhone 13 profile. Each run pushes the boulder, patches a module, looks around, sits and stands, then saves and reloads.

```sh
cd web/smoke && npm ci && npx playwright install chromium && cd ../..
node web/smoke/smoke.cjs http://127.0.0.1:8080/ smoke-out/desktop
node web/smoke/smoke.cjs http://127.0.0.1:8080/ smoke-out/phone --phone
```
