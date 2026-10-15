import assert from "node:assert/strict";
import test from "node:test";
import { applyClouds } from "../lib/clouds.js";

test("clouds sit in the sky, keep the ridge, and stay put", () => {
  const width = 360;
  const height = 120;
  const skyline = 70;
  function blank(w = width) {
    const bits = new Uint8Array(w * height);
    bits.fill(1);
    for (let x = 0; x < w; x++) bits[skyline * w + x] = 0;
    return bits;
  }
  const a = blank();
  applyClouds(a, width, height, 0, "wallpaper");
  const b = blank();
  applyClouds(b, width, height, 0, "wallpaper");
  assert.deepEqual(a, b);

  for (let x = 0; x < width; x++) {
    assert.equal(a[skyline * width + x], 0);
    for (let y = skyline + 1; y < height; y++) assert.equal(a[y * width + x], 1);
  }

  let cloud = 0;
  let paper = 0;
  for (let y = 0; y < skyline; y++) {
    for (let x = 0; x < width; x++) {
      if (a[y * width + x] === 0) cloud++;
      else paper++;
    }
  }
  assert.ok(cloud > 40, `expected dithered cloud ink, got ${cloud}`);
  assert.ok(paper > cloud, "a cloud is gray, not a solid black mass");

  const left = blank();
  applyClouds(left, width, height, 0, "wallpaper");
  const right = blank(80);
  applyClouds(right, 80, height, 280, "wallpaper");
  for (let y = 0; y < height; y++) {
    for (let x = 0; x < 80; x++) {
      assert.equal(right[y * 80 + x], left[y * width + (280 + x)]);
    }
  }
});
