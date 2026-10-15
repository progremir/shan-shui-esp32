import assert from "node:assert/strict";
import test from "node:test";
import { ditherStrip, packColumns, unpackColumns } from "../lib/dither.js";
import { encodeRgbPng, pngSize } from "../lib/png.js";

test("dither keeps blank paper and turns a gray field into a mix", () => {
  const w = 32;
  const h = 16;
  const gray = new Uint8Array(w * h);
  gray.fill(250);
  for (let y = 4; y < 12; y++) {
    for (let x = 4; x < 28; x++) gray[y * w + x] = 20;
  }
  for (let y = 0; y < 3; y++) {
    for (let x = 0; x < w; x++) gray[y * w + x] = 170;
  }
  const { bits } = ditherStrip(gray, w, h, null, { paperCutoff: 224, threshold: 128 });
  assert.equal(bits[3 * w], 1);
  let dark = 0;
  for (let y = 4; y < 12; y++) {
    for (let x = 4; x < 28; x++) if (bits[y * w + x] === 0) dark++;
  }
  assert.ok(dark > 140);
  let grayInk = 0;
  for (let y = 0; y < 3; y++) {
    for (let x = 0; x < w; x++) if (bits[y * w + x] === 0) grayInk++;
  }
  assert.ok(grayInk > 8 && grayInk < 3 * w - 8);
  assert.deepEqual(unpackColumns(packColumns(bits, w, h), w, h), bits);
});

test("png is 400 by 300 and only the two reflective colors", () => {
  const bits = new Uint8Array(400 * 300);
  bits.fill(1);
  bits[10] = 0;
  const png = encodeRgbPng(400, 300, bits, [244, 240, 230], [20, 20, 20]);
  assert.deepEqual(pngSize(png), { width: 400, height: 300 });
  assert.equal(png.subarray(0, 8).toString("hex"), "89504e470d0a1a0a");
});
