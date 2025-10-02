#!/bin/bash
# Fetch pre-built WASM SIDE modules from wasm.discere.cloud
# Copyright 2025 Superstruct Ltd

set -euo pipefail

# Configuration
CDN_BASE="${WASM_CDN_BASE:-https://wasm.discere.cloud/v1/modules}"
CACHE_DIR="${WASM_CACHE_DIR:-$HOME/.cache/discere-wasm}"
INSTALL_DIR="${WASM_INSTALL_DIR:-./install/lib}"
VERIFY_INTEGRITY="${WASM_VERIFY:-true}"
USE_CACHE="${WASM_USE_CACHE:-true}"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Usage
usage() {
  cat <<EOF
Usage: $0 [OPTIONS] MODULE[:VERSION] [MODULE[:VERSION] ...]

Fetch pre-built WASM SIDE modules from wasm.discere.cloud CDN.

OPTIONS:
  --no-cache          Skip cache, always fetch from CDN
  --no-verify         Skip SHA-256 integrity verification
  --cache-dir DIR     Cache directory (default: ~/.cache/discere-wasm)
  --install-dir DIR   Install directory (default: ./install/lib)
  --cdn-base URL      CDN base URL (default: https://wasm.discere.cloud/v1/modules)
  --manifest FILE     Read dependencies from manifest file
  --help              Show this help message

EXAMPLES:
  $0 zlib:latest libpng:v1.6.44 pixman:sha-abc123
  $0 --manifest dependencies.json
  $0 --no-cache zlib:latest

VERSIONS:
  latest              Latest stable version (302 redirect)
  v1.2.3              Semantic version (immutable)
  sha-abc123          Content-addressable by SHA-256 (immutable, highest cache)
EOF
  exit 1
}

# Parse options
MODULES=()
MANIFEST_FILE=""

while [[ $# -gt 0 ]]; do
  case $1 in
    --no-cache) USE_CACHE=false; shift ;;
    --no-verify) VERIFY_INTEGRITY=false; shift ;;
    --cache-dir) CACHE_DIR="$2"; shift 2 ;;
    --install-dir) INSTALL_DIR="$2"; shift 2 ;;
    --cdn-base) CDN_BASE="$2"; shift 2 ;;
    --manifest) MANIFEST_FILE="$2"; shift 2 ;;
    --help) usage ;;
    -*) echo "Unknown option: $1"; usage ;;
    *) MODULES+=("$1"); shift ;;
  esac
done

# Load dependencies from manifest if provided
if [[ -n "$MANIFEST_FILE" ]]; then
  if [[ ! -f "$MANIFEST_FILE" ]]; then
    echo -e "${RED}Error: Manifest file not found: $MANIFEST_FILE${NC}"
    exit 1
  fi

  if command -v jq &> /dev/null; then
    mapfile -t MANIFEST_DEPS < <(jq -r '.dependencies[]' "$MANIFEST_FILE")
    MODULES+=("${MANIFEST_DEPS[@]}")
  else
    echo -e "${YELLOW}Warning: jq not found, cannot parse manifest${NC}"
  fi
fi

# Validate we have modules to fetch
if [[ ${#MODULES[@]} -eq 0 ]]; then
  echo -e "${RED}Error: No modules specified${NC}"
  usage
fi

# Create directories
mkdir -p "$CACHE_DIR" "$INSTALL_DIR"

# Stats
FETCHED=0
CACHED=0
FAILED=0

# Fetch function
fetch_module() {
  local dep="$1"
  IFS=':' read -r module version <<< "$dep"

  # Default to latest if no version specified
  [[ -z "$version" ]] && version="latest"

  echo -e "${BLUE}📦 Fetching ${module}@${version}...${NC}"

  local wasm_file="${module}-side.wasm"
  local remote_url="$CDN_BASE/$module/$version/$wasm_file"
  local cache_path="$CACHE_DIR/$module/$version/$wasm_file"
  local install_path="$INSTALL_DIR/$wasm_file"

  # Check cache first
  if [[ "$USE_CACHE" == "true" ]] && [[ -f "$cache_path" ]]; then
    echo -e "  ${GREEN}✓${NC} Using cached version"
    cp "$cache_path" "$install_path"
    ((CACHED++))
    return 0
  fi

  # Fetch from CDN
  echo "  🌐 Downloading from CDN..."

  if ! curl -fL --max-time 30 --progress-bar "$remote_url" -o "$install_path"; then
    echo -e "  ${RED}✗${NC} Download failed"
    ((FAILED++))
    return 1
  fi

  # Verify integrity
  if [[ "$VERIFY_INTEGRITY" == "true" ]]; then
    echo "  🔒 Verifying integrity..."

    local sha_url="${remote_url}.sha256"
    local sha_file="${install_path}.sha256"

    if curl -fL --max-time 10 "$sha_url" -o "$sha_file" 2>/dev/null; then
      if (cd "$INSTALL_DIR" && sha256sum -c "$wasm_file.sha256" &>/dev/null); then
        echo -e "  ${GREEN}✓${NC} SHA-256 verified"
        rm -f "$sha_file"
      else
        echo -e "  ${RED}✗${NC} SHA-256 verification failed"
        rm -f "$install_path" "$sha_file"
        ((FAILED++))
        return 1
      fi
    else
      echo -e "  ${YELLOW}⚠${NC}  SHA-256 file not available, skipping verification"
    fi
  fi

  # Save to cache
  if [[ "$USE_CACHE" == "true" ]]; then
    mkdir -p "$(dirname "$cache_path")"
    cp "$install_path" "$cache_path"
  fi

  # Download manifest for metadata
  local manifest_url="$CDN_BASE/$module/$version/manifest.json"
  local manifest_path="$INSTALL_DIR/${module}.manifest.json"

  if curl -fL --max-time 10 "$manifest_url" -o "$manifest_path" 2>/dev/null; then
    if command -v jq &> /dev/null; then
      local size=$(jq -r '.size.human' "$manifest_path")
      local build_date=$(jq -r '.build.timestamp' "$manifest_path")
      echo "  📋 Size: $size | Built: $build_date"
    fi
  fi

  echo -e "  ${GREEN}✓${NC} ${module}@${version} installed"
  ((FETCHED++))
  return 0
}

# Main loop
echo -e "${BLUE}🚀 Fetching ${#MODULES[@]} WASM modules${NC}"
echo "  CDN: $CDN_BASE"
echo "  Install: $INSTALL_DIR"
echo "  Cache: $CACHE_DIR"
echo ""

for module_spec in "${MODULES[@]}"; do
  fetch_module "$module_spec" || true
  echo ""
done

# Summary
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${GREEN}✓${NC} Fetched: $FETCHED"
echo -e "${YELLOW}↻${NC} Cached:  $CACHED"

if [[ $FAILED -gt 0 ]]; then
  echo -e "${RED}✗${NC} Failed:  $FAILED"
  exit 1
else
  echo -e "${GREEN}🎉 All modules fetched successfully!${NC}"
fi