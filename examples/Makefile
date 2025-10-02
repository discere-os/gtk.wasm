# GTK WebGPU WASM Example Makefile
# Copyright © 2025 Superstruct Ltd, New Zealand

# Emscripten configuration
CC = emcc
PKG_CONFIG = pkg-config

# Build configuration
BUILD_TYPE ?= debug
TARGET_DIR = build
DIST_DIR = dist

# Source files
C_SOURCES = gtk-webgpu-app.c
WASM_OUTPUT = $(DIST_DIR)/gtk-webgpu-app.wasm
JS_OUTPUT = $(DIST_DIR)/gtk-webgpu-app.js
HTML_OUTPUT = $(DIST_DIR)/gtk-webgpu-app.html

# GTK/GLib dependencies (when available)
# Note: These would need to be cross-compiled for WASM
# For now we'll use simplified includes
GTK_CFLAGS = -I../gsk -I../gtk -I../gdk -I.. -DGTK_COMPILATION -DGSK_WEBGPU_ENABLED
GTK_LIBS = 

# Emscripten flags
BASE_CFLAGS = -std=c11 -Wall -Wextra
BASE_LDFLAGS = -s WASM=1 -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]'

# WebGPU specific flags
WEBGPU_FLAGS = --use-port=emdawnwebgpu -sASYNCIFY -sASYNCIFY_STACK_SIZE=16384

# Build type specific flags
ifeq ($(BUILD_TYPE),release)
    CFLAGS = $(BASE_CFLAGS) -O3 -DNDEBUG $(GTK_CFLAGS)
    LDFLAGS = $(BASE_LDFLAGS) -O3 --closure=1 $(WEBGPU_FLAGS)
else ifeq ($(BUILD_TYPE),debug)
    CFLAGS = $(BASE_CFLAGS) -O1 -g -DDEBUG -gsource-map $(GTK_CFLAGS)
    LDFLAGS = $(BASE_LDFLAGS) -O1 -g -gsource-map $(WEBGPU_FLAGS)
else
    $(error Invalid BUILD_TYPE: $(BUILD_TYPE). Use 'debug' or 'release')
endif

# Exported functions for JavaScript interface
EXPORTED_FUNCTIONS = _main,_gtk_webgpu_init,_gtk_webgpu_run,_gtk_webgpu_create_window,_gtk_webgpu_add_widget,_gtk_webgpu_get_widget_count,_gtk_webgpu_clear_widgets,_gtk_webgpu_get_renderer_info,_gtk_webgpu_cleanup

# Main build target
.PHONY: all
all: $(WASM_OUTPUT) $(HTML_OUTPUT)

# Create directories
$(TARGET_DIR) $(DIST_DIR):
	@mkdir -p $@

# Build the WASM module
$(WASM_OUTPUT): $(C_SOURCES) | $(DIST_DIR)
	@echo "Building GTK WebGPU WASM module ($(BUILD_TYPE))..."
	$(CC) $(CFLAGS) $(C_SOURCES) \
		$(LDFLAGS) \
		-sEXPORTED_FUNCTIONS='[$(EXPORTED_FUNCTIONS)]' \
		-o $(JS_OUTPUT)
	@echo "WASM module built successfully!"

# Create the HTML file that loads our demo
$(HTML_OUTPUT): webgpu-real-demo.html $(WASM_OUTPUT) | $(DIST_DIR)
	@echo "Creating integrated demo page..."
	@cp webgpu-real-demo.html $(HTML_OUTPUT)
	@echo "Demo page created!"

# Development server
.PHONY: serve
serve: all
	@echo "Starting development server on http://localhost:8000"
	@echo "Open http://localhost:8000/$(HTML_OUTPUT) in your browser"
	@cd $(DIST_DIR) && python3 -m http.server 8000

# Clean build artifacts
.PHONY: clean
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(TARGET_DIR) $(DIST_DIR)
	@echo "Clean completed!"

# Install dependencies (placeholder for future use)
.PHONY: install-deps
install-deps:
	@echo "Installing Emscripten dependencies..."
	@echo "Note: Ensure Emscripten SDK is properly installed and activated"
	@echo "Run: source /path/to/emsdk/emsdk_env.sh"

# Build with different configurations
.PHONY: debug release
debug:
	@$(MAKE) BUILD_TYPE=debug

release:
	@$(MAKE) BUILD_TYPE=release

# Help target
.PHONY: help
help:
	@echo "GTK WebGPU WASM Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  all          - Build the WASM module and demo page"
	@echo "  debug        - Build debug version"
	@echo "  release      - Build optimized release version"
	@echo "  serve        - Build and start development server"
	@echo "  clean        - Clean build artifacts"
	@echo "  install-deps - Install build dependencies"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Build types:"
	@echo "  debug        - Debug build with source maps"
	@echo "  release      - Optimized build with closure compiler"
	@echo ""
	@echo "Examples:"
	@echo "  make debug   - Build debug version"
	@echo "  make release - Build release version"
	@echo "  make serve   - Build and serve locally"

# Print configuration for debugging
.PHONY: config
config:
	@echo "Build Configuration:"
	@echo "  BUILD_TYPE: $(BUILD_TYPE)"
	@echo "  CC: $(CC)"
	@echo "  CFLAGS: $(CFLAGS)"
	@echo "  LDFLAGS: $(LDFLAGS)"
	@echo "  SOURCES: $(C_SOURCES)"
	@echo "  OUTPUT: $(WASM_OUTPUT)"

# Default target
.DEFAULT_GOAL := all