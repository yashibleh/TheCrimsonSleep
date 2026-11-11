// audio.js - every sound in THE CRIMSON SLEEP is synthesised at runtime.
// No external audio assets: ambience, music, footsteps, screams and gunshots
// are built from oscillators and filtered noise.

export class SoundEngine {
  constructor() {
    this.ctx = null;
    this.ready = false;
    this.masterVol = 0.85;
    this.musicVol = 0.55;
    this.sfxVol = 0.9;
    this.mood = 0;
    this.danger = 0;
    this.nextNote = 0;
    this.moodTime = 0;
    this.heartTimer = 0;
    this.enabled = true;
  }

  async start() {
    if (this.ctx) { await this.ctx.resume(); return; }
    const AC = window.AudioContext || window.webkitAudioContext;
    if (!AC) { this.enabled = false; return; }
    const ctx = new AC();
    this.ctx = ctx;
    this.master = ctx.createGain();
    this.master.gain.value = this.masterVol;
    const comp = ctx.createDynamicsCompressor();
    comp.threshold.value = -18;
    comp.knee.value = 22;
    comp.ratio.value = 5;
    this.master.connect(comp);
    comp.connect(ctx.destination);

    this.musicBus = ctx.createGain();
    this.musicBus.gain.value = this.musicVol;
    this.musicBus.connect(this.master);

    this.sfxBus = ctx.createGain();
    this.sfxBus.gain.value = this.sfxVol;
    this.sfxBus.connect(this.master);

    // reusable noise buffer
    const len = ctx.sampleRate * 2;
    const buf = ctx.createBuffer(1, len, ctx.sampleRate);
    const data = buf.getChannelData(0);
    let last = 0;
    for (let i = 0; i < len; i++) {
      const white = Math.random() * 2 - 1;
      last = 0.97 * last + 0.03 * white;
      data[i] = white * 0.6 + last * 2.4;
    }
    this.noise = buf;

    this.buildDrone();
    this.ready = true;
  }

  setVolumes(master, music) {
    this.masterVol = master;
    this.musicVol = music;
    if (this.master) this.master.gain.value = master;
    if (this.musicBus) this.musicBus.gain.value = music * 0.9;
  }

  // ---------------------------------------------------------------- helpers
  now() { return this.ctx ? this.ctx.currentTime : 0; }

  env(gain, t0, attack, hold, release, peak) {
    gain.gain.cancelScheduledValues(t0);
    gain.gain.setValueAtTime(0.0001, t0);
    gain.gain.linearRampToValueAtTime(peak, t0 + attack);
    gain.gain.setValueAtTime(peak, t0 + attack + hold);
    gain.gain.exponentialRampToValueAtTime(0.0001, t0 + attack + hold + release);
  }

  noiseHit({ dur = 0.2, type = 'lowpass', freq = 1200, q = 1, gain = 0.3, sweep = 0, delay = 0 }) {
    if (!this.ready) return;
    const ctx = this.ctx, t0 = ctx.currentTime + delay;
    const src = ctx.createBufferSource();
    src.buffer = this.noise;
    src.loop = true;
    const filt = ctx.createBiquadFilter();
    filt.type = type;
    filt.frequency.setValueAtTime(freq, t0);
    if (sweep) filt.frequency.exponentialRampToValueAtTime(Math.max(40, freq + sweep), t0 + dur);
    filt.Q.value = q;
    const g = ctx.createGain();
    this.env(g, t0, 0.005, dur * 0.35, dur * 0.65, gain);
    src.connect(filt).connect(g).connect(this.sfxBus);
    src.start(t0);
    src.stop(t0 + dur + 0.1);
  }

  tone({ freq = 220, to = null, dur = 0.25, type = 'sine', gain = 0.2, delay = 0, bus = null, detune = 0 }) {
    if (!this.ready) return;
    const ctx = this.ctx, t0 = ctx.currentTime + delay;
    const osc = ctx.createOscillator();
    osc.type = type;
    osc.frequency.setValueAtTime(freq, t0);
    if (to) osc.frequency.exponentialRampToValueAtTime(Math.max(20, to), t0 + dur);
    osc.detune.value = detune;
    const g = ctx.createGain();
    this.env(g, t0, 0.01, dur * 0.3, dur * 0.7, gain);
    osc.connect(g).connect(bus || this.sfxBus);
    osc.start(t0);
    osc.stop(t0 + dur + 0.1);
  }

