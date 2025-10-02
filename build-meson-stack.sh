#!/bin/bash
set -euo pipefail

# Full Stack Meson Build for GTK WebGPU Demo
# Uses Meson build system for all modules

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

# Build configuration - Initialize PKG_CONFIG_PATH first
if [ -z "${PKG_CONFIG_PATH:-}" ]; then
    export PKG_CONFIG_PATH="$SCRIPT_DIR/install/lib/pkgconfig"
else
    export PKG_CONFIG_PATH="$SCRIPT_DIR/install/lib/pkgconfig:$PKG_CONFIG_PATH"
fi

export EM_PKG_CONFIG_PATH="$SCRIPT_DIR/install/lib/pkgconfig"
export CFLAGS="-O3 -msimd128 -fPIC -DUSE_WEB_NATIVE_POSIX -include /home/isaac/src/discere/discere-nucleus/client/emscripten/zlib.wasm/wasm/web_native_posix.h"
export CXXFLAGS="-O3 -msimd128 -fPIC"
export LDFLAGS="-L$SCRIPT_DIR/install/lib"

# Create install directories
setup_directories() {
    log_info "Setting up directories..."
    mkdir -p install/{lib,include,bin,share/pkgconfig}
    mkdir -p install/wasm
    mkdir -p build-meson
    log_success "Directories created"
}

# Build a module with Meson
build_meson_module() {
    local name="$1"
    local src_dir="../$name.wasm"
    local build_dir="build-meson/$name"
    local extra_args="${2:-}"

    log_info "Building $name with Meson..."

    if [ ! -d "$src_dir" ]; then
        log_error "$src_dir not found"
        return 1
    fi

    # Clean previous build
    rm -rf "$build_dir"

    # Configure with Meson
    log_info "Configuring $name..."
    meson setup "$build_dir" "$src_dir" \
        --cross-file=emscripten-cross.ini \
        --prefix="$SCRIPT_DIR/install" \
        --libdir=lib \
        --buildtype=release \
        --default-library=static \
        -Dstrip=true \
        -Db_lto=true \
        $extra_args || {
            log_error "Failed to configure $name"
            return 1
        }

    # Build
    log_info "Building $name..."
    meson compile -C "$build_dir" || {
        log_error "Failed to build $name"
        return 1
    }

    # Install
    log_info "Installing $name..."
    meson install -C "$build_dir" || {
        log_error "Failed to install $name"
        return 1
    }

    # Create SIDE module WASM
    if [ -f "$SCRIPT_DIR/install/lib/lib${name}.a" ]; then
        log_info "Creating ${name}-side.wasm..."
        emcc "$SCRIPT_DIR/install/lib/lib${name}.a" \
            -O3 -flto -msimd128 \
            -sSIDE_MODULE=2 \
            -sSTANDALONE_WASM=1 \
            -sEXPORT_ALL=1 \
            -sWASM_BIGINT=1 \
            -o "$SCRIPT_DIR/install/wasm/${name}-side.wasm"

        local size=$(stat -f%z "$SCRIPT_DIR/install/wasm/${name}-side.wasm" 2>/dev/null || stat -c%s "$SCRIPT_DIR/install/wasm/${name}-side.wasm" 2>/dev/null)
        local size_kb=$((size / 1024))
        log_success "$name built successfully (${size_kb}KB)"
    else
        log_warning "Static library for $name not found, skipping WASM generation"
    fi

    return 0
}

# Build foundation libraries
build_foundation() {
    log_info "Building foundation libraries..."

    # Build in dependency order
    build_meson_module "zlib" ""
    build_meson_module "libpng" ""
    build_meson_module "pixman" "-Dtests=disabled -Ddemos=disabled"
    build_meson_module "freetype" ""
    build_meson_module "libexpat" ""

    log_success "Foundation libraries built"
}

# Build text/font libraries
build_text_libs() {
    log_info "Building text/font libraries..."

    build_meson_module "harfbuzz" "-Dglib=disabled -Dgobject=disabled -Dfreetype=disabled -Dcairo=disabled"
    build_meson_module "fontconfig" ""
    build_meson_module "fribidi" "" 2>/dev/null || log_warning "fribidi not available"

    log_success "Text libraries built"
}

# Build graphics libraries
build_graphics() {
    log_info "Building graphics libraries..."

    build_meson_module "cairo" ""
    build_meson_module "pango" ""

    log_success "Graphics libraries built"
}

# Build GLib and dependencies
build_glib() {
    log_info "Building GLib..."

    # GLib has specific requirements for cross-compilation
    build_meson_module "glib" ""

    log_success "GLib built"
}

