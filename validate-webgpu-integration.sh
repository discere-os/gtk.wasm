#!/bin/bash
# WebGPU Integration Validation Script
# Copyright 2025 Superstruct Ltd, New Zealand
# Licensed under LGPL-2.1-or-later

set -euo pipefail

readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Colors for output
readonly GREEN='\033[0;32m'
readonly RED='\033[0;31m'
readonly YELLOW='\033[1;33m'
readonly BLUE='\033[0;34m'
readonly NC='\033[0m'

log() {
    echo -e "${GREEN}[Validation]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[Validation] WARNING:${NC} $1"
}

error() {
    echo -e "${RED}[Validation] ERROR:${NC} $1" >&2
}

info() {
    echo -e "${BLUE}[Validation] INFO:${NC} $1"
}

# Track validation results
declare -i errors=0
declare -i warnings=0

check_file() {
    local file="$1"
    local description="$2"
    
    if [[ -f "$file" ]]; then
        log "✓ $description: $file"
        return 0
    else
        error "✗ Missing $description: $file"
        ((errors++))
        return 1
    fi
}

check_directory() {
    local dir="$1"
    local description="$2"
    
    if [[ -d "$dir" ]]; then
        log "✓ $description: $dir"
        return 0
    else
        error "✗ Missing $description: $dir"
        ((errors++))
        return 1
    fi
}

check_file_content() {
    local file="$1"
    local pattern="$2"
    local description="$3"
    
    if [[ -f "$file" ]] && grep -q "$pattern" "$file"; then
        log "✓ $description found in $file"
        return 0
    else
        error "✗ $description not found in $file"
        ((errors++))
        return 1
    fi
}

validate_webgpu_files() {
    info "Validating WebGPU implementation files..."
    
    # Core WebGPU files
    check_file "gsk/webgpu/gskwebgpurenderer.h" "WebGPU renderer header"
    check_file "gsk/webgpu/gskwebgpurenderer.c" "WebGPU renderer implementation" 
    check_file "gsk/webgpu/gskwebgpudevice.h" "WebGPU device header"
    check_file "gsk/webgpu/gskwebgpudevice.c" "WebGPU device implementation"
    check_file "gsk/webgpu/gskwebgpushaders.h" "WebGPU shaders header"
    check_file "gsk/webgpu/gskwebgpushaders.c" "WebGPU shaders implementation"
    check_file "gsk/webgpu/gskwebgpurendernodes.c" "WebGPU render nodes implementation"
    check_file "gsk/webgpu/gskwebgpuintegration.c" "WebGPU integration layer"
    
    # Private headers
    check_file "gsk/webgpu/gskwebgpurendererprivate.h" "WebGPU renderer private header"
    check_file "gsk/webgpu/gskwebgpupipelines.h" "WebGPU pipelines header"
    check_file "gsk/webgpu/gskwebgpucomputeshaders.h" "WebGPU compute shaders header"
}

validate_build_system() {
    info "Validating build system integration..."
    
    # Build system files
    check_file "gsk/webgpu/meson.build" "WebGPU meson build file"
    check_file "gsk/webgpu/webgpu-config.cmake" "WebGPU CMake config"
    
    # Check if WebGPU options are in meson.options
    check_file_content "meson.options" "webgpu-renderer" "WebGPU renderer option"
    check_file_content "meson.options" "simd-support" "SIMD support option"
    check_file_content "meson.options" "threading-support" "Threading support option"
    
    # Check if WebGPU is integrated in main GSK build
    check_file_content "gsk/meson.build" "webgpu_enabled" "WebGPU build integration"
    check_file_content "gsk/meson.build" "subdir('webgpu')" "WebGPU subdirectory inclusion"
}

validate_demo_and_tests() {
    info "Validating demo and test files..."
    
    check_file "examples/webgpu-demo.html" "WebGPU demo HTML"
    check_file "tests/webgpu-integration-test.c" "WebGPU integration test"
    check_file "test-webgpu.sh" "WebGPU test runner script"
    
    # Check if test is integrated in build system
    check_file_content "tests/meson.build" "webgpu-integration-test" "WebGPU test integration"
}

