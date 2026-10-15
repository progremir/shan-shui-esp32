import zlib from "node:zlib";

function crc32(buf) {
  let c = ~0;
  for (let i = 0; i < buf.length; i++) {
    c ^= buf[i];
    for (let k = 0; k < 8; k++) c = (c >>> 1) ^ (0xedb88320 & -(c & 1));
  }
  return ~c >>> 0;
}

function chunk(type, data) {
  const body = Buffer.isBuffer(data) ? data : Buffer.from(data);
  const out = Buffer.alloc(12 + body.length);
  out.writeUInt32BE(body.length, 0);
  out.write(type, 4, 4, "ascii");
  body.copy(out, 8);
  const crcBuf = Buffer.alloc(4 + body.length);
  crcBuf.write(type, 0, 4, "ascii");
  body.copy(crcBuf, 4);
  out.writeUInt32BE(crc32(crcBuf), 8 + body.length);
  return out;
}

// 1-bit decisions painted as two flat colors: warm paper and ink.
export function encodeRgbPng(width, height, bits, paper, ink) {
  if (bits.length !== width * height) {
    throw new Error(`png bits ${bits.length} != ${width}*${height}`);
  }
  const raw = Buffer.alloc(height * (1 + width * 3));
  let o = 0;
  for (let y = 0; y < height; y++) {
    raw[o++] = 0;
    for (let x = 0; x < width; x++) {
      const c = bits[y * width + x] ? paper : ink;
      raw[o++] = c[0];
      raw[o++] = c[1];
      raw[o++] = c[2];
    }
  }
  const ihdr = Buffer.alloc(13);
  ihdr.writeUInt32BE(width, 0);
  ihdr.writeUInt32BE(height, 4);
  ihdr[8] = 8;
  ihdr[9] = 2;
  const sig = Buffer.from([137, 80, 78, 71, 13, 10, 26, 10]);
  const idat = chunk("IDAT", zlib.deflateSync(raw));
  return Buffer.concat([sig, chunk("IHDR", ihdr), idat, chunk("IEND", Buffer.alloc(0))]);
}

export function pngSize(buf) {
  const sig = "89504e470d0a1a0a";
  if (buf.subarray(0, 8).toString("hex") !== sig) throw new Error("not a png");
  if (buf.toString("ascii", 12, 16) !== "IHDR") throw new Error("missing IHDR");
  return { width: buf.readUInt32BE(16), height: buf.readUInt32BE(20) };
}
