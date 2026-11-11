// ui.js - DOM interface: boot screen, menus, HUD, inventory, journal, map,
// keypad, settings, death and endings. Presentation only; all state is C++.
import { ITEMS, CLUES, PROMPTS, MESSAGES, SUBTITLES, OBJECTIVES, ENDINGS, ROOMS, ENEMY_INFO, CREDITS } from './content.js';

const $ = (id) => document.getElementById(id);

const ICONS = {
  weapon: '<svg viewBox="0 0 24 24"><path d="M3 15l9-9 3 3-9 9H3v-3z" fill="none" stroke="currentColor" stroke-width="1.4"/><path d="M14 4l6 6" stroke="currentColor" stroke-width="1.4"/></svg>',
  ammo: '<svg viewBox="0 0 24 24"><rect x="7" y="7" width="10" height="12" fill="none" stroke="currentColor" stroke-width="1.4"/><path d="M9 7V4h2v3M13 7V4h2v3" stroke="currentColor" stroke-width="1.3"/></svg>',
  heal: '<svg viewBox="0 0 24 24"><path d="M12 5v14M5 12h14" stroke="currentColor" stroke-width="2"/></svg>',
  key: '<svg viewBox="0 0 24 24"><circle cx="8" cy="9" r="4" fill="none" stroke="currentColor" stroke-width="1.5"/><path d="M11 12l8 8M16 17l2-2M19 20l2-2" stroke="currentColor" stroke-width="1.5"/></svg>',
  quest: '<svg viewBox="0 0 24 24"><path d="M12 3l7 6-7 12L5 9z" fill="none" stroke="currentColor" stroke-width="1.4"/></svg>',
  battery: '<svg viewBox="0 0 24 24"><rect x="6" y="5" width="12" height="16" fill="none" stroke="currentColor" stroke-width="1.4"/><path d="M10 3h4" stroke="currentColor" stroke-width="2"/><path d="M12 8l-2 5h3l-2 5" fill="none" stroke="currentColor" stroke-width="1.2"/></svg>',
  none: '<svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="6" fill="none" stroke="currentColor" stroke-width="1.2"/></svg>'
};
const CAT_TO_ICON = { 1: 'weapon', 2: 'ammo', 3: 'heal', 4: 'key', 5: 'quest', 6: 'battery' };

export class UI {
  constructor(core, hooks = {}) {
    this.core = core;
    this.hooks = hooks;
    this.mode = 'boot';
    this.toasts = [];
    this.lastMessage = 0;
    this.seenEnemies = new Set();
    this.invSelected = 0;
    this.journalTab = 'journal';
    this.settings = this.loadSettings();
    this.buildInventory();
    this.buildJournalTabs();
    this.buildKeypad();
    this.buildCredits();
    this.bindSettings();
    this.bindButtons();
  }

  // ------------------------------------------------------------- settings
  loadSettings() {
    const def = { master: 0.85, music: 0.6, grain: true, shake: true };
    try {
      const raw = localStorage.getItem('tcs_settings');
      if (!raw) return def;
      return { ...def, ...JSON.parse(raw) };
    } catch (e) { return def; }
  }
  saveSettings() {
    try { localStorage.setItem('tcs_settings', JSON.stringify(this.settings)); } catch (e) { /* ignore */ }
    if (this.hooks.applySettings) this.hooks.applySettings(this.settings);
  }
  bindSettings() {
    const master = $('set-master'), music = $('set-music'), grain = $('set-grain'), shake = $('set-shake');
    master.value = Math.round(this.settings.master * 100);
    music.value = Math.round(this.settings.music * 100);
    grain.checked = this.settings.grain;
    shake.checked = this.settings.shake;
    master.addEventListener('input', () => { this.settings.master = master.value / 100; this.saveSettings(); });
    music.addEventListener('input', () => { this.settings.music = music.value / 100; this.saveSettings(); });
    grain.addEventListener('change', () => { this.settings.grain = grain.checked; this.saveSettings(); });
    shake.addEventListener('change', () => { this.settings.shake = shake.checked; this.saveSettings(); });
    $('btn-fullscreen').addEventListener('click', () => {
      if (!document.fullscreenElement) document.documentElement.requestFullscreen?.();
      else document.exitFullscreen?.();
    });
  }

