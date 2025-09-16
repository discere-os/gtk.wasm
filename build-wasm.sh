#!/bin/bash
# GTK WASM Production Build System
# Copyright 2025 Superstruct Ltd, New Zealand  
# Licensed under LGPL-2.1-or-later

set -euo pipefail

# Configuration
readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly BUILD_DIR="${SCRIPT_DIR}/_build_wasm"
readonly DIST_DIR="${SCRIPT_DIR}/dist"
readonly RESOURCES_DIR="${SCRIPT_DIR}/resources"

# Build options with comprehensive defaults
BUILD_TYPE="${1:-release}"
TARGET="${2:-web}"
FEATURES="${3:-standard}"
RENDERER="${4:-webgpu}"

# Colors for output  
readonly RED='\033[0;31m'
readonly GREEN='\033[0;32m'
readonly YELLOW='\033[1;33m'
readonly BLUE='\033[0;34m'
readonly NC='\033[0m'

log() {
    echo -e "${GREEN}[$(date +'%H:%M:%S')]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[$(date +'%H:%M:%S')] WARNING:${NC} $1"
}

error() {
    echo -e "${RED}[$(date +'%H:%M:%S')] ERROR:${NC} $1" >&2
}

# Check comprehensive dependencies
check_dependencies() {
    local deps=("emcc" "meson" "ninja" "pkg-config" "wasm-opt")
    
    for dep in "${deps[@]}"; do
        if ! command -v "$dep" &> /dev/null; then
            error "$dep is not installed or not in PATH"
            exit 1
        fi
    done
    
    # Check Emscripten version
    local emcc_version=$(emcc --version | head -n1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')
    log "Using Emscripten version: $emcc_version"
    
    # Verify minimum version
    if ! printf '%s\n%s\n' "3.1.40" "$emcc_version" | sort -V -C; then
        warn "Emscripten version $emcc_version may be outdated (recommended: 3.1.40+)"
    fi
    
    log "All dependencies verified"
}

# Setup comprehensive Emscripten environment
setup_environment() {
    # Ensure Emscripten environment is properly loaded
    if [ -z "${EMSDK:-}" ]; then
        if [ -f "$HOME/emsdk/emsdk_env.sh" ]; then
            source "$HOME/emsdk/emsdk_env.sh"
        elif [ -f "/opt/emsdk/emsdk_env.sh" ]; then
            source "/opt/emsdk/emsdk_env.sh"
        else
            error "EMSDK environment not found. Please install and activate Emscripten."
            exit 1
        fi
    fi

    export CC="emcc"
    export CXX="em++"
    export AR="emar"
    export RANLIB="emranlib"
    export PKG_CONFIG="emconfigure pkg-config"
    
    # Base optimization flags
    local base_cflags="-fno-strict-aliasing -fwrapv"
    local base_ldflags=""
    
    # Build type specific flags
    case "$BUILD_TYPE" in
        "release")
            export CFLAGS="-O3 -DNDEBUG -flto $base_cflags"
            export LDFLAGS="-O3 -flto --closure=1 $base_ldflags"
            ;;
        "debug")
            export CFLAGS="-O1 -g -DDEBUG -gsource-map $base_cflags"
            export LDFLAGS="-O1 -g -gsource-map $base_ldflags"
            ;;
        "size")
            export CFLAGS="-Os -DNDEBUG -flto $base_cflags"
            export LDFLAGS="-Os -flto --closure=1 $base_ldflags"
            ;;
    esac
    
    # Threading support
    if [[ "$FEATURES" == *"threading"* ]] || [[ "$FEATURES" == "standard" ]] || [[ "$FEATURES" == "full" ]]; then
        export CFLAGS="$CFLAGS -pthread -sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency"
        export LDFLAGS="$LDFLAGS -pthread -sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency -sPROXY_TO_PTHREAD"
        log "Threading support enabled"
    fi
    
    # SIMD support  
    if [[ "$FEATURES" == *"simd"* ]] || [[ "$FEATURES" == "standard" ]] || [[ "$FEATURES" == "full" ]]; then
        export CFLAGS="$CFLAGS -msimd128 -DGTK_ENABLE_SIMD=1"
        export LDFLAGS="$LDFLAGS -msimd128"
        log "SIMD acceleration enabled"
    fi
    
    # WebGPU support flags
    if [[ "$RENDERER" == "webgpu" ]] || [[ "$RENDERER" == "hybrid" ]]; then
        export CFLAGS="$CFLAGS -DGTK_ENABLE_WEBGPU=1 -sUSE_WEBGPU=1 -sASYNCIFY"
        export LDFLAGS="$LDFLAGS -sUSE_WEBGPU=1 -sASYNCIFY -sEXPORTED_FUNCTIONS=_main,_malloc,_free -sEXPORTED_RUNTIME_METHODS=ccall,cwrap,getValue,setValue"
        log "WebGPU renderer enabled"
        
        # Add WebGPU compute shader support
        if [[ "$FEATURES" == *"compute"* ]] || [[ "$FEATURES" == "full" ]]; then
            export CFLAGS="$CFLAGS -DGTK_WEBGPU_COMPUTE_SHADERS=1"
            log "WebGPU compute shaders enabled"
        fi
    fi
    
    # Memory configuration based on features
    local initial_memory="134217728"  # 128MB
    local maximum_memory="1073741824" # 1GB
    local stack_size="2097152"        # 2MB
    
    if [[ "$FEATURES" == "full" ]]; then
        initial_memory="268435456"    # 256MB  
        maximum_memory="2147483648"   # 2GB
        stack_size="4194304"          # 4MB
    fi
    
    export LDFLAGS="$LDFLAGS -sINITIAL_MEMORY=$initial_memory -sMAXIMUM_MEMORY=$maximum_memory -sSTACK_SIZE=$stack_size"
    
    log "Environment configured for $BUILD_TYPE build with $FEATURES features"
}

