#!/bin/bash
set -euo pipefail

echo "[Build] GTK widget factory with ASYNCIFY support"

cd "$(dirname "$0")"

# Download fonts if needed
if [ ! -f "fonts/Roboto-Regular.ttf" ]; then
    echo "[Build] Downloading fonts..."
    bash fonts/download-roboto.sh
fi

# Verify fonts are present
echo "[Build] Verifying fonts..."
for font in Roboto-Regular.ttf Roboto-Bold.ttf Roboto-Italic.ttf Roboto-BoldItalic.ttf; do
    if [ ! -f "fonts/$font" ]; then
        echo "ERROR: Missing font file: fonts/$font"
        echo "Run: bash fonts/download-roboto.sh"
        exit 1
    fi
done
echo "[Build] ✓ All fonts present"

# Check for GTK install directory
if [ -d "../../install" ]; then
    GTK_INSTALL="../../install"
    echo "[Build] Using GTK installation: $GTK_INSTALL"
elif [ -d "/opt/gtk-wasm" ]; then
    GTK_INSTALL="/opt/gtk-wasm"
    echo "[Build] Using system GTK installation: $GTK_INSTALL"
else
    echo "[Build] WARNING: No GTK installation found"
    echo "[Build] Building with system emscripten packages"
    GTK_INSTALL=""
fi

# Build compiler flags
CFLAGS="-I/opt/wasm/include"
LDFLAGS="-L/opt/wasm/lib"

if [ -n "$GTK_INSTALL" ]; then
    CFLAGS="$CFLAGS -I$GTK_INSTALL/include/cairo -I$GTK_INSTALL/include/pango-1.0 -I$GTK_INSTALL/include/glib-2.0 -I$GTK_INSTALL/lib/glib-2.0/include -I$GTK_INSTALL/include/fontconfig -I$GTK_INSTALL/include/freetype2 -I$GTK_INSTALL/include/harfbuzz -I$GTK_INSTALL/include/fribidi"
    LDFLAGS="$LDFLAGS -L$GTK_INSTALL/lib"
fi

LIBS="-lcairo -lpangocairo-1.0 -lpango-1.0 -lfontconfig -lfreetype -lharfbuzz -lfribidi -lglib-2.0"

echo "[Build] Compiling with ASYNCIFY support..."

# Build with ASYNCIFY enabled
emcc main.c \
    $CFLAGS \
    $LDFLAGS \
    $LIBS \
    --preload-file fonts@/fonts \
    --preload-file fonts/fonts.conf@/etc/fonts/fonts.conf \
    -sASYNCIFY=1 \
    -sASYNCIFY_STACK_SIZE=12KB \
    -sASYNCIFY_IMPORTS='["emscripten_sleep"]' \
    -sEXPORTED_FUNCTIONS='["_main","_start_demo","_init_webgpu","_render_widgets","_cleanup","_get_fps","_get_frame_time","_get_widget_count","_get_simd_speedup"]' \
    -sEXPORTED_RUNTIME_METHODS='["ccall","cwrap","callMain"]' \
    -sINVOKE_RUN=0 \
    -sEXPORT_ES6=1 \
    -sMODULARIZE=1 \
    -sEXPORT_NAME=GtkWidgetFactory \
    -sSINGLE_FILE=0 \
    -sENVIRONMENT=web,webview,worker \
    -sALLOW_MEMORY_GROWTH=1 \
    -sINITIAL_MEMORY=128MB \
    -sMAXIMUM_MEMORY=1GB \
    -sSTACK_SIZE=2MB \
    -O3 -flto \
    -o gtk-widget-factory-native.js

echo ""
echo "✅ Build complete!"
echo ""
echo "Outputs:"
echo "  - gtk-widget-factory-native.js (JS wrapper)"
echo "  - gtk-widget-factory-native.wasm (WASM module with ASYNCIFY)"
echo "  - gtk-widget-factory-native.data (1.4MB fonts)"
echo ""
echo "ASYNCIFY is enabled for async font loading!"
echo ""
echo "To test:"
echo "  deno test --allow-read --allow-write --import-map=import_map.json test-wasm-init.ts"