  // --------------------------------------------------------------- screens
  show(id) {
    for (const el of document.querySelectorAll('.screen')) el.classList.remove('visible');
    const el = $(id);
    if (el) el.classList.add('visible');
  }
  hideScreens() {
    for (const el of document.querySelectorAll('.screen')) el.classList.remove('visible');
  }
  enterBoot() {
    this.mode = 'boot';
    this.show('boot');
    $('hud').classList.remove('visible');
  }
  enterMenu(hasSave) {
    this.mode = 'menu';
    this.show('menu');
    $('btn-continue').disabled = !hasSave;
    $('btn-continue').classList.toggle('disabled', !hasSave);
    $('hud').classList.remove('visible');
    this.updateContinueNote(hasSave);
  }
  updateContinueNote(hasSave) {
    const note = $('continue-note');
    if (!note) return;
    note.textContent = hasSave ? 'A candle was lit. The dream remembers.' : 'No one has lit a candle yet.';
  }
  enterPlay() {
    this.mode = 'playing';
    this.hideScreens();
    $('hud').classList.add('visible');
  }
  openPause() {
    this.mode = 'paused';
    this.show('pause');
  }
  openSettings(back = 'pause') {
    this.settingsBack = back;
    this.mode = 'settings';
    this.show('settings');
  }
  openInventory() {
    this.mode = 'inventory';
    this.invSelected = 0;
    this.refreshInventory();
    this.show('inventory');
  }
  openJournal(tab = 'journal') {
    this.mode = 'journal';
    this.journalTab = tab;
    this.refreshJournal();
    this.show('journal');
  }
  openKeypad() {
    this.mode = 'keypad';
    this.refreshKeypad();
    this.show('keypad');
  }
  openCredits() {
    this.mode = 'credits';
    this.show('credits');
    const roll = $('credits-roll');
    roll.scrollTop = 0;
    roll.style.animation = 'none';
    void roll.offsetWidth;
    roll.style.animation = 'creditsRoll 42s linear forwards';
  }
  showDeath(stats) {
    this.mode = 'dead';
    $('death-stats').innerHTML = stats;
    this.show('death');
  }
  showEnding(id) {
    this.mode = 'ending';
    const data = ENDINGS[id] || ENDINGS[2];
    $('ending-title').textContent = data.title;
    const body = $('ending-body');
    body.innerHTML = '';
    data.body.forEach((p, i) => {
      const el = document.createElement('p');
      el.textContent = p;
      el.style.animationDelay = `${i * 0.9}s`;
      body.appendChild(el);
    });
    this.show('ending');
  }
  showQuit() {
    this.mode = 'quit';
    this.show('quit');
  }

  // ------------------------------------------------------------------ hud
  updateHud(core, dt) {
    const p = core.player();
    const hpFrac = Math.max(0, p.hp / p.hpMax);
    $('hp-fill').style.width = `${hpFrac * 100}%`;
    $('hp-fill').classList.toggle('critical', hpFrac < 0.3);
    $('hp-text').textContent = `${Math.ceil(p.hp)}`;
    $('stam-fill').style.width = `${Math.max(0, (p.stamina / p.staminaMax) * 100)}%`;
    $('battery-fill').style.width = `${Math.max(0, (p.battery / p.batteryMax) * 100)}%`;
    $('battery-fill').classList.toggle('low', p.battery < 25);

    const weapon = p.weapon === 2 ? 'REVOLVER' : 'CROWBAR';
    $('weapon-name').textContent = weapon;
    if (p.weapon === 2) {
      $('ammo-text').textContent = `${Math.round(p.ammo)} / ${p.ammoReserve}`;
    } else {
      $('ammo-text').textContent = '—';
    }

    // objective
    const obj = OBJECTIVES[core.objective] || '';
    const objEl = $('objective');
    if (objEl.dataset.text !== obj) {
      objEl.dataset.text = obj;
      objEl.textContent = obj;
      objEl.classList.remove('flash');
      void objEl.offsetWidth;
      objEl.classList.add('flash');
    }

    // prompt
    const prompt = core.prompt();
    const promptEl = $('prompt');
    if (prompt.id && PROMPTS[prompt.label]) {
      promptEl.innerHTML = `<span class="key">E</span> ${PROMPTS[prompt.label]}`;
      promptEl.classList.add('visible');
    } else if (p.hiding) {
      promptEl.innerHTML = '<span class="key">E</span> Leave';
      promptEl.classList.add('visible');
    } else {
      promptEl.classList.remove('visible');
    }

    // toasts from C++ messages
    if (core.message !== 0 && core.message !== this.lastMessage) {
      this.lastMessage = core.message;
      this.toast(MESSAGES[core.message] || '');
    }
    if (core.message === 0) this.lastMessage = 0;

    // subtitles
    const sub = $('subtitle');
    if (core.subtitleTimer > 0 && SUBTITLES[core.subtitle]) {
      if (sub.textContent !== SUBTITLES[core.subtitle]) {
        sub.textContent = SUBTITLES[core.subtitle];
        sub.classList.add('visible');
      }
    } else {
      sub.classList.remove('visible');
    }

    // danger vignette
    document.body.classList.toggle('danger', core.danger > 0.55);

    // seen enemies for bestiary
    for (const e of core.enemies()) this.seenEnemies.add(Math.round(e.type));
  }

