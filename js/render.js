// render.js - the 2D renderer. Everything is drawn procedurally at a 480x270
// internal resolution and upscaled, giving hand-made pixel-art weight without
// any stock imagery. The C++ core is the only source of game state.

export const VIEW_W = 480;
export const VIEW_H = 270;

const PALETTES = [
  // 0 ward 3
  { bg: '#07090a', wall: '#3f4d47', wallDark: '#2a342f', wallLight: '#56655c', floor: '#39463f', floorAlt: '#2f3a34', accent: '#8a5a44', fog: 'rgba(20,32,30,0.55)' },
  // 1 corridor
  { bg: '#080706', wall: '#4b3a33', wallDark: '#33251f', wallLight: '#63504a', floor: '#33251f', floorAlt: '#2a1e19', accent: '#7d2b2f', fog: 'rgba(28,18,16,0.6)' },
  // 2 nurse station
  { bg: '#070908', wall: '#47534b', wallDark: '#303a34', wallLight: '#5d6b61', floor: '#3c4740', floorAlt: '#333d37', accent: '#7e3b3b', fog: 'rgba(18,28,26,0.5)' },
  // 3 supply closet
  { bg: '#060504', wall: '#3b3029', wallDark: '#281f1a', wallLight: '#4f4238', floor: '#2f2a24', floorAlt: '#27231e', accent: '#8a6a3a', fog: 'rgba(20,16,12,0.7)' },
  // 4 boiler
  { bg: '#070605', wall: '#37302a', wallDark: '#241f1b', wallLight: '#4b423a', floor: '#2b2823', floorAlt: '#232019', accent: '#a05a2a', fog: 'rgba(26,18,12,0.6)' },
  // 5 flooded passage
  { bg: '#040707', wall: '#26302e', wallDark: '#182120', wallLight: '#33403d', floor: '#1f2a29', floorAlt: '#1a2423', accent: '#3e6f66', fog: 'rgba(10,22,22,0.72)' },
  // 6 east corridor
  { bg: '#060504', wall: '#3d2b26', wallDark: '#291c18', wallLight: '#523b34', floor: '#2c211d', floorAlt: '#241b17', accent: '#8d3324', fog: 'rgba(24,14,12,0.62)' },
  // 7 chapel
  { bg: '#070406', wall: '#3f2429', wallDark: '#2a171b', wallLight: '#55333a', floor: '#332628', floorAlt: '#2a1f21', accent: '#8c2530', fog: 'rgba(32,14,18,0.58)' },
  // 8 morgue
  { bg: '#060808', wall: '#3d4642', wallDark: '#28302d', wallLight: '#55605b', floor: '#36403c', floorAlt: '#2e3734', accent: '#5f7a74', fog: 'rgba(16,26,26,0.55)' },
  // 9 sleep chamber
  { bg: '#080304', wall: '#2d1116', wallDark: '#1c0a0e', wallLight: '#451a21', floor: '#241114', floorAlt: '#1c0d10', accent: '#a1161f', fog: 'rgba(40,8,12,0.62)' }
];

const T = { VOID: 0, WALL: 1, WOOD: 2, STONE: 3, TILE: 4, CARPET: 5, WATER: 6, BLOOD: 7, RUBBLE: 8, GRASS: 9, STAIRS: 10, METAL: 11 };
const TILE = 32;

function hash(x, y, s = 0) {
  let h = (x * 374761393 + y * 668265263 + s * 2147483647) | 0;
  h = (h ^ (h >>> 13)) * 1274126177;
  h = h ^ (h >>> 16);
  return ((h >>> 0) % 10000) / 10000;
}

function clamp(v, a, b) { return v < a ? a : v > b ? b : v; }

export class Renderer {
  constructor(canvas) {
    this.canvas = canvas;
    this.ctx = canvas.getContext('2d');
    this.lightCanvas = document.createElement('canvas');
    this.lightCanvas.width = VIEW_W;
    this.lightCanvas.height = VIEW_H;
    this.lctx = this.lightCanvas.getContext('2d');
    this.grain = this.makeGrain();
    this.vignette = this.makeVignette();
    this.camX = 0;
    this.camY = 0;
    this.shakeX = 0;
    this.shakeY = 0;
    this.shakeT = 0;
    this.shakePower = 0;
    this.particles = [];
    this.motes = [];
    this.flash = 0;
    this.flashColor = '#ff2a2a';
    this.distort = 0;
    this.time = 0;
    this.lastRoom = -1;
    this.footprints = [];
    for (let i = 0; i < 70; i++) {
      this.motes.push({
        x: Math.random() * VIEW_W, y: Math.random() * VIEW_H,
        vx: (Math.random() - 0.5) * 4, vy: -2 - Math.random() * 6,
        r: 0.4 + Math.random() * 1.1, a: 0.05 + Math.random() * 0.18
      });
    }
  }

  resize() {
    const vw = window.innerWidth, vh = window.innerHeight;
    const scale = Math.max(1, Math.min(vw / VIEW_W, vh / VIEW_H));
    this.canvas.width = VIEW_W;
    this.canvas.height = VIEW_H;
    this.canvas.style.width = `${Math.floor(VIEW_W * scale)}px`;
    this.canvas.style.height = `${Math.floor(VIEW_H * scale)}px`;
    this.dpr = 1;
  }

  makeGrain() {
    const c = document.createElement('canvas');
    c.width = 128; c.height = 128;
    const g = c.getContext('2d');
    const img = g.createImageData(128, 128);
    for (let i = 0; i < img.data.length; i += 4) {
      const v = Math.random() * 255;
      img.data[i] = v; img.data[i + 1] = v; img.data[i + 2] = v;
      img.data[i + 3] = 22 + Math.random() * 26;
    }
    g.putImageData(img, 0, 0);
    return c;
  }

  makeVignette() {
    const c = document.createElement('canvas');
    c.width = VIEW_W; c.height = VIEW_H;
    const g = c.getContext('2d');
    const grad = g.createRadialGradient(VIEW_W / 2, VIEW_H / 2, VIEW_H * 0.28, VIEW_W / 2, VIEW_H / 2, VIEW_H * 0.82);
    grad.addColorStop(0, 'rgba(0,0,0,0)');
    grad.addColorStop(0.55, 'rgba(0,0,0,0.35)');
    grad.addColorStop(1, 'rgba(0,0,0,0.92)');
    g.fillStyle = grad;
    g.fillRect(0, 0, VIEW_W, VIEW_H);
    return c;
  }

  shake(power, duration) {
    this.shakePower = Math.max(this.shakePower, power);
    this.shakeT = Math.max(this.shakeT, duration);
  }

  flashScreen(alpha, color = '#ff2a2a') {
    this.flash = Math.max(this.flash, alpha);
    this.flashColor = color;
  }

