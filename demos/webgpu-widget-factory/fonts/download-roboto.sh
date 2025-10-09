#!/bin/bash
# Download Roboto fonts for WASM demo

set -e

cd "$(dirname "$0")"

echo "Downloading Roboto fonts..."

# Roboto v2.138
ROBOTO_VERSION="2.138"
BASE_URL="https://github.com/googlefonts/roboto/releases/download/v${ROBOTO_VERSION}"

# Download and extract
wget "${BASE_URL}/roboto-unhinted.zip" -O roboto.zip

# Extract just the variants we need
unzip -j roboto.zip \
  "Roboto-Regular.ttf" \
  "Roboto-Bold.ttf" \
  "Roboto-Italic.ttf" \
  "Roboto-BoldItalic.ttf"

rm roboto.zip

echo "✅ Downloaded Roboto fonts:"
ls -lh *.ttf

echo ""
echo "Fonts ready for WASM build!"
