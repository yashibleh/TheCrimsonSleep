// tools/logo.mjs - renders the game's title card to assets/logo.png.
// Build-time art tool (node-canvas + Google Fonts TTFs in /tmp/fonts); the game
// only ever loads the finished PNG, so the shipped site has no font dependency.
import fs from 'fs';
import path from 'path';
import { createRequire } from 'module';

const require = createRequire(import.meta.url);
const { createCanvas, registerFont } = require('/tmp/viz/node_modules/canvas');

registerFont('/tmp/fonts/Cinzel-var.ttf', { family: 'Cinzel' });
registerFont('/tmp/fonts/EBGaramond-var.ttf', { family: 'EB Garamond' });

const W = 1200, H = 420;
const canvas = createCanvas(W, H);
const x = canvas.getContext('2d');

const SERIF = '"EB Garamond", serif';
const DISPLAY = '"Cinzel", serif';

function layout(text, font, spacing) {
  x.save();
  x.font = font;
  const chars = [...text];
  const widths = chars.map((ch) => x.measureText(ch).width + spacing);
  const total = widths.reduce((a, b) => a + b, 0) - spacing;
  x.restore();
  return { chars, widths, total };
}

function drawTracked(text, cx, y, font, spacing, fill, opts = {}) {
  const { chars, widths, total } = layout(text, font, spacing);
  x.save();
  x.font = font;
  x.textBaseline = 'alphabetic';
  x.fillStyle = fill;
  if (opts.glow) { x.shadowColor = opts.glow; x.shadowBlur = opts.blur || 18; }
  let px = cx - total / 2;
  chars.forEach((ch, i) => { x.fillText(ch, px, y); px += widths[i]; });
  if (opts.stroke) {
    x.shadowBlur = 0;
    x.strokeStyle = opts.stroke;
    x.lineWidth = 1;
    px = cx - total / 2;
    chars.forEach((ch, i) => { x.strokeText(ch, px, y); px += widths[i]; });
  }
  x.restore();
  return total;
}

// backlight
const halo = x.createRadialGradient(W / 2, 205, 40, W / 2, 205, 560);
halo.addColorStop(0, 'rgba(126,14,22,0.52)');
halo.addColorStop(0.55, 'rgba(80,8,14,0.16)');
halo.addColorStop(1, 'rgba(0,0,0,0)');
x.fillStyle = halo;
x.fillRect(0, 0, W, H);

// "THE"
drawTracked('THE', W / 2, 76, `400 26px ${DISPLAY}`, 22, 'rgba(208,178,136,0.86)',
  { glow: 'rgba(200,70,60,0.55)', blur: 14 });

x.strokeStyle = 'rgba(176,138,90,0.40)';
x.lineWidth = 1;
x.beginPath();
x.moveTo(W / 2 - 230, 68); x.lineTo(W / 2 - 66, 68);
x.moveTo(W / 2 + 66, 68); x.lineTo(W / 2 + 230, 68);
x.stroke();
x.save();
x.translate(W / 2, 68);
x.rotate(Math.PI / 4);
x.strokeStyle = 'rgba(176,138,90,0.62)';
x.strokeRect(-3.5, -3.5, 7, 7);
x.restore();

// main title, auto-fitted to the plate width
const title = 'CRIMSON SLEEP';
let size = 108, spacing = 10, font = `600 ${size}px ${DISPLAY}`;
let metrics = layout(title, font, spacing);
while (metrics.total > 1050 && size > 40) {
  size -= 2;
  font = `600 ${size}px ${DISPLAY}`;
  metrics = layout(title, font, spacing);
}

const grad = x.createLinearGradient(0, 128, 0, 268);
grad.addColorStop(0, '#f6bdb2');
grad.addColorStop(0.16, '#dc4b52');
grad.addColorStop(0.48, '#b0242f');
grad.addColorStop(0.8, '#74101a');
grad.addColorStop(1, '#42060e');

const baseline = 262;
x.save();
x.shadowColor = 'rgba(140,12,22,0.9)';
x.shadowBlur = 44;
x.font = font;
x.textBaseline = 'alphabetic';
let px = W / 2 - metrics.total / 2;
x.fillStyle = grad;
metrics.chars.forEach((ch, i) => { x.fillText(ch, px, baseline); px += metrics.widths[i]; });
x.restore();

// letterpress shadow + engraved edge
x.save();
x.globalCompositeOperation = 'multiply';
x.fillStyle = 'rgba(30,0,5,0.55)';
x.font = font;
px = W / 2 - metrics.total / 2 + 2;
metrics.chars.forEach((ch, i) => { x.fillText(ch, px, baseline + 3); px += metrics.widths[i]; });
x.restore();

x.save();
x.font = font;
x.strokeStyle = 'rgba(255,220,208,0.13)';
x.lineWidth = 1;
px = W / 2 - metrics.total / 2;
metrics.chars.forEach((ch, i) => { x.strokeText(ch, px, baseline); px += metrics.widths[i]; });
x.restore();

// drips
let seed = 20261111;
const rnd = () => { seed = (seed * 1664525 + 1013904223) % 4294967296; return seed / 4294967296; };
for (let i = 0; i < 56; i++) {
  const dx = W / 2 - metrics.total / 2 + rnd() * metrics.total;
  const len = 4 + rnd() * rnd() * 104;
  const w = 0.7 + rnd() * 2.0;
  const g2 = x.createLinearGradient(0, baseline - 6, 0, baseline - 6 + len);
  g2.addColorStop(0, 'rgba(146,18,26,0.88)');
  g2.addColorStop(0.6, 'rgba(98,10,18,0.44)');
  g2.addColorStop(1, 'rgba(60,4,10,0)');
  x.fillStyle = g2;
  x.fillRect(dx, baseline - 6, w, len);
  x.beginPath();
  x.arc(dx + w / 2, baseline - 6 + len, w * 0.95, 0, Math.PI * 2);
  x.fillStyle = 'rgba(80,8,14,0.30)';
  x.fill();
}

// subtitle + year
drawTracked('A QUIET HOUR AT ASHGROVE SANATORIUM', W / 2, 330, `400 21px ${SERIF}`, 6,
  'rgba(190,170,142,0.78)');
drawTracked('1917', W / 2, 362, `400 17px ${SERIF}`, 12, 'rgba(154,124,96,0.42)');

// weathering
for (let i = 0; i < 3600; i++) {
  const wx = rnd() * W, wy = rnd() * H;
  const a = rnd() * 0.05;
  x.fillStyle = rnd() > 0.5 ? `rgba(0,0,0,${a})` : `rgba(255,230,210,${a * 0.8})`;
  x.fillRect(wx, wy, 1 + rnd() * 2, 1);
}

// edge fade into the dark
const vig = x.createRadialGradient(W / 2, 200, 260, W / 2, 200, 660);
vig.addColorStop(0, 'rgba(0,0,0,0)');
vig.addColorStop(1, 'rgba(0,0,0,0.66)');
x.globalCompositeOperation = 'destination-out';
x.fillStyle = vig;
x.fillRect(0, 0, W, H);
x.globalCompositeOperation = 'source-over';

fs.mkdirSync('assets', { recursive: true });
fs.writeFileSync(path.join('assets', 'logo.png'), canvas.toBuffer('image/png'));
console.log(`assets/logo.png written (${W}x${H}, ${fs.statSync('assets/logo.png').size} bytes), title ${size}px`);