# Configure Meson build with comprehensive options
configure_build() {
    log "Configuring Meson build for GTK WASM..."
    
    # Remove existing build directory
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    
    # Determine backend configuration based on features
    local backend_args=""
    case "$FEATURES" in
        "minimal")
            backend_args="-Dbroadway-backend=true -Dx11-backend=false -Dwayland-backend=false -Dwin32-backend=false -Dmacos-backend=false -Dandroid-backend=false"
            ;;
        "standard"|"full") 
            backend_args="-Dbroadway-backend=true -Dx11-backend=false -Dwayland-backend=false -Dwin32-backend=false -Dmacos-backend=false -Dandroid-backend=false"
            ;;
    esac
    
    # WebGPU renderer option
    local webgpu_option="disabled"
    if [[ "$RENDERER" == "webgpu" ]] || [[ "$RENDERER" == "hybrid" ]]; then
        webgpu_option="enabled"
    fi
    
    # Feature-based options
    local feature_args=""
    case "$FEATURES" in
        "minimal")
            feature_args="-Dmedia-gstreamer=disabled -Dprint-cups=disabled -Dprint-cpdb=disabled -Dvulkan=disabled -Dcloudproviders=disabled -Dsysprof=disabled -Dtracker=disabled -Dcolord=disabled -Df16c=disabled"
            ;;
        "standard")  
            feature_args="-Dmedia-gstreamer=disabled -Dprint-cups=disabled -Dprint-cpdb=disabled -Dvulkan=disabled -Dcloudproviders=disabled -Dsysprof=disabled -Dtracker=disabled -Dcolord=disabled -Df16c=enabled"
            ;;
        "full")
            feature_args="-Dmedia-gstreamer=enabled -Dprint-cups=disabled -Dprint-cpdb=disabled -Dvulkan=disabled -Dcloudproviders=disabled -Dsysprof=enabled -Dtracker=disabled -Dcolord=disabled -Df16c=enabled"
            ;;
    esac
    
    # Build configuration
    local build_args="-Dintrospection=disabled -Ddocumentation=false -Dman-pages=false -Dbuild-demos=false -Dbuild-testsuite=false -Dbuild-examples=false -Dbuild-tests=false"
    
    # Meson setup command  
    meson setup "$BUILD_DIR" \
        --cross-file=wasm-cross.txt \
        --buildtype="$BUILD_TYPE" \
        $backend_args \
        -Dwebgpu-renderer="$webgpu_option" \
        $feature_args \
        $build_args \
        -Ddefault_library=static
        
    log "Meson configuration completed successfully"
}