  // ------------------------------------------------------------------- sfx
  play(id, volume = 1) {
    if (!this.ready || !this.enabled) return;
    const v = Math.max(0, Math.min(1.6, volume));
    switch (Math.round(id)) {
      case 1: this.noiseHit({ dur: 0.09, type: 'lowpass', freq: 700, gain: 0.10 * v }); break;
      case 2: this.noiseHit({ dur: 0.12, type: 'lowpass', freq: 1500, gain: 0.20 * v }); this.tone({ freq: 90, to: 60, dur: 0.09, gain: 0.07 * v }); break;
      case 3: // gunshot
        this.noiseHit({ dur: 0.32, type: 'highpass', freq: 900, gain: 0.75 * v, sweep: -800 });
        this.noiseHit({ dur: 0.5, type: 'lowpass', freq: 500, gain: 0.5 * v });
        this.tone({ freq: 130, to: 42, dur: 0.28, type: 'triangle', gain: 0.5 * v });
        break;
      case 4: this.noiseHit({ dur: 0.2, type: 'bandpass', freq: 600, q: 0.7, gain: 0.28 * v, sweep: -450 }); break;
      case 5: // door creak
        this.tone({ freq: 180, to: 96, dur: 0.55, type: 'sawtooth', gain: 0.09 * v });
        this.tone({ freq: 92, to: 60, dur: 0.5, type: 'triangle', gain: 0.1 * v, delay: 0.05 });
        break;
      case 6: this.tone({ freq: 900, dur: 0.05, type: 'square', gain: 0.06 * v });
        this.tone({ freq: 640, dur: 0.08, type: 'square', gain: 0.05 * v, delay: 0.06 });
        this.noiseHit({ dur: 0.18, type: 'lowpass', freq: 500, gain: 0.12 * v, delay: 0.02 }); break;
      case 7: this.tone({ freq: 660, to: 990, dur: 0.2, type: 'sine', gain: 0.15 * v });
        this.tone({ freq: 1320, dur: 0.12, type: 'sine', gain: 0.06 * v, delay: 0.05 }); break;
      case 8: this.noiseHit({ dur: 0.22, type: 'highpass', freq: 2600, gain: 0.10 * v });
        this.noiseHit({ dur: 0.12, type: 'highpass', freq: 3200, gain: 0.07 * v, delay: 0.1 }); break;
      case 9: this.tone({ freq: 1400, dur: 0.035, type: 'square', gain: 0.07 * v }); break;
      case 10: this.tone({ freq: 880, dur: 0.07, type: 'square', gain: 0.06 * v }); break;
      case 11: for (let i = 0; i < 3; i++) this.tone({ freq: 240 + i * 30, dur: 0.05, type: 'square', gain: 0.05 * v, delay: i * 0.06 }); break;
      case 12: this.noiseHit({ dur: 0.18, type: 'lowpass', freq: 420, gain: 0.34 * v });
        this.tone({ freq: 70, to: 48, dur: 0.2, type: 'sine', gain: 0.24 * v }); break;
      case 13: this.noiseHit({ dur: 0.35, type: 'highpass', freq: 2200, gain: 0.12 * v });
        this.tone({ freq: 1200, to: 700, dur: 0.12, type: 'sine', gain: 0.04 * v }); break;
      case 14: this.noiseHit({ dur: 1.3, type: 'bandpass', freq: 1300, q: 0.5, gain: 0.22 * v, sweep: -700 }); break;
      case 15: this.tone({ freq: 55, dur: 1.8, type: 'sine', gain: 0.16 * v });
        this.noiseHit({ dur: 1.6, type: 'lowpass', freq: 260, gain: 0.1 * v }); break;
      case 16: this.tone({ freq: 240, to: 90, dur: 0.35, type: 'sawtooth', gain: 0.2 * v });
        this.noiseHit({ dur: 0.25, type: 'lowpass', freq: 600, gain: 0.28 * v }); break;
      case 17: this.tone({ freq: 60, to: 120, dur: 1.1, type: 'sawtooth', gain: 0.12 * v });
        this.tone({ freq: 120, to: 240, dur: 1.0, type: 'triangle', gain: 0.08 * v }); break;
      case 18: this.tone({ freq: 60, to: 38, dur: 0.35, type: 'sine', gain: 0.3 * v });
        this.noiseHit({ dur: 0.25, type: 'lowpass', freq: 300, gain: 0.2 * v }); break;
      case 19: // organ chord, minor
        [110, 130.8, 164.8, 220].forEach((f, i) => this.tone({ freq: f, dur: 2.6, type: 'sine', gain: 0.09 * v, delay: i * 0.03 }));
        break;
      case 20: this.noiseHit({ dur: 0.08, type: 'lowpass', freq: 600, gain: 0.07 * v }); break;
      case 21: this.noiseHit({ dur: 0.25, type: 'highpass', freq: 2400, gain: 0.16 * v });
        this.tone({ freq: 2400, to: 1600, dur: 0.18, type: 'triangle', gain: 0.05 * v }); break;
      case 22: this.noiseHit({ dur: 0.5, type: 'lowpass', freq: 700, gain: 0.32 * v, sweep: -500 });
        this.tone({ freq: 200, to: 50, dur: 0.55, type: 'sawtooth', gain: 0.16 * v }); break;
      case 23: this.noiseHit({ dur: 0.22, type: 'bandpass', freq: 420, q: 0.6, gain: 0.16 * v, sweep: 300 }); break;
      case 24: this.noiseHit({ dur: 0.16, type: 'bandpass', freq: 900, q: 1.2, gain: 0.3 * v });
        this.tone({ freq: 160, to: 90, dur: 0.18, type: 'square', gain: 0.12 * v }); break;
      case 25: // crawler rasp
        this.noiseHit({ dur: 0.7, type: 'bandpass', freq: 340, q: 1.4, gain: 0.16 * v, sweep: 120 });
        break;
      case 26: // hound growl
        this.tone({ freq: 88, to: 72, dur: 0.75, type: 'sawtooth', gain: 0.13 * v, detune: 12 });
        break;
      case 30: this.tone({ freq: 40, to: 28, dur: 1.6, type: 'sine', gain: 0.4 * v });
        this.noiseHit({ dur: 1.5, type: 'lowpass', freq: 240, gain: 0.3 * v }); break;
      case 31: // weaver scream
        this.tone({ freq: 720, to: 280, dur: 1.1, type: 'sawtooth', gain: 0.22 * v });
        this.tone({ freq: 880, to: 340, dur: 1.0, type: 'square', gain: 0.09 * v, detune: -20 });
        this.noiseHit({ dur: 1.0, type: 'bandpass', freq: 1600, q: 1.4, gain: 0.2 * v, sweep: -900 });
        break;
      case 32: // waking on the ward floor: a breath, a far bell, rising unease
        this.noiseHit({ dur: 1.1, type: 'lowpass', freq: 520, gain: 0.16 * v, sweep: -260 });
        this.tone({ freq: 82, to: 112, dur: 1.8, type: 'triangle', gain: 0.10 * v });
        this.tone({ freq: 220, to: 261.6, dur: 2.4, type: 'sine', gain: 0.05 * v, delay: 0.45, bus: this.musicBus });
        this.tone({ freq: 440, to: 349.2, dur: 2.0, type: 'sine', gain: 0.03 * v, delay: 0.9, bus: this.musicBus });
        break;
      default: break;
    }
  }

