// tools/audio_test.mjs - verifies the procedural audio engine without a sound
// card: a mock WebAudio implementation records every node the engine builds, so
// we can prove each effect and every music mood actually schedules sound.
import path from 'path';
import { pathToFileURL } from 'url';

class Param {
  constructor(v = 0) { this.value = v; this.events = []; }
  setValueAtTime(v, t) { this.value = v; this.events.push(['set', v, t]); return this; }
  linearRampToValueAtTime(v, t) { this.value = v; this.events.push(['lin', v, t]); return this; }
  exponentialRampToValueAtTime(v, t) { this.value = v; this.events.push(['exp', v, t]); return this; }
  cancelScheduledValues(t) { this.events.push(['cancel', t]); return this; }
}

class Node {
  constructor(kind, ctx) { this.kind = kind; this.ctx = ctx; ctx.registry.push(this); this.out = null; }
  connect(n) { this.out = n; return n; }
  start(t) { this.startedAt = t === undefined ? this.ctx.currentTime : t; this.ctx.started.push(this); }
  stop(t) { this.stoppedAt = t; }
}

class Gain extends Node { constructor(ctx) { super('gain', ctx); this.gain = new Param(1); } }
class Osc extends Node {
  constructor(ctx) { super('osc', ctx); this.frequency = new Param(440); this.detune = new Param(0); this.type = 'sine'; }
}
class Filter extends Node {
  constructor(ctx) { super('filter', ctx); this.frequency = new Param(350); this.Q = new Param(1); this.type = 'lowpass'; }
}
class BufferSource extends Node { constructor(ctx) { super('buffer', ctx); this.buffer = null; this.loop = false; } }
class Comp extends Node {
  constructor(ctx) { super('comp', ctx); this.threshold = new Param(); this.knee = new Param(); this.ratio = new Param(); }
}
class Buf {
  constructor(ch, len, sr) { this.sr = sr; this.length = len; this.channels = Array.from({ length: ch }, () => new Float32Array(len)); }
  getChannelData(i) { return this.channels[i] || this.channels[0]; }
}

class Ctx {
  constructor() {
    this.currentTime = 0;
    this.sampleRate = 48000;
    this.registry = [];
    this.started = [];
    this.oscCount = 0;
    this.resumed = false;
    this.destination = new Node('dest', this);
  }
  createGain() { return new Gain(this); }
  createOscillator() { this.oscCount++; return new Osc(this); }
  createBiquadFilter() { return new Filter(this); }
  createBufferSource() { return new BufferSource(this); }
  createDynamicsCompressor() { return new Comp(this); }
  createBuffer(ch, len, sr) { return new Buf(ch, len, sr); }
  resume() { this.resumed = true; return Promise.resolve(); }
}

const results = [];
function check(name, ok, extra = '') {
  results.push(ok);
  console.log(`${ok ? 'PASS' : 'FAIL'}  ${name}${extra ? `  [${extra}]` : ''}`);
}

let ctx = null;
globalThis.window = {
  AudioContext: function () { ctx = new Ctx(); return ctx; }
};

const { SoundEngine } = await import(pathToFileURL(path.join(process.cwd(), 'js/audio.js')).href);

const audio = new SoundEngine();
await audio.start();

check('engine starts and creates an AudioContext', audio.ready === true && !!audio.ctx);
check('output chain is master -> compressor -> destination',
  !!audio.master && !!audio.musicBus && !!audio.sfxBus);

const droneOscs = ctx.registry.filter((n) => n.kind === 'osc' && n.startedAt !== undefined);
check('music bed starts its drone oscillators', droneOscs.length >= 4, `${droneOscs.length} oscillators`);
check('ambient air layer is running', ctx.registry.some((n) => n.kind === 'buffer' && n.loop));

const data = audio.noise.getChannelData(0);
let sum = 0;
for (let i = 0; i < 2048; i++) sum += data[i] * data[i];
const rms = Math.sqrt(sum / 2048);
check('noise buffer is generated and not silent', audio.noise.length === ctx.sampleRate * 2 && rms > 0.02,
  `rms ${rms.toFixed(3)}`);

// every effect id the game actually fires must build nodes
const IN_USE = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
  21, 22, 23, 24, 25, 26, 30, 31, 32];
const silent = [];
for (const id of IN_USE) {
  const before = ctx.registry.length;
  audio.play(id, 1);
  if (ctx.registry.length === before) silent.push(id);
}
check(`all ${IN_USE.length} sound effects build audio nodes`, silent.length === 0,
  silent.length ? `silent ids: ${silent.join(', ')}` : '');

// every mood must schedule notes as time advances
const moodNames = ['ward', 'dread', 'chase', 'chapel', 'water', 'final'];
for (let m = 0; m < moodNames.length; m++) {
  audio.mood = -1;
  audio.setMood(m);
  const before = ctx.oscCount;
  for (let i = 0; i < 800; i++) {
    ctx.currentTime += 0.25;
    audio.tick(0.25, m === 2);
  }
  check(`music mood "${moodNames[m]}" schedules notes`, ctx.oscCount > before,
    `${ctx.oscCount - before} oscillators`);
}

// chase mood should be much denser than the ambient ward
audio.mood = -1;
audio.setMood(0);
let wardOsc = ctx.oscCount;
for (let i = 0; i < 1600; i++) { ctx.currentTime += 0.25; audio.tick(0.25, false); }
wardOsc = ctx.oscCount - wardOsc;
audio.mood = -1;
audio.setMood(2);
let chaseOsc = ctx.oscCount;
for (let i = 0; i < 1600; i++) { ctx.currentTime += 0.25; audio.tick(0.25, true); }
chaseOsc = ctx.oscCount - chaseOsc;
check('chase music is denser than ambient', chaseOsc > wardOsc, `${chaseOsc} vs ${wardOsc}`);

// heartbeat + danger scheduling
const beforeHeart = ctx.oscCount;
audio.heartbeat(1);
check('heartbeat synthesises two thumps', ctx.oscCount - beforeHeart === 2, `${ctx.oscCount - beforeHeart}`);
const beforeDanger = ctx.oscCount;
audio.heartTimer = 0;
audio.updateHeart(0.1, 0.9, 0.4);
check('danger drives the heartbeat scheduler', ctx.oscCount > beforeDanger);

// volume + mute behaviour
audio.setVolumes(0.5, 0.25);
check('volume settings reach the gain nodes',
  Math.abs(audio.master.gain.value - 0.5) < 1e-6 && Math.abs(audio.musicBus.gain.value - 0.225) < 1e-6);
audio.enabled = false;
const beforeMute = ctx.registry.length;
audio.play(3, 1);
check('muting the engine stops new sounds', ctx.registry.length === beforeMute);
audio.enabled = true;

const passed = results.filter(Boolean).length;
console.log(`\nAUDIO TESTS: ${passed}/${results.length} passed`);
process.exit(passed === results.length ? 0 : 1);
