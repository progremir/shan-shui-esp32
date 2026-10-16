import { writeFile, mkdir } from "node:fs/promises";
import path from "node:path";
import { Generator } from "./lib/generator.js";
import { ColumnCache } from "./lib/dither.js";
import {
  DEFAULT_SEED,
  DITHER_THRESHOLD,
  INK,
  INK_SCALE,
  PAPER_CUTOFF,
  PANEL_H,
  PANEL_W,
  PAPER,
  SCENE_USER_H,
  SCROLL_PX_PER_SEC,
  STRIP_RENDER,
} from "./lib/config.js";
import { encodeRgbPng, pngSize } from "./lib/png.js";
import { existsSync } from "node:fs";
import { openSerial, listSerialPorts } from "./lib/serial.js";
import { runHostSession } from "./lib/session.js";
import { openBrowser, startPreviewServer } from "./lib/preview.js";

function parseArgs(argv) {
  const out = { _: [] };
  for (let i = 0; i < argv.length; i++) {
    const a = argv[i];
    if (a.startsWith("--")) {
      const key = a.slice(2);
      const next = argv[i + 1];
      if (next == null || next.startsWith("--")) out[key] = true;
      else out[key] = argv[++i];
    } else out._.push(a);
  }
  return out;
}

function makeCache(gen, seed) {
  return new ColumnCache(gen, {
    height: PANEL_H,
    stripWidth: STRIP_RENDER,
    inkScale: Number(process.env.INK_SCALE || INK_SCALE),
    paperCutoff: Number(process.env.PAPER_CUTOFF || PAPER_CUTOFF),
    threshold: Number(process.env.DITHER_THRESHOLD || DITHER_THRESHOLD),
    seed,
  });
}

async function withGenerator(seed, fn) {
  const userH = Number(process.env.SCENE_USER_H || SCENE_USER_H);
  const gen = new Generator({ seed, userH });
  await gen.start();
  try {
    return await fn(gen);
  } finally {
    await gen.close();
  }
}

function histogram(gray) {
  const bins = new Array(8).fill(0);
  for (const v of gray) bins[Math.min(7, v >> 5)]++;
  return bins;
}

async function frames(args) {
  const seed = String(args.seed || DEFAULT_SEED);
  const outDir = path.resolve(String(args.out));
  await mkdir(outDir, { recursive: true });
  const xs = [0, 400, 800, 1200];
  await withGenerator(seed, async (gen) => {
    const a = await gen.grayscale(0, 240);
    const b = await gen.grayscale(80, 240);
    let diff = 0;
    const overlap = 160;
    for (let y = 0; y < PANEL_H; y++) {
      for (let x = 0; x < overlap; x++) {
        diff += Math.abs(a[y * 240 + (80 + x)] - b[y * 240 + x]);
      }
    }
    const mean = diff / (overlap * PANEL_H);
    console.log(`grayscale overlap mean abs ${mean.toFixed(3)}`);
    console.log(`luma histogram ${histogram(a).join(",")}`);
    if (mean > 12) {
      throw new Error(`generator windows do not line up (mean abs ${mean})`);
    }
    const cache = makeCache(gen, seed);
    const written = [];
    for (const x of xs) {
      const bits = await cache.windowBits(x, PANEL_W);
      let ink = 0;
      for (const v of bits) if (!v) ink++;
      const ratio = ink / bits.length;
      console.log(`frame x=${x} ink ${(ratio * 100).toFixed(2)}%`);
      if (ratio < 0.004 || ratio > 0.65) {
        throw new Error(`frame x=${x} ink ratio ${ratio} is not a landscape`);
      }
      const png = encodeRgbPng(PANEL_W, PANEL_H, bits, PAPER, INK);
      const size = pngSize(png);
      if (size.width !== PANEL_W || size.height !== PANEL_H) throw new Error("png size");
      const file = path.join(outDir, `shan-shui-rlcd-${String(x).padStart(4, "0")}.png`);
      await writeFile(file, png);
      written.push(file);
    }
    const left = await cache.windowBits(0, PANEL_W);
    const right = await cache.windowBits(400, PANEL_W);
    let same = 0;
    for (let i = 0; i < left.length; i++) if (left[i] === right[i]) same++;
    if (same / left.length > 0.98) throw new Error("adjacent frames are identical");
    for (const file of written) console.log(file);
  });
}

async function preview(args) {
  const seed = String(args.seed || DEFAULT_SEED);
  const speed = Number(args.speed || SCROLL_PX_PER_SEC);
  const port = Number(args.port || 8737);
  const gen = new Generator({ seed, userH: Number(process.env.SCENE_USER_H || SCENE_USER_H) });
  await gen.start();
  const cache = makeCache(gen, seed);
  const server = await startPreviewServer({ cache, port, speed });
  console.log(server.url);
  if (args.open) openBrowser(server.url);
  const stop = () => {
    server.close();
    gen.close();
    process.exit(0);
  };
  process.on("SIGINT", stop);
  process.on("SIGTERM", stop);
}

