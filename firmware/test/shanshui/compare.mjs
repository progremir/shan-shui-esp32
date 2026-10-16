// node compare.mjs ref.json port.json  -> first difference per case
import { readFileSync } from "node:fs";
const [a, b] = process.argv.slice(2).map((f) => JSON.parse(readFileSync(f, "utf8")));
let bad = 0;
for (const name of Object.keys(a)) {
  const x = a[name];
  const y = b[name];
  if (y === undefined) {
    console.log(`MISSING ${name}`);
    bad++;
    continue;
  }
  if (x === y) {
    console.log(`ok      ${name} (${x.length} chars)`);
    continue;
  }
  bad++;
  let i = 0;
  while (i < x.length && x[i] === y[i]) i++;
  const prims = x.slice(0, i).split("<polyline").length - 1;
  console.log(`DIFF    ${name} at char ${i}, polyline #${prims}`);
  console.log(`  js : ...${x.slice(Math.max(0, i - 80), i + 80)}`);
  console.log(`  c++: ...${y.slice(Math.max(0, i - 80), i + 80)}`);
}
for (const name of Object.keys(b)) if (!(name in a)) console.log(`EXTRA   ${name}`);
process.exit(bad ? 1 : 0);
