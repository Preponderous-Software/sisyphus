// Smoke test for the browser build in headless Chromium (Playwright).
//
//   node web/smoke/smoke.cjs <url> <out-dir> [--phone]
//
// Checks: the page starts with no page errors and shows How to play on a
// first visit; the canvas draws the hill (not one flat colour); walking into
// the boulder pushes it up the hill (W on a desktop, the stick on the phone
// profile); holding patch (E, or the Patch button) patches the module in
// reach; looking turns the view; sitting brings the words and standing ends
// them. Then a save survives a reload (IndexedDB): the boulder where it was,
// the patch counted. Writes PNGs to <out-dir>.
'use strict';
const fs = require('fs');
const path = require('path');
const zlib = require('zlib');
const { chromium, devices } = require('playwright');

const [url, outDir] = process.argv.slice(2);
const phone = process.argv.includes('--phone');
if (!url || !outDir) {
  console.error('usage: smoke.cjs <url> <out-dir> [--phone]');
  process.exit(2);
}
fs.mkdirSync(outDir, { recursive: true });

// Minimal PNG decoder (8-bit RGB/RGBA, non-interlaced: what Chromium writes).
function decodePng(buf) {
  let pos = 8, width = 0, height = 0, channels = 0;
  const idat = [];
  while (pos < buf.length) {
    const len = buf.readUInt32BE(pos);
    const type = buf.toString('ascii', pos + 4, pos + 8);
    const data = buf.subarray(pos + 8, pos + 8 + len);
    if (type === 'IHDR') {
      width = data.readUInt32BE(0);
      height = data.readUInt32BE(4);
      channels = { 2: 3, 6: 4 }[data[9]];
      if (data[8] !== 8 || !channels || data[12] !== 0) throw new Error('unsupported PNG');
    } else if (type === 'IDAT') idat.push(data);
    pos += 12 + len;
  }
  const raw = zlib.inflateSync(Buffer.concat(idat));
  const stride = width * channels;
  const out = Buffer.alloc(height * stride);
  for (let y = 0; y < height; y++) {
    const f = raw[y * (stride + 1)];
    for (let x = 0; x < stride; x++) {
      const v = raw[y * (stride + 1) + 1 + x];
      const a = x >= channels ? out[y * stride + x - channels] : 0;
      const b = y > 0 ? out[(y - 1) * stride + x] : 0;
      const c = x >= channels && y > 0 ? out[(y - 1) * stride + x - channels] : 0;
      let p;
      if (f === 0) p = 0;
      else if (f === 1) p = a;
      else if (f === 2) p = b;
      else if (f === 3) p = (a + b) >> 1;
      else {
        const pa = Math.abs(b - c), pb = Math.abs(a - c), pc = Math.abs(a + b - 2 * c);
        p = pa <= pb && pa <= pc ? a : pb <= pc ? b : c;
      }
      out[y * stride + x] = (v + p) & 255;
    }
  }
  return { width, height, channels, data: out };
}

// Distinct colours (quantised to 4 bits per channel) in a PNG.
function colourCount(png) {
  const img = decodePng(png);
  const seen = new Set();
  for (let i = 0; i < img.data.length; i += img.channels)
    seen.add(((img.data[i] >> 4) << 8) | ((img.data[i + 1] >> 4) << 4) | (img.data[i + 2] >> 4));
  return seen.size;
}

