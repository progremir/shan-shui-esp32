import http from "node:http";
import { readFile } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { ChromePage, launchChrome } from "./chrome.js";
import { ssCapture } from "./page-capture.js";
import { PANEL_H, PANEL_W, SCENE_USER_H } from "./config.js";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../..");
const indexPath = path.join(root, "vendor/shan-shui-inf/index.html");

function serveIndex() {
  const htmlPromise = readFile(indexPath);
  const server = http.createServer(async (req, res) => {
    if (!req.url.startsWith("/index.html")) {
      res.writeHead(404);
      res.end();
      return;
    }
    const html = await htmlPromise;
    res.writeHead(200, { "Content-Type": "text/html; charset=utf-8" });
    res.end(html);
  });
  return new Promise((resolve) => {
    server.listen(0, "127.0.0.1", () => {
      const addr = server.address();
      resolve({ server, port: addr.port });
    });
  });
}

export class Generator {
  constructor({ seed, panelW = PANEL_W, panelH = PANEL_H, userH = SCENE_USER_H } = {}) {
    this.seed = seed;
    this.panelW = panelW;
    this.panelH = panelH;
    this.userH = userH;
    this.userPerPx = userH / panelH;
  }

  async start() {
    const served = await serveIndex();
    this.server = served.server;
    const launched = await launchChrome();
    this.chromeProc = launched.proc;
    this.page = new ChromePage(launched.proc, launched.port);
    const url = `http://127.0.0.1:${served.port}/index.html?seed=${encodeURIComponent(this.seed)}`;
    console.error(`generator ${url}`);
    await this.page.open(url);
    const deadline = Date.now() + 180000;
    let ready = false;
    while (Date.now() < deadline) {
      const flag = await this.page.evaluate(
        `document.readyState === "complete" && !!document.getElementById("SVG") && !!document.getElementById("bgcanv")`,
      );
      if (flag) {
        ready = true;
        break;
      }
      console.error("waiting for the first shan-shui painting");
      await new Promise((r) => setTimeout(r, 2000));
    }
    if (!ready) throw new Error("shan-shui-inf did not finish its first painting");
    await this.page.evaluate(`window.__ssCapture = ${ssCapture.toString()};`);
    await this.page.evaluate(`document.body.scrollTo(0,0); true;`);
  }

  async capture(deviceX, deviceW, scale = 1) {
    const userX = deviceX * this.userPerPx;
    const userW = deviceW * this.userPerPx;
    const dw = deviceW * scale;
    const dh = this.panelH * scale;
    const expr = `window.__ssCapture(${dw}, ${dh}, ${userX}, ${userW}, ${this.userH})`;
    console.error(`raster ${dw}px at x=${deviceX}`);
    const b64 = await this.page.evaluate(expr, { awaitPromise: true, timeoutMs: 180000 });
    if (typeof b64 !== "string") throw new Error("capture returned no pixels");
    const buf = Buffer.from(b64, "base64");
    if (buf.length !== dw * dh) {
      throw new Error(`capture length ${buf.length}, expected ${dw * dh}`);
    }
    return buf;
  }

  async grayscale(deviceX, deviceW) {
    return this.capture(deviceX, deviceW, 1);
  }

  async detail(deviceX, deviceW, scale) {
    return this.capture(deviceX, deviceW, scale);
  }

  async close() {
    await this.page?.close();
    this.server?.close();
  }
}
