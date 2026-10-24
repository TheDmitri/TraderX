#!/usr/bin/env bash
# Compile every DZML source (.html) of TraderX, TraderXBanking and TraderXGarage to
# its .layout (fonts resolve through each folder's dzml.config.yaml).
#   scripts/build-layouts.sh            all layouts
#   scripts/build-layouts.sh <file.html> ...
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DZ="$ROOT/../dayz-tools/dayz-layout-tools/dist/bin/dzml.js"
# NavBtnTooltip.layout has hand edits its .html does not carry: never recompile it.
SKIP="NavBtnTooltip.html"
if [ $# -gt 0 ]; then files=("$@"); else
  mapfile -t files < <(find "$ROOT/src/TraderX/datasets/gui" "$ROOT/src/TraderXBanking/datasets/layouts" "$ROOT/src/TraderXGarage/datasets/layouts" \
    -name "*.html" ! -name "*.bundle.html" ! -name "gallery.html" ! -path "*_fixtures*" | sort)
fi
n=0; out=()
for h in "${files[@]}"; do
  [ "$(basename "$h")" = "$SKIP" ] && continue
  node "$DZ" compile "$h" -o "${h%.html}.layout" >/dev/null
  out+=("${h%.html}.layout")
  n=$((n+1))
done
echo "compiled $n layouts"
# 1:1 px design -> width-relative layouts (the game scales px by screen height; see the script)
python3 "$ROOT/scripts/fluid-layouts.py" "${out[@]}"
