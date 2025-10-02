#!/bin/bash
set -euo pipefail

# Build GTK WebGPU demo with SIMD optimizations
echo "🔨 Building GTK WebGPU demo with SIMD optimizations..."

# Source file
SOURCE="main_minimal.c"

# Output files
OUTPUT_JS="gtk-webgpu-main.js"
OUTPUT_WASM="gtk-webgpu-main.wasm"

# Clean previous build
rm -f "$OUTPUT_JS" "$OUTPUT_WASM"

# Compile with Emscripten
emcc "$SOURCE" \
    -O3 \
    -flto \
    -msimd128 \
    -sWASM=1 \
    -sMODULARIZE=1 \
    -sEXPORT_ES6=1 \
    -sEXPORT_NAME="GTKWebGPUModule" \
    --use-port=emdawnwebgpu \
    -sALLOW_MEMORY_GROWTH=1 \
    -sNO_FILESYSTEM=1 \
    -sENVIRONMENT=web,webview,worker \
    -sNODEJS_CATCH_EXIT=0 \
    -sNODEJS_CATCH_REJECTION=0 \
    -sINITIAL_MEMORY=67108864 \
    -sMAXIMUM_MEMORY=536870912 \
    -sEXPORTED_FUNCTIONS='["_main","_init_webgpu","_stress_test_widgets","_webgpu_factory_run_benchmark"]' \
    -sEXPORTED_RUNTIME_METHODS='["cwrap","ccall","UTF8ToString"]' \
    -sASYNCIFY=1 \
    -sASYNCIFY_STACK_SIZE=16384 \
    -o "$OUTPUT_JS"

echo "✅ Build completed successfully!"
echo "📁 Generated files:"
echo "   - $OUTPUT_JS"
echo "   - $OUTPUT_WASM"

# Show file sizes
echo "📊 File sizes:"
ls -lh "$OUTPUT_JS" "$OUTPUT_WASM"

echo ""
echo "🚀 SIMD optimization: ENABLED"
echo "🎮 WebGPU support: ENABLED"
echo "⚡ Functions exported: main, init_webgpu, stress_test_widgets, webgpu_factory_run_benchmark"