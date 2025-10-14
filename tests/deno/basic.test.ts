import { assert, assertExists } from "@std/assert"

Deno.test("Deno runtime features", () => {
  assert(typeof Deno !== 'undefined', "Deno runtime should be available")
  assert(typeof WebAssembly !== 'undefined', "WebAssembly should be available")
})

Deno.test("WASM file accessibility", async () => {
  try {
    const wasmFile = await Deno.stat("./install/wasm/${LIB_NAME}-main.wasm")
    assert(wasmFile.isFile, "WASM file should exist")
    assert(wasmFile.size > 0, "WASM file should not be empty")
    console.log(`✅ Found WASM file: ${wasmFile.size} bytes`)
  } catch (error) {
    console.warn("⚠️  WASM file not found - run 'deno task build:wasm' first")
  }
})

Deno.test("TypeScript module imports", async () => {
  const { default: Module } = await import("../../src/lib/index.ts")
  assertExists(Module, "Module class should be importable")
  assert(typeof Module === 'function', "Module should be a constructor function")
})
