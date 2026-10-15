import assert from "node:assert/strict";
import net from "node:net";
import test from "node:test";
import { columnStride } from "../lib/config.js";
import {
  Parser,
  TYPE,
  decodeColumns,
  encodeFrame,
} from "../lib/protocol.js";
import { runHostSession } from "../lib/session.js";
import { bridgePty, openSerial } from "../lib/serial.js";

const height = 300;
const stride = columnStride(height);
const chunk = 40;

function pattern(start, n) {
  const out = new Uint8Array(n * stride);
  out.fill(0xff);
  for (let i = 0; i < n; i++) {
    const y = (start + i) % height;
    out[i * stride + (y >> 3)] &= ~(1 << (7 - (y & 7)));
  }
  return out;
}

function source() {
  return {
    reset() {},
    async packedRange(start, n) {
      return pattern(start, n);
    },
  };
}

function encodeNeed(start, columns) {
  const p = Buffer.alloc(8);
  p.writeUInt32LE(start, 0);
  p.writeUInt16LE(columns, 4);
  p.writeUInt16LE(100, 6);
  return encodeFrame(TYPE.NEED, 0, 1, p);
}

function encodeAck(start, columns) {
  const p = Buffer.alloc(8);
  p.writeUInt32LE(start, 0);
  p.writeUInt16LE(columns, 4);
  p.writeUInt16LE(columns, 6);
  return encodeFrame(TYPE.ACK, 0, 2, p);
}

function encodeHello() {
  const p = Buffer.alloc(28);
  p.writeUInt16LE(400, 0);
  p.writeUInt16LE(300, 2);
  p.writeUInt16LE(1, 4);
  p.writeUInt16LE(1600, 6);
  p.writeUInt16LE(400, 8);
  p.writeUInt16LE(1, 10);
  p.write("ST7305");
  return encodeFrame(TYPE.HELLO, 0, 3, p);
}

function mailbox() {
  const q = [];
  let waiter = null;
  return {
    push(buf) {
      const copy = Buffer.from(buf);
      if (waiter) {
        const w = waiter;
        waiter = null;
        w(copy);
      } else q.push(copy);
    },
    shift(ms) {
      if (q.length) return Promise.resolve(q.shift());
      return new Promise((resolve) => {
        const t = setTimeout(() => {
          waiter = null;
          resolve(q.shift() || null);
        }, ms);
        waiter = (buf) => {
          clearTimeout(t);
          resolve(buf);
        };
      });
    },
  };
}

function link() {
  const a = mailbox();
  const b = mailbox();
  return [
    { write: (buf) => b.push(buf), read: (ms) => a.shift(ms) },
    { write: (buf) => a.push(buf), read: (ms) => b.shift(ms) },
  ];
}

async function mockDevice(port, chunks) {
  const parser = new Parser();
  port.write(encodeHello());
  let next = 0;
  let got = 0;
  let acked = false;
  const deadline = Date.now() + 8000;
  while (got < chunks && Date.now() < deadline) {
    const chunkBuf = await port.read(200);
    if (chunkBuf) parser.push(chunkBuf);
    let frame = parser.take();
    while (frame) {
      if (frame.type === TYPE.HELLO_ACK) {
        acked = true;
        port.write(encodeNeed(next, chunk));
      } else if (frame.type === TYPE.COLUMNS) {
        const col = decodeColumns(frame.payload);
        assert.equal(col.startX, next);
        assert.equal(col.height, height);
        assert.deepEqual(Buffer.from(col.pixels), Buffer.from(pattern(next, chunk)));
        port.write(encodeAck(next, chunk));
        next += chunk;
        got++;
        if (got < chunks) port.write(encodeNeed(next, chunk));
      }
      frame = parser.take();
    }
  }
  assert.equal(got, chunks);
  assert.equal(acked, true);
}

function socketPort(sock) {
  const q = [];
  let waiter = null;
  sock.on("data", (buf) => {
    if (waiter) {
      const w = waiter;
      waiter = null;
      w(buf);
    } else q.push(buf);
  });
  return {
    write(buf) {
      sock.write(buf);
    },
    read(ms) {
      if (q.length) return Promise.resolve(q.shift());
      return new Promise((resolve) => {
        const t = setTimeout(() => {
          waiter = null;
          resolve(q.shift() || null);
        }, ms);
        waiter = (buf) => {
          clearTimeout(t);
          resolve(buf);
        };
      });
    },
  };
}

test("host and device exchange columns over a byte pipe", async () => {
  const [hostPort, devPort] = link();
  await Promise.all([
    runHostSession(hostPort, source(), { height, speed: 6, stopAfter: chunk * 3, onLog() {} }),
    mockDevice(devPort, 3),
  ]);
});

test("session ends when the port closes", async () => {
  const port = {
    write() {},
    read() {
      return Promise.reject(Object.assign(new Error("gone"), { code: "PORT_CLOSED" }));
    },
  };
  await assert.rejects(
    runHostSession(port, source(), { height, speed: 6, onLog() {} }),
    (err) => err.code === "PORT_CLOSED",
  );
});

test("a dropped serial port rejects the pending read", async () => {
  const bridge = await bridgePty();
  const serial = openSerial(bridge.device);
  const pending = serial.read(3000);
  bridge.close();
  try {
    await assert.rejects(pending, (err) => err.code === "PORT_CLOSED");
  } finally {
    serial.close();
  }
});

test("a huge NEED is ignored and the next one is served", async () => {
  const [hostPort, devPort] = link();
  let rendered = 0;
  const tracked = {
    reset() {},
    count: 0,
    async packedRange(start, n) {
      rendered += n;
      this.count = Math.max(this.count, start + n);
      return pattern(start, n);
    },
  };
  const device = (async () => {
    devPort.write(encodeHello());
    devPort.write(encodeNeed(0, 4000));
    devPort.write(encodeNeed(0, chunk));
    const parser = new Parser();
    const deadline = Date.now() + 4000;
    let got = false;
    while (!got && Date.now() < deadline) {
      const buf = await devPort.read(200);
      if (buf) parser.push(buf);
      let frame = parser.take();
      while (frame) {
        if (frame.type === TYPE.COLUMNS) {
          const col = decodeColumns(frame.payload);
          assert.equal(col.startX, 0);
          assert.equal(col.columns, chunk);
          got = true;
        }
        frame = parser.take();
      }
    }
    assert.equal(got, true);
    devPort.write(encodeAck(0, chunk));
  })();
  await Promise.all([
    runHostSession(hostPort, tracked, { height, speed: 6, stopAfter: chunk, onLog() {} }),
    device,
  ]);
  assert.equal(rendered, chunk);
});

test("the same exchange round-trips through a PTY", async () => {
  const bridge = await bridgePty();
  const sock = net.connect(bridge.port, "127.0.0.1");
  await new Promise((resolve, reject) => {
    sock.once("connect", resolve);
    sock.once("error", reject);
  });
  const serial = openSerial(bridge.device);
  try {
    await Promise.all([
      runHostSession(serial, source(), { height, speed: 6, stopAfter: chunk * 2, onLog() {} }),
      mockDevice(socketPort(sock), 2),
    ]);
  } finally {
    serial.close();
    sock.destroy();
    bridge.close();
  }
});
