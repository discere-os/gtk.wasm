#!/bin/bash
set -euo pipefail

# Build MAIN module with WebGPU support
# Simplified build using available pre-built modules

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

log_info "Building WebGPU MAIN module..."

# Create build directory
mkdir -p build-main
cd build-main

# Build the MAIN module with minimal dependencies
log_info "Compiling MAIN module with WebGPU support..."

emcc ../demos/webgpu-widget-factory/main.c \
    -I../wasm \
    -I../gsk/gpu \
    -O3 -flto -msimd128 \
    -sMODULARIZE=1 \
    -sEXPORT_ES6=1 \
    -sEXPORT_NAME="GTKWebGPUModule" \
    -sEXPORTED_FUNCTIONS='["_main","_malloc","_free"]' \
    -sEXPORTED_RUNTIME_METHODS='["cwrap","ccall","UTF8ToString","stringToUTF8","lengthBytesUTF8"]' \
    -sALLOW_MEMORY_GROWTH=1 \
    -sINITIAL_MEMORY=134217728 \
    -sMAXIMUM_MEMORY=1073741824 \
    --use-port=emdawnwebgpu \
    -sASYNCIFY=1 \
    -sENVIRONMENT=web \
    -sWASM_BIGINT=1 \
    -sPTHREAD_POOL_SIZE=4 \
    -sMAIN_MODULE=1 \
    -o gtk-webgpu-main.js || {
        log_error "Failed to build MAIN module"
        exit 1
    }

# Copy outputs
log_info "Installing MAIN module..."
cp gtk-webgpu-main.js ../install/wasm/
cp gtk-webgpu-main.wasm ../install/wasm/

# Also copy to demo directory
cp gtk-webgpu-main.js ../demos/webgpu-widget-factory/
cp gtk-webgpu-main.wasm ../demos/webgpu-widget-factory/

cd ..

# Check sizes
MAIN_SIZE=$(stat -c%s install/wasm/gtk-webgpu-main.wasm 2>/dev/null || stat -f%z install/wasm/gtk-webgpu-main.wasm 2>/dev/null)
MAIN_SIZE_MB=$((MAIN_SIZE / 1024 / 1024))

log_success "✅ WebGPU MAIN module built successfully!"
echo ""
echo "📊 Build Results:"
echo "  MAIN Module: ${MAIN_SIZE_MB}MB"
echo "  Location: install/wasm/gtk-webgpu-main.js"
echo ""
echo "📦 Available WASM Modules:"
ls -lh install/wasm/*.wasm 2>/dev/null | awk '{print "  " $9 " (" $5 ")"}'
echo ""
echo "🚀 Next Steps:"
echo "  1. Run: ./serve-demo.ts"
echo "  2. Open: http://localhost:8080/demos/webgpu-widget-factory/"
echo ""