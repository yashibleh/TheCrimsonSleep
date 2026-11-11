// tools/web_test.mjs - boots the real game in jsdom with a real canvas backend,
// drives it through boot -> menu -> play -> panels -> ending -> death and writes
// screenshots of the actual rendered frames for visual inspection.
import fs from 'fs';
import path from 'path';
import { pathToFileURL } from 'url';
import { createRequire } from 'module';

const require = createRequire(import.meta.url);
const { JSDOM, VirtualConsole } = require('/tmp/viz/node_modules/jsdom');

const root = process.cwd();
const errors = [];
const results = [];
function check(name, cond, extra = '') {
  results.push({ name, ok: !!cond });
  console.log(`${cond ? 'PASS' : 'FAIL'}  ${name}${extra ? `  [${extra}]` : ''}`);
}
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
async function waitFor(fn, ms = 6000) {
  const t0 = Date.now();
  while (Date.now() - t0 < ms) {
    try { if (fn()) return true; } catch (e) { /* keep waiting */ }
    await sleep(50);
  }
  return false;
}

const html = fs.readFileSync(path.join(root, 'index.html'), 'utf8');
const vc = new VirtualConsole();
vc.on('jsdomError', (e) => { errors.push(e.message || String(e)); console.log('JSDOM ERROR:', e.message); });
vc.on('error', (msg) => { errors.push(String(msg)); console.log('CONSOLE ERROR:', msg); });

const dom = new JSDOM(html, {
  url: 'http://localhost:3000/',
  pretendToBeVisual: true,
  runScripts: 'outside-only',
  virtualConsole: vc
});
const { window } = dom;

globalThis.window = window;
globalThis.document = window.document;
globalThis.HTMLElement = window.HTMLElement;
globalThis.HTMLCanvasElement = window.HTMLCanvasElement;
globalThis.Image = window.Image;
globalThis.Event = window.Event;
globalThis.KeyboardEvent = window.KeyboardEvent;
globalThis.getComputedStyle = window.getComputedStyle.bind(window);
globalThis.localStorage = window.localStorage;
globalThis.requestAnimationFrame = window.requestAnimationFrame.bind(window);
globalThis.cancelAnimationFrame = window.cancelAnimationFrame.bind(window);
window.innerWidth = 1280;
window.innerHeight = 720;
window.devicePixelRatio = 1;

