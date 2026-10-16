import assert from "node:assert/strict";
import test from "node:test";
import { ColumnCache } from "../lib/dither.js";

function fakeGenerator() {
  return {
    calls: [],
    async detail(x, n, scale) {
      this.calls.push(x);
      return new Uint8Array(n * scale * 300 * scale).fill(255);
    },
  };
}

function cache(gen) {
  return new ColumnCache(gen, {
    height: 300, stripWidth: 160, inkScale: 2, paperCutoff: 224, threshold: 128, seed: "t",
  });
}

test("a far column jumps there instead of painting everything before it", async () => {
  const gen = fakeGenerator();
  const c = cache(gen);
  const packed = await c.packedRange(50000, 48);
  assert.equal(packed.length, 48 * 38);
  assert.deepEqual(gen.calls, [50000]);
});

test("a board restart back at column 0 is served after a long run", async () => {
  const gen = fakeGenerator();
  const c = cache(gen);
  await c.packedRange(50000, 48);
  const packed = await c.packedRange(0, 48);
  assert.equal(packed.length, 48 * 38);
  assert.equal(gen.calls.at(-1), 0);
});

test("the painted window stays bounded on long runs", async () => {
  const c = cache(fakeGenerator());
  for (let x = 0; x < 20000; x += 48) await c.packedRange(x, 48);
  assert.ok(c.count - c.base <= 8000 + 160, `${c.count - c.base} columns kept`);
  assert.equal((await c.packedRange(19968, 32)).length, 32 * 38);
});
