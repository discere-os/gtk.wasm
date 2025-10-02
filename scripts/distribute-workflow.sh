#!/bin/bash
# Distribute deployment workflow to all discere-os/*.wasm repositories
# Copyright 2025 Superstruct Ltd

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEMPLATE="${SCRIPT_DIR}/deploy-wasm-template.yml"
REPO_DIR="$(dirname "$SCRIPT_DIR")"

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

usage() {
  cat <<EOF
Usage: $0 [OPTIONS] [MODULES...]

Distribute deployment workflow to discere-os/*.wasm repositories.

Options:
  --dry-run         Show what would be done without making changes
  --force           Overwrite existing workflow files
  --commit          Commit changes after distribution
  --help            Show this help message

Arguments:
  MODULES           Specific modules to update (default: all)
                    Special values: "foundation", "gtk", "all"

Examples:
  $0 --dry-run                    # Preview distribution
  $0 foundation                   # Update 6 foundation modules only
  $0 zlib libpng pixman          # Update specific modules
  $0 --commit gtk                # Update GTK stack and commit
  $0 --force all                 # Force overwrite all 130 repos

Module Groups:
  foundation: zlib libpng pixman freetype harfbuzz expat (6 repos)
  gtk:        foundation + fontconfig cairo pango glib gdk-pixbuf atk gtk4 (13 repos)
  all:        All 130 repositories
EOF
  exit 1
}

# Parse arguments
DRY_RUN=false
FORCE=false
COMMIT=false
MODULES=()

while [[ $# -gt 0 ]]; do
  case $1 in
    --dry-run)
      DRY_RUN=true
      shift
      ;;
    --force)
      FORCE=true
      shift
      ;;
    --commit)
      COMMIT=true
      shift
      ;;
    --help)
      usage
      ;;
    *)
      MODULES+=("$1")
      shift
      ;;
  esac
done

# Expand module groups
EXPANDED_MODULES=()
for mod in "${MODULES[@]}"; do
  case "$mod" in
    foundation)
      EXPANDED_MODULES+=(zlib libpng pixman freetype harfbuzz expat)
      ;;
    gtk)
      EXPANDED_MODULES+=(zlib libpng pixman freetype harfbuzz expat fontconfig cairo pango glib gdk-pixbuf atk gtk4)
      ;;
    all)
      # Find all .wasm directories
      mapfile -t ALL_REPOS < <(find "$REPO_DIR/.." -maxdepth 1 -type d -name "*.wasm" -exec basename {} .wasm \;)
      EXPANDED_MODULES+=("${ALL_REPOS[@]}")
      ;;
    *)
      EXPANDED_MODULES+=("$mod")
      ;;
  esac
done

# Default to foundation if no modules specified
if [[ ${#EXPANDED_MODULES[@]} -eq 0 ]]; then
  EXPANDED_MODULES=(zlib libpng pixman freetype harfbuzz expat)
  echo -e "${YELLOW}No modules specified, defaulting to foundation stack${NC}"
fi

# Verify template exists
if [[ ! -f "$TEMPLATE" ]]; then
  echo -e "${RED}❌ Template not found: $TEMPLATE${NC}"
  exit 1
fi

echo -e "${BLUE}📋 Workflow Distribution Plan${NC}"
echo -e "Template: ${TEMPLATE}"
echo -e "Modules: ${#EXPANDED_MODULES[@]}"
echo -e "Dry run: ${DRY_RUN}"
echo -e "Force: ${FORCE}"
echo -e "Commit: ${COMMIT}"
echo ""

# Statistics
SUCCESS=0
SKIPPED=0
FAILED=0
CREATED=0
UPDATED=0

for module in "${EXPANDED_MODULES[@]}"; do
  REPO_PATH="${REPO_DIR}/../${module}.wasm"
  WORKFLOW_DIR="${REPO_PATH}/.github/workflows"
  WORKFLOW_FILE="${WORKFLOW_DIR}/deploy-wasm.yml"

  # Check if repo exists
  if [[ ! -d "$REPO_PATH" ]]; then
    echo -e "${RED}❌ ${module}: Repository not found${NC}"
    ((FAILED++))
    continue
  fi

  # Check if workflow already exists
  if [[ -f "$WORKFLOW_FILE" ]] && [[ "$FORCE" != true ]]; then
    echo -e "${YELLOW}⏭️  ${module}: Workflow exists (use --force to overwrite)${NC}"
    ((SKIPPED++))
    continue
  fi

  # Dry run mode
  if [[ "$DRY_RUN" == true ]]; then
    if [[ -f "$WORKFLOW_FILE" ]]; then
      echo -e "${BLUE}📝 ${module}: Would update existing workflow${NC}"
      ((UPDATED++))
    else
      echo -e "${GREEN}📄 ${module}: Would create new workflow${NC}"
      ((CREATED++))
    fi
    ((SUCCESS++))
    continue
  fi

  # Create .github/workflows directory if needed
  mkdir -p "$WORKFLOW_DIR"

  # Copy workflow template
  if cp "$TEMPLATE" "$WORKFLOW_FILE"; then
    if [[ -f "${WORKFLOW_FILE}.bak" ]]; then
      echo -e "${GREEN}✅ ${module}: Updated workflow (backup: deploy-wasm.yml.bak)${NC}"
      ((UPDATED++))
    else
      echo -e "${GREEN}✅ ${module}: Created workflow${NC}"
      ((CREATED++))
    fi
    ((SUCCESS++))

    # Commit if requested
    if [[ "$COMMIT" == true ]]; then
      cd "$REPO_PATH"
      if git add .github/workflows/deploy-wasm.yml; then
        if git commit -m "feat: add R2 deployment workflow

- Auto-detects package name from repository
- Builds with Deno + Meson
- Deploys with pnpm + wrangler
- Deploys to discere-os-wasm-production R2 bucket
- Supports versioned, SHA, and latest paths
- No manual configuration needed" 2>/dev/null; then
          echo -e "  ${BLUE}└─ Committed${NC}"
        else
          echo -e "  ${YELLOW}└─ No changes to commit${NC}"
        fi
      fi
      cd - > /dev/null
    fi
  else
    echo -e "${RED}❌ ${module}: Failed to copy workflow${NC}"
    ((FAILED++))
  fi
done

# Summary
echo ""
echo -e "${BLUE}📊 Distribution Summary${NC}"
echo -e "─────────────────────────────"
echo -e "Total modules: ${#EXPANDED_MODULES[@]}"
echo -e "${GREEN}✅ Success: ${SUCCESS}${NC}"
if [[ "$DRY_RUN" == true ]]; then
  echo -e "  ${GREEN}📄 Would create: ${CREATED}${NC}"
  echo -e "  ${BLUE}📝 Would update: ${UPDATED}${NC}"
else
  echo -e "  ${GREEN}📄 Created: ${CREATED}${NC}"
  echo -e "  ${BLUE}📝 Updated: ${UPDATED}${NC}"
fi
echo -e "${YELLOW}⏭️  Skipped: ${SKIPPED}${NC}"
echo -e "${RED}❌ Failed: ${FAILED}${NC}"
echo ""

if [[ "$DRY_RUN" == true ]]; then
  echo -e "${YELLOW}This was a dry run. Run without --dry-run to apply changes.${NC}"
elif [[ "$COMMIT" == false ]] && [[ $SUCCESS -gt 0 ]]; then
  echo -e "${BLUE}💡 Tip: Review changes, then commit with:${NC}"
  echo -e "   $0 --commit ${MODULES[*]}"
fi

exit 0