# Build GTK with comprehensive error handling  
build_library() {
    log "Building GTK WASM library..."
    
    # Build with ninja
    ninja -C "$BUILD_DIR" -v
    
    # Verify critical build artifacts
    local required_libs=(
        "$BUILD_DIR/gtk/libgtk-4.a"
        "$BUILD_DIR/gsk/libgsk-4.a" 
        "$BUILD_DIR/gdk/libgdk-4.a"
    )
    
    for lib in "${required_libs[@]}"; do
        if [[ ! -f "$lib" ]]; then
            error "Required library not found: $lib"
            exit 1
        fi
        log "✓ Found: $(basename "$lib")"
    done
    
    log "GTK WASM library build completed successfully"
}

# Prepare embedded resources
prepare_resources() {
    log "Preparing embedded resources..."
    
    mkdir -p "$RESOURCES_DIR"
    
    # Create minimal theme and icon resources
    mkdir -p "$RESOURCES_DIR/themes/Adwaita"
    mkdir -p "$RESOURCES_DIR/icons/hicolor"  
    mkdir -p "$RESOURCES_DIR/schemas"
    
    # Copy essential theme files if they exist
    if [[ -d "/usr/share/themes/Adwaita" ]]; then
        cp -r /usr/share/themes/Adwaita/* "$RESOURCES_DIR/themes/Adwaita/" 2>/dev/null || true
    fi
    
    # Copy essential icon files if they exist
    if [[ -d "/usr/share/icons/hicolor" ]]; then
        find /usr/share/icons/hicolor -name "*.png" -size -10k -exec cp {} "$RESOURCES_DIR/icons/hicolor/" \; 2>/dev/null || true
    fi
    
    # Copy GLib schemas if they exist
    if [[ -d "/usr/share/glib-2.0/schemas" ]]; then
        cp /usr/share/glib-2.0/schemas/*.compiled "$RESOURCES_DIR/schemas/" 2>/dev/null || true
    fi
    
    log "Resources prepared ($(du -sh "$RESOURCES_DIR" 2>/dev/null | cut -f1 || echo "unknown size"))"
}

# Create final WASM module with comprehensive linking
create_wasm_module() {
    log "Creating final GTK WASM module..."
    
    mkdir -p "$DIST_DIR"
    
    # Base Emscripten linking flags
    local emcc_flags=(
        "-sWASM=1"
        "-sMODULARIZE=1"
        "-sEXPORT_NAME=GTKModule"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sNO_EXIT_RUNTIME=1"
        "-sASSERTIONS=0"
        "-sFORCE_FILESYSTEM=1"
        "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','setValue','getValue','UTF8ToString','stringToUTF8','FS','PATH','ERRNO_CODES']"
    )
    
    # Target-specific configuration
    case "$TARGET" in
        "web")
            emcc_flags+=("-sENVIRONMENT=web")
            ;;
        "node")
            emcc_flags+=("-sENVIRONMENT=node")
            ;;
        "worker")
            emcc_flags+=("-sENVIRONMENT=worker")
            ;;
        "all")
            emcc_flags+=("-sENVIRONMENT=web,worker,node")
            ;;
    esac
    
    # Threading configuration
    if [[ "$FEATURES" == *"threading"* ]] || [[ "$FEATURES" == "standard" ]] || [[ "$FEATURES" == "full" ]]; then
        emcc_flags+=("-pthread" "-sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency" "-sPROXY_TO_PTHREAD")
    fi
    
    # SIMD configuration
    if [[ "$FEATURES" == *"simd"* ]] || [[ "$FEATURES" == "standard" ]] || [[ "$FEATURES" == "full" ]]; then
        emcc_flags+=("-msimd128")
    fi
    
    # Embed resources
    if [[ -d "$RESOURCES_DIR" ]]; then
        emcc_flags+=(
            "--embed-file" "$RESOURCES_DIR/themes@/usr/share/themes"
            "--embed-file" "$RESOURCES_DIR/icons@/usr/share/icons"
            "--embed-file" "$RESOURCES_DIR/schemas@/usr/share/glib-2.0/schemas"
        )
    fi
    
    # Collect all static libraries
    local static_libs=(
        "$BUILD_DIR/gtk/libgtk-4.a"
        "$BUILD_DIR/gsk/libgsk-4.a"
        "$BUILD_DIR/gdk/libgdk-4.a"
    )
    
    # Create exports file for public GTK API
    create_exports_file
    emcc_flags+=("-sEXPORTED_FUNCTIONS=@$DIST_DIR/exports.json")
    
    # Link final WASM module  
    emcc "${static_libs[@]}" "${emcc_flags[@]}" \
        $LDFLAGS \
        -o "$DIST_DIR/gtk4.js"
    
    log "GTK WASM module created successfully"
}

# Create exports file for public GTK API
create_exports_file() {
    log "Creating API exports file..."
    
    cat > "$DIST_DIR/exports.json" << 'EOF'
[
  "_main",
  "_gtk_init",
  "_gtk_main",
  "_gtk_main_quit",
  "_gtk_application_new",
  "_gtk_application_run",
  "_gtk_window_new",
  "_gtk_window_set_title", 
  "_gtk_window_set_default_size",
  "_gtk_window_present",
  "_gtk_button_new_with_label",
  "_gtk_box_new",
  "_gtk_box_append",
  "_gtk_widget_show",
  "_gtk_widget_hide",
  "_gtk_widget_destroy",
  "_g_signal_connect_data",
  "_g_object_ref",
  "_g_object_unref",
  "_g_main_loop_new",
  "_g_main_loop_run",
  "_g_main_loop_quit"
]
EOF
    
    log "Exports file created with $(jq '. | length' "$DIST_DIR/exports.json") functions"
}

# Generate TypeScript definitions
create_typescript_definitions() {
    log "Creating TypeScript definitions..."
    
    cat > "$DIST_DIR/gtk4.d.ts" << 'EOF'
/**
 * GTK WASM - TypeScript Definitions
 * Copyright 2025 Superstruct Ltd, New Zealand
 * Licensed under LGPL-2.1-or-later
 */

export interface GTKModuleOptions {
    canvas?: HTMLCanvasElement;
    onReady?: () => void;
    onError?: (error: Error) => void;
    enableThreading?: boolean;
    enableSIMD?: boolean;
    enableWebGPU?: boolean;
}

export interface GTKApplication {
    new(appId: string, flags: number): GTKApplication;
    run(args: string[]): number;
    quit(): void;
}

export interface GTKWindow {
    new(): GTKWindow;
    setTitle(title: string): void;
    setDefaultSize(width: number, height: number): void;
    present(): void;
    destroy(): void;
    setChild(child: GTKWidget): void;
}

export interface GTKWidget {
    show(): void;
    hide(): void;
    destroy(): void;
    setSizeRequest(width: number, height: number): void;
    connect(signal: string, callback: Function): number;
}

export interface GTKButton extends GTKWidget {
    new(): GTKButton;
    newWithLabel(label: string): GTKButton;
    setLabel(label: string): void;
    getLabel(): string;
}

export interface GTKBox extends GTKWidget {
    new(orientation: GTKOrientation, spacing: number): GTKBox;
    append(child: GTKWidget): void;
    prepend(child: GTKWidget): void;
    remove(child: GTKWidget): void;
}

export enum GTKOrientation {
    HORIZONTAL = 0,
    VERTICAL = 1
}

export interface GTKApplicationFlags {
    FLAGS_NONE: number;
    HANDLES_OPEN: number;
    HANDLES_COMMAND_LINE: number;
    SEND_ENVIRONMENT: number;
    NON_UNIQUE: number;
}

export interface GTKModule {
    GTKApplication: typeof GTKApplication;
    GTKWindow: typeof GTKWindow;
    GTKWidget: typeof GTKWidget;
    GTKButton: typeof GTKButton;
    GTKBox: typeof GTKBox;
    GTKOrientation: typeof GTKOrientation;
    GTKApplicationFlags: GTKApplicationFlags;
    
    // Low-level WASM interface
    ccall: (name: string, returnType: string, argTypes: string[], args: any[]) => any;
    cwrap: (name: string, returnType: string, argTypes: string[]) => Function;
    
    // Memory management
    _malloc: (size: number) => number;
    _free: (ptr: number) => void;
    
    // String utilities
    UTF8ToString: (ptr: number) => string;
    stringToUTF8: (str: string, ptr: number, maxLength: number) => void;
    
    // Filesystem
    FS: any;
}

declare function GTKModule(options?: GTKModuleOptions): Promise<GTKModule>;

export default GTKModule;
EOF
    
    log "TypeScript definitions created"
}

