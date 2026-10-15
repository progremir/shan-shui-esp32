// USB serial framing shared with firmware/shan_shui_wallpaper/protocol.cpp.
//
// Frame (little-endian):
//   A5 5A | type u8 | flags u8 | seq u16 | len u16 | payload[len] | crc16
// CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF) over type..payload.
//
// HELLO 0x01 device->host
//   u16 width, height, bpp, strip_cols, view_cols, version
//   u8 name[16]
// HELLO_ACK 0x02 host->device
//   u16 scroll_px_per_sec, u16 reserved, u32 reserved
// NEED 0x10 device->host
//   u32 start_x, u16 columns, u16 free_cols
// COLUMNS 0x11 host->device
//   u32 start_x, u16 columns, u16 height, u16 stride, u8 pixels[columns*stride]
//   Column-major. Bit (7 - y%8) of byte y/8 is pixel y. 1 = paper, 0 = ink.
// ACK 0x12 device->host
//   u32 start_x, u16 columns, u16 stored
// LOG 0x20 device->host
//   utf-8 text

export const SYNC0 = 0xa5;
export const SYNC1 = 0x5a;
export const TYPE = {
  HELLO: 0x01,
  HELLO_ACK: 0x02,
  NEED: 0x10,
  COLUMNS: 0x11,
  ACK: 0x12,
  LOG: 0x20,
};
export const MAX_PAYLOAD = 3072;

export function crc16(data) {
  let crc = 0xffff;
  for (let i = 0; i < data.length; i++) {
    crc ^= data[i] << 8;
    for (let b = 0; b < 8; b++) {
      if (crc & 0x8000) crc = ((crc << 1) ^ 0x1021) & 0xffff;
      else crc = (crc << 1) & 0xffff;
    }
  }
  return crc;
}

export function encodeFrame(type, flags, seq, payload) {
  const body = Buffer.isBuffer(payload) ? payload : Buffer.from(payload || []);
  if (body.length > MAX_PAYLOAD) {
    throw new Error(`payload ${body.length} exceeds ${MAX_PAYLOAD}`);
  }
  const covered = Buffer.alloc(6 + body.length);
  covered[0] = type;
  covered[1] = flags & 0xff;
  covered.writeUInt16LE(seq & 0xffff, 2);
  covered.writeUInt16LE(body.length, 4);
  body.copy(covered, 6);
  const crc = crc16(covered);
  const out = Buffer.alloc(2 + covered.length + 2);
  out[0] = SYNC0;
  out[1] = SYNC1;
  covered.copy(out, 2);
  out.writeUInt16LE(crc, 2 + covered.length);
  return out;
}

export function encodeHelloAck(seq, speed) {
  const p = Buffer.alloc(8);
  p.writeUInt16LE(speed, 0);
  return encodeFrame(TYPE.HELLO_ACK, 0, seq, p);
}

export function encodeColumns(seq, startX, height, packed, columns, stride) {
  if (packed.length !== columns * stride) {
    throw new Error(`packed ${packed.length} != ${columns}*${stride}`);
  }
  const p = Buffer.alloc(10 + packed.length);
  p.writeUInt32LE(startX >>> 0, 0);
  p.writeUInt16LE(columns, 4);
  p.writeUInt16LE(height, 6);
  p.writeUInt16LE(stride, 8);
  Buffer.from(packed).copy(p, 10);
  return encodeFrame(TYPE.COLUMNS, 0, seq, p);
}

export function decodeHello(payload) {
  if (payload.length < 28) throw new Error("short HELLO");
  const name = Buffer.from(payload.subarray(12, 28)).toString("utf8").replace(/\0+$/, "");
  return {
    width: payload.readUInt16LE(0),
    height: payload.readUInt16LE(2),
    bpp: payload.readUInt16LE(4),
    stripCols: payload.readUInt16LE(6),
    viewCols: payload.readUInt16LE(8),
    version: payload.readUInt16LE(10),
    name,
  };
}

export function decodeNeed(payload) {
  return {
    startX: payload.readUInt32LE(0),
    columns: payload.readUInt16LE(4),
    freeCols: payload.readUInt16LE(6),
  };
}

export function decodeColumns(payload) {
  const startX = payload.readUInt32LE(0);
  const columns = payload.readUInt16LE(4);
  const height = payload.readUInt16LE(6);
  const stride = payload.readUInt16LE(8);
  const pixels = Buffer.from(payload.subarray(10));
  return { startX, columns, height, stride, pixels };
}

export function decodeAck(payload) {
  return {
    startX: payload.readUInt32LE(0),
    columns: payload.readUInt16LE(4),
    stored: payload.readUInt16LE(6),
  };
}

export class Parser {
  constructor() {
    this.state = 0;
    this.buf = Buffer.alloc(6 + MAX_PAYLOAD);
    this.got = 0;
    this.need = 0;
    this.frames = [];
  }

  push(chunk) {
    const bytes = Buffer.isBuffer(chunk) ? chunk : Buffer.from(chunk);
    for (let i = 0; i < bytes.length; i++) this._byte(bytes[i]);
  }

  take() {
    return this.frames.shift() || null;
  }

  _byte(b) {
    if (this.state === 0) {
      if (b === SYNC0) this.state = 1;
      return;
    }
    if (this.state === 1) {
      this.state = b === SYNC1 ? 2 : b === SYNC0 ? 1 : 0;
      return;
    }
    if (this.state === 2) {
      this.buf[this.got++] = b;
      if (this.got === 6) {
        this.need = this.buf.readUInt16LE(4);
        if (this.need > MAX_PAYLOAD) {
          this._resync();
          return;
        }
        this.state = this.need === 0 ? 4 : 3;
      }
      return;
    }
    if (this.state === 3) {
      this.buf[this.got++] = b;
      if (this.got === 6 + this.need) this.state = 4;
      return;
    }
    if (this.state === 4) {
      this._crc0 = b;
      this.state = 5;
      return;
    }
    const crc = this._crc0 | (b << 8);
    const covered = this.buf.subarray(0, 6 + this.need);
    if (crc16(covered) === crc) {
      this.frames.push({
        type: covered[0],
        flags: covered[1],
        seq: covered.readUInt16LE(2),
        payload: Buffer.from(covered.subarray(6)),
      });
    }
    this.got = 0;
    this.need = 0;
    this.state = 0;
  }

  _resync() {
    const kept = Buffer.from(this.buf.subarray(1, this.got));
    this.got = 0;
    this.need = 0;
    this.state = 0;
    for (const b of kept) this._byte(b);
  }
}
