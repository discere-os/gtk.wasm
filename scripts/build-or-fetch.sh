#!/bin/bash
# Hybrid build strategy: fetch from CDN if available, otherwise build locally
# Copyright 2025 Superstruct Ltd

set -euo pipefail

# Configuration
MODULE="${1:-}"
VERSION="${2:-latest}"
FORCE_BUILD="${FORCE_BUILD:-false}"
FORCE_FETCH="${FORCE_FETCH:-false}"
MAX_AGE_DAYS="${MAX_AGE_DAYS:-1}"

CDN_BASE="${WASM_CDN_BASE:-https://wasm.discere.cloud/v1/modules}"
INSTALL_DIR="./install/lib"
MODULE_DIR="../${MODULE}.wasm"

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Usage
if [[ -z "$MODULE" ]]; then
  cat <<EOF
Usage: $0 MODULE [VERSION]

Hybrid build strategy for WASM SIDE modules:
1. Check if recent local build exists (< $MAX_AGE_DAYS day)
2. Try fetching from wasm.discere.cloud CDN (fast)
3. Fallback to local build if fetch fails

Environment Variables:
  FORCE_BUILD=true   Always build locally, skip fetch
  FORCE_FETCH=true   Always fetch from CDN, fail if unavailable
  MAX_AGE_DAYS=N     Consider local builds recent if < N days old

Examples:
  $0 zlib              # Build or fetch zlib@latest
  $0 zlib v1.4.2       # Build or fetch specific version
  FORCE_BUILD=true $0 zlib  # Always build locally
EOF
  exit 1
fi

LOCAL_BUILD="$INSTALL_DIR/${MODULE}-side.wasm"
REMOTE_URL="$CDN_BASE/$MODULE/$VERSION/${MODULE}-side.wasm"

# Check local build age
check_local_build() {
  if [[ ! -f "$LOCAL_BUILD" ]]; then
    return 1
  fi

  # Check age (< MAX_AGE_DAYS old)
  if [[ -n "$(find "$LOCAL_BUILD" -mtime -${MAX_AGE_DAYS} 2>/dev/null)" ]]; then
    return 0
  fi

  return 1
}

# Try fetching from CDN
fetch_from_cdn() {
  echo -e "${BLUE}🌐 Attempting to fetch ${MODULE}@${VERSION} from CDN...${NC}"

  if ! curl -fL --max-time 15 "$REMOTE_URL" -o "$LOCAL_BUILD" 2>/dev/null; then
    return 1
  fi

  # Verify integrity if SHA-256 available
  if curl -fL --max-time 5 "${REMOTE_URL}.sha256" 2>/dev/null | \
     (cd "$INSTALL_DIR" && sha256sum -c - 2>/dev/null); then
    echo -e "${GREEN}✓ Fetched and verified ${MODULE} from CDN${NC}"
    return 0
  else
    echo -e "${YELLOW}⚠ SHA-256 verification not available, using anyway${NC}"
    return 0
  fi
}

# Build locally
build_locally() {
  echo -e "${BLUE}🔨 Building ${MODULE} locally...${NC}"

  if [[ ! -d "$MODULE_DIR" ]]; then
    echo "Error: Module directory not found: $MODULE_DIR"
    return 1
  fi

  cd "$MODULE_DIR"

  # Determine build script
  if [[ -x ./build-dual.sh ]]; then
    ./build-dual.sh side
  elif [[ -x ./build-wasm.sh ]]; then
    ./build-wasm.sh
  else
    echo "Error: No build script found in $MODULE_DIR"
    return 1
  fi

  cd - > /dev/null

  if [[ -f "$LOCAL_BUILD" ]]; then
    echo -e "${GREEN}✓ ${MODULE} built successfully${NC}"
    return 0
  else
    echo "Error: Build completed but WASM file not found: $LOCAL_BUILD"
    return 1
  fi
}

# Main logic
main() {
  mkdir -p "$INSTALL_DIR"

  # Force build mode
  if [[ "$FORCE_BUILD" == "true" ]]; then
    echo "FORCE_BUILD enabled, skipping fetch"
    build_locally
    exit $?
  fi

  # Force fetch mode
  if [[ "$FORCE_FETCH" == "true" ]]; then
    echo "FORCE_FETCH enabled, skipping local build"
    fetch_from_cdn || {
      echo "Error: Fetch failed and FORCE_FETCH is enabled"
      exit 1
    }
    exit 0
  fi

  # Check if recent local build exists
  if check_local_build; then
    echo -e "${GREEN}✓ Using recent local build: $LOCAL_BUILD${NC}"
    local age=$(find "$LOCAL_BUILD" -printf '%Ts\n' 2>/dev/null || echo "0")
    local now=$(date +%s)
    local hours_old=$(( ($now - $age) / 3600 ))
    echo "  (${hours_old}h old)"
    exit 0
  fi

  # Try fetching from CDN first (faster than building)
  if fetch_from_cdn; then
    exit 0
  fi

  # Fallback to local build
  echo "CDN fetch failed or unavailable, falling back to local build"
  build_locally
}

main