  // ---------------------------------------------------------------- music
  buildDrone() {
    const ctx = this.ctx;
    this.droneGain = ctx.createGain();
    this.droneGain.gain.value = 0.06;
    this.droneGain.connect(this.musicBus);

    this.droneFilter = ctx.createBiquadFilter();
    this.droneFilter.type = 'lowpass';
    this.droneFilter.frequency.value = 420;
    this.droneFilter.Q.value = 0.8;
    this.droneFilter.connect(this.droneGain);

    this.droneOscs = [];
    [55, 82.5, 110, 164.8].forEach((f, i) => {
      const o = ctx.createOscillator();
      o.type = i % 2 === 0 ? 'sine' : 'triangle';
      o.frequency.value = f;
      o.detune.value = (i - 1.5) * 6;
      const g = ctx.createGain();
      g.gain.value = 0.4 / (i + 1);
      o.connect(g).connect(this.droneFilter);
      o.start();
      this.droneOscs.push(o);
    });

    // slow filter LFO gives the room a pulse
    const lfo = ctx.createOscillator();
    lfo.frequency.value = 0.06;
    const lfoGain = ctx.createGain();
    lfoGain.gain.value = 110;
    lfo.connect(lfoGain).connect(this.droneFilter.frequency);
    lfo.start();

    // airy noise bed
    const air = ctx.createBufferSource();
    air.buffer = this.noise;
    air.loop = true;
    const airFilter = ctx.createBiquadFilter();
    airFilter.type = 'bandpass';
    airFilter.frequency.value = 640;
    airFilter.Q.value = 0.6;
    this.airGain = ctx.createGain();
    this.airGain.gain.value = 0.05;
    air.connect(airFilter).connect(this.airGain).connect(this.musicBus);
    air.start();
  }