  // --------------------------------------------------------------- main draw
  draw(core, opts, dt) {
    this.time += dt;
    const ctx = this.ctx;
    const pal = PALETTES[clamp(core.palette, 0, PALETTES.length - 1)];
    const player = core.player();
    const roomW = 40 * TILE, roomH = 22 * TILE;

    // camera
    let targetX = player.x - VIEW_W / 2;
    let targetY = player.y - VIEW_H / 2;
    targetX = clamp(targetX, 0, roomW - VIEW_W);
    targetY = clamp(targetY, 0, roomH - VIEW_H);
    if (this.lastRoom !== core.roomId) { this.camX = targetX; this.camY = targetY; this.lastRoom = core.roomId; }
    const lerp = 1 - Math.pow(0.0016, dt);
    this.camX += (targetX - this.camX) * lerp;
    this.camY += (targetY - this.camY) * lerp;

    if (this.shakeT > 0) {
      this.shakeT -= dt;
      this.shakeX = (Math.random() - 0.5) * this.shakePower;
      this.shakeY = (Math.random() - 0.5) * this.shakePower;
      this.shakePower *= 0.90;
    } else { this.shakeX = 0; this.shakeY = 0; this.shakePower = 0; }

    const camX = Math.round(this.camX + this.shakeX);
    const camY = Math.round(this.camY + this.shakeY);

    ctx.setTransform(1, 0, 0, 1, 0, 0);
    ctx.fillStyle = pal.bg;
    ctx.fillRect(0, 0, VIEW_W, VIEW_H);

    const tiles = core.tiles();
    const decor = core.decor();
    const startCX = Math.max(0, Math.floor(camX / TILE));
    const startCY = Math.max(0, Math.floor(camY / TILE));
    const endCX = Math.min(39, Math.ceil((camX + VIEW_W) / TILE));
    const endCY = Math.min(21, Math.ceil((camY + VIEW_H) / TILE));

    // ---- floor
    for (let cy = startCY; cy <= endCY; cy++) {
      for (let cx = startCX; cx <= endCX; cx++) {
        const idx = cy * 40 + cx;
        const t = tiles[idx];
        const sx = cx * TILE - camX;
        const sy = cy * TILE - camY;
        this.drawFloor(ctx, t, sx, sy, decor[idx], pal, cx, cy);
      }
    }

    // ---- blood pools / water shimmer are part of the floor pass

    // ---- props, items, enemies, player are y-sorted
    const props = core.props();
    const items = core.items();
    const doors = core.doors();
    const enemies = core.enemies();
    const drawList = [];

    for (const p of props) drawList.push({ kind: 'prop', y: p.y, data: p });
    for (const it of items) drawList.push({ kind: 'item', y: it.y, data: it });
    for (const e of enemies) drawList.push({ kind: 'enemy', y: e.y, data: e });
    drawList.push({ kind: 'player', y: player.y, data: player });

    // walls are drawn after floors but before entities, with a face + shadow
    for (let cy = startCY; cy <= endCY; cy++) {
      for (let cx = startCX; cx <= endCX; cx++) {
        const idx = cy * 40 + cx;
        const t = tiles[idx];
        if (t !== T.WALL && t !== T.VOID && t !== T.RUBBLE) continue;
        const sx = cx * TILE - camX;
        const sy = cy * TILE - camY;
        if (t === T.WALL) this.drawWall(ctx, sx, sy, decor[idx], pal, cx, cy, tiles);
        else if (t === T.RUBBLE) this.drawRubble(ctx, sx, sy, decor[idx], pal, cx, cy);
        else {
          ctx.fillStyle = '#040305';
          ctx.fillRect(sx, sy, TILE, TILE);
        }
      }
    }

    // doors drawn over wall gaps
    for (const d of doors) this.drawDoor(ctx, d, camX, camY, pal, this.time);

    drawList.sort((a, b) => a.y - b.y);
    for (const entry of drawList) {
      if (entry.kind === 'prop') this.drawProp(ctx, entry.data, camX, camY, pal, this.time, core);
      else if (entry.kind === 'item') this.drawItem(ctx, entry.data, camX, camY, this.time);
      else if (entry.kind === 'enemy') this.drawEnemy(ctx, entry.data, camX, camY, this.time);
      else this.drawPlayer(ctx, entry.data, camX, camY, this.time);
    }

    // ---- lighting
    this.drawLighting(core, player, camX, camY, dt);

    // ---- atmosphere
    this.drawMotes(ctx, dt);
    this.drawFog(ctx, pal, player, core);

    // ---- post
    ctx.drawImage(this.vignette, 0, 0);

    if (opts.grain !== false) {
      ctx.globalAlpha = 0.10;
      const gx = -Math.random() * 64, gy = -Math.random() * 64;
      ctx.drawImage(this.grain, gx, gy);
      ctx.drawImage(this.grain, gx + 64, gy);
      ctx.drawImage(this.grain, gx, gy + 64);
      ctx.drawImage(this.grain, gx + 64, gy + 64);
      ctx.globalAlpha = 1;
    }

    if (this.flash > 0) {
      ctx.globalAlpha = this.flash * 0.55;
      ctx.fillStyle = this.flashColor;
      ctx.fillRect(0, 0, VIEW_W, VIEW_H);
      ctx.globalAlpha = 1;
      this.flash *= 0.90;
    }

    // low health: crimson pulse at the edges
    const hpFrac = player.hp / player.hpMax;
    if (hpFrac < 0.45) {
      const pulse = (0.25 + Math.sin(this.time * 4) * 0.08) * (0.45 - hpFrac) * 2.2;
      const grad = ctx.createRadialGradient(VIEW_W / 2, VIEW_H / 2, VIEW_H * 0.3, VIEW_W / 2, VIEW_H / 2, VIEW_H * 0.72);
      grad.addColorStop(0, 'rgba(120,0,0,0)');
      grad.addColorStop(1, `rgba(140,6,10,${clamp(pulse, 0, 0.5)})`);
      ctx.fillStyle = grad;
      ctx.fillRect(0, 0, VIEW_W, VIEW_H);
    }

    // chase distortion
    if (core.danger > 0.65 && Math.random() < 0.18) {
      const sliceY = Math.random() * VIEW_H;
      const h = 2 + Math.random() * 6;
      const off = (Math.random() - 0.5) * 6;
      try {
        const slice = ctx.getImageData(0, sliceY, VIEW_W, h);
        ctx.putImageData(slice, off, sliceY);
      } catch (e) { /* ignore */ }
    }
  }

  // ---------------------------------------------------------------- floors
  drawFloor(ctx, t, x, y, d, pal, cx, cy) {
    const h = hash(cx, cy, 3);
    switch (t) {
      case T.VOID:
        ctx.fillStyle = '#040305';
        ctx.fillRect(x, y, TILE, TILE);
        return;
      case T.WALL:
      case T.RUBBLE:
        ctx.fillStyle = pal.floorAlt;
        ctx.fillRect(x, y, TILE, TILE);
        return;
      case T.WOOD: {
        ctx.fillStyle = h > 0.5 ? pal.floor : pal.floorAlt;
        ctx.fillRect(x, y, TILE, TILE);
        ctx.strokeStyle = 'rgba(0,0,0,0.34)';
        ctx.lineWidth = 1;
        for (let i = 0; i < 4; i++) {
          const yy = y + i * 8 + (h > 0.5 ? 0 : 4);
          ctx.beginPath(); ctx.moveTo(x, yy + 0.5); ctx.lineTo(x + TILE, yy + 0.5); ctx.stroke();
        }
        if (h > 0.72) {
          ctx.fillStyle = 'rgba(0,0,0,0.22)';
          ctx.fillRect(x + 6 + h * 12, y + 10, 10, 2);
        }
        if (h < 0.18) {
          ctx.fillStyle = 'rgba(255,220,180,0.035)';
          ctx.fillRect(x + 4, y + 4, 12, 6);
        }
        break;
      }
      case T.STONE: {
        ctx.fillStyle = h > 0.45 ? pal.floor : pal.floorAlt;
        ctx.fillRect(x, y, TILE, TILE);
        ctx.strokeStyle = 'rgba(0,0,0,0.45)';
        ctx.lineWidth = 1;
        ctx.strokeRect(x + 0.5, y + 0.5, TILE - 1, TILE - 1);
        if (h > 0.6) {
          ctx.strokeStyle = 'rgba(0,0,0,0.5)';
          ctx.beginPath();
          ctx.moveTo(x + 4, y + 8); ctx.lineTo(x + 12, y + 14); ctx.lineTo(x + 9, y + 24);
          ctx.stroke();
        }
        if (h < 0.2) {
          ctx.fillStyle = 'rgba(120,140,120,0.05)';
          ctx.fillRect(x + 18, y + 18, 10, 8);
        }
        break;
      }
      case T.TILE: {
        ctx.fillStyle = h > 0.5 ? pal.floor : pal.floorAlt;
        ctx.fillRect(x, y, TILE, TILE);
        ctx.strokeStyle = 'rgba(0,0,0,0.40)';
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.moveTo(x, y + 16.5); ctx.lineTo(x + TILE, y + 16.5);
        ctx.moveTo(x + 16.5, y); ctx.lineTo(x + 16.5, y + TILE);
        ctx.stroke();
        if (h > 0.78) {
          ctx.fillStyle = 'rgba(80,60,50,0.25)';
          ctx.fillRect(x + 2, y + 20, 14, 8);
        }
        break;
      }
      case T.CARPET: {
        ctx.fillStyle = '#4a1a1e';
        ctx.fillRect(x, y, TILE, TILE);
        ctx.fillStyle = h > 0.5 ? 'rgba(120,30,34,0.30)' : 'rgba(60,14,18,0.4)';
        ctx.fillRect(x + 1, y + 1, TILE - 2, TILE - 2);
        ctx.strokeStyle = 'rgba(150,110,60,0.10)';
        ctx.strokeRect(x + 4.5, y + 4.5, TILE - 9, TILE - 9);
        break;
      }
      case T.WATER: {
        ctx.fillStyle = '#16211f';
        ctx.fillRect(x, y, TILE, TILE);
        const wob = Math.sin(this.time * 1.6 + cx * 0.7 + cy * 0.4);
        ctx.strokeStyle = `rgba(120,190,180,${0.05 + 0.04 * wob})`;
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.moveTo(x, y + 10 + wob * 3);
        ctx.quadraticCurveTo(x + 16, y + 6 + wob * 5, x + TILE, y + 12 + wob * 2);
        ctx.stroke();
        if (h > 0.8) {
          ctx.fillStyle = 'rgba(255,255,255,0.04)';
          ctx.fillRect(x + 6, y + 18, 8, 1);
        }
        break;
      }
      case T.BLOOD: {
        ctx.fillStyle = '#3d0d12';
        ctx.fillRect(x, y, TILE, TILE);
        ctx.fillStyle = h > 0.5 ? 'rgba(90,12,18,0.5)' : 'rgba(40,6,10,0.55)';
        ctx.beginPath();
        ctx.ellipse(x + 16, y + 16, 13 + h * 4, 11 + h * 3, h * 3, 0, Math.PI * 2);
        ctx.fill();
        ctx.fillStyle = 'rgba(200,60,60,0.05)';
        ctx.fillRect(x + 8, y + 12, 6, 2);
        break;
      }
      case T.METAL: {
        ctx.fillStyle = h > 0.5 ? '#2e3330' : '#282d2b';
        ctx.fillRect(x, y, TILE, TILE);
        ctx.strokeStyle = 'rgba(0,0,0,0.5)';
        ctx.lineWidth = 1;
        for (let i = 0; i < 4; i++) {
          ctx.beginPath(); ctx.moveTo(x, y + i * 8 + 0.5); ctx.lineTo(x + TILE, y + i * 8 + 0.5); ctx.stroke();
        }
        ctx.fillStyle = 'rgba(255,255,255,0.05)';
        ctx.fillRect(x + 2, y + 2, 28, 1);
        break;
      }
      case T.GRASS: {
        ctx.fillStyle = '#18231a';
        ctx.fillRect(x, y, TILE, TILE);
        for (let i = 0; i < 5; i++) {
          const gx = x + hash(cx + i, cy, 9) * 30;
          const gy = y + 8 + hash(cx, cy + i, 7) * 22;
          ctx.strokeStyle = 'rgba(70,95,60,0.35)';
          ctx.beginPath(); ctx.moveTo(gx, gy); ctx.lineTo(gx + 1, gy - 5); ctx.stroke();
        }
        break;
      }
      case T.STAIRS: {
        ctx.fillStyle = '#232323';
        ctx.fillRect(x, y, TILE, TILE);
        for (let i = 0; i < 4; i++) {
          ctx.fillStyle = i % 2 === 0 ? 'rgba(255,255,255,0.045)' : 'rgba(0,0,0,0.35)';
          ctx.fillRect(x, y + i * 8, TILE, 4);
        }
        break;
      }
      default: {
        ctx.fillStyle = pal.floor;
        ctx.fillRect(x, y, TILE, TILE);
      }
    }
  }

