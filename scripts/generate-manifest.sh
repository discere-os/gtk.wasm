#!/bin/bash
# Generate manifest.json for WASM SIDE module deployment
# Copyright 2025 Superstruct Ltd

set -euo pipefail

# Parse arguments
NAME=""
VERSION=""
SHA=""
OUTPUT=""
WASM_FILE=""

while [[ $# -gt 0 ]]; do
  case $1 in
    --name) NAME="$2"; shift 2 ;;
    --version) VERSION="$2"; shift 2 ;;
    --sha) SHA="$2"; shift 2 ;;
    --output) OUTPUT="$2"; shift 2 ;;
    --wasm-file) WASM_FILE="$2"; shift 2 ;;
    *) echo "Unknown option: $1"; exit 1 ;;
  esac
done

# Validation
[[ -z "$NAME" ]] && { echo "Error: --name required"; exit 1; }
[[ -z "$VERSION" ]] && { echo "Error: --version required"; exit 1; }
[[ -z "$SHA" ]] && { echo "Error: --sha required"; exit 1; }
[[ -z "$OUTPUT" ]] && { echo "Error: --output required"; exit 1; }
[[ -z "$WASM_FILE" ]] && WASM_FILE="install/wasm/${NAME}-side.wasm"

# Check WASM file exists
[[ ! -f "$WASM_FILE" ]] && { echo "Error: WASM file not found: $WASM_FILE"; exit 1; }

# Get file size
SIZE_BYTES=$(stat -f%z "$WASM_FILE" 2>/dev/null || stat -c%s "$WASM_FILE")
SIZE_HUMAN=$(du -h "$WASM_FILE" | awk '{print $1}')

# Extract exports and imports using wasm-objdump (if available)
EXPORTS="[]"
IMPORTS="{}"

if command -v wasm-objdump &> /dev/null; then
  EXPORTS=$(wasm-objdump -x "$WASM_FILE" | \
    grep -A 1000 "Export\[" | \
    grep "func" | \
    awk '{print "\"" $4 "\""}' | \
    jq -s '.')

  IMPORTS=$(wasm-objdump -x "$WASM_FILE" | \
    grep -A 1000 "Import\[" | \
    grep "func" | \
    awk '{print $4 "." $6}' | \
    sed 's/"//g' | \
    awk -F'.' '{print $1}' | \
    sort -u | \
    jq -R . | \
    jq -s 'group_by(.) | map({(.[0]): []}) | add')
fi

# Detect performance features from build flags
SIMD=false
THREADS=false
WEBGPU=false

if grep -q "msimd128" <<< "$(cat $WASM_FILE 2>/dev/null || echo '')"; then
  SIMD=true
fi

if grep -q "pthread" <<< "$(cat $WASM_FILE 2>/dev/null || echo '')"; then
  THREADS=true
fi

if grep -q "webgpu" <<< "$(cat $WASM_FILE 2>/dev/null || echo '')"; then
  WEBGPU=true
fi

# Get git info
GIT_COMMIT=$(git rev-parse HEAD 2>/dev/null || echo "unknown")
GIT_BRANCH=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "unknown")
GIT_REMOTE=$(git config --get remote.origin.url 2>/dev/null || echo "unknown")

# Get build environment
EMSCRIPTEN_VERSION=$(emcc --version 2>/dev/null | head -1 | awk '{print $NF}' || echo "unknown")
BUILD_TIMESTAMP=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
BUILD_HOST=$(hostname)

# CI information
CI_RUN=""
if [[ -n "${GITHUB_RUN_ID:-}" ]]; then
  CI_RUN="https://github.com/${GITHUB_REPOSITORY}/actions/runs/${GITHUB_RUN_ID}"
fi

# Generate manifest JSON
cat > "$OUTPUT" <<EOF
{
  "name": "${NAME}-side.wasm",
  "module": "$NAME",
  "version": "$VERSION",
  "sha256": "$SHA",
  "build": {
    "emscripten": "$EMSCRIPTEN_VERSION",
    "timestamp": "$BUILD_TIMESTAMP",
    "host": "$BUILD_HOST",
    "commit": "$GIT_COMMIT",
    "branch": "$GIT_BRANCH",
    "repository": "$GIT_REMOTE",
    "ci_run": "$CI_RUN"
  },
  "size": {
    "bytes": $SIZE_BYTES,
    "human": "$SIZE_HUMAN"
  },
  "exports": $EXPORTS,
  "imports": $IMPORTS,
  "performance": {
    "simd": $SIMD,
    "threads": $THREADS,
    "webgpu": $WEBGPU
  },
  "cdn": {
    "latest": "https://wasm.discere.cloud/v1/modules/$NAME/latest/${NAME}-side.wasm",
    "version": "https://wasm.discere.cloud/v1/modules/$NAME/$VERSION/${NAME}-side.wasm",
    "sha": "https://wasm.discere.cloud/v1/modules/$NAME/sha-$SHA/${NAME}-side.wasm"
  },
  "generated": "$BUILD_TIMESTAMP"
}
EOF

echo "✅ Manifest generated: $OUTPUT"
echo "📋 Module: $NAME@$VERSION"
echo "🔒 SHA-256: $SHA"
echo "📦 Size: $SIZE_HUMAN ($SIZE_BYTES bytes)"