# Create package.json for npm distribution
create_package_json() {
    log "Creating package.json..."
    
    cat > "$DIST_DIR/package.json" << EOF
{
  "name": "@superstruct/gtk4-wasm",
  "version": "4.20.1-wasm.1",
  "description": "GTK 4 compiled to WebAssembly for browser applications",
  "main": "gtk4.js",
  "types": "gtk4.d.ts",
  "files": [
    "gtk4.js",
    "gtk4.wasm", 
    "gtk4.d.ts",
    "README.md"
  ],
  "keywords": [
    "gtk",
    "gui",
    "webassembly",
    "wasm",
    "widgets",
    "ui",
    "graphics"
  ],
  "author": "Superstruct Ltd",
  "license": "LGPL-2.1-or-later",
  "homepage": "https://github.com/superstruct/gtk.wasm",
  "repository": {
    "type": "git",
    "url": "https://github.com/superstruct/gtk.wasm.git"
  },
  "bugs": {
    "url": "https://github.com/superstruct/gtk.wasm/issues"
  },
  "engines": {
    "node": ">=16.0.0"
  },
  "browser": {
    "fs": false,
    "path": false,
    "os": false,
    "crypto": false
  },
  "scripts": {
    "test": "echo 'Testing requires browser environment'",
    "serve": "python3 -m http.server 8080 --directory .",
    "validate": "node validate-wasm.js"
  },
  "peerDependencies": {},
  "devDependencies": {},
  "sideEffects": false,
  "publishConfig": {
    "access": "public"
  }
}
EOF
    
    log "Package.json created"
}

