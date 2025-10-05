#!/bin/bash
set -euo pipefail

echo "🔨 Building GTK Widget Factory with Native Rendering..."

# Use the GTK build's dependencies
BUILD_DIR="../../_build_wasm"

# Check if dependencies are built
if [ ! -d "$BUILD_DIR" ]; then
    echo "❌ GTK build not found. Please build GTK first with:"
    echo "   cd ../.. && ./build-wasm.sh"
    exit 1
fi

SOURCE="main.c"
OUTPUT_JS="gtk-widget-factory-native.js"
OUTPUT_WASM="gtk-widget-factory-native.wasm"

rm -f "$OUTPUT_JS" "$OUTPUT_WASM"

echo "📦 Using native GTK rendering stack (Cairo + Pango + FreeType + HarfBuzz)"

# Build with USE_PANGO flag to enable native rendering
emcc "$SOURCE" \
    -DUSE_PANGO=1 \
    -I${BUILD_DIR}/pango \
    -I${BUILD_DIR}/cairo \
    `pkg-config --cflags cairo pangocairo 2>/dev/null || echo ""` \
    -O3 \
    -flto \
    -msimd128 \
    -sWASM=1 \
    -sMODULARIZE=1 \
    -sEXPORT_ES6=1 \
    -sEXPORT_NAME="GTKWidgetFactory" \
    -sALLOW_MEMORY_GROWTH=1 \
    -sINITIAL_MEMORY=134217728 \
    -sENVIRONMENT=web,webview,worker \
    -sEXPORTED_FUNCTIONS='["_main","_init_webgpu","_render_widgets","_update_performance","_get_fps","_get_frame_time","_get_widget_count","_get_simd_speedup","_cleanup"]' \
    -sEXPORTED_RUNTIME_METHODS='["cwrap","ccall","UTF8ToString"]' \
    `pkg-config --libs cairo pangocairo 2>/dev/null || echo "-lz -lm"` \
    -o "$OUTPUT_JS"

echo "✅ Build completed!"
ls -lh "$OUTPUT_JS" "$OUTPUT_WASM"
echo ""
echo "🎨 100% Native GTK Rendering - ZERO Canvas API hacks!"
