import { execFileSync, spawn } from "node:child_process";
import fs from "node:fs";
import { readdirSync } from "node:fs";

export function listSerialPorts() {
  let names = [];
  try {
    names = readdirSync("/dev");
  } catch {
    return [];
  }
  return names
    .filter((n) => /^(cu\.usb|tty\.usb|ttyACM|ttyUSB|cu\.wchusb|cu\.SLAB)/.test(n))
    .map((n) => `/dev/${n}`)
    .sort();
}

function sttyRaw(device) {
  const flag = process.platform === "darwin" ? "-f" : "-F";
  try {
    execFileSync("stty", [flag, device, "115200", "raw", "cs8", "-cstopb", "-parenb", "-echo", "-ixon", "clocal"], {
      stdio: "ignore",
    });
  } catch {
    /* a pty may already be raw */
  }
}

export class PortClosed extends Error {
  constructor(device, cause) {
    super(`${device} is gone${cause ? `: ${cause}` : ""}`);
    this.code = "PORT_CLOSED";
    this.device = device;
  }
}

export function openSerial(device) {
  sttyRaw(device);
  const fd = fs.openSync(device, fs.constants.O_RDWR | fs.constants.O_NOCTTY | fs.constants.O_NONBLOCK);
  const inbox = [];
  let waiter = null;
  let closed = false;
  let gone = null;
  const buf = Buffer.alloc(8192);

  function noteGone(cause) {
    if (closed) return;
    closed = true;
    clearInterval(timer);
    gone = new PortClosed(device, cause);
    if (waiter) {
      const w = waiter;
      waiter = null;
      w(null);
    }
  }

  let ticks = 0;
  function pump() {
    if (closed) return;
    // Power-off removes the device node. A stuck fd may not surface EIO on
    // its own, so a missing path is the same as an unplug.
    if ((++ticks & 31) === 0 && !fs.existsSync(device)) {
      noteGone("unplugged");
      return;
    }
    try {
      const n = fs.readSync(fd, buf, 0, buf.length, null);
      if (n > 0) {
        const piece = Buffer.from(buf.subarray(0, n));
        if (waiter) {
          const w = waiter;
          waiter = null;
          w(piece);
        } else inbox.push(piece);
      }
    } catch (err) {
      if (err.code === "EAGAIN" || err.code === "EWOULDBLOCK") return;
      noteGone(err.code || err.message);
    }
  }

  const timer = setInterval(pump, 5);
  if (timer.unref) timer.unref();

  return {
    write(data) {
      if (gone) throw gone;
      const bytes = Buffer.from(data);
      let off = 0;
      const pause = new Int32Array(new SharedArrayBuffer(4));
      while (off < bytes.length) {
        if (gone) throw gone;
        pump();
        if (gone) throw gone;
        try {
          const n = fs.writeSync(fd, bytes, off, bytes.length - off);
          if (n > 0) off += n;
        } catch (err) {
          if (err.code === "EAGAIN" || err.code === "EWOULDBLOCK") {
            Atomics.wait(pause, 0, 0, 2);
            continue;
          }
          noteGone(err.code || err.message);
          throw gone;
        }
      }
    },
    read(ms) {
      if (gone) return Promise.reject(gone);
      pump();
      if (gone) return Promise.reject(gone);
      if (inbox.length) return Promise.resolve(inbox.shift());
      return new Promise((resolve, reject) => {
        const t = setTimeout(() => {
          if (waiter) {
            waiter = null;
            pump();
            if (gone) reject(gone);
            else resolve(inbox.shift() || null);
          }
        }, ms);
        waiter = (chunk) => {
          clearTimeout(t);
          if (gone) reject(gone);
          else resolve(chunk);
        };
      });
    },
    close() {
      closed = true;
      clearInterval(timer);
      try {
        fs.closeSync(fd);
      } catch {
        /* the power button already took the device */
      }
    },
  };
}

// PTY slave plus a TCP socket bridged to the master, so a test can run the
// real openSerial() path against another process.
export function bridgePty() {
  const py = `
import os, pty, socket, select, tty, sys
master, slave = pty.openpty()
tty.setraw(master)
srv = socket.socket()
srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
srv.bind(("127.0.0.1", 0))
srv.listen(1)
sys.stdout.buffer.write((os.ttyname(slave) + " " + str(srv.getsockname()[1]) + "\\n").encode())
sys.stdout.buffer.flush()
conn, _ = srv.accept()
while True:
    r, _, _ = select.select([master, conn], [], [], 0.5)
    if master in r:
        data = os.read(master, 8192)
        if not data:
            break
        conn.sendall(data)
    if conn in r:
        data = conn.recv(8192)
        if not data:
            break
        os.write(master, data)
`;
  const proc = spawn("python3", ["-c", py], { stdio: ["ignore", "pipe", "pipe"] });
  let err = "";
  proc.stderr.on("data", (d) => {
    err += d.toString();
  });
  return new Promise((resolve, reject) => {
    let buf = "";
    proc.stdout.on("data", (d) => {
      buf += d.toString();
      const nl = buf.indexOf("\n");
      if (nl >= 0) {
        const [device, port] = buf.slice(0, nl).trim().split(" ");
        resolve({
          device,
          port: Number(port),
          close() {
            proc.kill("SIGTERM");
          },
        });
      }
    });
    proc.on("exit", (code) => {
      if (!buf.includes(" ")) reject(new Error(`pty bridge exited ${code} ${err}`));
    });
  });
}