  setMood(mood) {
    if (mood === this.mood) return;
    this.mood = mood;
    this.moodTime = 0;
    this.nextNote = this.now() + 0.6;
    if (!this.ready) return;
    const t = this.now();
    const targets = [
      { drone: 0.055, filter: 300 },  // ward
      { drone: 0.075, filter: 420 },  // dread
      { drone: 0.10, filter: 620 },   // chase
      { drone: 0.05, filter: 820 },   // chapel
      { drone: 0.085, filter: 260 },  // water
      { drone: 0.13, filter: 180 }    // final
    ][mood] || { drone: 0.06, filter: 400 };
    this.droneGain.gain.cancelScheduledValues(t);
    this.droneGain.gain.linearRampToValueAtTime(targets.drone, t + 2.5);
    this.droneFilter.frequency.cancelScheduledValues(t);
    this.droneFilter.frequency.linearRampToValueAtTime(targets.filter, t + 3);
  }

  setDanger(d) {
    this.danger = d;
    if (!this.ready) return;
    const t = this.now();
    if (this.airGain) this.airGain.gain.linearRampToValueAtTime(0.04 + d * 0.09, t + 0.6);
    this.droneGain.gain.linearRampToValueAtTime(0.05 + d * 0.06 + (this.mood === 2 ? 0.03 : 0), t + 0.8);
  }

  heartbeat(intensity = 1) {
    if (!this.ready) return;
    this.tone({ freq: 58, to: 34, dur: 0.20, type: 'sine', gain: 0.20 * intensity, bus: this.musicBus });
    this.tone({ freq: 52, to: 30, dur: 0.24, type: 'sine', gain: 0.14 * intensity, bus: this.musicBus, delay: 0.17 });
  }

  tick(dt, beyond = false) {
    if (!this.ready) return;
    this.moodTime += dt;
    const t = this.now();
    if (t < this.nextNote) return;

    // mood-specific sparse note patterns (A minor / phrygian colour)
    const minor = [110, 130.8, 146.8, 164.8, 196, 220, 261.6, 293.7];
    const dark = [55, 65.4, 73.4, 82.4, 98, 110, 130.8];
    const pick = (arr) => arr[Math.floor(Math.random() * arr.length)];
    let wait = 4 + Math.random() * 4;

    switch (this.mood) {
      case 0:
        this.tone({ freq: pick(minor) * 2, dur: 3.2, type: 'sine', gain: 0.05, bus: this.musicBus });
        wait = 6 + Math.random() * 5;
        break;
      case 1:
        this.tone({ freq: pick(dark), dur: 4.0, type: 'triangle', gain: 0.06, bus: this.musicBus });
        if (Math.random() < 0.5) this.noiseHit({ dur: 1.4, type: 'lowpass', freq: 300, gain: 0.03 });
        wait = 3.5 + Math.random() * 3;
        break;
      case 2: // chase: driving pulse
        this.tone({ freq: 55, dur: 0.35, type: 'square', gain: 0.12, bus: this.musicBus });
        this.tone({ freq: 110, dur: 0.18, type: 'sawtooth', gain: 0.05, bus: this.musicBus, delay: 0.22 });
        if (Math.random() < 0.55) {
          this.tone({ freq: pick([466, 493.9, 523.3, 587.3]), dur: 0.9, type: 'sawtooth', gain: 0.035, bus: this.musicBus });
        }
        wait = 0.42;
        break;
      case 3: // chapel
        [130.8, 164.8, 196, 261.6].forEach((f, i) => {
          this.tone({ freq: f, dur: 6.5, type: 'sine', gain: 0.035, bus: this.musicBus, delay: i * 0.05 });
        });
        wait = 7 + Math.random() * 4;
        break;
      case 4: // water
        this.noiseHit({ dur: 2.2, type: 'bandpass', freq: 480, q: 0.9, gain: 0.055 });
        this.tone({ freq: pick(dark) * 0.5, dur: 3.5, type: 'sine', gain: 0.06, bus: this.musicBus });
        wait = 3 + Math.random() * 3;
        break;
      case 5: // final
        this.tone({ freq: 41, dur: 5, type: 'sine', gain: 0.12, bus: this.musicBus });
        this.tone({ freq: pick([329.6, 349.2, 392]), dur: 4.5, type: 'triangle', gain: 0.03, bus: this.musicBus });
        this.noiseHit({ dur: 2.6, type: 'highpass', freq: 3000, gain: 0.02 });
        wait = 3.4;
        break;
      default: break;
    }

    if (beyond) wait *= 0.55;
    this.nextNote = t + wait;
  }

  // danger heartbeat scheduler
  updateHeart(dt, danger, hpFrac) {
    if (!this.ready) return;
    this.heartTimer -= dt;
    if (this.heartTimer > 0) return;
    const rate = danger > 0.55 ? 0.55 : danger > 0.3 ? 0.85 : hpFrac < 0.35 ? 1.1 : 1.8;
    if (danger > 0.18 || hpFrac < 0.4) {
      this.heartbeat(0.5 + danger * 0.8);
      this.heartTimer = rate;
    } else {
      this.heartTimer = 1.2;
    }
  }
}
