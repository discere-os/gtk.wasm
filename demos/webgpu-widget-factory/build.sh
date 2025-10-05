#!/bin/bash
set -euo pipefail

echo "🔨 Building GTK WebGPU Widget Factory - Pure Native Rendering..."

SOURCE="main.c"
OUTPUT_JS="gtk-widget-factory.js"
OUTPUT_WASM="gtk-widget-factory.wasm"

# Library paths
PANGO_DIR="../../../pango.wasm/install"
CAIRO_DIR="../../../cairo.wasm/install"
GLIB_DIR="../../../glib.wasm/install"
FREETYPE_DIR="../../../freetype.wasm/install"
HARFBUZZ_DIR="../../../harfbuzz.wasm/install"
FONTCONFIG_DIR="../../../fontconfig.wasm/install"
PIXMAN_DIR="../../../pixman.wasm/install"
LIBPNG_DIR="../../../libpng.wasm/install"
ZLIB_DIR="../../../zlib.wasm/install"
FRIBIDI_DIR="../../../fribidi.wasm/install"

# Include paths
INCLUDES="
-I${PANGO_DIR}/include/pango-1.0
-I${CAIRO_DIR}/include/cairo
-I${GLIB_DIR}/include/glib-2.0
-I${GLIB_DIR}/lib/glib-2.0/include
-I${FREETYPE_DIR}/include/freetype2
-I${HARFBUZZ_DIR}/include/harfbuzz
-I${FONTCONFIG_DIR}/include
-I${PIXMAN_DIR}/include/pixman-1
-I${LIBPNG_DIR}/include
-I${FRIBIDI_DIR}/include/fribidi
"

# Library paths and links
LIBS="
${PANGO_DIR}/lib/libpangocairo-1.0.a
${PANGO_DIR}/lib/libpangoft2-1.0.a
${PANGO_DIR}/lib/libpango-1.0.a
${CAIRO_DIR}/lib/libcairo.a
${GLIB_DIR}/lib/libgobject-2.0.a
${GLIB_DIR}/lib/libglib-2.0.a
${FREETYPE_DIR}/lib/libfreetype.a
${HARFBUZZ_DIR}/lib/libharfbuzz.a
${FONTCONFIG_DIR}/lib/libfontconfig.a
${PIXMAN_DIR}/lib/libpixman-1.a
${LIBPNG_DIR}/lib/libpng.a
${ZLIB_DIR}/lib/libz.a
${FRIBIDI_DIR}/lib/libfribidi.a
"

rm -f "$OUTPUT_JS" "$OUTPUT_WASM"

echo "📦 Linking native libraries:"
echo "  - Pango (text layout)"
echo "  - Cairo (2D graphics)"
echo "  - Glib/GObject (object system)"
echo "  - FreeType (font rasterization)"
echo "  - HarfBuzz (text shaping with SIMD)"
echo "  - FontConfig (font selection)"
echo "  - Pixman (pixel manipulation)"
echo "  - libpng (image format)"
echo "  - zlib (compression)"
echo "  - fribidi (bidirectional text)"

emcc "$SOURCE" \
    $INCLUDES \
    $LIBS \
    -O3 \
    -flto \
    -msimd128 \
    -sWASM=1 \
    -sMODULARIZE=1 \
    -sEXPORT_ES6=1 \
    -sEXPORT_NAME="GTKWidgetFactory" \
    -sALLOW_MEMORY_GROWTH=1 \
    -sINITIAL_MEMORY=67108864 \
    -sENVIRONMENT=web,webview,worker \
    -sEXPORTED_FUNCTIONS='["_main","_init_webgpu","_render_widgets","_update_performance","_get_fps","_get_frame_time","_get_widget_count","_get_simd_speedup","_get_performance_metrics","_cleanup"]' \
    -sEXPORTED_RUNTIME_METHODS='["cwrap","ccall","UTF8ToString"]' \
    -sERROR_ON_UNDEFINED_SYMBOLS=0 \
    -o "$OUTPUT_JS"

echo "✅ Build completed!"
echo "📁 Files:"
ls -lh "$OUTPUT_JS" "$OUTPUT_WASM"
echo ""
echo "🎨 Pure Native GTK Rendering:"
echo "  ✅ Cairo 2D graphics"
echo "  ✅ Pango text layout"
echo "  ✅ FreeType font rasterization"
echo "  ✅ HarfBuzz text shaping (SIMD-accelerated)"
echo "  ✅ 10 widget types with native rendering"
echo "  ✅ ZERO JavaScript Canvas hacks"
echo ""
echo "Every pixel rendered natively - just like GTK on desktop! 🚀"
