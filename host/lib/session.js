import {
  Parser,
  TYPE,
  decodeAck,
  decodeHello,
  decodeNeed,
  encodeColumns,
  encodeHelloAck,
} from "./protocol.js";
import { columnStride } from "./config.js";

// Speaks the ESP32 protocol on any byte port: { write(buf), read(ms) -> Buffer|null }.
export async function runHostSession(port, source, opts = {}) {
  const parser = new Parser();
  const height = opts.height;
  const stride = columnStride(height);
  const speed = opts.speed;
  const stopAfter = opts.stopAfter ?? Infinity;
  let seq = 1;
  let sawTraffic = false;
  let acked = 0;
  let lastRx = Date.now();
  const silentMs = opts.silentMs ?? 10000;
  const logs = [];
  const onLog = opts.onLog || ((line) => console.log(`[board] ${line}`));
  const onAck = opts.onAck || (() => {});

  while (acked < stopAfter) {
    if (opts.signal?.aborted) break;
    if (typeof source.demand === "function" && source._want > source.count) source.demand(source._want);
    const chunk = await port.read(200);
    if (chunk) {
      parser.push(chunk);
      lastRx = Date.now();
    } else if (sawTraffic && Date.now() - lastRx > silentMs) {
      // A running board talks at least every 1.5 s. Silence this long means
      // its USB link is wedged; reopening the port resets the line state.
      const err = new Error(`board silent for ${Math.round(silentMs / 1000)} s`);
      err.code = "PORT_CLOSED";
      throw err;
    }
    let frame = parser.take();
    if (!frame) continue;
    while (frame) {
      if (frame.type === TYPE.HELLO) {
        const hello = decodeHello(frame.payload);
        // Painted columns stay valid across a board restart. The board asks
        // for whatever column it needs, so there is nothing to reset here.
        if (sawTraffic) onLog("board said hello again");
        port.write(encodeHelloAck(seq++, speed));
        onLog(
          `hello ${hello.name} ${hello.width}x${hello.height} ${hello.bpp}-bit strip ${hello.stripCols}`,
        );
        if (opts.onHello) opts.onHello(hello);
        if (typeof source.demand === "function") source.demand(880);
      } else if (frame.type === TYPE.NEED) {
        // No HELLO gate: if this process restarted or the port reopened while
        // the board kept running, the board never says hello again and only
        // sends NEED. Ignoring it froze the picture for good.
        sawTraffic = true;
        const need = decodeNeed(frame.payload);
        if (need.columns < 1 || need.columns > 96) {
          onLog(`ignored need for ${need.columns} columns`);
          frame = parser.take();
          continue;
        }
        if (source._job) await source._job.catch(() => {});
        let packed;
        try {
          packed = await source.packedRange(need.startX, need.columns);
        } catch (err) {
          // The board repeats NEED after 1.5 s, so one bad render is not fatal.
          if (opts.onRenderError) opts.onRenderError(err);
          else onLog(`render failed at column ${need.startX}: ${err.message || err}`);
          frame = parser.take();
          continue;
        }
        port.write(
          encodeColumns(seq++, need.startX, height, packed, need.columns, stride),
        );
        if (typeof source.demand === "function") source.demand(need.startX + need.columns + 960);
      } else if (frame.type === TYPE.ACK) {
        sawTraffic = true;
        const ack = decodeAck(frame.payload);
        acked += ack.columns;
        onAck(ack);
      } else if (frame.type === TYPE.LOG) {
        const text = frame.payload.toString("utf8");
        logs.push(text);
        onLog(text);
      }
      frame = parser.take();
    }
  }
  return { acked, logs };
}
