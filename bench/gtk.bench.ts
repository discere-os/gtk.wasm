/**
 * ${LIB_TITLE} WASM Benchmarks
 */

import ${LIB_TITLE}WASM from "../src/lib/index.ts"

Deno.bench("${LIB_NAME} initialization", {
  baseline: true
}, async () => {
  const lib = new ${LIB_TITLE}WASM()
  await lib.initialize()
})
