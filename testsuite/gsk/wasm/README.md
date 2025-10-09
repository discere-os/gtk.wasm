# GSK WASM Compare Tests

Automated testing of GSK render nodes against reference PNGs using WebAssembly and Deno.

## Overview

This directory contains infrastructure for running GTK's GSK (GTK Scene Kit) rendering tests in WebAssembly. It compiles GSK to WASM, renders `.node` files to PNG, and compares them against reference images using pixelmatch.

**Status**: ✅ Sprint 1 Complete - Core infrastructure ready

## Architecture

```
.node files (GSK DSL)
    ↓
gsk-node-renderer.c (WASM module)
    ↓ gsk_renderer_render_texture()
    ↓ copy to Cairo surface
    ↓ cairo_image_surface_get_data()
    ↓
deno-canvas (Skia-backed)
    ↓ canvas.toBuffer()
    ↓
pixelmatch comparison vs reference PNG
    ↓
Test report (JSON + diff PNGs)
```

## Files

- **gsk-node-renderer.c** - WASM renderer module
- **run-compare-tests.ts** - Deno test harness
- **deno.json** - Deno configuration
- **meson.build** - Build system integration
- **test-output/** - Generated test outputs (git ignored)

## Requirements

- Deno 1.x (`curl -fsSL https://deno.land/install.sh | sh`)
- Emscripten SDK (for building WASM)
- Meson build system
- GTK/GSK dependencies

## Building

### Via Meson (Recommended)

```bash
# From gtk.wasm root
meson setup build
meson compile -C build

# WASM module will be at: build/testsuite/gsk/wasm/gsk-node-renderer.{js,wasm}
```

### Manual Build

```bash
cd testsuite/gsk/wasm

# Ensure Emscripten is activated
source $EMSDK/emsdk_env.sh

# Build WASM module
emcc gsk-node-renderer.c \
  -I../../../gtk \
  -I../../../gsk \
  -I../../../gdk \
  # ... (see meson.build for full flags)
  -o gsk-node-renderer.js
```

## Running Tests

### All Tests

```bash
cd testsuite/gsk/wasm
deno task test
```

### Single Test

```bash
deno task test --test blend-modes
```

### With Verbose Output

```bash
deno task test --verbose
```

### Via Meson

```bash
# From build directory
meson test gsk-compare-wasm

# Run smoke tests only
meson test gsk-wasm-smoke

# Run specific test
meson test gsk-wasm-smoke-blend-modes
```

## Test Output

Results are saved to `test-output/`:

- **test-results.json** - JSON report with pass/fail status
- **{test}-{variation}.out.png** - Rendered output (on failure)
- **{test}-{variation}.diff.png** - Visual diff (on failure)

### Example JSON Report

```json
{
  "timestamp": "2025-10-09T22:00:00Z",
  "totalTests": 227,
  "passed": 220,
  "failed": 7,
  "successRate": 96.9,
  "results": [
    {
      "name": "blend-modes",
      "variation": "plain",
      "passed": true,
      "diffPixels": 0,
      "diffPercentage": 0
    },
    ...
  ]
}
```

## Image Comparison

Uses **pixelmatch** for pixel-perfect comparison:

- **Threshold**: 0.1 (anti-aliasing tolerance)
- **Max Difference**: 0.01% (acceptable pixel difference)
- **Output**: Visual diff PNGs with red highlighting

### Comparison Features

- ✅ Anti-aliasing tolerance
- ✅ Visual diff output
- ✅ Configurable thresholds
- ✅ Sub-pixel accuracy

## Test Variations

Currently implemented:
- ✅ `plain` - As-is rendering

TODO (Sprint 3):
- ⏳ `flip` - Horizontal flip
- ⏳ `rotate` - 90° rotation
- ⏳ `repeat` - 3x3 tiled repeat
- ⏳ `mask` - Alpha mask
- ⏳ `replay` - Snapshot replay
- ⏳ `clip` - Random clip rect
- ⏳ `colorflip` - R/G channel swap

## Development

### Adding New Tests

Reference tests are in `../compare/`:
- Add `{name}.node` (GSK render tree)
- Add `{name}.png` (reference image)
- Test will be auto-discovered

### Debugging Failed Tests

```bash
# Run single test with verbose output
deno task test --test {name} --verbose

# Check test output
ls test-output/{name}-plain.*
  # .out.png  - Rendered output
  # .diff.png - Visual difference
  # Compare visually or with image viewer
```

### Modifying Comparison Threshold

Edit `run-compare-tests.ts`:
```typescript
const COMPARISON_THRESHOLD = 0.1; // Anti-aliasing tolerance
const MAX_DIFF_PERCENTAGE = 0.01; // Max 0.01% difference
```

## CI/CD Integration

Tests run automatically in GitHub Actions on:
- Push to `main` or `wasm` branches
- Pull requests modifying GTK/GSK code

See `.github/workflows/gsk-wasm-tests.yml` (TODO: Sprint 4)

## Performance

**Current (Sprint 1)**:
- ~10-15 seconds per test (including WASM load)
- Sequential execution
- Single variation per test

**Planned Optimizations**:
- Parallel test execution (Deno workers)
- WASM module reuse across tests
- Smart caching for unchanged tests
- **Target**: < 10 minutes for all 227 tests

## Troubleshooting

### "Deno not found"

```bash
curl -fsSL https://deno.land/install.sh | sh
export PATH="$HOME/.deno/bin:$PATH"
```

### "WASM module not found"

```bash
# Build WASM module first
cd testsuite/gsk/wasm
meson setup build
meson compile -C build
```

### "Renderer initialization failed"

Check that:
- GTK dependencies are installed
- WASM module built successfully
- Node files are valid GSK DSL

### High failure rate

- Check reference PNGs are up-to-date
- Verify font availability (Roboto required)
- Review diff PNGs in test-output/
- Adjust COMPARISON_THRESHOLD if needed

## Related Documentation

- [GTK WASM Testsuite Port Strategy](../../../../docs/implementation/gtk-wasm-testsuite-port.md)
- [GTK WASM Widget Factory Status](../../../../docs/implementation/gtk-wasm-widget-factory-status.md)
- [GSK Render Node Format](https://docs.gtk.org/gsk4/class.RenderNode.html)

## License

LGPL-2.1-or-later (same as GTK)

## Credits

- **Architecture**: Based on widget-factory demo proven model
- **Image Comparison**: pixelmatch library
- **Canvas Rendering**: deno-canvas (Skia)
- **Implementation**: Claude Code + Superstruct Ltd