  toast(text, ms = 4200) {
    if (!text) return;
    const host = $('toasts');
    const el = document.createElement('div');
    el.className = 'toast';
    el.textContent = text;
    host.appendChild(el);
    setTimeout(() => { el.classList.add('out'); }, ms);
    setTimeout(() => { el.remove(); }, ms + 800);
  }

  roomBanner(nameId) {
    const room = ROOMS[nameId] || { name: '—', sub: '' };
    const el = $('room-title');
    el.innerHTML = `<div class="room-name">${room.name}</div>${room.sub ? `<div class="room-sub">${room.sub}</div>` : ''}`;
    el.classList.remove('visible');
    void el.offsetWidth;
    el.classList.add('visible');
    clearTimeout(this.roomTimer);
    this.roomTimer = setTimeout(() => el.classList.remove('visible'), 3600);
  }

  // ------------------------------------------------------------ inventory
  buildInventory() {
    const grid = $('inv-grid');
    grid.innerHTML = '';
    for (let i = 0; i < 16; i++) {
      const cell = document.createElement('button');
      cell.className = 'inv-cell';
      cell.addEventListener('click', () => { this.invSelected = i; this.refreshInventory(); });
      grid.appendChild(cell);
    }
    $('inv-use').addEventListener('click', () => {
      const inv = this.core.inventory();
      const item = inv[this.invSelected];
      if (!item || !item.usable) return;
      this.core.useItem(item.id);
      this.refreshInventory();
    });
    $('inv-close').addEventListener('click', () => this.hooks.closePanel?.());
  }

  refreshInventory() {
    const inv = this.core.inventory();
    const cells = $('inv-grid').children;
    for (let i = 0; i < cells.length; i++) {
      const cell = cells[i];
      const item = inv[i];
      cell.className = 'inv-cell' + (i === this.invSelected ? ' selected' : '');
      if (!item) {
        cell.innerHTML = '';
        cell.disabled = true;
        continue;
      }
      cell.disabled = false;
      const def = ITEMS[item.id] || { name: 'Unknown' };
      const icon = CAT_TO_ICON[item.cat] || 'none';
      cell.innerHTML = `<span class="inv-icon">${ICONS[icon]}</span><span class="inv-qty">${item.qty > 1 ? item.qty : ''}</span><span class="inv-label">${def.name}</span>`;
    }
    const sel = inv[this.invSelected];
    const detail = $('inv-detail');
    const useBtn = $('inv-use');
    if (sel) {
      const def = ITEMS[sel.id] || { name: 'Unknown', desc: '' };
      detail.innerHTML = `<h3>${def.name}</h3><p>${def.desc}</p>`;
      useBtn.disabled = !sel.usable;
      useBtn.classList.toggle('disabled', !sel.usable);
    } else {
      detail.innerHTML = '<h3>Empty</h3><p>Nothing here but the shape of your pockets.</p>';
      useBtn.disabled = true;
      useBtn.classList.add('disabled');
    }
  }

  // --------------------------------------------------------------- journal
  buildJournalTabs() {
    document.querySelectorAll('.journal-tab').forEach((tab) => {
      tab.addEventListener('click', () => {
        this.journalTab = tab.dataset.tab;
        this.refreshJournal();
      });
    });
    $('journal-close').addEventListener('click', () => this.hooks.closePanel?.());
    $('map-canvas').width = 480;
    $('map-canvas').height = 300;
  }

