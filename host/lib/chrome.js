import { spawn, spawnSync } from "node:child_process";
import { existsSync } from "node:fs";

function chromeBin() {
  const list = [
    process.env.CHROME,
    "google-chrome",
    "google-chrome-stable",
    "chromium",
    "chromium-browser",
    "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome",
    "/Applications/Chromium.app/Contents/MacOS/Chromium",
  ].filter(Boolean);
  for (const bin of list) {
    if (bin.includes("/") && existsSync(bin)) return bin;
    const found = spawnSync("which", [bin], { encoding: "utf8" });
    if (found.status === 0) return found.stdout.trim();
  }
  throw new Error(
    "Google Chrome was not found. Install it, or set CHROME to the binary path.",
  );
}

function cdpClient(ws) {
  let id = 0;
  const pending = new Map();
  const listeners = [];
  ws.addEventListener("message", (ev) => {
    const msg = JSON.parse(typeof ev.data === "string" ? ev.data : ev.data.toString());
    if (msg.id && pending.has(msg.id)) {
      const done = pending.get(msg.id);
      pending.delete(msg.id);
      done(msg);
    } else {
      for (const fn of listeners) fn(msg);
    }
  });
  return {
    on(fn) {
      listeners.push(fn);
    },
    send(method, params = {}, timeoutMs = 180000) {
      const my = ++id;
      return new Promise((resolve, reject) => {
        const timer = setTimeout(() => {
          pending.delete(my);
          reject(new Error(`CDP timeout: ${method}`));
        }, timeoutMs);
        pending.set(my, (msg) => {
          clearTimeout(timer);
          if (msg.error) reject(new Error(`${method}: ${JSON.stringify(msg.error)}`));
          else resolve(msg.result);
        });
        ws.send(JSON.stringify({ id: my, method, params }));
      });
    },
  };
}

async function waitForPort(port, proc) {
  const start = Date.now();
  while (Date.now() - start < 20000) {
    if (proc.exitCode != null) throw new Error("Chrome exited before DevTools opened");
    try {
      const res = await fetch(`http://127.0.0.1:${port}/json/version`);
      if (res.ok) return;
    } catch {
      /* not up yet */
    }
    await new Promise((r) => setTimeout(r, 100));
  }
  throw new Error("Chrome DevTools did not open");
}

export async function launchChrome() {
  const bin = chromeBin();
  const profile = `/tmp/shan-shui-chrome-${process.pid}`;
  let port = 0;
  const proc = spawn(
    bin,
    [
      "--headless=new",
      "--no-sandbox",
      "--disable-dev-shm-usage",
      "--disable-extensions",
      "--hide-scrollbars",
      "--mute-audio",
      "--remote-debugging-port=0",
      `--user-data-dir=${profile}`,
      "about:blank",
    ],
    { stdio: ["ignore", "pipe", "pipe"] },
  );
  proc.stderr.on("data", (buf) => {
    const m = String(buf).match(/127\.0\.0\.1:(\d+)/);
    if (m) port = Number(m[1]);
    if (process.env.SS_DEBUG) process.stderr.write(buf);
  });
  const start = Date.now();
  while (!port && Date.now() - start < 15000) {
    if (proc.exitCode != null) throw new Error("Chrome failed to start");
    await new Promise((r) => setTimeout(r, 50));
  }
  if (!port) throw new Error("Chrome did not report a DevTools port");
  await waitForPort(port, proc);
  return { proc, port, profile };
}

export class ChromePage {
  constructor(proc, port) {
    this.proc = proc;
    this.port = port;
    this.ws = null;
    this.cdp = null;
  }

  async open(url) {
    const blank = await fetch(`http://127.0.0.1:${this.port}/json/new?about:blank`, {
      method: "PUT",
    });
    if (!blank.ok) {
      const again = await fetch(`http://127.0.0.1:${this.port}/json/new?about:blank`);
      if (!again.ok) throw new Error(`json/new failed: ${again.status}`);
      this.target = await again.json();
    } else {
      this.target = await blank.json();
    }
    this.ws = new WebSocket(this.target.webSocketDebuggerUrl);
    await new Promise((resolve, reject) => {
      this.ws.addEventListener("open", resolve, { once: true });
      this.ws.addEventListener("error", () => reject(new Error("devtools websocket failed")), {
        once: true,
      });
    });
    this.cdp = cdpClient(this.ws);
    if (process.env.SS_DEBUG) {
      this.cdp.on((msg) => {
        if (msg.method === "Runtime.consoleAPICalled") {
          const text = (msg.params.args || [])
            .map((a) => a.value ?? a.description ?? "")
            .join(" ");
          console.error(`[page] ${text}`);
        }
      });
    }
    await this.cdp.send("Page.enable");
    await this.cdp.send("Runtime.enable");
    await this.cdp.send("Page.addScriptToEvaluateOnNewDocument", {
      source: `
        const __noop = function(){};
        window.scrollTo = __noop;
        window.scroll = __noop;
        if (window.Element) Element.prototype.scrollTo = __noop;
      `,
    });
    await this.cdp.send("Emulation.setDeviceMetricsOverride", {
      width: 900,
      height: 700,
      deviceScaleFactor: 1,
      mobile: false,
    });
    await this.cdp.send("Page.navigate", { url });
  }

  async evaluate(expression, { awaitPromise = false, timeoutMs = 180000 } = {}) {
    const result = await this.cdp.send(
      "Runtime.evaluate",
      {
        expression,
        awaitPromise,
        returnByValue: true,
      },
      timeoutMs,
    );
    if (result.exceptionDetails) {
      const d = result.exceptionDetails;
      throw new Error(d.exception?.description || d.text || "page exception");
    }
    return result.result?.value;
  }

  async close() {
    try {
      this.ws?.close();
    } catch {
      /* already gone */
    }
    this.proc.kill("SIGTERM");
  }
}
