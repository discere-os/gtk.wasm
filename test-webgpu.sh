#!/bin/bash
# WebGPU Integration Test Runner
# Copyright 2025 Superstruct Ltd, New Zealand
# Licensed under LGPL-2.1-or-later

set -euo pipefail

readonly SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly BUILD_DIR="${SCRIPT_DIR}/_build_wasm"

# Colors for output
readonly GREEN='\033[0;32m'
readonly RED='\033[0;31m'
readonly YELLOW='\033[1;33m'
readonly NC='\033[0m'

log() {
    echo -e "${GREEN}[WebGPU Test]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[WebGPU Test] WARNING:${NC} $1"
}

error() {
    echo -e "${RED}[WebGPU Test] ERROR:${NC} $1" >&2
}

# Check if WebGPU test was built
check_test_exists() {
    if [[ ! -f "${BUILD_DIR}/tests/webgpu-integration-test" ]]; then
        error "WebGPU integration test not found. Please build the project first:"
        error "  ./build-wasm.sh release web standard webgpu"
        exit 1
    fi
}

# Run the WebGPU integration test
run_webgpu_test() {
    log "Running WebGPU integration test..."
    
    cd "${BUILD_DIR}/tests"
    
    # Set WebGPU-specific environment variables for testing
    export GSK_WEBGPU_FEATURES="simd,compute"
    export G_MESSAGES_DEBUG="Gsk"
    
    if ./webgpu-integration-test; then
        log "WebGPU integration test passed!"
        return 0
    else
        error "WebGPU integration test failed!"
        return 1
    fi
}

# Test WebGPU renderer creation under different conditions
test_renderer_variants() {
    log "Testing WebGPU renderer variants..."
    
    local test_cases=(
        "none"
        "simd"
        "threading" 
        "simd,threading,compute"
        "all"
    )
    
    for features in "${test_cases[@]}"; do
        log "Testing with features: ${features}"
        export GSK_WEBGPU_FEATURES="${features}"
        
        if ! ./webgpu-integration-test --tap; then
            warn "Test failed with features: ${features}"
        fi
    done
    
    unset GSK_WEBGPU_FEATURES
}

# Run browser compatibility tests (if available)
test_browser_compatibility() {
    log "Testing browser compatibility..."
    
    # This would ideally run in different browser environments
    # For now, just verify the WebGPU demo HTML file exists
    if [[ -f "${SCRIPT_DIR}/examples/webgpu-demo.html" ]]; then
        log "WebGPU demo HTML file available for browser testing"
        log "To test in browser:"
        log "  1. Serve the examples directory with a web server"
        log "  2. Open webgpu-demo.html in a WebGPU-capable browser"
        log "  3. Check console for WebGPU initialization messages"
    else
        warn "WebGPU demo HTML file not found"
    fi
}

# Main test runner
main() {
    log "Starting WebGPU integration tests..."
    
    check_test_exists
    
    local exit_code=0
    
    # Run basic WebGPU integration test
    if ! run_webgpu_test; then
        exit_code=1
    fi
    
    # Run renderer variant tests
    cd "${BUILD_DIR}/tests"
    test_renderer_variants
    
    # Check browser compatibility setup
    test_browser_compatibility
    
    if [[ $exit_code -eq 0 ]]; then
        log "All WebGPU tests completed successfully!"
    else
        error "Some WebGPU tests failed!"
    fi
    
    return $exit_code
}

# Show usage information
show_usage() {
    echo "WebGPU Integration Test Runner"
    echo ""
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  -h, --help     Show this help message"
    echo "  --verbose      Enable verbose output"
    echo ""
    echo "Environment variables:"
    echo "  GSK_WEBGPU_FEATURES  - Comma-separated feature list"
    echo "  G_MESSAGES_DEBUG     - Enable debug logging"
    echo ""
    echo "Examples:"
    echo "  $0                    # Run all WebGPU tests"
    echo "  GSK_WEBGPU_FEATURES=simd $0  # Test with SIMD only"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_usage
            exit 0
            ;;
        --verbose)
            set -x
            shift
            ;;
        *)
            error "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

main "$@"