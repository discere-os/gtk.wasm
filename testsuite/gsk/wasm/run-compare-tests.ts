#!/usr/bin/env -S deno run --allow-read --allow-write
/**
 * GSK Compare Tests Runner
 * Automated testing of GSK render nodes against reference PNGs
 */

import { createCanvas } from "https://deno.land/x/canvas@v1.4.2/mod.ts";
import { walk } from "https://deno.land/std@0.208.0/fs/walk.ts";
import { assertEquals, assertExists } from "https://deno.land/std@0.208.0/assert/mod.ts";
import { parse as parseFlags } from "https://deno.land/std@0.208.0/flags/mod.ts";
import pixelmatch from "npm:pixelmatch@5.3.0";
import { PNG } from "npm:pngjs@7.0.0";

interface TestCase {
  nodePath: string;
  refPath: string;
  name: string;
  width?: number;
  height?: number;
}

interface TestResult {
  name: string;
  variation: string;
  passed: boolean;
  diffPixels?: number;
  diffPercentage?: number;
  ssim?: number;
  error?: string;
}

interface WASMModule {
  _init_renderer(width: number, height: number): number;
  _render_node_file(pathPtr: number): number;
  _get_render_data(): number;
  _get_render_width(): number;
  _get_render_height(): number;
  _get_render_stride(): number;
  _cleanup_renderer(): void;
  UTF8ToString(ptr: number): string;
  stringToUTF8(str: string, outPtr: number, maxLength: number): void;
  _malloc(size: number): number;
  _free(ptr: number): void;
  HEAPU8: Uint8Array;
}

const TEST_VARIATIONS = [
  "plain",
  // TODO: Implement these variations
  // "flip", "rotate", "repeat", "mask", "replay", "clip", "colorflip"
];

const COMPARISON_THRESHOLD = 0.1; // Anti-aliasing tolerance
const MAX_DIFF_PERCENTAGE = 0.01; // 0.01% max difference

async function loadWASMRenderer(): Promise<{ module: WASMModule; canvas: any }> {
  console.log("📦 Loading WASM renderer...");

  // Load the JS wrapper
  const moduleFactory = await import("./gsk-node-renderer.js");

  // Load WASM binary
  const wasmBytes = await Deno.readFile("./gsk-node-renderer.wasm");
  console.log(`   Loaded WASM: ${wasmBytes.length} bytes`);

  // Create canvas (max bounds)
  const canvas = createCanvas(1000, 1000);

  // Mock document for canvas access
  (globalThis as any).document = {
    getElementById: (id: string) => {
      if (id === 'gsk-canvas') return canvas;
      return null;
    }
  };

  // Initialize module
  const module: WASMModule = await moduleFactory.default({
    wasmBinary: wasmBytes.buffer,
    canvas: canvas,
    print: (text: string) => console.log(`   [WASM] ${text}`),
    printErr: (text: string) => console.error(`   [WASM ERROR] ${text}`),
  });

  console.log("✅ WASM renderer loaded\n");
  return { module, canvas };
}

async function findAllTests(baseDir: string): Promise<TestCase[]> {
  const tests: TestCase[] = [];

  for await (const entry of walk(baseDir, { exts: [".node"] })) {
    const nodePath = entry.path;
    const refPath = nodePath.replace(".node", ".png");

    // Check if reference PNG exists
    try {
      await Deno.stat(refPath);
      tests.push({
        nodePath,
        refPath,
        name: entry.name.replace(".node", ""),
      });
    } catch {
      // No reference PNG, skip this test
      continue;
    }
  }

  return tests.sort((a, b) => a.name.localeCompare(b.name));
}

async function renderTest(
  module: WASMModule,
  canvas: any,
  testCase: TestCase
): Promise<Uint8Array> {
  // Allocate memory for file path
  const pathBytes = new TextEncoder().encode(testCase.nodePath + '\0');
  const pathPtr = module._malloc(pathBytes.length);
  module.HEAPU8.set(pathBytes, pathPtr);

  try {
    // Initialize renderer with test dimensions (or max)
    const initResult = module._init_renderer(1000, 1000);
    if (initResult !== 0) {
      throw new Error(`Renderer initialization failed: ${initResult}`);
    }

    // Render node file
    const renderResult = module._render_node_file(pathPtr);
    if (renderResult !== 0) {
      throw new Error(`Render failed with code: ${renderResult}`);
    }

    // Get rendered dimensions
    const width = module._get_render_width();
    const height = module._get_render_height();

    // Store dimensions for comparison
    testCase.width = width;
    testCase.height = height;

    // Export to PNG via canvas
    return canvas.toBuffer();

  } finally {
    module._free(pathPtr);
    module._cleanup_renderer();
  }
}

function loadPNG(buffer: Uint8Array): PNG {
  return PNG.sync.read(buffer);
}