async function stream(args) {
  const device = args.port && args.port !== true ? String(args.port) : "";
  if (!device) {
    const found = listSerialPorts();
    console.error("Pass the ESP32 serial device, for example:");
    console.error("  node host/cli.js stream --port /dev/cu.usbmodem1101");
    if (found.length) console.error(`Candidates: ${found.join(", ")}`);
    else console.error("No USB serial device is connected.");
    process.exit(1);
  }
  const seed = String(args.seed || DEFAULT_SEED);
  const speed = Number(args.speed || SCROLL_PX_PER_SEC);
  const userH = Number(process.env.SCENE_USER_H || SCENE_USER_H);
  let gen = new Generator({ seed, userH });
  await gen.start();
  let cache = makeCache(gen, seed);
  let renderFailures = 0;
  let port = null;
  let stopping = false;
  const stop = () => {
    stopping = true;
    try {
      port?.close();
    } catch {
      /* already closed */
    }
    gen.close();
    process.exit(0);
  };
  process.on("SIGINT", stop);
  process.on("SIGTERM", stop);
  console.log(`streaming seed ${seed} at ${speed} px/s on ${device}`);
  let columns = 0;
  while (!stopping) {
    const path = serialPath(device);
    if (!path) {
      console.error(`waiting for ${device}. short-press the middle button to turn the board on`);
      await new Promise((r) => setTimeout(r, 500));
      continue;
    }
    try {
      port = openSerial(path);
    } catch (err) {
      console.error(`waiting for ${path} (${err.code || err.message})`);
      await new Promise((r) => setTimeout(r, 500));
      continue;
    }
    console.log(`board port ${path}`);
    try {
      await runHostSession(port, cache, {
        height: PANEL_H,
        speed,
        onRenderError(err) {
          renderFailures++;
          console.error(`render failed (${renderFailures}): ${err.stack || err.message || err}`);
          if (renderFailures >= 3) {
            const e = new Error("generator keeps failing");
            e.code = "GENERATOR_FAILED";
            throw e;
          }
        },
        onAck(ack) {
          renderFailures = 0;
          columns += ack.columns;
          if (columns % 240 < ack.columns) console.log(`sent through column ${ack.startX + ack.columns}`);
        },
        onHello(hello) {
          if (hello.width !== PANEL_W || hello.height !== PANEL_H || hello.bpp !== 1) {
            console.error(
              `board reports ${hello.width}x${hello.height} bpp ${hello.bpp}, host is ${PANEL_W}x${PANEL_H} 1-bit`,
            );
          }
        },
      });
      break;
    } catch (err) {
      if (stopping) break;
      if (err?.code !== "PORT_CLOSED") {
        // Never leave the board without a host: restart Chrome and carry on.
        console.error(`host error, restarting the generator: ${err?.stack || err}`);
        try {
          port?.close();
        } catch {
          /* already closed */
        }
        port = null;
        try {
          await gen.close();
        } catch {
          /* Chrome may already be gone */
        }
        renderFailures = 0;
        gen = new Generator({ seed, userH });
        try {
          await gen.start();
        } catch (startErr) {
          console.error(`generator restart failed: ${startErr.message}`);
          await new Promise((r) => setTimeout(r, 5000));
        }
        cache = makeCache(gen, seed);
        continue;
      }
      console.error("board went away. short-press the middle button to turn it on. waiting");
      try {
        port.close();
      } catch {
        /* already closed */
      }
      port = null;
      await new Promise((r) => setTimeout(r, 500));
    }
  }
}

function serialPath(preferred) {
  if (existsSync(preferred)) return preferred;
  const found = listSerialPorts().filter((p) => /cu\.usb|ttyACM|ttyUSB|tty\.usb/.test(p));
  return found.length === 1 ? found[0] : "";
}

function help() {
  console.log(`Usage:
  node host/cli.js preview [--port 8737] [--seed wallpaper] [--speed 6] [--open]
  node host/cli.js stream --port /dev/cu.usbmodem1101 [--seed wallpaper] [--speed 6]
  node host/cli.js frames --out ../media [--seed wallpaper]`);
}

const argv = parseArgs(process.argv.slice(2));
const cmd = argv._[0];
const run =
  cmd === "frames" ? frames(argv) : cmd === "preview" ? preview(argv) : cmd === "stream" ? stream(argv) : null;
if (!run) {
  help();
  process.exit(cmd ? 1 : 0);
}
run.catch((err) => {
  console.error(err.stack || err.message || err);
  process.exit(1);
});