  drawWall(ctx, x, y, d, pal, cx, cy, tiles) {
    const belowIsWall = cy + 1 < 22 && tiles[(cy + 1) * 40 + cx] === T.WALL;
    ctx.fillStyle = pal.wall;
    ctx.fillRect(x, y, TILE, TILE);
    // vertical panel seams + peeling wallpaper
    ctx.strokeStyle = pal.wallDark;
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(x + 0.5, y); ctx.lineTo(x + 0.5, y + TILE);
    ctx.stroke();
    if (d > 0.55) {
      ctx.fillStyle = 'rgba(0,0,0,0.18)';
      const px = x + 4 + (d * 20);
      ctx.beginPath();
      ctx.moveTo(px, y + 6);
      ctx.lineTo(px + 6, y + 18);
      ctx.lineTo(px - 2, y + 26);
      ctx.lineTo(px - 8, y + 14);
      ctx.closePath();
      ctx.fill();
    }
    if (d < 0.2) {
      ctx.fillStyle = 'rgba(120,140,120,0.06)';
      ctx.fillRect(x + 8, y + 6, 14, 10);
    }
    // top highlight
    ctx.fillStyle = pal.wallLight;
    ctx.fillRect(x, y, TILE, 2);
    // bottom grime + shadow spilling onto the floor
    ctx.fillStyle = 'rgba(0,0,0,0.28)';
    ctx.fillRect(x, y + TILE - 5, TILE, 5);
    if (!belowIsWall) {
      const grad = ctx.createLinearGradient(0, y + TILE, 0, y + TILE + 14);
      grad.addColorStop(0, 'rgba(0,0,0,0.45)');
      grad.addColorStop(1, 'rgba(0,0,0,0)');
      ctx.fillStyle = grad;
      ctx.fillRect(x, y + TILE, TILE, 14);
    }
  }

  drawRubble(ctx, x, y, d, pal, cx, cy) {
    ctx.fillStyle = pal.floorAlt;
    ctx.fillRect(x, y, TILE, TILE);
    const stones = 4 + Math.floor(d * 4);
    for (let i = 0; i < stones; i++) {
      const sx = x + 2 + hash(cx + i, cy, 11) * 26;
      const sy = y + 4 + hash(cx, cy + i, 13) * 22;
      const w = 5 + hash(cx + i, cy + i, 17) * 9;
      const hh = 4 + hash(cx + i, cy, 19) * 7;
      ctx.fillStyle = i % 2 ? '#3a3630' : '#2d2a25';
      ctx.fillRect(sx, sy, w, hh);
      ctx.fillStyle = 'rgba(255,255,255,0.06)';
      ctx.fillRect(sx, sy, w, 1);
    }
    ctx.strokeStyle = 'rgba(0,0,0,0.6)';
    ctx.beginPath(); ctx.moveTo(x + 2, y + 2); ctx.lineTo(x + 30, y + 30); ctx.stroke();
  }

  // ----------------------------------------------------------------- doors
  drawDoor(ctx, d, camX, camY, pal, time) {
    const x = d.x - camX, y = d.y - camY;
    if (x < -48 || x > VIEW_W + 48 || y < -48 || y > VIEW_H + 48) return;
    const w = 30, h = 44;
    ctx.fillStyle = '#171310';
    ctx.fillRect(x - w / 2, y - h / 2 - 4, w, h);
    if (d.open) {
      const grad = ctx.createLinearGradient(0, y - h / 2, 0, y + h / 2);
      grad.addColorStop(0, 'rgba(120,90,60,0.20)');
      grad.addColorStop(1, 'rgba(5,4,4,0.95)');
      ctx.fillStyle = grad;
      ctx.fillRect(x - w / 2 + 2, y - h / 2 - 2, w - 4, h - 2);
      ctx.fillStyle = 'rgba(0,0,0,0.7)';
      ctx.fillRect(x - w / 2, y - h / 2, 5, h);
    } else {
      const grad = ctx.createLinearGradient(x - w / 2, 0, x + w / 2, 0);
      grad.addColorStop(0, '#2a1f19');
      grad.addColorStop(0.5, '#3a2b22');
      grad.addColorStop(1, '#241a15');
      ctx.fillStyle = grad;
      ctx.fillRect(x - w / 2, y - h / 2, w, h);
      ctx.strokeStyle = 'rgba(0,0,0,0.5)';
      ctx.strokeRect(x - w / 2 + 2.5, y - h / 2 + 2.5, w - 5, h - 5);
      ctx.fillStyle = '#b39a52';
      ctx.beginPath();
      ctx.arc(x + w / 2 - 7, y + 2, 1.8, 0, Math.PI * 2);
      ctx.fill();
      if (d.locked) {
        ctx.fillStyle = '#8f8f8f';
        ctx.fillRect(x - 3, y - 6, 7, 6);
        ctx.strokeStyle = '#c9c9c9';
        ctx.lineWidth = 1;
        ctx.beginPath(); ctx.arc(x + 0.5, y - 7, 3, Math.PI, 0); ctx.stroke();
        ctx.fillStyle = '#2a2a2a';
        ctx.fillRect(x + 0.5, y - 4, 1.5, 3);
      }
    }
    // frame
    ctx.strokeStyle = 'rgba(0,0,0,0.55)';
    ctx.lineWidth = 2;
    ctx.strokeRect(x - w / 2 - 1, y - h / 2 - 5, w + 2, h + 4);
  }

