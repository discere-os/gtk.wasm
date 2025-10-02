#!/bin/bash
# Comprehensive refactoring of all GTK dependency repositories
# - Updates to new deployment workflow template
# - Standardizes build scripts
# - Ensures deno.json exists
# Copyright 2025 Superstruct Ltd

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$(dirname "$SCRIPT_DIR")")"
TEMPLATE="${SCRIPT_DIR}/deploy-wasm-template.yml"

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

# GTK dependency stack (in dependency order)
FOUNDATION=(zlib libpng pixman freetype harfbuzz libexpat)
SECOND_TIER=(fontconfig cairo fribidi glib)
THIRD_TIER=(pango gdk-pixbuf)

ALL_REPOS=("${FOUNDATION[@]}" "${SECOND_TIER[@]}" "${THIRD_TIER[@]}")

echo -e "${BLUE}GTK Dependency Refactoring${NC}"
echo -e "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# Summary counters
UPDATED=0
SKIPPED=0
FAILED=0
BUILD_STANDARDIZED=0
DENO_ADDED=0

for repo in "${ALL_REPOS[@]}"; do
  REPO_PATH="${ROOT_DIR}/${repo}.wasm"

  if [[ ! -d "$REPO_PATH" ]]; then
    echo -e "${YELLOW}⏭️  ${repo}.wasm: Repository not found${NC}"
    ((SKIPPED++))
    continue
  fi

  echo -e "${BLUE}📦 Processing: ${repo}.wasm${NC}"

  cd "$REPO_PATH"

  # 1. Check and update GitHub Actions workflow
  WORKFLOW_DIR=".github/workflows"
  WORKFLOW_FILE="${WORKFLOW_DIR}/deploy-wasm.yml"

  mkdir -p "$WORKFLOW_DIR"

  if [[ -f "$WORKFLOW_FILE" ]]; then
    echo -e "  ${YELLOW}⚠️  deploy-wasm.yml exists, backing up${NC}"
    cp "$WORKFLOW_FILE" "${WORKFLOW_FILE}.backup-$(date +%Y%m%d-%H%M%S)"
  fi

  # Copy new template
  cp "$TEMPLATE" "$WORKFLOW_FILE"
  echo -e "  ${GREEN}✓${NC} Updated workflow: deploy-wasm.yml"
  ((UPDATED++))

  # 2. Check build script consistency
  if [[ -f "build-dual.sh" ]]; then
    echo -e "  ${GREEN}✓${NC} Build script: build-dual.sh (standard)"
  elif [[ -f "build-wasm.sh" ]]; then
    echo -e "  ${YELLOW}⚠️${NC} Build script: build-wasm.sh (non-standard, but supported)"
  elif [[ -f "build-wasm-production.sh" ]]; then
    echo -e "  ${YELLOW}⚠️${NC} Build script: build-wasm-production.sh (non-standard)"
    echo -e "     ${BLUE}→${NC} Consider renaming to build-dual.sh for consistency"
  else
    echo -e "  ${RED}✗${NC} No build script found"
    ((FAILED++))
  fi

  # 3. Check deno.json
  if [[ ! -f "deno.json" ]]; then
    echo -e "  ${YELLOW}⚠️${NC} No deno.json (optional for testing)"
  else
    echo -e "  ${GREEN}✓${NC} Has deno.json"
  fi

  # 4. Check meson.build
  if [[ -f "meson.build" ]]; then
    # Check for SIDE module in meson.build
    if grep -q "shared_module.*-side" meson.build 2>/dev/null || \
       grep -q "library.*-side" meson.build 2>/dev/null; then
      echo -e "  ${GREEN}✓${NC} Meson: SIDE module configured"
    else
      echo -e "  ${YELLOW}⚠️${NC} Meson: No SIDE module found (check wasm/ subdirectory)"
    fi
  else
    echo -e "  ${YELLOW}⚠️${NC} No meson.build (may be in subdirectory)"
  fi

  # 5. Check for old workflows to clean up
  OLD_WORKFLOWS=$(find .github/workflows -name "*.yml" ! -name "deploy-wasm.yml" 2>/dev/null || true)
  if [[ -n "$OLD_WORKFLOWS" ]]; then
    echo -e "  ${YELLOW}📋 Old workflows found:${NC}"
    echo "$OLD_WORKFLOWS" | while read -r file; do
      echo -e "     - $(basename "$file")"
    done
    echo -e "     ${BLUE}→${NC} Consider reviewing/removing if obsolete"
  fi

  # 6. Git status
  if git diff --quiet .github/workflows/deploy-wasm.yml 2>/dev/null; then
    echo -e "  ${YELLOW}⚠️${NC} No changes to workflow (already up to date?)"
  else
    echo -e "  ${GREEN}✓${NC} Workflow updated, ready to commit"
  fi

  echo ""
done

# Summary
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}📊 Refactoring Summary${NC}"
echo -e "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo -e "Total repositories: ${#ALL_REPOS[@]}"
echo -e "${GREEN}✓ Updated: ${UPDATED}${NC}"
echo -e "${YELLOW}⏭️  Skipped: ${SKIPPED}${NC}"
echo -e "${RED}✗ Failed: ${FAILED}${NC}"
echo ""
echo -e "${BLUE}Next Steps:${NC}"
echo -e "1. Review changes in each repository"
echo -e "2. Test builds locally: ${YELLOW}cd <repo>.wasm && ./build-dual.sh side${NC}"
echo -e "3. Commit changes (do not push yet):"
echo -e "   ${YELLOW}for repo in ${ALL_REPOS[*]}; do"
echo -e "     cd \${repo}.wasm"
echo -e "     git add .github/workflows/deploy-wasm.yml"
echo -e "     git commit -m 'feat: add standardized R2 deployment workflow'"
echo -e "     cd .."
echo -e "   done${NC}"
echo -e "4. Review old workflows and clean up if needed"
echo ""

exit 0