  refreshJournal() {
    document.querySelectorAll('.journal-tab').forEach((t) => {
      t.classList.toggle('active', t.dataset.tab === this.journalTab);
    });
    $('journal-clue-pane').style.display = this.journalTab === 'journal' ? 'flex' : 'none';
    $('journal-map-pane').style.display = this.journalTab === 'map' ? 'flex' : 'none';
    $('journal-bestiary-pane').style.display = this.journalTab === 'bestiary' ? 'flex' : 'none';

    if (this.journalTab === 'journal') {
      const list = $('clue-list');
      const ids = this.core.clues();
      list.innerHTML = '';
      if (!ids.length) {
        list.innerHTML = '<div class="clue-empty">Nothing written yet. The building has more to say.</div>';
        $('clue-body').innerHTML = '';
        return;
      }
      ids.forEach((id, i) => {
        const clue = CLUES[id];
        if (!clue) return;
        const btn = document.createElement('button');
        btn.className = 'clue-item' + (i === this.clueSelected ? ' active' : '');
        btn.innerHTML = `<span class="clue-index">${String(i + 1).padStart(2, '0')}</span><span>${clue.title}</span>`;
        btn.addEventListener('click', () => { this.clueSelected = i; this.refreshJournal(); });
        list.appendChild(btn);
      });
      if (this.clueSelected == null || this.clueSelected >= ids.length) this.clueSelected = 0;
      const chosen = CLUES[ids[this.clueSelected]];
      $('clue-body').innerHTML = chosen
        ? `<h3>${chosen.title}</h3><div class="clue-meta">${ROOMS[chosen.room]?.name || 'Unknown'}</div><pre>${chosen.body}</pre>`
        : '';
    } else if (this.journalTab === 'map') {
      this.drawMap();
    } else {
      const host = $('bestiary-list');
      host.innerHTML = '';
      ENEMY_INFO.forEach((info, idx) => {
        const seen = this.seenEnemies.has(idx);
        const card = document.createElement('div');
        card.className = 'beast' + (seen ? '' : ' unknown');
        card.innerHTML = `<h3>${seen ? info.name : '???'}</h3><p>${seen ? info.desc : 'You have not met this one yet. It has met you.'}</p>`;
        host.appendChild(card);
      });
    }
  }

  drawMap() {
    const canvas = $('map-canvas');
    const ctx = canvas.getContext('2d');
    const w = canvas.width, h = canvas.height;
    ctx.clearRect(0, 0, w, h);
    ctx.fillStyle = '#07060a';
    ctx.fillRect(0, 0, w, h);
    // parchment grid
    ctx.strokeStyle = 'rgba(150,40,50,0.08)';
    for (let x = 0; x < w; x += 24) { ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, h); ctx.stroke(); }
    for (let y = 0; y < h; y += 24) { ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(w, y); ctx.stroke(); }

    const nodes = this.core.mapNodes();
    const edges = this.core.mapEdges();
    const scaleX = (w - 80) / 1100;
    const scaleY = (h - 60) / 260;
    const px = (n) => 40 + n.cx * scaleX;
    const py = (n) => 30 + (n.cy - 0) * scaleY;

    for (const e of edges) {
      const a = nodes.find((n) => n.id === e.a);
      const b = nodes.find((n) => n.id === e.b);
      if (!a || !b) continue;
      const known = this.core.visited(a.id) && this.core.visited(b.id);
      ctx.strokeStyle = known ? (e.open ? 'rgba(200,180,140,0.35)' : 'rgba(140,40,50,0.5)') : 'rgba(90,80,70,0.12)';
      ctx.setLineDash(e.open ? [] : [3, 4]);
      ctx.beginPath();
      ctx.moveTo(px(a), py(a));
      ctx.lineTo(px(b), py(b));
      ctx.stroke();
    }
    ctx.setLineDash([]);