  // ------------------------------------------------------------------ props
  drawProp(ctx, p, camX, camY, pal, time, core) {
    const x = Math.round(p.x - camX);
    const y = Math.round(p.y - camY);
    if (x < -120 || x > VIEW_W + 120 || y < -120 || y > VIEW_H + 120) return;
    const w = p.w, h = p.h;
    const flick = 0.75 + 0.25 * Math.sin(time * 9 + p.x);

    switch (p.type) {
      case 3: { // bed
        ctx.fillStyle = '#20201d';
        ctx.fillRect(x - w / 2, y - h / 2, w, h);
        ctx.fillStyle = '#4a4a42';
        ctx.fillRect(x - w / 2 + 2, y - h / 2 + 3, w - 4, h - 6);
        ctx.fillStyle = '#6a6459';
        ctx.fillRect(x - w / 2 + 4, y - h / 2 + 5, w - 8, 16);
        ctx.fillStyle = 'rgba(90,20,24,0.35)';
        ctx.fillRect(x - w / 2 + 6, y + 2, w - 14, 10);
        ctx.strokeStyle = '#15140f';
        ctx.beginPath();
        ctx.moveTo(x - w / 2, y - h / 2); ctx.lineTo(x - w / 2, y + h / 2);
        ctx.moveTo(x + w / 2, y - h / 2); ctx.lineTo(x + w / 2, y + h / 2);
        ctx.stroke();
        break;
      }
      case 4: { // locker / hiding spot
        ctx.fillStyle = '#2f3634';
        ctx.fillRect(x - w / 2, y - h / 2, w, h);
        ctx.fillStyle = 'rgba(0,0,0,0.45)';
        ctx.fillRect(x - w / 2 + 3, y - h / 2 + 3, w - 6, h - 6);
        ctx.strokeStyle = 'rgba(160,170,165,0.12)';
        for (let i = 0; i < 5; i++) {
          const ly = y - h / 2 + 8 + i * 5;
          ctx.beginPath(); ctx.moveTo(x - w / 2 + 7, ly); ctx.lineTo(x + w / 2 - 7, ly); ctx.stroke();
        }
        ctx.fillStyle = '#8a8f7a';
        ctx.fillRect(x + w / 2 - 8, y, 2, 6);
        if (p.state === 1) {
          ctx.fillStyle = 'rgba(0,0,0,0.85)';
          ctx.fillRect(x - w / 2 + 2, y - h / 2 + 2, w - 4, h - 4);
        }
        break;
      }
      case 5: { // mirror
        ctx.fillStyle = '#2b241c';
        ctx.beginPath();
        ctx.ellipse(x, y, w / 2, h / 2, 0, 0, Math.PI * 2);
        ctx.fill();
        const grad = ctx.createLinearGradient(x - w / 2, y - h / 2, x + w / 2, y + h / 2);
        grad.addColorStop(0, 'rgba(140,160,160,0.35)');
        grad.addColorStop(0.6, 'rgba(40,55,55,0.55)');
        grad.addColorStop(1, 'rgba(10,15,15,0.85)');
        ctx.fillStyle = grad;
        ctx.beginPath();
        ctx.ellipse(x, y, w / 2 - 4, h / 2 - 4, 0, 0, Math.PI * 2);
        ctx.fill();
        // cracks
        ctx.strokeStyle = 'rgba(220,230,230,0.16)';
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.moveTo(x - 10, y - 20); ctx.lineTo(x - 2, y - 2); ctx.lineTo(x - 8, y + 14);
        ctx.moveTo(x - 2, y - 2); ctx.lineTo(x + 12, y - 12);
        ctx.stroke();
        // a face that is only there when you look
        const looks = p.state === 1 ? 0.30 : 0.10;
        ctx.fillStyle = `rgba(180,160,150,${looks + 0.05 * Math.sin(time * 2)})`;
        ctx.beginPath();
        ctx.ellipse(x, y - 4, 7, 10, 0, 0, Math.PI * 2);
        ctx.fill();
        ctx.fillStyle = `rgba(10,4,4,${0.5 + looks})`;
        ctx.fillRect(x - 4, y - 7, 2, 2);
        ctx.fillRect(x + 2, y - 7, 2, 2);
        break;
      }
      case 6: { // breaker box
        ctx.fillStyle = '#2a2a26';
        ctx.fillRect(x - w / 2, y - h / 2, w, h);
        ctx.strokeStyle = '#151512';
        ctx.strokeRect(x - w / 2 + 1.5, y - h / 2 + 1.5, w - 3, h - 3);
        const mask = core.fuseMask;
        for (let i = 0; i < 3; i++) {
          const sy = y - h / 2 + 8 + i * 13;
          ctx.fillStyle = '#12120f';
          ctx.fillRect(x - w / 2 + 8, sy, w - 16, 9);
          if (mask & (1 << i)) {
            ctx.fillStyle = ['#4a7fd0', '#c04a4a', '#c0a24a'][i];
            ctx.fillRect(x - w / 2 + 10, sy + 2, w - 20, 5);
          }
        }
        ctx.fillStyle = 'rgba(255,190,90,0.6)';
        if (mask === 7) ctx.fillRect(x + w / 2 - 6, y - h / 2 + 4, 3, 3);
        break;
      }
      case 7: { // valve wheel
        ctx.fillStyle = '#2a2621';
        ctx.beginPath(); ctx.arc(x, y, 13, 0, Math.PI * 2); ctx.fill();
        ctx.save();
        ctx.translate(x, y);
        ctx.rotate(p.state === 1 ? Math.PI / 3 : 0);
        ctx.strokeStyle = p.state === 1 ? '#a8894a' : '#6d6257';
        ctx.lineWidth = 3;
        ctx.beginPath(); ctx.arc(0, 0, 11, 0, Math.PI * 2); ctx.stroke();
        for (let i = 0; i < 4; i++) {
          ctx.beginPath();
          ctx.moveTo(0, 0);
          ctx.lineTo(Math.cos(i * Math.PI / 2) * 11, Math.sin(i * Math.PI / 2) * 11);
          ctx.stroke();
        }
        ctx.restore();
        break;
      }
      case 8: { // code lock
        ctx.fillStyle = '#232523';
        ctx.fillRect(x - w / 2, y - h / 2, w, h);
        ctx.strokeStyle = '#0c0d0c';
        ctx.strokeRect(x - w / 2 + 1.5, y - h / 2 + 1.5, w - 3, h - 3);
        const c = core.codeState();
        for (let i = 0; i < 4; i++) {
          const bx = x - w / 2 + 6 + i * 12;
          ctx.fillStyle = '#0a0d0a';
          ctx.fillRect(bx, y - 12, 9, 13);
          ctx.fillStyle = i < c.progress ? '#7fd0a0' : '#1d2a20';
          ctx.fillRect(bx + 2, y - 9, 5, 7);
        }
        ctx.fillStyle = core.ambient > 20 ? '#7fd0a0' : '#20201c';
        ctx.fillRect(x - 6, y + 6, 12, 2);
        break;
      }
      case 9: { // drawer (morgue / cabinet)
        const open = p.state === 1;
        ctx.fillStyle = '#3a4240';
        ctx.fillRect(x - w / 2, y - h / 2, w, h);
        ctx.strokeStyle = '#1b2121';
        ctx.strokeRect(x - w / 2 + 1.5, y - h / 2 + 1.5, w - 3, h - 3);
        ctx.fillStyle = '#161a19';
        ctx.fillRect(x - w / 2 + 5, y - h / 2 + 5, w - 10, h - 10);
        if (open) {
          ctx.fillStyle = '#0a0c0c';
          ctx.fillRect(x - w / 2 + 3, y - h / 2 + 3, w - 6, h - 6);
          ctx.fillStyle = 'rgba(200,200,190,0.10)';
          ctx.fillRect(x - w / 2 + 6, y - h / 2 + 8, w - 12, 5);
          ctx.fillStyle = '#4a5350';
          ctx.fillRect(x - w / 2 + 1, y + 2, w - 2, 8);
        } else {
          ctx.fillStyle = '#8d918a';
          ctx.fillRect(x - 6, y - 2, 12, 3);
        }
        break;
      }
      case 2: { // candle (save point)
        ctx.fillStyle = '#2a231c';
        ctx.fillRect(x - 9, y - 4, 18, 10);
        ctx.fillStyle = '#d8cbb0';
        ctx.fillRect(x - 2, y - 14, 4, 11);
        const glow = ctx.createRadialGradient(x, y - 16, 0, x, y - 16, 26 * flick);
        glow.addColorStop(0, `rgba(255,190,90,${0.55 * flick})`);
        glow.addColorStop(1, 'rgba(255,140,50,0)');
        ctx.fillStyle = glow;
        ctx.beginPath(); ctx.arc(x, y - 16, 26 * flick, 0, Math.PI * 2); ctx.fill();
        ctx.fillStyle = '#ffd08a';
        ctx.beginPath();
        ctx.ellipse(x, y - 17 + Math.sin(time * 12) * 0.8, 1.6, 3.2, 0, 0, Math.PI * 2);
        ctx.fill();
        break;
      }
      case 1: { // note
        ctx.fillStyle = p.state === 1 ? '#5c5344' : '#b9ad92';
        ctx.save();
        ctx.translate(x, y);
        ctx.rotate(-0.12);
        ctx.fillRect(-6, -8, 12, 16);
        ctx.fillStyle = 'rgba(60,40,30,0.55)';
        for (let i = 0; i < 4; i++) ctx.fillRect(-4, -5 + i * 3, 8, 1);
        ctx.restore();
        break;
      }
      case 10: { // ritual candle
        ctx.fillStyle = '#1e1a16';
        ctx.fillRect(x - 8, y - 6, 16, 12);
        ctx.fillStyle = '#cfc0a2';
        ctx.fillRect(x - 3, y - 26, 6, 22);
        if (p.state === 1) {
          const glow = ctx.createRadialGradient(x, y - 28, 0, x, y - 28, 42 * flick);
          glow.addColorStop(0, `rgba(255,150,70,${0.5 * flick})`);
          glow.addColorStop(1, 'rgba(255,80,30,0)');
          ctx.fillStyle = glow;
          ctx.beginPath(); ctx.arc(x, y - 28, 42 * flick, 0, Math.PI * 2); ctx.fill();
          ctx.fillStyle = '#ffdb9a';
          ctx.beginPath();
          ctx.ellipse(x, y - 29 + Math.sin(time * 11 + p.id) * 0.9, 2, 4, 0, 0, Math.PI * 2);
          ctx.fill();
        }
        break;
      }
      case 11: { // cradle
        ctx.fillStyle = '#20161a';
        ctx.fillRect(x - 22, y - 4, 44, 6);
        ctx.fillRect(x - 20, y - 30, 3, 30);
        ctx.fillRect(x + 17, y - 30, 3, 30);
        ctx.fillStyle = '#3b2228';
        ctx.fillRect(x - 20, y - 34, 40, 6);
        // bars
        ctx.strokeStyle = 'rgba(180,150,140,0.3)';
        for (let i = 0; i < 6; i++) {
          const bx = x - 17 + i * 7;
          ctx.beginPath(); ctx.moveTo(bx, y - 28); ctx.lineTo(bx, y - 6); ctx.stroke();
        }
        // red light inside
        const pulse = 0.4 + 0.25 * Math.sin(time * 1.6);
        const glow = ctx.createRadialGradient(x, y - 16, 0, x, y - 16, 60);
        glow.addColorStop(0, `rgba(220,30,40,${0.55 * pulse})`);
        glow.addColorStop(1, 'rgba(120,0,10,0)');
        ctx.fillStyle = glow;
        ctx.beginPath(); ctx.arc(x, y - 16, 60, 0, Math.PI * 2); ctx.fill();
        ctx.fillStyle = 'rgba(240,200,190,0.85)';
        ctx.beginPath(); ctx.ellipse(x, y - 14, 12, 4.5, 0, 0, Math.PI * 2); ctx.fill();
        ctx.fillStyle = 'rgba(60,20,20,0.7)';
        ctx.fillRect(x - 12, y - 16, 24, 1.5);
        break;
      }
      case 12: { // body
        ctx.fillStyle = 'rgba(0,0,0,0.35)';
        ctx.beginPath(); ctx.ellipse(x, y + 6, 26, 7, 0, 0, Math.PI * 2); ctx.fill();
        ctx.fillStyle = '#8b8474';
        ctx.beginPath();
        ctx.ellipse(x, y, 26, 12, 0.05, 0, Math.PI * 2);
        ctx.fill();
        ctx.fillStyle = '#6f675a';
        ctx.beginPath(); ctx.ellipse(x + 16, y - 5, 8, 7, 0, 0, Math.PI * 2); ctx.fill();
        ctx.fillStyle = 'rgba(120,10,16,0.35)';
        ctx.fillRect(x - 12, y - 2, 18, 4);
        break;
      }
      case 13: { // lamp
        ctx.strokeStyle = '#22221e';
        ctx.lineWidth = 2;
        ctx.beginPath(); ctx.moveTo(x, y + 4); ctx.lineTo(x, y - h / 2); ctx.stroke();
        ctx.fillStyle = '#3a3a32';
        ctx.fillRect(x - 10, y - h / 2 - 6, 20, 8);
        const glow = ctx.createRadialGradient(x, y - h / 2, 0, x, y - h / 2, 60 * flick);
        glow.addColorStop(0, `rgba(255,220,150,${0.30 * flick})`);
        glow.addColorStop(1, 'rgba(255,200,120,0)');
        ctx.fillStyle = glow;
        ctx.beginPath(); ctx.arc(x, y - h / 2, 60 * flick, 0, Math.PI * 2); ctx.fill();
        break;
      }
      case 14: { // sigil
        ctx.save();
        ctx.translate(x, y);
        ctx.rotate(time * 0.05 + p.id);
        ctx.strokeStyle = 'rgba(170,30,40,0.28)';
        ctx.lineWidth = 1.4;
        ctx.beginPath(); ctx.arc(0, 0, w / 2, 0, Math.PI * 2); ctx.stroke();
        ctx.beginPath();
        for (let i = 0; i < 5; i++) {
          const a = (i * 2 * Math.PI) / 5 - Math.PI / 2;
          const px = Math.cos(a) * w / 2, py = Math.sin(a) * w / 2;
          if (i === 0) ctx.moveTo(px, py); else ctx.lineTo(px, py);
        }
        ctx.closePath(); ctx.stroke();
        ctx.restore();
        break;
      }
      case 15: { // boarded passage
        ctx.fillStyle = '#1a1512';
        ctx.fillRect(x - w / 2, y - h / 2, w, h);
        if (p.state === 0) {
          ctx.fillStyle = '#3d2f22';
          for (let i = 0; i < 3; i++) {
            ctx.save();
            ctx.translate(x, y - 10 + i * 12);
            ctx.rotate(i % 2 ? 0.08 : -0.06);
            ctx.fillRect(-w / 2, -4, w, 8);
            ctx.restore();
          }
          ctx.fillStyle = 'rgba(190,190,190,0.4)';
          ctx.fillRect(x - 8, y - 12, 2, 2);
          ctx.fillRect(x + 6, y + 8, 2, 2);
        } else {
          ctx.fillStyle = '#080606';
          ctx.fillRect(x - w / 2, y - h / 2, w, h);
          ctx.fillStyle = 'rgba(60,40,30,0.4)';
          ctx.fillRect(x - w / 2, y + h / 2 - 4, w, 4);
        }
        break;
      }
      case 16: { // organ
        ctx.fillStyle = '#2a1c1c';
        ctx.fillRect(x - w / 2, y - h / 2 + 18, w, h - 18);
        for (let i = 0; i < 7; i++) {
          const px = x - w / 2 + 6 + i * 10;
          const ph = 22 + (i % 3) * 12;
          ctx.fillStyle = '#4a3a34';
          ctx.fillRect(px, y - h / 2, 6, ph + 8);
          ctx.fillStyle = 'rgba(255,220,180,0.08)';
          ctx.fillRect(px, y - h / 2, 2, ph + 8);
        }
        ctx.fillStyle = '#141010';
        ctx.fillRect(x - w / 2 + 4, y + h / 2 - 16, w - 8, 12);
        break;
      }
      case 17: { // gurney / table
        ctx.fillStyle = '#2c2c28';
        ctx.fillRect(x - w / 2, y - h / 2, w, h - 6);
        ctx.fillStyle = '#43433c';
        ctx.fillRect(x - w / 2 + 2, y - h / 2 + 2, w - 4, h - 10);
        ctx.fillStyle = '#22221e';
        ctx.fillRect(x - w / 2 + 6, y + h / 2 - 8, 4, 8);
        ctx.fillRect(x + w / 2 - 10, y + h / 2 - 8, 4, 8);
        ctx.fillStyle = 'rgba(120,20,26,0.22)';
        ctx.fillRect(x - w / 2 + 8, y - 2, w - 20, 6);
        break;
      }
      case 18: { // window
        ctx.fillStyle = '#151b1d';
        ctx.fillRect(x - w / 2, y - h / 2, w, h);
        ctx.fillStyle = 'rgba(70,90,110,0.28)';
        ctx.fillRect(x - w / 2 + 3, y - h / 2 + 3, w - 6, h - 6);
        ctx.strokeStyle = '#0c0f10';
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.moveTo(x, y - h / 2); ctx.lineTo(x, y + h / 2);
        ctx.moveTo(x - w / 2, y); ctx.lineTo(x + w / 2, y);
        ctx.stroke();
        // rain running down the glass
        ctx.strokeStyle = 'rgba(180,210,220,0.18)';
        for (let i = 0; i < 5; i++) {
          const rx = x - w / 2 + 8 + i * (w - 16) / 4;
          const ry = y - h / 2 + 4 + ((time * 18 + i * 23 + p.id * 11) % (h - 8));
          ctx.beginPath(); ctx.moveTo(rx, ry); ctx.lineTo(rx - 1, ry + 6); ctx.stroke();
        }
        break;
      }
      default: {
        ctx.fillStyle = 'rgba(120,120,110,0.15)';
        ctx.fillRect(x - w / 2, y - h / 2, w, h);
      }
    }
  }

