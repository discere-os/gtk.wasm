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

  // Track file loading completion
  let filesLoaded = false;

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
    },
    // Monitor file loading dependencies
    monitorRunDependencies: (left: number) => {
      console.log(`[Deno] Run dependencies remaining: ${left}`);

      if (left === 0) {
        console.log("[Deno] All files loaded, starting demo...");
        filesLoaded = true;

        // Now safe to start demo - fonts are loaded
        // Note: This will throw "unwind" which is expected for emscripten_set_main_loop
        try {
          module.ccall('start_demo', null, [], []);
        } catch (e) {
          if (e !== "unwind") {
            console.error("[Deno] Error starting demo:", e);
            throw e;
          }
          // "unwind" is expected - main loop started successfully
        }
      }
    }
  });

  // Call main() to trigger file loading (with INVOKE_RUN=0, just loads files)
  module.callMain();

  console.log("[Deno] Waiting for file loading to complete...");

  // Wait for files to load and demo to start
  let waitCount = 0;
  while (!filesLoaded && waitCount < 50) {
    await new Promise(resolve => setTimeout(resolve, 100));
    waitCount++;
  }

  if (!filesLoaded) {
    throw new Error("Timeout waiting for files to load");
  }

  // Wait a bit more for initialization to complete
  await new Promise(resolve => setTimeout(resolve, 300));

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

    // start_demo() was already called by monitorRunDependencies callback
    // The "unwind" exception is expected - it's how emscripten_set_main_loop works
    // Wait a bit for initialization to complete and render some frames
    await new Promise(resolve => setTimeout(resolve, 200));

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
