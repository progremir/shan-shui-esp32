import { applyClouds } from "./clouds.js";
import { columnStride } from "./config.js";

export function downsample(detail, width, height, scale) {
  if (detail.length !== width * scale * height * scale) {
    throw new Error(`detail ${detail.length} != ${width}*${scale} by ${height}*${scale}`);
  }
  const gray = new Uint8Array(width * height);
  const sw = width * scale;
  const area = scale * scale;
  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      let sum = 0;
      for (let dy = 0; dy < scale; dy++) {
        const row = (y * scale + dy) * sw + x * scale;
        for (let dx = 0; dx < scale; dx++) sum += detail[row + dx];
      }
      gray[y * width + x] = Math.round(sum / area);
    }
  }
  return gray;
}

// Floyd–Steinberg over the whole painting. Near-white paper is pinned so the
// empty sky stays blank; every darker tone, including a translucent stroke,
// becomes a mix of ink and paper. `carry` crosses into the next strip.
export function ditherStrip(gray, width, height, carryIn, opts = {}) {
  const cutoff = opts.paperCutoff ?? 224;
  const threshold = opts.threshold ?? 128;
  const px = new Float64Array(width * height);
  for (let i = 0; i < px.length; i++) px[i] = gray[i] >= cutoff ? 255 : gray[i];
  if (carryIn) {
    for (let y = 0; y < height; y++) px[y * width] += carryIn[y];
  }
  const bits = new Uint8Array(width * height);
  const carry = new Float64Array(height);
  for (let y = 0; y < height; y++) {
    const row = y * width;
    for (let x = 0; x < width; x++) {
      const i = row + x;
      const old = px[i];
      const on = old >= threshold ? 255 : 0;
      bits[i] = on === 255 ? 1 : 0;
      const err = old - on;
      if (x + 1 < width) px[i + 1] += (err * 7) / 16;
      else carry[y] += (err * 7) / 16;
      if (y + 1 < height) {
        if (x > 0) px[i + width - 1] += (err * 3) / 16;
        px[i + width] += (err * 5) / 16;
        if (x + 1 < width) px[i + width + 1] += err / 16;
        else carry[y + 1] += err / 16;
      }
    }
  }
  return { bits, carry };
}

export function skylineFromGray(gray, width, height, maxLuma) {
  const skyline = new Int16Array(width);
  for (let x = 0; x < width; x++) {
    let y = 0;
    for (; y < height; y++) if (gray[y * width + x] < maxLuma) break;
    skyline[x] = y;
  }
  return skyline;
}

export function packColumns(bits, width, height) {
  const stride = columnStride(height);
  const out = new Uint8Array(width * stride);
  out.fill(0);
  for (let x = 0; x < width; x++) {
    for (let y = 0; y < height; y++) {
      if (!bits[y * width + x]) continue;
      const bi = x * stride + (y >> 3);
      out[bi] |= 1 << (7 - (y & 7));
    }
  }
  return out;
}

export function unpackColumns(packed, width, height, stride = columnStride(height)) {
  const bits = new Uint8Array(width * height);
  for (let x = 0; x < width; x++) {
    for (let y = 0; y < height; y++) {
      const bi = x * stride + (y >> 3);
      const white = (packed[bi] >> (7 - (y & 7))) & 1;
      bits[y * width + x] = white ? 1 : 0;
    }
  }
  return bits;
}

export class ColumnCache {
  constructor(generator, opts) {
    this.generator = generator;
    this.height = opts.height;
    this.stripWidth = opts.stripWidth;
    this.inkScale = opts.inkScale;
    this.paperCutoff = opts.paperCutoff;
    this.threshold = opts.threshold;
    this.seed = opts.seed;
    this.stride = columnStride(this.height);
    this.count = 0;
    this.packed = new Uint8Array(0);
    this.carry = new Float64Array(this.height);
    this._want = 0;
    this._job = null;
  }

  reset() {
    this.count = 0;
    this.packed = new Uint8Array(0);
    this.carry = new Float64Array(this.height);
    this._want = 0;
  }

  // Paint further columns while the serial loop is waiting. One strip at a
  // time, so a NEED can be answered as soon as its columns exist.
  demand(n) {
    if (n > this._want) this._want = n;
    if (this._job || this.count >= this._want) return;
    const upto = Math.min(this._want, this.count + this.stripWidth);
    const job = this.ensure(upto).finally(() => {
      if (this._job === job) this._job = null;
    });
    this._job = job;
    job.catch(() => {});
  }

  async ensure(nColumns) {
    while (this.count < nColumns) {
      const n = Math.min(this.stripWidth, nColumns - this.count);
      const detail = await this.generator.detail(this.count, n, this.inkScale);
      const gray = downsample(detail, n, this.height, this.inkScale);
      const { bits, carry } = ditherStrip(gray, n, this.height, this.carry, {
        paperCutoff: this.paperCutoff,
        threshold: this.threshold,
      });
      this.carry = carry;
      const skyline = skylineFromGray(gray, n, this.height, 200);
      applyClouds(bits, n, this.height, this.count, this.seed, skyline);
      const piece = packColumns(bits, n, this.height);
      const next = new Uint8Array(this.packed.length + piece.length);
      next.set(this.packed, 0);
      next.set(piece, this.packed.length);
      this.packed = next;
      this.count += n;
    }
  }

  async packedRange(start, n) {
    const end = start + n;
    while (this.count < end) {
      if (this._job) await this._job;
      else await this.ensure(Math.min(end, this.count + this.stripWidth));
    }
    const stride = this.stride;
    return this.packed.subarray(start * stride, (start + n) * stride);
  }

  async windowBits(start, n) {
    const packed = await this.packedRange(start, n);
    return unpackColumns(packed, n, this.height, this.stride);
  }
}