  drawItem(ctx, it, camX, camY, time) {
    const x = Math.round(it.x - camX);
    const y = Math.round(it.y - camY + Math.sin(time * 2.2 + it.x * 0.1) * 2);
    if (x < -20 || x > VIEW_W + 20 || y < -20 || y > VIEW_H + 20) return;
    const glow = ctx.createRadialGradient(x, y, 0, x, y, 16);
    glow.addColorStop(0, 'rgba(255,220,160,0.28)');
    glow.addColorStop(1, 'rgba(255,200,120,0)');
    ctx.fillStyle = glow;
    ctx.beginPath(); ctx.arc(x, y, 16, 0, Math.PI * 2); ctx.fill();

    // item glyphs
    const id = it.itemId;
    if (id === 2) { // revolver
      ctx.fillStyle = '#4a4a48';
      ctx.fillRect(x - 6, y - 3, 11, 3);
      ctx.fillStyle = '#6b5a3a';
      ctx.fillRect(x - 4, y, 3, 5);
    } else if (id === 1) { // crowbar
      ctx.strokeStyle = '#7a5040';
      ctx.lineWidth = 2;
      ctx.beginPath(); ctx.moveTo(x - 6, y + 5); ctx.lineTo(x + 5, y - 5); ctx.stroke();
      ctx.beginPath(); ctx.arc(x + 5, y - 6, 3, -0.6, 1.8); ctx.stroke();
    } else if (id === 3) { // ammo
      ctx.fillStyle = '#8a7a4a';
      ctx.fillRect(x - 4, y - 3, 8, 6);
      ctx.fillStyle = '#c0a860';
      ctx.fillRect(x - 3, y - 5, 2, 3);
      ctx.fillRect(x + 1, y - 5, 2, 3);
    } else if (id === 4) { // bandage
      ctx.fillStyle = '#d8d2c0';
      ctx.fillRect(x - 4, y - 4, 8, 8);
      ctx.fillStyle = '#b03a3a';
      ctx.fillRect(x - 1, y - 4, 2, 8);
      ctx.fillRect(x - 4, y - 1, 8, 2);
    } else if (id >= 5 && id <= 7) { // fuses
      ctx.fillStyle = ['#4a7fd0', '#c04a4a', '#c0a24a'][id - 5];
      ctx.fillRect(x - 3, y - 5, 6, 10);
      ctx.fillStyle = '#222';
      ctx.fillRect(x - 3, y - 5, 6, 2);
      ctx.fillRect(x - 3, y + 3, 6, 2);
    } else if (id === 8 || id === 10) { // keys
      ctx.fillStyle = '#c8b070';
      ctx.beginPath(); ctx.arc(x - 3, y - 2, 3, 0, Math.PI * 2); ctx.fill();
      ctx.fillRect(x - 1, y - 2, 7, 2);
      ctx.fillRect(x + 4, y - 2, 2, 4);
    } else if (id === 9) { // valve handle
      ctx.strokeStyle = '#9a8a6a';
      ctx.lineWidth = 2;
      ctx.beginPath(); ctx.arc(x, y, 5, 0, Math.PI * 2); ctx.stroke();
      ctx.beginPath(); ctx.moveTo(x, y - 5); ctx.lineTo(x, y + 5); ctx.stroke();
    } else if (id === 11) { // battery
      ctx.fillStyle = '#3a4a3a';
      ctx.fillRect(x - 3, y - 5, 6, 10);
      ctx.fillStyle = '#c8a040';
      ctx.fillRect(x - 2, y - 6, 4, 2);
    } else if (id === 12) { // syringe
      ctx.fillStyle = '#cfd8d8';
      ctx.fillRect(x - 5, y - 1, 9, 2);
      ctx.fillStyle = '#8a2a3a';
      ctx.fillRect(x - 6, y - 2, 3, 4);
    } else if (id === 13) { // locket
      ctx.fillStyle = '#c8a850';
      ctx.fillRect(x - 3, y - 3, 6, 6);
      ctx.fillStyle = '#5a4020';
      ctx.fillRect(x - 1, y - 1, 2, 2);
    } else if (id === 14) { // matches
      ctx.fillStyle = '#8a5a3a';
      ctx.fillRect(x - 4, y - 3, 8, 6);
      ctx.fillStyle = '#c03a2a';
      ctx.fillRect(x - 2, y - 4, 2, 2);
    } else {
      ctx.fillStyle = '#d8d0b8';
      ctx.fillRect(x - 3, y - 3, 6, 6);
    }
  }