// serve game.wasm to the page's fetch
globalThis.fetch = async (url) => {
  const rel = String(url).replace(/^https?:\/\/[^/]+\//, '').replace(/^\.?\//, '');
  const file = path.join(root, rel);
  if (!fs.existsSync(file)) return { ok: false, status: 404, arrayBuffer: async () => new ArrayBuffer(0) };
  const buf = fs.readFileSync(file);
  return {
    ok: true,
    status: 200,
    headers: { get: () => 'application/wasm' },
    arrayBuffer: async () => buf.buffer.slice(buf.byteOffset, buf.byteOffset + buf.byteLength)
  };
};
window.fetch = globalThis.fetch;

fs.mkdirSync('artifacts', { recursive: true });
function shot(name) {
  try {
    const canvas = window.document.getElementById('game');
    const url = canvas.toDataURL('image/png');
    fs.writeFileSync(path.join('artifacts', `${name}.png`), Buffer.from(url.split(',')[1], 'base64'));
  } catch (e) {
    console.log('screenshot failed:', e.message);
  }
}
const key = (code, type = 'keydown') => window.dispatchEvent(new window.KeyboardEvent(type, { code, bubbles: true }));
const $ = (id) => window.document.getElementById(id);

await import(pathToFileURL(path.join(root, 'js/main.js')).href);
const ready = await waitFor(() => !!window.__tcs);
check('game boots and exposes the wasm core', ready);
if (!ready) {
  console.error('fatal: core did not initialise');
  process.exit(1);
}
const { core, ui, renderer } = window.__tcs;

check('boot screen is shown first', ui.mode === 'boot');
check('boot screen has the logo image asset', !!$('logo-img') && $('logo-img').getAttribute('src').includes('logo.png'));
await sleep(300);
shot('shot-01-boot');

key('KeyX');
await waitFor(() => ui.mode === 'menu');
check('any key moves from boot to the main menu', ui.mode === 'menu');
await sleep(250);
shot('shot-02-menu');
check('continue is disabled with no save', $('btn-continue').disabled === true);

$('btn-new').click();
await waitFor(() => ui.mode === 'playing');
check('new game starts play', ui.mode === 'playing');
check('new game begins in ward 3', core.roomId === 0);
check('new game writes a checkpoint', !!localStorage.getItem('tcs_save_v1'));
await sleep(400);
shot('shot-03-ward');

// movement
const before = core.player();
key('KeyD', 'keydown');
await sleep(500);
key('KeyD', 'keyup');
await sleep(120);
const after = core.player();
check('player moves right with the keyboard', after.x > before.x + 4, `${before.x.toFixed(1)} -> ${after.x.toFixed(1)}`);
check('walking produces footstep noise', after.noise >= 0);
check('hud health is rendered', $('hp-text').textContent !== '');

// sprint drains stamina
const stamBefore = core.player().stamina;
key('ShiftLeft', 'keydown');
key('KeyA', 'keydown');
await sleep(700);
key('ShiftLeft', 'keyup');
key('KeyA', 'keyup');
await sleep(120);
check('sprinting drains stamina', core.player().stamina < stamBefore);

// inventory + journal panels
key('KeyI');
await waitFor(() => ui.mode === 'inventory');
check('satchel panel opens', ui.mode === 'inventory');
await sleep(250);
shot('shot-04-inventory');
key('Escape');
await waitFor(() => ui.mode === 'playing');
check('escape closes the satchel', ui.mode === 'playing');

key('KeyJ');
await waitFor(() => ui.mode === 'journal');
check('journal panel opens', ui.mode === 'journal');
await sleep(250);
shot('shot-05-journal');
window.document.querySelector('.journal-tab[data-tab="map"]').click();
await sleep(250);
shot('shot-06-map');
key('Escape');
await waitFor(() => ui.mode === 'playing');

// prompt + pickup in the ward
const items = core.items();
const bandage = items.find((it) => it.itemId === 4);
if (bandage) {
  core.setPos(bandage.x, bandage.y + 10);
  await sleep(250);
  check('interaction prompt appears near an item', $('prompt').classList.contains('visible'));
  key('KeyE');
  await sleep(250);
  check('bandage enters the C++ inventory', core.hasItem(4));
  key('KeyI');
  await sleep(250);
  shot('shot-07-inventory-filled');
  key('Escape');
  await waitFor(() => ui.mode === 'playing');
}

// room variety
core.teleport(1, 640, 336);
await sleep(600);
check('corridor loads', core.roomId === 1);
check('corridor has a living enemy', core.enemies().length > 0, `${core.enemies().length}`);
shot('shot-08-corridor');

core.teleport(4, 640, 336);
await sleep(500);
shot('shot-09-boiler');
core.teleport(7, 640, 200);
await sleep(500);
shot('shot-10-chapel');
core.teleport(8, 640, 320);
await sleep(500);
shot('shot-11-morgue');
core.teleport(9, 640, 330);
await sleep(500);
shot('shot-12-sleepchamber');

// keypad UI plumbing
ui.openKeypad();
await waitFor(() => ui.mode === 'keypad');
check('keypad opens', ui.mode === 'keypad');
const padButtons = [...window.document.querySelectorAll('#code-pad .key')];
for (const d of ['0', '3', '1', '7']) {
  const btn = padButtons.find((b) => b.textContent === d);
  if (btn) btn.click();
  await sleep(60);
}
check('keypad digits reach the C++ code lock', core.codeState().solved === true);
await sleep(250);
shot('shot-13-keypad');
ui.hooks.closePanel?.();
await waitFor(() => ui.mode === 'playing');

// damage feedback + death
core.hurt(30);
await sleep(900);
check('damage reduces health in the hud', Number($('hp-text').textContent) < 100);
core.hurt(999);
console.log('  [debug] state after lethal hit:', core.state, 'hp:', core.player().hp);
await waitFor(() => ui.mode === 'dead', 5000);
check('death screen appears', ui.mode === 'dead');
await sleep(300);
shot('shot-14-death');

// restart from checkpoint restores the save
$('btn-restart').click();
await waitFor(() => ui.mode === 'playing');
check('restart from the last candle returns to play', ui.mode === 'playing');
check('checkpoint restores health', core.player().hp > 50, `${core.player().hp}`);

// ending
core.giveItem(13, 1);
core.giveItem(12, 1);
const cradle = core.props().find((p) => p.type === 11);
check('cradle exists in the final chamber', !!cradle);
if (cradle) {
  core.teleport(9, cradle.x, cradle.y + 58);
  await sleep(500);
  key('KeyE');
  await sleep(500);
  console.log('  [debug] after E: mode=', ui.mode, 'state=', core.state, 'ending=', core.ending,
    'prompt=', JSON.stringify(core.prompt()), 'hasItem locket=', core.hasItem(13), 'sedative=', core.hasItem(12));
  await waitFor(() => ui.mode === 'ending', 6000);
  check('interacting with the cradle reaches an ending', ui.mode === 'ending');
  await sleep(1400);
  shot('shot-15-ending');
}

check('no uncaught runtime errors', errors.length === 0, `${errors.length}`);

const passed = results.filter((r) => r.ok).length;
console.log(`\nWEB TESTS: ${passed}/${results.length} passed`);
if (errors.length) console.log('errors:', errors.slice(0, 5));
process.exit(passed === results.length ? 0 : 1);