    for (const n of nodes) {
      const known = this.core.visited(n.id);
      const current = n.id === this.core.roomId;
      ctx.fillStyle = current ? '#b8232c' : known ? 'rgba(210,190,150,0.75)' : 'rgba(90,80,70,0.25)';
      ctx.beginPath();
      ctx.arc(px(n), py(n), current ? 5 : 3.4, 0, Math.PI * 2);
      ctx.fill();
      if (known) {
        const room = ROOMS[n.nameId];
        ctx.fillStyle = current ? 'rgba(240,210,190,0.95)' : 'rgba(190,175,150,0.55)';
        ctx.font = '9px monospace';
        ctx.fillText(room ? room.name : '—', px(n) + 7, py(n) + 3);
      }
    }
    ctx.fillStyle = 'rgba(160,140,120,0.5)';
    ctx.font = '9px monospace';
    ctx.fillText('ASHGROVE — west wing (recovered plan)', 12, h - 12);
  }

  // ---------------------------------------------------------------- keypad
  buildKeypad() {
    const pad = $('code-pad');
    pad.innerHTML = '';
    for (const label of ['1', '2', '3', '4', '5', '6', '7', '8', '9', 'CLR', '0', 'UNDO']) {
      const btn = document.createElement('button');
      btn.textContent = label;
      btn.className = 'key' + (label === 'CLR' || label === 'UNDO' ? ' small' : '');
      btn.addEventListener('click', () => {
        if (label === 'CLR') this.core.codeReset();
        else if (label === 'UNDO') this.core.puzzleUndo();
        else this.core.codeDigit(Number(label));
        this.refreshKeypad();
        const st = this.core.codeState();
        if (st.solved) setTimeout(() => this.hooks.closePanel?.(), 600);
      });
      pad.appendChild(btn);
    }
    $('code-close').addEventListener('click', () => this.hooks.closePanel?.());
  }

  refreshKeypad() {
    const st = this.core.codeState();
    const host = $('code-display');
    host.innerHTML = '';
    for (let i = 0; i < 4; i++) {
      const digit = document.createElement('span');
      digit.className = 'code-digit';
      const filled = i < st.progress;
      digit.textContent = filled ? String(st.digits[i]) : '·';
      if (filled) digit.classList.add('filled');
      host.appendChild(digit);
    }
  }

  buildCredits() {
    $('credits-roll').innerHTML = CREDITS.map((line) => `<div>${line || '&nbsp;'}</div>`).join('');
    $('btn-credits-back').addEventListener('click', () => this.hooks.backFromCredits?.());
  }

  bindButtons() {
    $('btn-new').addEventListener('click', () => this.hooks.newGame?.());
    $('btn-continue').addEventListener('click', () => this.hooks.continueGame?.());
    $('btn-settings').addEventListener('click', () => this.openSettings('menu'));
    $('btn-credits').addEventListener('click', () => this.openCredits());
    $('btn-quit').addEventListener('click', () => this.showQuit());
    $('btn-pause-settings').addEventListener('click', () => this.openSettings('pause'));
    $('btn-settings-back').addEventListener('click', () => {
      if (this.settingsBack === 'menu') this.enterMenu(this.hooks.hasSave?.());
      else this.openPause();
    });
    $('btn-resume').addEventListener('click', () => this.hooks.closePanel?.());
    $('btn-pause-title').addEventListener('click', () => this.hooks.saveAndTitle?.());
    $('btn-restart').addEventListener('click', () => this.hooks.restart?.());
    $('btn-death-menu').addEventListener('click', () => this.hooks.toTitle?.());
    $('btn-ending-menu').addEventListener('click', () => this.hooks.toTitle?.());
    $('btn-ending-credits').addEventListener('click', () => this.openCredits());
    $('btn-quit-back').addEventListener('click', () => this.enterBoot());
    $('inventory-close-x')?.addEventListener('click', () => this.hooks.closePanel?.());
    $('journal-close-x')?.addEventListener('click', () => this.hooks.closePanel?.());
  }

  deathStats(core) {
    return `Days survived: <b>one</b><br>Clues recovered: <b>${core.clues().length}</b><br>Things put down: <b>${core.kills}</b><br>Deaths in the dream: <b>${core.deaths}</b>`;
  }
  endingStats(core) {
    return `Clues recovered: <b>${core.clues().length} / 18</b><br>Secrets found: <b>${core.secrets}</b><br>Things put down: <b>${core.kills}</b><br>Deaths in the dream: <b>${core.deaths}</b><br>Candles lit: <b>${core.checkpoints}</b>`;
  }
}