  // ---------------------------------------------------------------- actors
  drawPlayer(ctx, p, camX, camY, time) {
    const x = Math.round(p.x - camX);
    const y = Math.round(p.y - camY);
    const moving = Math.abs(p.vx) + Math.abs(p.vy) > 8;
    const bob = moving ? Math.sin(p.walkPhase * 1.4) * 1.2 : Math.sin(time * 1.6) * 0.5;
    const hiding = p.hiding;

    if (hiding) {
      ctx.globalAlpha = 0.35;
    }

    // shadow
    ctx.fillStyle = 'rgba(0,0,0,0.45)';
    ctx.beginPath();
    ctx.ellipse(x, y + 12, 9, 4, 0, 0, Math.PI * 2);
    ctx.fill();

    const faceSide = Math.abs(p.dirX) > Math.abs(p.dirY);
    const facing = faceSide ? (p.dirX > 0 ? 1 : -1) : 0;
    const legPhase = moving ? Math.sin(p.walkPhase * 1.5) : 0;

    // legs
    ctx.fillStyle = '#1b1c1a';
    if (faceSide) {
      ctx.fillRect(x - 5, y + 2, 4, 10 + legPhase * 2);
      ctx.fillRect(x + 1, y + 2, 4, 10 - legPhase * 2);
    } else {
      ctx.fillRect(x - 5, y + 2, 4, 10 + legPhase * 2);
      ctx.fillRect(x + 1, y + 2, 4, 10 - legPhase * 2);
    }
    // boots
    ctx.fillStyle = '#0d0d0c';
    ctx.fillRect(x - 6, y + 11, 5, 3);
    ctx.fillRect(x + 1, y + 11, 5, 3);

    // coat
    const coat = '#3a4038';
    const coatDark = '#2a2f29';
    ctx.fillStyle = coat;
    ctx.fillRect(x - 7 + bob * 0.2, y - 11, 14, 15);
    ctx.fillStyle = coatDark;
    ctx.fillRect(x - 7 + bob * 0.2, y - 2, 14, 6);
    // coat seam
    ctx.fillStyle = 'rgba(0,0,0,0.35)';
    ctx.fillRect(x - 1, y - 10, 1, 13);
    // scarf
    ctx.fillStyle = '#8c2029';
    ctx.fillRect(x - 6, y - 13, 12, 4);
    ctx.fillStyle = '#6d161e';
    ctx.fillRect(x + (facing >= 0 ? 3 : -6), y - 11, 4, 7);

    // arms + weapon
    ctx.fillStyle = coatDark;
    ctx.fillRect(x - 10, y - 9, 3, 11);
    ctx.fillRect(x + 7, y - 9, 3, 11);
    const swing = p.attackTimer > 0 ? (1 - p.attackTimer / 0.3) : 0;
    if (p.weapon === 2) {
      ctx.fillStyle = '#4d4d4b';
      const gx = x + (facing >= 0 ? 8 : -14);
      ctx.fillRect(gx, y - 6 + swing * 2, 8, 2);
      ctx.fillStyle = '#6b5a3a';
      ctx.fillRect(gx + (facing >= 0 ? 1 : 5), y - 4 + swing * 2, 3, 3);
    } else if (p.weapon === 1) {
      ctx.strokeStyle = '#8a6a4a';
      ctx.lineWidth = 2;
      const hx = x + (facing >= 0 ? 9 : -9);
      const hy = y - 8 + swing * 3;
      ctx.save();
      ctx.translate(hx, hy);
      ctx.rotate((facing >= 0 ? -0.5 : 0.5) + swing * 1.6 * (facing >= 0 ? 1 : -1));
      ctx.beginPath(); ctx.moveTo(0, 0); ctx.lineTo(0, -14); ctx.stroke();
      ctx.restore();
    }

    // head
    ctx.fillStyle = '#cbb8a4';
    ctx.fillRect(x - 4, y - 21 + bob * 0.3, 8, 8);
    // hair
    ctx.fillStyle = '#171512';
    ctx.fillRect(x - 5, y - 23 + bob * 0.3, 10, 5);
    ctx.fillRect(x - 5, y - 20 + bob * 0.3, 3, 5);
    // eyes
    if (facing !== 0) {
      ctx.fillStyle = '#0c0a0a';
      ctx.fillRect(x + facing * 1.5, y - 17 + bob * 0.3, 2, 2);
    } else if (p.dirY > 0) {
      ctx.fillStyle = '#0c0a0a';
      ctx.fillRect(x - 3, y - 18 + bob * 0.3, 2, 2);
      ctx.fillRect(x + 1, y - 18 + bob * 0.3, 2, 2);
    }

    if (p.hiding) ctx.globalAlpha = 1;
  }

