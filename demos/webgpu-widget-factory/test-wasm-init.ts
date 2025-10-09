#!/usr/bin/env -S deno test --allow-read --allow-write
/**
 * Direct WASM initialization test
 * Tests GTK/Pango/Fontconfig initialization without browser
 */

import { assertEquals, assertExists } from "https://deno.land/std@0.208.0/assert/mod.ts";
import { createCanvas } from "https://deno.land/x/canvas@v1.4.2/mod.ts";

interface WASMModule {
  _start_demo(): void;
  _init_webgpu(): number;
  _render_widgets(): void;
  _cleanup(): void;
  _get_fps(): number;
  _get_frame_time(): number;
  _get_widget_count(): number;
  _get_simd_speedup(): number;
  callMain(): void;
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

  // Load preloaded data file (fonts)
  const dataBytes = await Deno.readFile("./gtk-widget-factory-native.data");
  console.log(`Loaded data file: ${dataBytes.length} bytes`);

  // Create real canvas using deno-canvas (backed by Skia)
  const canvas = createCanvas(800, 600);

  // Mock document.getElementById for canvas access
  (globalThis as any).document = {
    getElementById: (id: string) => {
      if (id === 'gtk-canvas') return canvas;
      return null;
    }
  };

  // Initialize module with WASM binary and preloaded data
  const module: WASMModule = await moduleFactory.default({
    wasmBinary: wasmBytes.buffer,
    preloadedImages: {
      "gtk-widget-factory-native.data": dataBytes
    },
    getPreloadedPackage: (name: string) => {
      if (name === "gtk-widget-factory-native.data") {
        return dataBytes.buffer;
      }
      return null;
    },
    print: (text: string) => console.log(`[WASM] ${text}`),
    printErr: (text: string) => console.error(`[WASM ERROR] ${text}`),
    canvas: canvas,
    onAbort: (what: any) => {
      console.error("WASM ABORTED:", what);
      throw new Error(`WASM aborted: ${what}`);
    },
    onRuntimeInitialized: () => {
      console.log("✅ WASM runtime initialized");
    }
  });

  // Call main() after module is initialized
  module.callMain();

  // Wait a bit for filesystem to start loading
  await new Promise(resolve => setTimeout(resolve, 100));

  // Call start_demo - it will retry if fonts aren't ready yet
  // Note: This will throw "unwind" which is expected behavior for emscripten_set_main_loop
  try {
    module._start_demo();
  } catch (e) {
    if (e !== "unwind") {
      throw e; // Re-throw if it's not the expected unwind exception
    }
    // "unwind" is expected - it means the main loop started successfully
  }

  // Wait for initialization to complete (with retries)
  await new Promise(resolve => setTimeout(resolve, 500));

  return { module, canvas };
}

Deno.test({
  name: "WASM module loads and initializes",
  sanitizeResources: false, // Animation loop creates persistent timer
  sanitizeOps: false, // Animation loop keeps async operations running
  async fn() {
    const { module, canvas } = await loadWASMModule();
    assertExists(module);
    assertExists(module._start_demo);
    assertExists(module._init_webgpu);
    assertExists(canvas);

    console.log("\n🧪 Testing initialization...");

    // start_demo() was already called by loadWASMModule()
    // The "unwind" exception is expected - it's how emscripten_set_main_loop works
    // Wait a bit for initialization to complete and render some frames
    await new Promise(resolve => setTimeout(resolve, 300));

    // Get metrics to verify initialization succeeded
    const widgetCount = module._get_widget_count();
    const simdSpeedup = module._get_simd_speedup();

    console.log(`✅ Demo initialized successfully`);
    console.log(`   Widgets initialized: ${widgetCount}`);
    console.log(`   SIMD speedup: ${simdSpeedup}x`);

    assertEquals(widgetCount > 0, true, "Should have initialized widgets");
    assertEquals(widgetCount, 13, "Should have 13 widgets");

    // Export rendered frame to PNG for visual validation
    console.log("📸 Exporting rendered frame to PNG...");
    const pngBuffer = canvas.toBuffer();
    await Deno.writeFile("gtk-widget-factory-test-output.png", pngBuffer);
    console.log(`✅ Saved rendered frame (${pngBuffer.length} bytes)`);

    // Cleanup
    try {
      module._cleanup();
    } catch (e) {
      // Cleanup may also throw if main loop is running
      console.warn("Cleanup warning:", e);
    }
  }
});
