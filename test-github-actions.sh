#!/bin/bash
# Local GitHub Actions Testing Script
# Copyright 2025 Superstruct Ltd, New Zealand
# Licensed under LGPL-2.1-or-later

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check prerequisites
check_prerequisites() {
    log_info "Checking prerequisites..."
    
    # Check if act is installed
    if ! command -v act &> /dev/null; then
        log_error "act is not installed. Install it with:"
        echo "  curl https://raw.githubusercontent.com/nektos/act/master/install.sh | sudo bash"
        echo "  Or: brew install act (on macOS)"
        exit 1
    fi
    
    # Check if Docker is running
    if ! docker info &> /dev/null; then
        log_error "Docker is not running. Please start Docker and try again."
        exit 1
    fi
    
    # Check available disk space (need at least 10GB for images and builds)
    available_space=$(df -BG . | tail -1 | awk '{print $4}' | sed 's/G//')
    if [[ $available_space -lt 10 ]]; then
        log_warning "Low disk space detected: ${available_space}GB available. Recommend at least 10GB."
    fi
    
    log_success "Prerequisites check passed"
}

# Pull required Docker images
pull_images() {
    log_info "Pulling required Docker images..."
    
    docker pull catthehacker/ubuntu:act-22.04
    docker pull catthehacker/ubuntu:act-latest
    
    log_success "Docker images ready"
}

# Test individual jobs
test_job() {
    local job_name=$1
    local timeout=${2:-1800}  # 30 minutes default
    
    log_info "Testing job: $job_name"
    
    # Create logs directory
    mkdir -p logs
    
    # Run the job with timeout
    if timeout $timeout act -j "$job_name" --artifact-server-path ./artifacts 2>&1 | tee "logs/${job_name}.log"; then
        log_success "Job '$job_name' completed successfully"
        return 0
    else
        log_error "Job '$job_name' failed"
        return 1
    fi
}

# Test workflow in stages
test_workflow_stages() {
    log_info "Testing GitHub Actions workflow in stages..."
    
    local failed_jobs=()
    
    # Stage 1: Environment setup and validation
    log_info "=== Stage 1: Environment Setup ==="
    if ! test_job "setup-cache" 300; then
        failed_jobs+=("setup-cache")
    fi
    
    if ! test_job "build-dependencies" 900; then
        failed_jobs+=("build-dependencies")
    fi
    
    if ! test_job "validate-environment" 300; then
        failed_jobs+=("validate-environment")
    fi
    
    # Stage 2: Build (test one configuration to save time)
    log_info "=== Stage 2: Build Testing ==="
    if ! test_job "build-wasm" 2700; then  # 45 minutes
        failed_jobs+=("build-wasm")
    fi
    
    # Stage 3: Testing (if build succeeded)
    if [[ " ${failed_jobs[@]} " != *" build-wasm "* ]]; then
        log_info "=== Stage 3: Testing ==="
        
        if ! test_job "test-node" 600; then
            failed_jobs+=("test-node")
        fi
        
        # Skip browser tests in local testing due to complexity
        log_warning "Skipping browser tests in local testing (use CI for full browser test coverage)"
        
        if ! test_job "benchmark" 600; then
            failed_jobs+=("benchmark")
        fi
        
        if ! test_job "security-scan" 300; then
            failed_jobs+=("security-scan")
        fi
    else
        log_warning "Skipping testing stages due to build failure"
    fi
    
    # Report results
    if [[ ${#failed_jobs[@]} -eq 0 ]]; then
        log_success "All tested jobs passed!"
        return 0
    else
        log_error "Failed jobs: ${failed_jobs[*]}"
        return 1
    fi
}

# Quick validation test
test_quick() {
    log_info "Running quick validation tests..."
    
    # Test workflow syntax
    if act --list &> /dev/null; then
        log_success "Workflow syntax is valid"
    else
        log_error "Workflow syntax error detected"
        return 1
    fi
    
    # Test specific lightweight jobs
    local quick_jobs=("setup-cache" "validate-environment")
    local failed=0
    
    for job in "${quick_jobs[@]}"; do
        if ! test_job "$job" 600; then
            ((failed++))
        fi
    done
    
    if [[ $failed -eq 0 ]]; then
        log_success "Quick validation passed"
        return 0
    else
        log_error "$failed quick tests failed"
        return 1
    fi
}

# Full workflow test
test_full() {
    log_info "Running full workflow test..."
    
    # This runs the entire workflow but may take a very long time
    log_warning "Full workflow test may take 1-2 hours. Consider using --stages instead."
    
    if act --artifact-server-path ./artifacts 2>&1 | tee logs/full-workflow.log; then
        log_success "Full workflow test passed"
        return 0
    else
        log_error "Full workflow test failed"
        return 1
    fi
}

# Clean up function
cleanup() {
    log_info "Cleaning up..."
    
    # Stop any running containers
    docker container prune -f &> /dev/null || true
    
    # Clean up act cache if requested
    if [[ "${CLEAN_CACHE:-}" == "true" ]]; then
        log_info "Cleaning act cache..."
        docker image prune -f &> /dev/null || true
    fi
    
    log_info "Cleanup complete"
}

# Main execution
main() {
    local test_type="${1:-quick}"
    
    echo "🎯 GTK WASM GitHub Actions Local Testing"
    echo "========================================"
    
    # Trap cleanup on exit
    trap cleanup EXIT
    
    # Create necessary directories
    mkdir -p logs artifacts
    
    case "$test_type" in
        "quick")
            check_prerequisites
            test_quick
            ;;
        "stages")
            check_prerequisites
            pull_images
            test_workflow_stages
            ;;
        "full")
            check_prerequisites
            pull_images
            test_full
            ;;
        "list")
            log_info "Available GitHub Actions jobs:"
            act --list
            ;;
        "job")
            if [[ -z "${2:-}" ]]; then
                log_error "Please specify a job name: $0 job <job-name>"
                exit 1
            fi
            check_prerequisites
            pull_images
            test_job "$2"
            ;;
        *)
            echo "Usage: $0 {quick|stages|full|list|job <name>}"
            echo ""
            echo "  quick   - Quick validation of workflow syntax and lightweight jobs"
            echo "  stages  - Test workflow in logical stages (recommended)"
            echo "  full    - Run complete workflow (very slow)"
            echo "  list    - List all available jobs"
            echo "  job     - Test specific job by name"
            echo ""
            echo "Environment variables:"
            echo "  CLEAN_CACHE=true  - Clean Docker cache after testing"
            exit 1
            ;;
    esac
}

# Handle script arguments
main "$@"