  drawEnemy(ctx, e, camX, camY, time) {
    const x = Math.round(e.x - camX);
    const y = Math.round(e.y - camY);
    if (x < -80 || x > VIEW_W + 80 || y < -80 || y > VIEW_H + 80) return;
    const flip = e.dirX < 0 ? -1 : 1;
    const chase = e.state === 4 || e.state === 5;
    const phase = e.animPhase;

    ctx.save();
    ctx.translate(x, y);

    // shadow
    ctx.fillStyle = 'rgba(0,0,0,0.45)';
    ctx.beginPath(); ctx.ellipse(0, 10, 12, 5, 0, 0, Math.PI * 2); ctx.fill();

    if (e.type === 0) {
      // Crawler: low, wrong, dragging
      const bob = Math.sin(phase * 2.4) * 1.5;
      ctx.fillStyle = e.hitFlash > 0.2 ? '#9a6a6a' : '#5a5148';
      ctx.beginPath();
      ctx.ellipse(0, 2 + bob, 12, 6, 0, 0, Math.PI * 2);
      ctx.fill();
      ctx.fillStyle = '#433b34';
      ctx.fillRect(-13 * flip, 1 + bob, 9, 3);
      ctx.fillRect(5 * flip, 1 + bob, 10, 3);
      // arms
      ctx.strokeStyle = '#6f6357';
      ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.moveTo(-8, 0 + bob); ctx.lineTo(-16, 8 + Math.sin(phase * 3) * 3);
      ctx.moveTo(8, 0 + bob); ctx.lineTo(16, 8 + Math.cos(phase * 3) * 3);
      ctx.stroke();
      // head
      ctx.fillStyle = '#b7a89a';
      ctx.beginPath();
      ctx.ellipse(7 * flip, -5 + bob, 5.5, 4, 0.3 * flip, 0, Math.PI * 2);
      ctx.fill();
      ctx.fillStyle = '#241a16';
      ctx.beginPath(); ctx.ellipse(9 * flip, -6 + bob, 2, 1.4, 0, 0, Math.PI * 2); ctx.fill();
      // mouth
      ctx.fillStyle = '#3a0f12';
      ctx.fillRect(6 * flip, -2 + bob, 4, 1.5);
    } else if (e.type === 1) {
      // Hound: long, fast, wrong angles
      const bob = Math.sin(phase * 3) * 1.2;
      ctx.fillStyle = e.hitFlash > 0.2 ? '#9a6a6a' : '#3c3530';
      ctx.beginPath();
      ctx.ellipse(0, 0 + bob, 14, 6, 0, 0, Math.PI * 2);
      ctx.fill();
      // ribs
      ctx.strokeStyle = 'rgba(190,180,160,0.26)';
      ctx.lineWidth = 1;
      for (let i = 0; i < 3; i++) {
        ctx.beginPath();
        ctx.arc(-4 + i * 4, 0 + bob, 5, -0.9, 0.9);
        ctx.stroke();
      }
      // legs
      ctx.strokeStyle = '#332c27';
      ctx.lineWidth = 2.4;
      for (let i = 0; i < 4; i++) {
        const lx = -9 + i * 6;
        const swing = Math.sin(phase * 4 + i * 1.7) * 3;
        ctx.beginPath(); ctx.moveTo(lx, 4 + bob); ctx.lineTo(lx + swing, 11); ctx.stroke();
      }
      // head
      ctx.fillStyle = '#4a4239';
      ctx.beginPath();
      ctx.ellipse(13 * flip, -3 + bob, 7, 5, 0.2 * flip, 0, Math.PI * 2);
      ctx.fill();
      ctx.fillStyle = '#100c0a';
      ctx.beginPath(); ctx.ellipse(16 * flip, -2 + bob, 2, 2.4, 0, 0, Math.PI * 2); ctx.fill();
      // teeth
      ctx.strokeStyle = '#cfc4ad';
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.moveTo(14 * flip, 1 + bob); ctx.lineTo(18 * flip, 1 + bob);
      ctx.stroke();
      // eyes
      ctx.fillStyle = chase ? '#ff3a2a' : '#c8b070';
      ctx.fillRect(13 * flip - 1, -5 + bob, 2, 1.5);
    } else {
      // Weaver: tall, veiled, patient
      const sway = Math.sin(phase * 0.9) * 2;
      const bob = Math.sin(phase * 1.4) * 1.4;
      ctx.fillStyle = '#0e0b0d';
      ctx.beginPath();
      ctx.moveTo(-9, 10);
      ctx.quadraticCurveTo(-12, -12 + bob, -5 + sway, -30 + bob);
      ctx.lineTo(7 + sway, -30 + bob);
      ctx.quadraticCurveTo(12, -10 + bob, 9, 10);
      ctx.closePath();
      ctx.fill();
      // long arms
      ctx.strokeStyle = '#171215';
      ctx.lineWidth = 3;
      ctx.beginPath();
      ctx.moveTo(-7, -20 + bob);
      ctx.quadraticCurveTo(-18, -2 + bob, -14, 14 + Math.sin(phase * 1.2) * 2);
      ctx.moveTo(7, -20 + bob);
      ctx.quadraticCurveTo(18, -2 + bob, 14, 14 + Math.cos(phase * 1.2) * 2);
      ctx.stroke();
      // fingers
      ctx.strokeStyle = 'rgba(150,140,140,0.5)';
      ctx.lineWidth = 1;
      for (let i = 0; i < 3; i++) {
        ctx.beginPath();
        ctx.moveTo(-14, 14); ctx.lineTo(-16 + i * 2, 19);
        ctx.moveTo(14, 14); ctx.lineTo(16 - i * 2, 19);
        ctx.stroke();
      }
      // veil/face
      ctx.fillStyle = 'rgba(200,195,190,0.85)';
      ctx.beginPath();
      ctx.ellipse(sway * 0.5, -32 + bob, 5.5, 8, 0, 0, Math.PI * 2);
      ctx.fill();
      ctx.fillStyle = 'rgba(30,10,12,0.85)';
      ctx.fillRect(-3 + sway * 0.5, -34 + bob, 6, 3);
      ctx.fillStyle = chase ? '#ff2020' : '#a8a8a0';
      ctx.fillRect(-3 + sway * 0.5, -33 + bob, 2, 1.4);
      ctx.fillRect(1 + sway * 0.5, -33 + bob, 2, 1.4);
      // red thread from the veil
      ctx.strokeStyle = 'rgba(170,20,26,0.6)';
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.moveTo(sway * 0.5, -24 + bob);
      ctx.bezierCurveTo(8, -14, -6, -6, 4, 6);
      ctx.stroke();
    }

    // alert indicator
    if (e.state === 2 || e.state === 3) {
      ctx.fillStyle = 'rgba(240,220,160,0.75)';
      ctx.font = 'bold 9px monospace';
      ctx.fillText('?', -2, -30);
    } else if (chase) {
      ctx.fillStyle = 'rgba(255,60,40,0.85)';
      ctx.font = 'bold 10px monospace';
      ctx.fillText('!', -2, -32);
    }

    ctx.restore();
  }