validate_build_scripts() {
    info "Validating build scripts..."
    
    check_file "build-wasm.sh" "WASM build script"
    check_file "validate-webgpu-integration.sh" "WebGPU validation script (this file)"
    
    # Check if build script has WebGPU support
    check_file_content "build-wasm.sh" "webgpu" "WebGPU build support"
    check_file_content "build-wasm.sh" "USE_WEBGPU" "WebGPU compilation flags"
    check_file_content "build-wasm.sh" "ASYNCIFY" "Asyncify support for WebGPU"
}

validate_includes_and_exports() {
    info "Validating includes and exports..."
    
    # Check if main GSK header would include WebGPU on WASM
    if ! check_file_content "gsk/gsk.h" "webgpu" "WebGPU header inclusion"; then
        warn "WebGPU headers may not be exposed in main GSK header"
        ((warnings++))
    fi
    
    # Check for proper header guards and includes
    local webgpu_headers=(
        "gsk/webgpu/gskwebgpurenderer.h"
        "gsk/webgpu/gskwebgpudevice.h"
        "gsk/webgpu/gskwebgpushaders.h"
    )
    
    for header in "${webgpu_headers[@]}"; do
        if [[ -f "$header" ]]; then
            if ! grep -q "#pragma once\|#ifndef.*_H\|#define.*_H" "$header"; then
                warn "Header $header may be missing header guards"
                ((warnings++))
            fi
        fi
    done
}

validate_documentation() {
    info "Validating documentation..."
    
    # Check for documentation updates
    if [[ -f "README.md" ]]; then
        if grep -q -i "webgpu\|web.*gpu" "README.md"; then
            log "✓ WebGPU mentioned in README.md"
        else
            warn "WebGPU not documented in README.md"
            ((warnings++))
        fi
    fi
    
    # Check consolidated documentation files
    local doc_files=(
        "CONSOLIDATED-WASM-ECOSYSTEM-TECHNICAL-REFERENCE.md"
        "HOLISTIC-WASM-ECOSYSTEM-STRATEGY.md"
    )
    
    for doc_file in "${doc_files[@]}"; do
        if [[ -f "$doc_file" ]]; then
            if grep -q -i "webgpu\|web.*gpu" "$doc_file"; then
                log "✓ WebGPU documented in $doc_file"
            else
                warn "WebGPU not documented in $doc_file"
                ((warnings++))
            fi
        fi
    done
}

show_summary() {
    echo
    info "Validation Summary:"
    info "=================="
    
    if [[ $errors -eq 0 ]] && [[ $warnings -eq 0 ]]; then
        log "🎉 All validations passed! WebGPU integration looks complete."
    elif [[ $errors -eq 0 ]]; then
        warn "⚠️  Validation completed with $warnings warnings."
        warn "WebGPU integration is functional but could be improved."
    else
        error "❌ Validation failed with $errors errors and $warnings warnings."
        error "WebGPU integration is incomplete or has issues."
    fi
    
    echo
    info "Next steps:"
    if [[ $errors -gt 0 ]]; then
        info "1. Fix the errors listed above"
        info "2. Run this validation script again"
    else
        info "1. Test the build: ./build-wasm.sh release web standard webgpu"
        info "2. Run WebGPU tests: ./test-webgpu.sh"
        info "3. Test in browser with examples/webgpu-demo.html"
    fi
    
    return $errors
}

main() {
    log "Starting WebGPU integration validation..."
    log "Working directory: $SCRIPT_DIR"
    echo
    
    validate_webgpu_files
    echo
    validate_build_system  
    echo
    validate_demo_and_tests
    echo
    validate_build_scripts
    echo
    validate_includes_and_exports
    echo
    validate_documentation
    echo
    
    show_summary
}

# Change to script directory
cd "$SCRIPT_DIR"

main "$@"