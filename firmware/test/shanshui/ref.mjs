// Runs the unmodified shan-shui-inf generator in Node and prints the SVG text
// each test case produces, as JSON {name: svg}. The C++ port prints the same
// JSON from run_cases; compare.mjs diffs the two.
//
//   node ref.mjs cases/*.json > ref.json
import { readFileSync } from "node:fs";
import path from "node:path";
import vm from "node:vm";
import { fileURLToPath } from "node:url";

const here = path.dirname(fileURLToPath(import.meta.url));
const html = readFileSync(path.join(here, "../../../vendor/shan-shui-inf/index.html"), "utf8");
const end = html.indexOf('<script id="downloader">');
const code = [...html.slice(0, end).matchAll(/<script[^>]*>([\s\S]*?)<\/script>/g)]
  .map((m) => m[1])
  // Drop the URL seed parsing; each case seeds itself.
  .map((s) => s.replace(/SEED = ""[\s\S]*?Math\.seed\(SEED\);/, ""))
  .join("\n;\n");
const script = new vm.Script(code, { filename: "shan-shui-inf.js" });

function context() {
  const ctx = {
    console: { log() {} },
    document: { addEventListener() {}, getElementById() { return { setAttribute() {}, innerHTML: "" }; } },
    btoa: (s) => Buffer.from(s, "latin1").toString("base64"),
    JSON,
    Math: Object.create(Math),
  };
  ctx.window = ctx;
  ctx.self = ctx;
  vm.createContext(ctx);
  // Math.random is reassigned by the script; give it its own Math object.
  vm.runInContext("Math = Object.assign(Object.create(Object.getPrototypeOf(Math)), {}); " +
    Object.getOwnPropertyNames(Math).map((k) => `Math.${k} = this.__M.${k};`).join(""),
    Object.assign(ctx, { __M: Math }));
  script.runInContext(ctx);
  return ctx;
}

const out = {};
for (const file of process.argv.slice(2)) {
  for (const c of JSON.parse(readFileSync(file, "utf8"))) {
    const ctx = context();
    vm.runInContext(`Math.seed(${JSON.stringify(c.seed)});`, ctx);
    let svg = vm.runInContext(c.js, ctx);
    if (typeof svg !== "string") svg = JSON.stringify(svg);
    out[c.name] = svg.replace(/NaN/g, "-1000");
  }
}
process.stdout.write(JSON.stringify(out, null, 1) + "\n");
