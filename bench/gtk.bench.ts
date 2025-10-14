/**
 * Gtk WASM Benchmarks
 */

import GtkWASM from "../src/lib/index.ts"

Deno.bench("gtk initialization", {
  baseline: true
}, async () => {
  const lib = new GtkWASM()
  await lib.initialize()
})