(async () => {
  const browser = await chromium.launch({ args: ['--use-angle=swiftshader', '--enable-unsafe-swiftshader'] });
  const base = phone ? devices['iPhone 13'] : { viewport: { width: 1280, height: 800 } };
  // "headless" in the UA keeps arcade from counting a smoke run as a play.
  const context = await browser.newContext({ ...base, userAgent: (base.userAgent || 'Mozilla/5.0') + ' sisyphus-smoke headless' });
  const page = await context.newPage();
  const errors = [];
  page.on('pageerror', (e) => errors.push(String(e)));
  page.on('console', (m) => { if (m.type() === 'error') errors.push('console: ' + m.text()); });
  const fail = async (why) => {
    console.error('FAIL: ' + why);
    await page.screenshot({ path: path.join(outDir, 'failure.png') }).catch(() => {});
    await browser.close();
    process.exit(1);
  };

  // The main loop keeps the load event from ever firing: wait for commit only.
  const open = async () => {
    await page.goto(url, { waitUntil: 'commit' });
    try {
      await page.waitForFunction(() => window.sisyphus && window.sisyphusStatus, null, { timeout: 90000 });
    } catch (e) {
      await fail('the game never became ready: ' + errors.join(' | '));
    }
  };
  await open();
  const status = async () => JSON.parse(await page.evaluate(() => window.sisyphus.status()));
  // Input is handled on the next frame, and SwiftShader frames are slow: poll.
  const until = async (pred, ms = 30000) => {
    const end = Date.now() + ms;
    let st = await status();
    while (!pred(st) && Date.now() < end) {
      await page.waitForTimeout(200);
      st = await status();
    }
    return st;
  };

  if (!(await page.locator('#help').isVisible())) await fail('How to play was not shown on a first visit');
  await page.locator('#helpClose').click();
  let s = await status();
  console.log(`ready: version ${s.version}, boulder at ${s.boulder}, Sisyphus at (${s.x}, ${s.z})`);
  if (s.version !== '0.0' || s.modules !== 6) await fail('a first visit should start at 0.0 with six modules');

  await page.waitForTimeout(1500);
  const shot = await page.screenshot({ path: path.join(outDir, 'sisyphus.png'), timeout: 180000 });
  const colours = colourCount(shot);
  console.log(`screenshot: ${colours} distinct colours`);
  if (colours < 12) await fail('the canvas looks blank (' + colours + ' colours)');

  // Walk into the boulder and push it up the hill: W on a desktop, the stick
  // held forward on the phone.
  const start = s;
  const box = await page.locator('#canvas').boundingBox();
  let cdp = null;
  const touch = (type, pts) => cdp.send('Input.dispatchTouchEvent', { type, touchPoints: pts });
  if (phone) {
    if (!(await page.locator('#panel').isVisible())) await fail('the HUD panel is not shown on a narrow page');
    const panel = await page.locator('#panel').innerText();
    if (!/SISYPHUS/.test(panel)) await fail('the HUD panel does not show the version: ' + panel);
    cdp = await context.newCDPSession(page);
    const sx = box.x + box.width * 0.2, sy = box.y + box.height * 0.6;
    await touch('touchStart', [{ x: sx, y: sy, id: 1 }]);
    for (let i = 1; i <= 6; i++) await touch('touchMove', [{ x: sx, y: sy - 8 * i, id: 1 }]);
    s = await until((st) => st.boulder > start.boulder + 0.5, 90000);
    await touch('touchEnd', []);
  } else {
    await page.keyboard.down('KeyW');
    s = await until((st) => st.boulder > start.boulder + 0.5, 90000);
    await page.keyboard.up('KeyW');
  }
  console.log(`after pushing: boulder at ${s.boulder}, Sisyphus at (${s.x}, ${s.z})`);
  if (s.boulder <= start.boulder + 0.5) await fail('walking into the boulder did not push it');
  await page.screenshot({ path: path.join(outDir, 'sisyphus-pushed.png'), timeout: 180000 });

  // Up against it, looking ahead, a module is in reach: patch it (hold E, or
  // hold the Patch button).
  s = await until((st) => st.aimed >= 0, 10000);
  if (s.aimed < 0) await fail('no module in reach after pushing');
  if (phone) {
    await page.waitForTimeout(400);
    await page.locator('#patchBtn').dispatchEvent('pointerdown');
    s = await until((st) => st.patched > 0, 90000);
    await page.locator('#patchBtn').dispatchEvent('pointerup');
  } else {
    await page.keyboard.down('KeyE');
    s = await until((st) => st.patched > 0, 90000);
    await page.keyboard.up('KeyE');
  }
  console.log(`patched: ${s.patched}`);
  if (s.patched < 1) await fail('holding patch on a module did not patch it');

  // Looking turns the view (on a desktop, through the page's own look() call:
  // headless Chromium fakes pointer-lock movement).
  if (phone) {
    const lx = box.x + box.width * 0.75, ly = box.y + box.height * 0.4;
    await touch('touchStart', [{ x: lx, y: ly, id: 2 }]);
    for (let i = 1; i <= 5; i++) await touch('touchMove', [{ x: lx + 12 * i, y: ly, id: 2 }]);
    await touch('touchEnd', []);
  } else {
    s = await until((st) => st.captured, 10000);
    if (!s.captured) await fail('the pointer lock was not taken when How to play closed');
    await page.evaluate(() => window.sisyphus.look(300, 0));
  }
  const yaw0 = s.yaw;
  s = await until((st) => Math.abs(st.yaw - yaw0) > 0.05);
  if (Math.abs(s.yaw - yaw0) <= 0.05) await fail('looking did not turn the view');

  // Sit, and the words come; get up again.
  if (phone) {
    await page.waitForTimeout(400);
    await page.locator('#pad [data-key="20"]').tap();
  } else await page.keyboard.press('KeyQ');
  s = await until((st) => st.sitting && st.quote.length > 0);
  if (!s.sitting) await fail('Sit did not sit');
  await page.waitForTimeout(1500);
  await page.screenshot({ path: path.join(outDir, 'sisyphus-sitting.png'), timeout: 180000 });
  if (phone) await page.locator('#pad [data-key="20"]').tap();
  else await page.keyboard.press('KeyQ');
  s = await until((st) => !st.sitting);
  if (s.sitting) await fail('Sit again did not stand up');

  if (!phone) {
    await page.evaluate(() => document.exitPointerLock());
    s = await until((st) => !st.captured, 10000);
    if (s.captured) await fail('releasing the pointer lock did not reach the game');
  }

  // Save, reload, and the boulder is where it was, with the patch counted.
  // An earlier autosave may already have set the note: clear it, so the wait
  // below is for this save's flush to IndexedDB.
  await page.evaluate(() => { document.getElementById('saveNote').textContent = ''; });
  await page.locator('#saveBtn').click();
  await page.waitForFunction(() => /Saved in this browser/.test(document.getElementById('saveNote').textContent), null,
    { timeout: 20000 }).catch(() => {});
  const note = await page.locator('#saveNote').innerText();
  if (!/Saved in this browser/.test(note)) await fail('the save note did not confirm: ' + note);
  const saved = await status();
  await open();
  const back = await status();
  console.log(`after a reload: boulder at ${back.boulder}, patched ${back.patched}; "${back.message}"`);
  if (Math.abs(back.boulder - saved.boulder) > 0.25) await fail('the boulder did not survive a reload');
  if (back.patched !== saved.patched) await fail('the patch count did not survive a reload');
  if (!/Loaded your save/.test(back.message)) await fail('no "Loaded your save" message after a reload: ' + back.message);
  if (await page.locator('#help').isVisible()) await fail('How to play came back after it was closed once');

  if (errors.length) await fail('page errors: ' + errors.join(' | '));
  console.log(`OK (${phone ? 'phone' : 'desktop'})`);
  await browser.close();
})().catch((e) => {
  console.error(e);
  process.exit(1);
});