# Optimize WASM binary
optimize_wasm() {
    log "Optimizing WASM binary..."
    
    if command -v wasm-opt &> /dev/null; then
        # Create optimized versions
        local wasm_file="$DIST_DIR/gtk4.wasm"
        
        # Size optimization
        wasm-opt -Os --enable-simd --dce --vacuum \
            --strip-debug --strip-producers \
            "$wasm_file" -o "$DIST_DIR/gtk4-min.wasm"
            
        # Speed optimization  
        wasm-opt -O3 --enable-simd \
            --strip-debug --strip-producers \
            "$wasm_file" -o "$DIST_DIR/gtk4-fast.wasm"
            
        log "WASM binary optimized ($(du -h "$DIST_DIR"/gtk4*.wasm | cut -f1 | tr '\n' ' '))"
    else
        warn "wasm-opt not found, skipping binary optimization"
    fi
    
    # Compress with Brotli if available
    if command -v brotli &> /dev/null; then
        brotli -9 -k "$DIST_DIR"/*.wasm 2>/dev/null || true
        log "WASM binaries compressed with Brotli"
    fi
}

# Create example HTML file
create_example() {
    log "Creating example HTML file..."
    
    cat > "$DIST_DIR/example.html" << 'EOF'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta http-equiv="Cross-Origin-Opener-Policy" content="same-origin">
    <meta http-equiv="Cross-Origin-Embedder-Policy" content="require-corp">
    <title>GTK WASM Example</title>
    <style>
        body {
            font-family: -apple-system, BlinkMacSystemFont, sans-serif;
            margin: 0;
            padding: 20px;
            background: #f5f5f5;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
        }
        .header {
            text-align: center;
            margin-bottom: 30px;
        }
        .canvas-container {
            background: white;
            border: 1px solid #ddd;
            border-radius: 8px;
            padding: 20px;
            margin: 20px 0;
            box-shadow: 0 2px 8px rgba(0,0,0,0.1);
        }
        canvas {
            border: 1px solid #ccc;
            display: block;
            margin: 0 auto;
        }
        .status {
            text-align: center;
            margin: 20px 0;
            padding: 10px;
            border-radius: 4px;
        }
        .loading {
            background: #fff3cd;
            color: #856404;
        }
        .ready {
            background: #d4edda;
            color: #155724;
        }
        .error {
            background: #f8d7da;
            color: #721c24;
        }
        .controls {
            text-align: center;
            margin: 20px 0;
        }
        button {
            background: #007bff;
            color: white;
            border: none;
            padding: 10px 20px;
            margin: 5px;
            border-radius: 4px;
            cursor: pointer;
        }
        button:hover {
            background: #0056b3;
        }
        button:disabled {
            background: #6c757d;
            cursor: not-allowed;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>GTK WASM Example</h1>
            <p>GTK 4 running in WebAssembly</p>
        </div>
        
        <div class="status loading" id="status">
            Loading GTK WASM module...
        </div>
        
        <div class="canvas-container">
            <canvas id="gtk-canvas" width="800" height="600"></canvas>
        </div>
        
        <div class="controls">
            <button id="createWindow" disabled>Create Window</button>
            <button id="addButton" disabled>Add Button</button>
            <button id="showDemo" disabled>Show Demo</button>
        </div>
    </div>

    <script type="module">
        let GTK = null;
        let app = null;
        
        const statusEl = document.getElementById('status');
        const canvas = document.getElementById('gtk-canvas');
        
        function setStatus(message, type = 'loading') {
            statusEl.textContent = message;
            statusEl.className = `status ${type}`;
        }
        
        async function initGTK() {
            try {
                setStatus('Loading GTK WASM module...');
                
                // Import GTK WASM module
                const GTKModule = (await import('./gtk4.js')).default;
                
                // Initialize with canvas
                GTK = await GTKModule({
                    canvas: canvas,
                    onReady: () => {
                        setStatus('GTK WASM ready!', 'ready');
                        enableControls();
                    },
                    onError: (error) => {
                        setStatus(`Error: ${error.message}`, 'error');
                        console.error('GTK WASM Error:', error);
                    }
                });
                
                // Initialize GTK
                GTK.ccall('gtk_init', null, [], []);
                
                setStatus('GTK WASM initialized successfully', 'ready');
                enableControls();
                
            } catch (error) {
                setStatus(`Failed to load GTK WASM: ${error.message}`, 'error');
                console.error('GTK WASM Loading Error:', error);
            }
        }
        
        function enableControls() {
            document.getElementById('createWindow').disabled = false;
            document.getElementById('addButton').disabled = false;
            document.getElementById('showDemo').disabled = false;
        }
        
        function createWindow() {
            if (!GTK) return;
            
            try {
                // Create GTK application
                if (!app) {
                    app = GTK.ccall('gtk_application_new', 'number', ['string', 'number'], 
                                   ['com.example.gtkwasm', 0]);
                }
                
                // Create window
                const window = GTK.ccall('gtk_window_new', 'number', [], []);
                GTK.ccall('gtk_window_set_title', null, ['number', 'string'], 
                         [window, 'GTK WASM Window']);
                GTK.ccall('gtk_window_set_default_size', null, ['number', 'number', 'number'], 
                         [window, 400, 300]);
                
                // Show window
                GTK.ccall('gtk_widget_show', null, ['number'], [window]);
                
                setStatus('GTK window created!', 'ready');
            } catch (error) {
                setStatus(`Error creating window: ${error.message}`, 'error');
                console.error('Window creation error:', error);
            }
        }
        
        function addButton() {
            if (!GTK) return;
            
            try {
                const button = GTK.ccall('gtk_button_new_with_label', 'number', ['string'], 
                                        ['Click Me!']);
                setStatus('Button created (not yet shown)', 'ready');
            } catch (error) {
                setStatus(`Error creating button: ${error.message}`, 'error');
                console.error('Button creation error:', error);
            }
        }
        
        function showDemo() {
            if (!GTK) return;
            
            setStatus('Running GTK demo...', 'loading');
            try {
                // This would typically show a more complex demo
                setStatus('GTK demo completed', 'ready');
            } catch (error) {
                setStatus(`Demo error: ${error.message}`, 'error');
                console.error('Demo error:', error);
            }
        }
        
        // Event listeners
        document.getElementById('createWindow').addEventListener('click', createWindow);
        document.getElementById('addButton').addEventListener('click', addButton);
        document.getElementById('showDemo').addEventListener('click', showDemo);
        
        // Initialize when page loads
        initGTK();
    </script>
</body>
</html>
EOF
    
    log "Example HTML file created"
}

# Generate comprehensive build report
generate_report() {
    log "Generating build report..."
    
    local report_file="$DIST_DIR/build-report.md"
    
    cat > "$report_file" << EOF
# GTK WASM Build Report

**Generated:** $(date)  
**Build Type:** $BUILD_TYPE  
**Target:** $TARGET  
**Features:** $FEATURES  
**Renderer:** $RENDERER  

## Build Configuration

- **Emscripten Version:** $(emcc --version | head -n1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')
- **Threading:** $(if [[ "$FEATURES" == *"threading"* ]] || [[ "$FEATURES" == "standard" ]] || [[ "$FEATURES" == "full" ]]; then echo "Enabled"; else echo "Disabled"; fi)
- **SIMD:** $(if [[ "$FEATURES" == *"simd"* ]] || [[ "$FEATURES" == "standard" ]] || [[ "$FEATURES" == "full" ]]; then echo "Enabled"; else echo "Disabled"; fi)
- **WebGPU:** $(if [[ "$RENDERER" == "webgpu" ]] || [[ "$RENDERER" == "hybrid" ]]; then echo "Enabled"; else echo "Disabled"; fi)

## File Sizes

EOF
    
    # Add file sizes to report
    if [[ -f "$DIST_DIR/gtk4.js" ]]; then
        echo "- **JavaScript:** $(du -h "$DIST_DIR/gtk4.js" | cut -f1)" >> "$report_file"
    fi
    
    if [[ -f "$DIST_DIR/gtk4.wasm" ]]; then
        echo "- **WASM Binary:** $(du -h "$DIST_DIR/gtk4.wasm" | cut -f1)" >> "$report_file"
    fi
    
    if [[ -f "$DIST_DIR/gtk4-min.wasm" ]]; then
        echo "- **WASM Minimized:** $(du -h "$DIST_DIR/gtk4-min.wasm" | cut -f1)" >> "$report_file"
    fi
    
    # Add resource information
    if [[ -d "$RESOURCES_DIR" ]]; then
        echo "- **Embedded Resources:** $(du -sh "$RESOURCES_DIR" | cut -f1)" >> "$report_file"
    fi
    
    cat >> "$report_file" << EOF

## Build Environment

- **CC:** $CC
- **CFLAGS:** $CFLAGS
- **LDFLAGS:** $LDFLAGS
- **Build Directory:** $BUILD_DIR
- **Distribution Directory:** $DIST_DIR

## Build Status

✅ Build completed successfully  
📦 Module ready for distribution  
🚀 See example.html for usage demonstration

EOF
    
    log "Build report saved to $report_file"
}

# Cleanup function
cleanup() {
    if [[ "${KEEP_BUILD:-false}" != "true" ]]; then
        log "Cleaning up temporary files..."
        rm -rf "$BUILD_DIR" 2>/dev/null || true
    else
        log "Build directory preserved at $BUILD_DIR"
    fi
}

# Main build process
main() {
    log "Starting GTK WASM production build..."
    log "Configuration: $BUILD_TYPE/$TARGET/$FEATURES/$RENDERER"
    
    check_dependencies
    setup_environment  
    prepare_resources
    configure_build
    build_library
    create_wasm_module
    create_typescript_definitions
    create_package_json
    optimize_wasm
    create_example
    generate_report
    
    log "GTK WASM build completed successfully!"
    log "Output directory: $DIST_DIR"
    
    # Display final artifacts
    echo ""
    echo -e "${BLUE}Build Artifacts:${NC}"
    ls -lh "$DIST_DIR"/*.{js,wasm,d.ts,json,html} 2>/dev/null || true
    
    echo ""
    echo -e "${GREEN}✅ Ready for deployment!${NC}"
    echo -e "${YELLOW}📖 See example.html for usage demonstration${NC}"
}

# Show usage information
show_usage() {
    cat << EOF
Usage: $0 [BUILD_TYPE] [TARGET] [FEATURES] [RENDERER]

BUILD_TYPE:
  release (default) - Optimized production build
  debug            - Development build with debug symbols  
  size             - Size-optimized build

TARGET:
  web (default)    - Browser environment
  node            - Node.js environment
  worker          - Web Worker environment
  all             - All environments

FEATURES:
  minimal         - Core widgets only
  standard        - Standard widgets + threading + SIMD
  full            - All features enabled
  threading       - Enable threading support
  simd            - Enable SIMD acceleration

RENDERER:
  broadway (default) - HTML5 Canvas rendering
  webgpu             - WebGPU hardware acceleration
  hybrid             - WebGPU + Canvas fallback

Examples:
  $0                                    # Default: release/web/standard/broadway
  $0 release web standard webgpu       # WebGPU accelerated build
  $0 debug web full broadway           # Debug build with all features
  $0 size web minimal broadway         # Minimal size-optimized build

Environment Variables:
  KEEP_BUILD=true                      # Preserve build directory
  EMSDK=/path/to/emsdk                 # Custom Emscripten SDK path

EOF
}

# Handle command line arguments
if [[ "${1:-}" == "--help" ]] || [[ "${1:-}" == "-h" ]]; then
    show_usage
    exit 0
fi

# Trap to ensure cleanup on exit
trap cleanup EXIT

# Run main build process
main "$@"