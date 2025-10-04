#!/bin/bash
set -euo pipefail

echo "🔨 Building GTK WebGPU Widget Factory Demo..."

SOURCE="main.c"
OUTPUT_JS="gtk-widget-factory.js"
OUTPUT_WASM="gtk-widget-factory.wasm"

rm -f "$OUTPUT_JS" "$OUTPUT_WASM"

emcc "$SOURCE" \
    -O3 \
    -flto \
    -msimd128 \
    -sWASM=1 \
    -sMODULARIZE=1 \
    -sEXPORT_ES6=1 \
    -sEXPORT_NAME="GTKWidgetFactory" \
    -sALLOW_MEMORY_GROWTH=1 \
    -sNO_FILESYSTEM=1 \
    -sENVIRONMENT=web,webview,worker \
    -sINITIAL_MEMORY=33554432 \
    -sEXPORTED_FUNCTIONS='["_main","_init_webgpu","_render_widgets","_update_performance","_get_fps","_get_frame_time","_get_widget_count","_get_simd_speedup","_get_performance_metrics"]' \
    -sEXPORTED_RUNTIME_METHODS='["cwrap","ccall","UTF8ToString"]' \
    -o "$OUTPUT_JS"

echo "✅ Build completed!"
echo "📁 Files:"
ls -lh "$OUTPUT_JS" "$OUTPUT_WASM"
echo ""
echo "🚀 SIMD: ENABLED"
echo "🎨 Widgets: Buttons, Labels, Entry, Checkbox, Radio, Slider, Progress, Spinner, Image, TextView"