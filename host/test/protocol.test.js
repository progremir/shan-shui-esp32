import assert from "node:assert/strict";
import test from "node:test";
import { TYPE, crc16, encodeFrame, Parser } from "../lib/protocol.js";

test("crc16 matches CCITT-FALSE", () => {
  assert.equal(crc16(Buffer.from("123456789")), 0x29b1);
});

test("parser accepts a frame and rejects a bad crc", () => {
  const payload = Buffer.from("Hi");
  const frame = encodeFrame(TYPE.LOG, 0, 9, payload);
  const parser = new Parser();
  parser.push(frame.subarray(0, 3));
  parser.push(frame.subarray(3));
  const got = parser.take();
  assert.equal(got.type, TYPE.LOG);
  assert.equal(got.seq, 9);
  assert.equal(got.payload.toString(), "Hi");
  assert.equal(parser.take(), null);

  const bad = Buffer.from(frame);
  bad[bad.length - 1] ^= 0xff;
  const parser2 = new Parser();
  parser2.push(bad);
  assert.equal(parser2.take(), null);
});

test("two frames in one buffer both decode", () => {
  const a = encodeFrame(TYPE.LOG, 0, 1, Buffer.from("a"));
  const b = encodeFrame(TYPE.LOG, 0, 2, Buffer.from("b"));
  const parser = new Parser();
  parser.push(Buffer.concat([a, b]));
  assert.equal(parser.take().payload.toString(), "a");
  assert.equal(parser.take().payload.toString(), "b");
});
