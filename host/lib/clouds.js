// Soft clouds in the open sky. The panel is only black and paper, so a
// cloud is a dithered patch: more ink near the middle, paper at the edge,
// which reads as gray. Placement is a function of world x and the seed, so
// a cloud is the same shape every time a column is drawn.

const CELL = 110;

const BAYER8 = [
  0, 32, 8, 40, 2, 34, 10, 42, 48, 16, 56, 24, 50, 18, 58, 26, 12, 44, 4, 36, 14, 46, 6, 38, 60, 28,
  52, 20, 62, 30, 54, 22, 3, 35, 11, 43, 1, 33, 9, 41, 51, 19, 59, 27, 49, 17, 57, 25, 15, 47, 7,
  39, 13, 45, 5, 37, 63, 31, 55, 23, 61, 29, 53, 21,
];

function hash32(n) {
  n >>>= 0;
  n = Math.imul(n ^ (n >>> 16), 0x7feb352d);
  n = Math.imul(n ^ (n >>> 15), 0x846ca68b);
  return (n ^ (n >>> 16)) >>> 0;
}

function unit(n) {
  return hash32(n) / 4294967296;
}

export function seedSalt(seed) {
  let h = 2166136261;
  const s = String(seed);
  for (let i = 0; i < s.length; i++) {
    h ^= s.charCodeAt(i);
    h = Math.imul(h, 16777619);
  }
  return h >>> 0;
}

function cloudForCell(cell, salt) {
  const h = hash32((cell * 0x9e3779b1) ^ salt);
  if (unit(h) > 0.8) return null;
  const rx = 26 + unit(h ^ 0x11) * 34;
  const ry = 8 + unit(h ^ 0x22) * 12;
  return {
    cx: cell * CELL + 12 + unit(h ^ 0x33) * (CELL - 24),
    cy: 10 + unit(h ^ 0x44) * 48,
    rx,
    ry,
    ox: (unit(h ^ 0x55) - 0.5) * rx * 0.7,
    oy: (unit(h ^ 0x66) - 0.5) * ry * 0.4,
    peak: 0.34 + unit(h ^ 0x77) * 0.22,
  };
}

function lobe(wx, y, cx, cy, rx, ry) {
  const dx = (wx - cx) / rx;
  const dy = (y - cy) / ry;
  const d2 = dx * dx + dy * dy;
  if (d2 >= 1) return 0;
  const t = 1 - Math.sqrt(d2);
  return t * t;
}

function densityAt(wx, y, clouds) {
  let density = 0;
  for (const cloud of clouds) {
    const a = lobe(wx, y, cloud.cx, cloud.cy, cloud.rx, cloud.ry);
    const b = lobe(wx, y, cloud.cx + cloud.ox, cloud.cy + cloud.oy, cloud.rx * 0.72, cloud.ry * 0.8);
    const d = cloud.peak * Math.max(a, b);
    if (d > density) density = d;
  }
  return density;
}

export function applyClouds(bits, width, height, originX, seed, skyline = null) {
  const salt = seedSalt(seed);
  const cell0 = Math.floor((originX - CELL) / CELL);
  const cell1 = Math.floor((originX + width + CELL) / CELL);
  const clouds = [];
  for (let cell = cell0; cell <= cell1; cell++) {
    const cloud = cloudForCell(cell, salt);
    if (cloud) clouds.push(cloud);
  }
  if (!clouds.length) return;

  for (let x = 0; x < width; x++) {
    let top = skyline ? skyline[x] : height;
    if (!skyline) {
      for (let y = 0; y < height; y++) {
        if (bits[y * width + x] === 0) {
          top = y;
          break;
        }
      }
    }
    if (top < 4) continue;
    const wx = originX + x;
    for (let y = 0; y < top; y++) {
      const density = densityAt(wx, y, clouds);
      if (density < 0.05) continue;
      const threshold = (BAYER8[((y & 7) << 3) + (wx & 7)] + 0.5) / 64;
      if (density > threshold) bits[y * width + x] = 0;
    }
  }
}
