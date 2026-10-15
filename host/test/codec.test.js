import assert from "node:assert/strict";
import { execFileSync } from "node:child_process";
import path from "node:path";
import { fileURLToPath } from "node:url";
import test from "node:test";
import { TYPE, encodeColumns, encodeFrame, Parser, decodeColumns } from "../lib/protocol.js";
import { PANEL_H, PANEL_W } from "../lib/config.js";

const here = path.dirname(fileURLToPath(import.meta.url));
const binDir = path.resolve(here, "../../firmware/test");

function build() {
  execFileSync("make", ["-C", binDir, "test"], { stdio: "pipe" });
}

test("firmware codec matches the host frame bytes", () => {
  build();
  const bin = "/tmp/shan-shui-host-test";
  const hex = execFileSync(bin, ["encode"], { encoding: "utf8" }).trim();
  const payload = Buffer.alloc(28);
  payload.writeUInt16LE(PANEL_W, 0);
  payload.writeUInt16LE(PANEL_H, 2);
  payload.writeUInt16LE(1, 4);
  payload.writeUInt16LE(1600, 6);
  payload.writeUInt16LE(PANEL_W, 8);
  payload.writeUInt16LE(1, 10);
  payload.write("ST7305", 12);
  const js = encodeFrame(TYPE.HELLO, 0, 7, payload);
  assert.equal(js.toString("hex"), hex);

  const packed = new Uint8Array(2 * 38);
  packed[0] = 0x7f;
  packed[38] = 0x80;
  const columns = encodeColumns(4, 20, PANEL_H, packed, 2, 38);
  const out = execFileSync(bin, ["decode"], { input: columns, encoding: "utf8" });
  assert.match(out, /type=17/);
  assert.match(out, /seq=4/);
  assert.match(out, /start=20 columns=2 height=300 stride=38/);

  const parser = new Parser();
  parser.push(columns);
  const frame = parser.take();
  const decoded = decodeColumns(frame.payload);
  assert.equal(decoded.startX, 20);
  assert.equal(decoded.pixels[0], 0x7f);
  assert.equal(decoded.pixels[38], 0x80);
});
