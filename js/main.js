// main.js - boot, input, the frame loop and save/load. Gameplay lives in C++.
import { CrimsonCore, INPUT } from './core.js';
import { Renderer } from './render.js';
import { SoundEngine } from './audio.js';
import { UI } from './ui.js';
import { MESSAGES, SUBTITLES, CLUES, ITEMS } from './content.js';

const EV = { SFX: 1, SHAKE: 2, FLASH: 3, TEXT: 4, JUMPSCARE: 5, QUEST: 6, DEATH: 7, SAVE: 8, CLUE: 9, ROOM: 10, TRANSITION: 11, ENDING: 12, PICKUP: 13, HIT: 14, ALERT: 15 };
const SAVE_KEY = 'tcs_save_v1';

function encodeSave(bytes) {
  let bin = '';
  const chunk = 0x8000;
  for (let i = 0; i < bytes.length; i += chunk) {
    bin += String.fromCharCode.apply(null, bytes.subarray(i, i + chunk));
  }
  return btoa(bin);
}
function decodeSave(text) {
  const bin = atob(text);
  const out = new Uint8Array(bin.length);
  for (let i = 0; i < bin.length; i++) out[i] = bin.charCodeAt(i);
  return out;
}

async function main() {
  const canvas = document.getElementById('game');
  let core;
  try {
    core = await CrimsonCore.load('assets/game.wasm');
  } catch (err) {
    document.getElementById('boot-prompt').textContent = 'The core failed to load. Serve this folder over http and reload.';
    console.error(err);
    return;
  }
  core.init();

  const renderer = new Renderer(canvas);
  const audio = new SoundEngine();
  const keys = new Set();
  const pulses = new Set();
  let saveExists = false;
  try { saveExists = !!localStorage.getItem(SAVE_KEY); } catch (e) { saveExists = false; }
  let deathTimer = -1;
  let endingTimer = -1;

  const saveGame = () => {
    try {
      const bytes = core.serialize();
      localStorage.setItem(SAVE_KEY, encodeSave(bytes));
      saveExists = true;
    } catch (e) { console.warn('save failed', e); }
  };
  const loadGame = () => {
    try {
      const raw = localStorage.getItem(SAVE_KEY);
      if (!raw) return false;
      return core.deserialize(decodeSave(raw));
    } catch (e) { console.warn('load failed', e); return false; }
  };

  const ui = new UI(core, {
    newGame: () => {
      core.newGame();
      saveGame();
      ui.enterPlay();
      ui.roomBanner(core.roomNameId);
      audio.start();
      audio.setMood(core.musicMood);
      audio.play(32, 0.3);
    },
    continueGame: () => {
      if (!loadGame()) core.newGame();
      ui.enterPlay();
      ui.roomBanner(core.roomNameId);
      audio.start();
      audio.setMood(core.musicMood);
    },
    closePanel: () => {
      core.clearUi();
      ui.enterPlay();
    },
    saveAndTitle: () => {
      saveGame();
      ui.enterMenu(saveExists);
    },
    restart: () => {
      if (!loadGame()) core.newGame();
      deathTimer = -1;
      endingTimer = -1;
      ui.enterPlay();
      ui.roomBanner(core.roomNameId);
      audio.setMood(core.musicMood);
    },
    toTitle: () => ui.enterMenu(saveExists),
    backFromCredits: () => ui.enterMenu(saveExists),
    hasSave: () => saveExists,
    applySettings: (s) => {
      audio.setVolumes(s.master, s.music);
      renderer.shakeEnabled = s.shake;
    }
  });

  renderer.shakeEnabled = ui.settings.shake;
  ui.enterBoot();
  // debug/test hook: lets the automated harness inspect the real running game
  window.__tcs = { core, renderer, audio, ui, get mode() { return ui.mode; } };
  renderer.resize();
  window.addEventListener('resize', () => renderer.resize());

  // ------------------------------------------------------------- boot flow
  const beginFromBoot = async () => {
    if (ui.mode !== 'boot') return;
    await audio.start();
    audio.setVolumes(ui.settings.master, ui.settings.music);
    audio.setMood(0);
    ui.enterMenu(saveExists);
    ui.roomBanner(core.roomNameId);
  };
  window.addEventListener('keydown', (e) => {
    if (ui.mode === 'boot' && !e.repeat) beginFromBoot();
  });
  window.addEventListener('pointerdown', () => {
    if (ui.mode === 'boot') beginFromBoot();
  });

  // --------------------------------------------------------------- input
  const KEYMAP = {
    KeyW: INPUT.UP, ArrowUp: INPUT.UP,
    KeyS: INPUT.DOWN, ArrowDown: INPUT.DOWN,
    KeyA: INPUT.LEFT, ArrowLeft: INPUT.LEFT,
    KeyD: INPUT.RIGHT, ArrowRight: INPUT.RIGHT,
    ShiftLeft: INPUT.SPRINT, ShiftRight: INPUT.SPRINT
  };

  window.addEventListener('keydown', (e) => {
    const c = e.code;
    if (['ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight', 'Space', 'Tab'].includes(c)) e.preventDefault();
    if (e.repeat) return;

    if (ui.mode === 'keypad' && /^Digit[0-9]$/.test(c)) {
      core.codeDigit(Number(c.slice(5)));
      ui.refreshKeypad();
      return;
    }

    if (ui.mode === 'playing') {
      if (KEYMAP[c]) keys.add(KEYMAP[c]);
      switch (c) {
        case 'KeyF': pulses.add(INPUT.ATTACK); break;
        case 'KeyE': pulses.add(INPUT.INTERACT); break;
        case 'KeyL': case 'KeyT': pulses.add(INPUT.LIGHT); break;
        case 'KeyR': pulses.add(INPUT.RELOAD); break;
        case 'KeyQ': pulses.add(INPUT.HEAL); break;
        case 'KeyX': pulses.add(INPUT.SWAP); break;
        case 'KeyI': ui.openInventory(); break;
        case 'KeyJ': ui.openJournal('journal'); break;
        case 'KeyM': ui.openJournal('map'); break;
        case 'Escape': ui.openPause(); break;
        default: break;
      }
    } else if (ui.mode === 'inventory' || ui.mode === 'journal') {
      if (c === 'Escape' || c === 'KeyI' || c === 'KeyJ') ui.hooks.closePanel?.();
    } else if (ui.mode === 'keypad') {
      if (c === 'Escape') { core.clearUi(); ui.enterPlay(); }
    } else if (ui.mode === 'paused') {
      if (c === 'Escape') ui.enterPlay();
    } else if (ui.mode === 'settings') {
      if (c === 'Escape') ui.hooks.closePanel?.();
    }
  });

  window.addEventListener('keyup', (e) => {
    const bit = KEYMAP[e.code];
    if (bit) keys.delete(bit);
  });
  window.addEventListener('blur', () => {
    keys.clear();
    if (ui.mode === 'playing') ui.openPause();
  });

  function buildInput() {
    let bits = 0;
    for (const k of keys) bits |= k;
    for (const p of pulses) bits |= p;
    pulses.clear();
    return bits;
  }

  // ---------------------------------------------------------- core events
  function handleEvents() {
    const events = core.events();
    core.clearEvents();
    for (const ev of events) {
      switch (ev.type) {
        case EV.SFX:
          audio.play(ev.a, Math.max(0.12, Math.min(1.2, ev.b || 0.5)));
          break;
        case EV.SHAKE:
          if (ui.settings.shake) renderer.shake(ev.a, ev.b);
          break;
        case EV.FLASH:
          renderer.flashScreen(Math.min(0.8, ev.a), ev.b === 2 ? '#ffe6b0' : '#8c0d12');
          break;
        case EV.TEXT:
          // the HUD already surfaces core.message; don't echo the same line twice
          if (core.message === Math.round(ev.a)) break;
          ui.toast(MESSAGES[ev.a] || '');
          break;
        case EV.JUMPSCARE: {
          const kind = Math.round(ev.a);
          renderer.flashScreen(1, '#c01018');
          if (ui.settings.shake) renderer.shake(16, 1.1);
          audio.play(kind === 2 ? 31 : 30, 1);
          document.body.classList.add('scare');
          setTimeout(() => document.body.classList.remove('scare'), 420);
          break;
        }
        case EV.QUEST: {
          const id = Math.round(ev.a);
          // narration lines already own the subtitle channel; don't echo them as toasts
          if (core.subtitleTimer > 0 && core.subtitle === id) break;
          const text = SUBTITLES[id] || MESSAGES[id];
          if (text) ui.toast(text, 6000);
          break;
        }
        case EV.SAVE:
          saveGame();
          ui.toast('The candle remembers this place.', 3000);
          break;
        case EV.CLUE: {
          const clue = CLUES[ev.a];
          ui.toast(clue ? `Journal: ${clue.title}` : 'Journal updated.', 3600);
          break;
        }
        case EV.ROOM:
          ui.roomBanner(core.roomNameId);
          saveGame();
          break;
        case EV.TRANSITION:
          renderer.flashScreen(0.35, '#050405');
          break;
        case EV.PICKUP: {
          const item = ITEMS[ev.a];
          if (item) ui.toast(`${item.name}${ev.b > 1 ? ' ×' + ev.b : ''}`, 2600);
          break;
        }
        case EV.HIT:
          renderer.flashScreen(0.5, '#7a0a10');
          renderer.shake(Math.min(14, 6 + ev.a * 0.2), 0.4);
          audio.play(16, 0.9);
          break;
        case EV.ALERT:
          audio.play(24, 0.3);
          break;
        case EV.DEATH:
          deathTimer = 1.4;
          break;
        case EV.ENDING:
          endingTimer = 1.8;
          break;
        default:
          break;
      }
    }
  }

  // ---------------------------------------------------------------- loop
  let last = performance.now();
  function frame(now) {
    const dt = Math.min(0.05, Math.max(0, (now - last) / 1000));
    last = now;

    if (ui.mode === 'playing') {
      core.update(dt, buildInput());
      handleEvents();
      ui.updateHud(core, dt);

      if (core.uiRequest === 1) {
        core.clearUi();
        ui.openKeypad();
      }

      const p = core.player();
      audio.setMood(core.musicMood);
      audio.setDanger(core.danger);
      audio.tick(dt, core.danger > 0.6);
      audio.updateHeart(dt, core.danger, p.hp / p.hpMax);

      if (core.state === 1 && deathTimer < 0) deathTimer = 1.4;
      if (core.state === 2 && endingTimer < 0) endingTimer = 1.8;

      if (deathTimer > 0) {
        deathTimer -= dt;
        if (deathTimer <= 0) {
          deathTimer = 0;
          if (ui.settings.shake) renderer.shake(10, 1);
          audio.play(30, 0.8);
          ui.showDeath(ui.deathStats(core));
        }
      }
      if (endingTimer > 0) {
        endingTimer -= dt;
        if (endingTimer <= 0) {
          endingTimer = 0;
          audio.setMood(0);
          ui.showEnding(core.ending);
          document.getElementById('ending-stats').innerHTML = ui.endingStats(core);
        }
      }
    } else if (ui.mode === 'menu' || ui.mode === 'boot') {
      audio.tick(dt * 0.4, false);
    }

    renderer.draw(core, { grain: ui.settings.grain }, dt);
    requestAnimationFrame(frame);
  }
  requestAnimationFrame(frame);
}

main();