  // --------------------------------------------------------------- lighting
  drawLighting(core, player, camX, camY, dt) {
    const lctx = this.lctx;
    const lights = core.lights();
    const power = core.player().light;
    let amb = clamp(core.ambient / 100, 0, 0.95);

    lctx.setTransform(1, 0, 0, 1, 0, 0);
    lctx.globalCompositeOperation = 'source-over';
    lctx.clearRect(0, 0, VIEW_W, VIEW_H);
    lctx.fillStyle = `rgba(4,4,7,${0.95 - amb * 0.72})`;
    lctx.fillRect(0, 0, VIEW_W, VIEW_H);

    lctx.globalCompositeOperation = 'destination-out';

    // room lights
    for (const l of lights) {
      const sx = l.x - camX;
      const sy = l.y - camY;
      if (sx < -l.radius || sx > VIEW_W + l.radius || sy < -l.radius || sy > VIEW_H + l.radius) continue;
      const flick = 0.72 + 0.28 * Math.sin(this.time * (3 + l.flicker * 9) + l.phase * 12);
      const strength = clamp(l.intensity * (0.65 + flick * 0.45), 0, 1);
      const r = l.radius * (0.9 + flick * 0.12);
      const grad = lctx.createRadialGradient(sx, sy, 0, sx, sy, r);
      grad.addColorStop(0, `rgba(0,0,0,${strength})`);
      grad.addColorStop(0.55, `rgba(0,0,0,${strength * 0.55})`);
      grad.addColorStop(1, 'rgba(0,0,0,0)');
      lctx.fillStyle = grad;
      lctx.beginPath(); lctx.arc(sx, sy, r, 0, Math.PI * 2); lctx.fill();
    }

    // player aura
    const px = player.x - camX;
    const py = player.y - camY;
    const aura = lctx.createRadialGradient(px, py, 0, px, py, 54);
    aura.addColorStop(0, 'rgba(0,0,0,0.42)');
    aura.addColorStop(1, 'rgba(0,0,0,0)');
    lctx.fillStyle = aura;
    lctx.beginPath(); lctx.arc(px, py, 54, 0, Math.PI * 2); lctx.fill();

    // torch cone
    if (power) {
      const dir = Math.atan2(player.dirY, player.dirX);
      const flick = 0.88 + 0.12 * Math.sin(this.time * 13);
      const len = 190 * flick;
      lctx.save();
      lctx.translate(px, py);
      lctx.rotate(dir);
      const cone = lctx.createLinearGradient(0, 0, len, 0);
      cone.addColorStop(0, 'rgba(0,0,0,0.85)');
      cone.addColorStop(0.6, 'rgba(0,0,0,0.55)');
      cone.addColorStop(1, 'rgba(0,0,0,0)');
      lctx.fillStyle = cone;
      lctx.beginPath();
      lctx.moveTo(0, 0);
      lctx.arc(0, 0, len, -0.30, 0.30);
      lctx.closePath();
      lctx.fill();
      lctx.restore();
    }

    lctx.globalCompositeOperation = 'source-over';

    const ctx = this.ctx;
    ctx.globalCompositeOperation = 'multiply';
    ctx.drawImage(this.lightCanvas, 0, 0);
    ctx.globalCompositeOperation = 'lighter';

    // warm bloom around each light
    for (const l of lights) {
      const sx = l.x - camX;
      const sy = l.y - camY;
      if (sx < -l.radius || sx > VIEW_W + l.radius || sy < -l.radius || sy > VIEW_H + l.radius) continue;
      const flick = 0.7 + 0.3 * Math.sin(this.time * (2 + l.flicker * 7) + l.phase * 9);
      const r = l.radius * 0.5;
      const grad = ctx.createRadialGradient(sx, sy, 0, sx, sy, r);
      grad.addColorStop(0, `rgba(${Math.round(l.r * 255)},${Math.round(l.g * 255)},${Math.round(l.b * 255)},${0.10 * l.intensity * flick})`);
      grad.addColorStop(1, 'rgba(0,0,0,0)');
      ctx.fillStyle = grad;
      ctx.beginPath(); ctx.arc(sx, sy, r, 0, Math.PI * 2); ctx.fill();
    }
    if (power) {
      const dir = Math.atan2(player.dirY, player.dirX);
      const grad = ctx.createRadialGradient(px, py, 0, px, py, 90);
      grad.addColorStop(0, 'rgba(255,235,190,0.07)');
      grad.addColorStop(1, 'rgba(255,220,160,0)');
      ctx.fillStyle = grad;
      ctx.beginPath(); ctx.arc(px, py, 90, 0, Math.PI * 2); ctx.fill();
      // visible beam
      ctx.save();
      ctx.translate(px, py);
      ctx.rotate(dir);
      const beam = ctx.createLinearGradient(0, 0, 180, 0);
      beam.addColorStop(0, 'rgba(255,236,190,0.045)');
      beam.addColorStop(1, 'rgba(255,220,160,0)');
      ctx.fillStyle = beam;
      ctx.beginPath(); ctx.moveTo(0, 0); ctx.arc(0, 0, 180, -0.28, 0.28); ctx.closePath(); ctx.fill();
      ctx.restore();
    }
    ctx.globalCompositeOperation = 'source-over';
  }

  drawMotes(ctx, dt) {
    for (const m of this.motes) {
      m.x += m.vx * dt;
      m.y += m.vy * dt;
      if (m.y < -4) { m.y = VIEW_H + 4; m.x = Math.random() * VIEW_W; }
      if (m.x < -4) m.x = VIEW_W + 4;
      if (m.x > VIEW_W + 4) m.x = -4;
      ctx.fillStyle = `rgba(220,210,190,${m.a * 0.55})`;
      ctx.fillRect(m.x, m.y, m.r, m.r);
    }
  }

  drawFog(ctx, pal, player, core) {
    const t = this.time;
    for (let i = 0; i < 3; i++) {
      const y = VIEW_H - 40 - i * 30;
      const grad = ctx.createLinearGradient(0, y, 0, y + 60);
      grad.addColorStop(0, 'rgba(0,0,0,0)');
      grad.addColorStop(0.6, pal.fog.replace('0.', `${0.06 + i * 0.02}`).replace(/[\d.]+\)$/, '0.10)'));
      grad.addColorStop(1, 'rgba(0,0,0,0)');
      ctx.fillStyle = grad;
      ctx.fillRect(0, y + Math.sin(t * 0.4 + i) * 6, VIEW_W, 60);
    }
    // red pulse when the Weaver is close
    if (core.danger > 0.72) {
      const a = (core.danger - 0.72) * 0.6;
      const grad = ctx.createRadialGradient(VIEW_W / 2, VIEW_H / 2, VIEW_H * 0.2, VIEW_W / 2, VIEW_H / 2, VIEW_H * 0.7);
      grad.addColorStop(0, 'rgba(120,0,0,0)');
      grad.addColorStop(1, `rgba(150,10,14,${a})`);
      ctx.fillStyle = grad;
      ctx.fillRect(0, 0, VIEW_W, VIEW_H);
    }
  }
}
