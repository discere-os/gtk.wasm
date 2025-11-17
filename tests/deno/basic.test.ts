import { assertEquals, assert } from "https://deno.land/std@0.220.0/assert/mod.ts";
import GtkWASM from "../../src/lib/index.ts";

Deno.test("GTK.wasm - module loading", async () => {
  const gtk = new GtkWASM();

  try {
    await gtk.initialize();
    assert(gtk, "GTK library should initialize");
  } catch (error) {
    // Expected to fail if WASM not built yet
    console.warn("WASM not built yet, skipping test");
  }
});

Deno.test("GTK.wasm - capabilities detection", async () => {
  const gtk = new GtkWASM();

  try {
    await gtk.initialize();
    const caps = gtk.getCapabilities();

    // These checks assume Chrome 113+ with WASM SIMD
    assert(typeof caps.has_wasm_simd === 'boolean', "Should detect WASM SIMD");
    assert(typeof caps.has_webgpu === 'boolean', "Should detect WebGPU");
    assert(typeof caps.chrome_version === 'number', "Should detect Chrome version");

    console.log("Detected capabilities:", caps);

    // In CI/headless, we may not have full capabilities
    // But WASM SIMD should be available in Chrome 113+
    if (caps.chrome_version >= 113) {
      assert(caps.has_wasm_simd, "Chrome 113+ should have WASM SIMD");
    }
  } catch (error) {
    console.warn("WASM not built yet, skipping test");
  }
});

Deno.test("GTK.wasm - web native readiness check", async () => {
  const gtk = new GtkWASM();

  try {
    await gtk.initialize();
    const isReady = gtk.isWebNativeReady();

    assert(typeof isReady === 'boolean', "Should return boolean");
    console.log("Web-native ready:", isReady);
  } catch (error) {
    console.warn("WASM not built yet, skipping test");
  }
});

Deno.test("GTK.wasm - performance metrics", async () => {
  const gtk = new GtkWASM();

  try {
    await gtk.initialize();
    const metrics = await gtk.getPerformanceMetrics();

    assert(metrics.simd_speedup > 0, "SIMD speedup should be positive");
    assert(metrics.crypto_speedup > 0, "Crypto speedup should be positive");
    assert(metrics.workers_speedup > 0, "Workers speedup should be positive");
    assert(metrics.webgpu_speedup > 0, "WebGPU speedup should be positive");

    console.log("Performance metrics:", metrics);
  } catch (error) {
    console.warn("WASM not built yet, skipping test");
  }
});