function compareImages(
  rendered: Uint8Array,
  reference: Uint8Array
): { passed: boolean; diffPixels: number; diffPercentage: number; diffPNG?: Uint8Array } {
  const renderedPNG = loadPNG(rendered);
  const referencePNG = loadPNG(reference);

  // Check dimensions match
  if (renderedPNG.width !== referencePNG.width ||
      renderedPNG.height !== referencePNG.height) {
    return {
      passed: false,
      diffPixels: -1,
      diffPercentage: 100,
    };
  }

  const { width, height } = renderedPNG;
  const diffPNG = new PNG({ width, height });

  // Use pixelmatch for precise comparison with anti-aliasing tolerance
  const diffPixels = pixelmatch(
    renderedPNG.data,
    referencePNG.data,
    diffPNG.data,
    width,
    height,
    {
      threshold: COMPARISON_THRESHOLD,
      includeAA: false, // Ignore anti-aliasing differences
      diffColor: [255, 0, 0], // Red for differences
      diffColorAlt: [255, 255, 0], // Yellow for anti-aliasing
    }
  );

  const totalPixels = width * height;
  const diffPercentage = (diffPixels / totalPixels) * 100;
  const passed = diffPercentage <= MAX_DIFF_PERCENTAGE;

  let diffBuffer: Uint8Array | undefined;
  if (!passed) {
    // Only generate diff PNG if test failed
    diffBuffer = PNG.sync.write(diffPNG);
  }

  return {
    passed,
    diffPixels,
    diffPercentage,
    diffPNG: diffBuffer,
  };
}

async function runSingleTest(
  module: WASMModule,
  canvas: any,
  testCase: TestCase,
  variation: string,
  outputDir: string
): Promise<TestResult> {
  const testName = `${testCase.name}-${variation}`;

  try {
    // Render with WASM
    const rendered = await renderTest(module, canvas, testCase);

    // Load reference PNG
    const reference = await Deno.readFile(testCase.refPath);

    // Compare images
    const comparison = compareImages(rendered, reference);

    // Save outputs if test failed or verbose mode
    if (!comparison.passed || Deno.args.includes("--verbose")) {
      await Deno.writeFile(
        `${outputDir}/${testName}.out.png`,
        rendered
      );

      if (comparison.diffPNG) {
        await Deno.writeFile(
          `${outputDir}/${testName}.diff.png`,
          comparison.diffPNG
        );
      }
    }

    return {
      name: testCase.name,
      variation,
      passed: comparison.passed,
      diffPixels: comparison.diffPixels,
      diffPercentage: comparison.diffPercentage,
    };

  } catch (error) {
    return {
      name: testCase.name,
      variation,
      passed: false,
      error: error.message,
    };
  }
}

async function runAllTests(args: any) {
  const baseDir = args.dir || "../compare";
  const outputDir = args.output || "./test-output";
  const singleTest = args.test;

  // Ensure output directory exists
  await Deno.mkdir(outputDir, { recursive: true });

  console.log("🔍 GSK Compare Test Runner\n");
  console.log(`Compare dir: ${baseDir}`);
  console.log(`Output dir:  ${outputDir}\n`);

  // Load WASM renderer
  const { module, canvas } = await loadWASMRenderer();

  // Find test cases
  console.log("📁 Finding test cases...");
  let tests = await findAllTests(baseDir);

  if (singleTest) {
    tests = tests.filter(t => t.name === singleTest);
    if (tests.length === 0) {
      console.error(`❌ Test '${singleTest}' not found`);
      Deno.exit(1);
    }
  }

  console.log(`   Found ${tests.length} test cases\n`);

  // Run tests
  const results: TestResult[] = [];
  let passed = 0;
  let failed = 0;

  console.log("🧪 Running tests...\n");

  for (const test of tests) {
    for (const variation of TEST_VARIATIONS) {
      const result = await runSingleTest(
        module,
        canvas,
        test,
        variation,
        outputDir
      );

      results.push(result);

      if (result.passed) {
        console.log(`✅ ${result.name}-${variation}`);
        passed++;
      } else {
        const errorMsg = result.error ||
          `${result.diffPercentage?.toFixed(3)}% diff (${result.diffPixels} px)`;
        console.log(`❌ ${result.name}-${variation} - ${errorMsg}`);
        failed++;
      }
    }
  }

  // Summary
  const total = passed + failed;
  const successRate = (passed / total) * 100;

  console.log(`\n📊 Test Results`);
  console.log(`   Total:   ${total}`);
  console.log(`   Passed:  ${passed}`);
  console.log(`   Failed:  ${failed}`);
  console.log(`   Success: ${successRate.toFixed(1)}%\n`);

  // Write JSON report
  const report = {
    timestamp: new Date().toISOString(),
    totalTests: total,
    passed,
    failed,
    successRate,
    results,
  };

  await Deno.writeTextFile(
    `${outputDir}/test-results.json`,
    JSON.stringify(report, null, 2)
  );

  console.log(`📄 Test report saved to ${outputDir}/test-results.json\n`);

  // Exit with error code if any tests failed
  if (failed > 0) {
    Deno.exit(1);
  }
}

if (import.meta.main) {
  const args = parseFlags(Deno.args, {
    string: ["dir", "output", "test"],
    boolean: ["verbose", "help"],
    alias: {
      d: "dir",
      o: "output",
      t: "test",
      v: "verbose",
      h: "help",
    },
  });

  if (args.help) {
    console.log(`
GSK Compare Test Runner

Usage:
  deno run --allow-read --allow-write run-compare-tests.ts [options]

Options:
  -d, --dir <path>      Test directory (default: ../compare)
  -o, --output <path>   Output directory (default: ./test-output)
  -t, --test <name>     Run single test by name
  -v, --verbose         Save all test outputs (not just failures)
  -h, --help            Show this help

Examples:
  # Run all tests
  deno task test

  # Run single test
  deno task test --test blend-modes

  # Custom directories
  deno task test --dir ../compare --output ./results
`);
    Deno.exit(0);
  }

  await runAllTests(args);
}
