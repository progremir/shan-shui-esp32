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
  let linked = false;
  let sawTraffic = false;
  let acked = 0;
  const logs = [];
  const onLog = opts.onLog || ((line) => console.log(`[board] ${line}`));
  const onAck = opts.onAck || (() => {});

  while (acked < stopAfter) {
    if (opts.signal?.aborted) break;
    if (typeof source.demand === "function" && source._want > source.count) source.demand(source._want);
    const chunk = await port.read(200);
    if (chunk) parser.push(chunk);
    let frame = parser.take();
    if (!frame) continue;
    while (frame) {
      if (frame.type === TYPE.HELLO) {
        const hello = decodeHello(frame.payload);
        if (sawTraffic) {
          if (source._job) await source._job;
          source.reset();
          sawTraffic = false;
          onLog("board restarted, scroll returns to the start");
        }
        linked = true;
        port.write(encodeHelloAck(seq++, speed));
        onLog(
          `hello ${hello.name} ${hello.width}x${hello.height} ${hello.bpp}-bit strip ${hello.stripCols}`,
        );
        if (opts.onHello) opts.onHello(hello);
        if (typeof source.demand === "function") source.demand(880);
      } else if (frame.type === TYPE.NEED) {
        if (!linked) {
          frame = parser.take();
          continue;
        }
        sawTraffic = true;
        const need = decodeNeed(frame.payload);
        if (need.columns < 1 || need.columns > 96) {
          onLog(`ignored need for ${need.columns} columns`);
          frame = parser.take();
          continue;
        }
        if (typeof source.count === "number" && need.startX > source.count + 8000) {
          onLog(`ignored need at column ${need.startX}`);
          frame = parser.take();
          continue;
        }
        if (source._job) await source._job;
        const packed = await source.packedRange(need.startX, need.columns);
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
