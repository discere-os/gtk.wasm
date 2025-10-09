#!/usr/bin/env -S deno test --allow-read --allow-write
/**
 * Direct WASM initialization test
 * Tests GTK/Pango/Fontconfig initialization without browser
 */

import { assertEquals, assertExists } from "https://deno.land/std@0.208.0/assert/mod.ts";

interface WASMModule {
  _init_webgpu(): number;
  _render_widgets(): void;
  _cleanup(): void;
  _get_fps(): number;
  _get_frame_time(): number;
  _get_widget_count(): number;
  _get_simd_speedup(): number;
  ccall: (name: string, returnType: string, argTypes: string[], args: any[]) => any;
  cwrap: (name: string, returnType: string, argTypes: string[]) => Function;
}

async function loadWASMModule(): Promise<WASMModule> {
  console.log("Loading WASM module...");

  // Load the JS wrapper
  const moduleFactory = await import("./gtk-widget-factory-native.js");

  // Load WASM binary
  const wasmBytes = await Deno.readFile("./gtk-widget-factory-native.wasm");

  console.log(`Loaded WASM: ${wasmBytes.length} bytes`);

  // Initialize module with WASM binary
  const module: WASMModule = await moduleFactory.default({
    wasmBinary: wasmBytes.buffer,
    print: (text: string) => console.log(`[WASM] ${text}`),
    printErr: (text: string) => console.error(`[WASM ERROR] ${text}`),
    canvas: createMockCanvas(),
    onAbort: (what: any) => {
      console.error("WASM ABORTED:", what);
      throw new Error(`WASM aborted: ${what}`);
    },
    onRuntimeInitialized: () => {
      console.log("✅ WASM runtime initialized");
    }
  });

  return module;
}

function createMockCanvas() {
  return {
    width: 800,
    height: 600,
    getContext: () => ({
      createImageData: (w: number, h: number) => ({
        data: new Uint8ClampedArray(w * h * 4),
        width: w,
        height: h
      }),
      putImageData: () => {}
    }),
    addEventListener: () => {},
    removeEventListener: () => {}
  };
}

Deno.test("WASM module loads", async () => {
  const module = await loadWASMModule();
  assertExists(module);
  assertExists(module._init_webgpu);
});

Deno.test("init_webgpu initializes successfully", async () => {
  const module = await loadWASMModule();

  console.log("\n🧪 Testing init_webgpu()...");

  try {
    const result = module._init_webgpu();
    console.log(`init_webgpu() returned: ${result}`);

    assertEquals(result, 0, "init_webgpu should return 0 on success");

    console.log("✅ Initialization successful");

    // Get metrics
    const widgetCount = module._get_widget_count();
    const simdSpeedup = module._get_simd_speedup();

    console.log(`Widgets initialized: ${widgetCount}`);
    console.log(`SIMD speedup: ${simdSpeedup}x`);

  } catch (error) {
    console.error("❌ Initialization failed:", error);
    throw error;
  } finally {
    // Cleanup
    try {
      module._cleanup();
    } catch (e) {
      console.warn("Cleanup warning:", e);
    }
  }
});

Deno.test("render_widgets works after init", async () => {
  const module = await loadWASMModule();

  const initResult = module._init_webgpu();
  assertEquals(initResult, 0);

  try {
    // Try rendering a frame
    module._render_widgets();
    console.log("✅ First render successful");

    // Try a few more frames
    for (let i = 0; i < 5; i++) {
      module._render_widgets();
    }
    console.log("✅ Multiple renders successful");

  } finally {
    module._cleanup();
  }
});
