#!/bin/bash
# GTK.wasm Unified Build System
# Copyright 2025 Superstruct Ltd
# Single entry point for all GTK WASM builds via Meson

set -euo pipefail

# Configuration
BUILD_TYPE="${1:-standard}"  # minimal, standard, webgpu
CLEAN="${CLEAN:-false}"
FETCH_ONLY="${FETCH_ONLY:-false}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
INSTALL_DIR="$PROJECT_ROOT/install"

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

log() { echo -e "${BLUE}[$(date +%H:%M:%S)]${NC} $1"; }
success() { echo -e "${GREEN}✓${NC} $1"; }
warn() { echo -e "${YELLOW}⚠${NC} $1"; }
error() { echo -e "${RED}✗${NC} $1" >&2; }

# Validate build type
case "$BUILD_TYPE" in
  minimal|standard|webgpu) ;;
  *)
    error "Invalid build type: $BUILD_TYPE"
    echo "Usage: $0 [minimal|standard|webgpu]"
    echo ""
    echo "Build types:"
    echo "  minimal  - Smallest size, no threading, no filesystem"
    echo "  standard - SIMD + threading, 128MB memory (default)"
    echo "  webgpu   - Full WebGPU renderer + ASYNCIFY + demos"
    exit 1
    ;;
esac

# Clean if requested
if [ "$CLEAN" = "true" ]; then
  log "Cleaning build artifacts..."
  rm -rf "$BUILD_DIR" "$INSTALL_DIR"
  success "Clean complete"
fi

cd "$PROJECT_ROOT"

# Step 1: Check dependencies
log "Checking build tools..."
for tool in emcc meson ninja deno; do
  if ! command -v "$tool" &> /dev/null; then
    error "$tool not found in PATH"
    exit 1
  fi
done
success "All build tools available"

# Step 2: Fetch or build dependencies
log "Fetching WASM dependencies..."
if [ -f "dependencies.json" ]; then
  bash "$SCRIPT_DIR/fetch-dependencies.sh" --manifest dependencies.json || {
    warn "CDN fetch failed for some modules, will build locally"
  }
else
  log "No dependencies.json, will build dependencies via Meson"
fi

if [ "$FETCH_ONLY" = "true" ]; then
  success "Dependencies fetched (FETCH_ONLY mode)"
  exit 0
fi

# Step 3: Configure Meson (if not already configured)
if [ ! -d "$BUILD_DIR" ]; then
  log "Configuring Meson build..."

  meson setup "$BUILD_DIR" \
    --cross-file=emscripten-cross.ini \
    --prefix="$INSTALL_DIR" \
    --libdir=wasm \
    --bindir=wasm \
    -Dbuildtype=release \
    -Dwasm_build_type="$BUILD_TYPE" \
    -Dbroadway-backend=true \
    -Dx11-backend=false \
    -Dwayland-backend=false \
    -Dwin32-backend=false \
    -Dmacos-backend=false \
    -Dintrospection=disabled \
    -Ddocumentation=false \
    -Dman-pages=false \
    -Dbuild-demos=false \
    -Dbuild-testsuite=false \
    -Dbuild-examples=false \
    -Dbuild-tests=false

  success "Meson configured for $BUILD_TYPE build"
else
  log "Meson already configured, reconfiguring for $BUILD_TYPE..."
  meson configure "$BUILD_DIR" -Dwasm_build_type="$BUILD_TYPE"
fi

# Step 4: Build GTK WASM modules
log "Building GTK WASM modules ($BUILD_TYPE)..."
meson compile -C "$BUILD_DIR" gtk-main gtk-side

if [ ! -f "$BUILD_DIR/wasm/gtk-main.js" ]; then
  error "Build failed: gtk-main.js not found"
  exit 1
fi

success "GTK WASM modules built"

# Step 5: Install to install/wasm
log "Installing WASM modules..."
meson install -C "$BUILD_DIR"

# Verify installation
if [ -f "$INSTALL_DIR/wasm/gtk-main.js" ] && [ -f "$INSTALL_DIR/wasm/gtk-side.wasm" ]; then
  success "WASM modules installed to $INSTALL_DIR/wasm/"
else
  error "Installation verification failed"
  exit 1
fi

# Step 6: Post-process with wasm-opt (if available)
if command -v wasm-opt &> /dev/null && [ "$BUILD_TYPE" != "minimal" ]; then
  log "Optimizing WASM binaries with wasm-opt..."

  for wasm_file in "$INSTALL_DIR/wasm"/*.wasm; do
    if [ -f "$wasm_file" ]; then
      wasm-opt -O3 --enable-simd "$wasm_file" -o "${wasm_file%.wasm}.opt.wasm"
      mv "${wasm_file%.wasm}.opt.wasm" "$wasm_file"
    fi
  done

  success "WASM binaries optimized"
else
  if command -v wasm-opt &> /dev/null; then
    log "Skipping wasm-opt for minimal build"
  else
    warn "wasm-opt not found, skipping post-optimization"
  fi
fi

# Step 7: Generate manifest
log "Generating WASM manifest..."
if command -v deno &> /dev/null; then
  deno run --allow-read --allow-write --allow-run \
    "$PROJECT_ROOT/../../../../scripts/generate-wasm-manifest.ts" "$PROJECT_ROOT" || {
    warn "Manifest generation failed (non-fatal)"
  }
else
  warn "Deno not found, skipping manifest generation"
fi

# Step 8: Build summary
echo ""
echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${GREEN}✓ GTK.wasm Build Complete ($BUILD_TYPE)${NC}"
echo -e "${GREEN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""
echo "Build artifacts:"
ls -lh "$INSTALL_DIR/wasm"/*.{js,wasm} 2>/dev/null || true
echo ""
echo "Next steps:"
echo "  deno task demo        # Run demo"
echo "  deno task test        # Run tests"
echo "  deno task clean       # Clean build"
echo ""
echo "Other build types:"
echo "  bash scripts/unified-build.sh minimal   # Smallest size"
echo "  bash scripts/unified-build.sh standard  # Default"
echo "  bash scripts/unified-build.sh webgpu    # Full WebGPU"
echo ""