# Build GTK4
build_gtk() {
    log_info "Building GTK4..."

    # Check if GTK is in current directory
    if [ -f "meson.build" ]; then
        log_info "Building GTK from current directory..."

        rm -rf build-meson/gtk
        meson setup build-meson/gtk . \
            --cross-file=emscripten-cross.ini \
            --prefix="$SCRIPT_DIR/install" \
            --libdir=lib \
            --buildtype=release \
            --default-library=static \
            -Dx11-backend=false \
            -Dwayland-backend=false \
            -Dbroadway-backend=true \
            -Dmedia-ffmpeg=disabled \
            -Dmedia-gstreamer=disabled \
            -Dprint-cups=disabled \
            -Dvulkan=disabled \
            -Dintrospection=disabled \
            -Ddemos=false \
            -Dbuild-testsuite=false \
            -Dbuild-examples=false

        meson compile -C build-meson/gtk
        meson install -C build-meson/gtk

        # Create GTK SIDE module
        if [ -f "$SCRIPT_DIR/install/lib/libgtk-4.a" ]; then
            emcc "$SCRIPT_DIR/install/lib/libgtk-4.a" \
                "$SCRIPT_DIR/install/lib/libgdk-4.a" \
                "$SCRIPT_DIR/install/lib/libgsk-4.a" \
                -O3 -flto -msimd128 \
                -sSIDE_MODULE=2 \
                -sSTANDALONE_WASM=1 \
                -sEXPORT_ALL=1 \
                -o "$SCRIPT_DIR/install/wasm/gtk-side.wasm"

            log_success "GTK built successfully"
        fi
    else
        log_warning "GTK not found in current directory"
    fi
}

# Build the MAIN module
build_main_module() {
    log_info "Building MAIN module with WebGPU..."

    mkdir -p build-main
    cd build-main

    # Compile the MAIN module
    emcc ../demos/webgpu-widget-factory/main.c \
        -I../install/include \
        -I../install/include/gtk-4.0 \
        -I../install/include/glib-2.0 \
        -I../install/lib/glib-2.0/include \
        -I../install/include/cairo \
        -I../install/include/pango-1.0 \
        -I../install/include/harfbuzz \
        -I../install/include/gdk-pixbuf-2.0 \
        -L../install/lib \
        -lgtk-4 -lgdk-4 -lgsk-4 \
        -lpangocairo-1.0 -lpango-1.0 \
        -lcairo -lpixman-1 \
        -lharfbuzz -lfreetype \
        -lfontconfig -lexpat \
        -lgobject-2.0 -lglib-2.0 \
        -lpng16 -lz \
        -O3 -flto -msimd128 \
        -sMODULARIZE=1 \
        -sEXPORT_ES6=1 \
        -sEXPORT_NAME="GTKWebGPUModule" \
        -sEXPORTED_FUNCTIONS='["_main","_malloc","_free"]' \
        -sEXPORTED_RUNTIME_METHODS='["cwrap","ccall","UTF8ToString"]' \
        -sALLOW_MEMORY_GROWTH=1 \
        -sINITIAL_MEMORY=134217728 \
        -sMAXIMUM_MEMORY=1073741824 \
        --use-port=emdawnwebgpu \
        -sASYNCIFY=1 \
        -sENVIRONMENT=web \
        -o gtk-webgpu-main.js || {
            log_error "Failed to build MAIN module"
            cd ..
            return 1
        }

    # Copy outputs
    cp gtk-webgpu-main.js ../install/wasm/
    cp gtk-webgpu-main.wasm ../install/wasm/
    cp gtk-webgpu-main.js ../demos/webgpu-widget-factory/
    cp gtk-webgpu-main.wasm ../demos/webgpu-widget-factory/

    cd ..
    log_success "MAIN module built successfully"
}

# Generate build report
generate_report() {
    log_info "Generating build report..."

    local wasm_count=$(ls -1 install/wasm/*.wasm 2>/dev/null | wc -l)
    local total_size=$(du -sh install/wasm 2>/dev/null | cut -f1)

    echo ""
    log_success "🎉 GTK WebGPU Full Stack Build Complete!"
    echo ""
    echo "📊 Build Summary:"
    echo "  WASM Modules: $wasm_count"
    echo "  Total Size: $total_size"
    echo "  Install Path: $SCRIPT_DIR/install/"
    echo ""
    echo "📦 Built Modules:"
    ls -lh install/wasm/*.wasm 2>/dev/null | awk '{print "  " $9 " (" $5 ")"}'
    echo ""
    echo "🚀 Next Steps:"
    echo "  1. Start server: ./serve-demo.ts"
    echo "  2. Open browser: http://localhost:8080/demos/webgpu-widget-factory/"
    echo ""
}

# Main build process
main() {
    log_info "🚀 GTK WebGPU Full Stack Meson Build"
    echo ""

    setup_directories

    # Build in dependency order
    build_foundation
    build_text_libs
    build_graphics
    build_gtk
    build_main_module

    generate_report
}

# Run the build
main "$@"