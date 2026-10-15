import http from "node:http";
import { readFile } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { spawn } from "node:child_process";
import { INK, PAPER } from "./config.js";
import { encodeRgbPng } from "./png.js";

const previewPath = path.join(path.dirname(fileURLToPath(import.meta.url)), "../preview.html");

export function startPreviewServer({ cache, port, speed }) {
  const page = readFile(previewPath);
  const server = http.createServer(async (req, res) => {
    const url = new URL(req.url, "http://127.0.0.1");
    if (url.pathname === "/" || url.pathname === "/preview.html") {
      const html = await page;
      res.writeHead(200, { "Content-Type": "text/html; charset=utf-8" });
      res.end(html);
      return;
    }
    if (url.pathname === "/columns.png") {
      const start = Number(url.searchParams.get("start") || 0);
      const n = Math.max(1, Math.min(400, Number(url.searchParams.get("n") || 160)));
      if (!Number.isFinite(start) || start < 0) {
        res.writeHead(400);
        res.end();
        return;
      }
      const bits = await cache.windowBits(start, n);
      const png = encodeRgbPng(n, cache.height, bits, PAPER, INK);
      res.writeHead(200, { "Content-Type": "image/png", "Cache-Control": "no-store" });
      res.end(png);
      return;
    }
    res.writeHead(404);
    res.end();
  });
  return new Promise((resolve) => {
    server.listen(port, "127.0.0.1", () => {
      const bound = server.address().port;
      const url = `http://127.0.0.1:${bound}/?speed=${speed}`;
      resolve({
        url,
        close() {
          server.close();
        },
      });
    });
  });
}

export function openBrowser(url) {
  if (process.platform === "darwin") spawn("open", [url], { stdio: "ignore", detached: true }).unref();
  else if (process.platform === "linux") spawn("xdg-open", [url], { stdio: "ignore", detached: true }).unref();
}
