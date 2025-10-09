# Fonts Directory

Place TrueType (.ttf) or OpenType (.otf) font files here.

## Recommended: Roboto Font

Download from: https://fonts.google.com/specimen/Roboto

Required files:
- Roboto-Regular.ttf
- Roboto-Bold.ttf
- Roboto-Italic.ttf

These fonts will be embedded in the WASM filesystem and configured for Pango/fontconfig.

## Download Script

```bash
#!/bin/bash
# Download Roboto fonts
ROBOTO_VERSION="2.138"
BASE_URL="https://github.com/googlefonts/roboto/releases/download/v${ROBOTO_VERSION}"

wget "${BASE_URL}/roboto-unhinted.zip" -O roboto.zip
unzip -j roboto.zip "*/Roboto-Regular.ttf" "*/Roboto-Bold.ttf" "*/Roboto-Italic.ttf" -d .
rm roboto.zip
```
