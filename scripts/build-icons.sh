#!/usr/bin/env bash
# Build TraderX icon assets on LINUX from the SVG source folder.
#   datasets/icons/svg/*.svg ──generate_icons.py──► traderx_icons.png + .imageset + dzml-icons.css
#                            ──eddsconv──► traderx_icons.edds (+ .edds.meta with stable GUID)
#
# The SVG folder is the single source of truth: same files render in the browser
# preview (via dzml-icons.css masks) and pack into the in-game imageset/atlas.
#
# Deps: python venv (pillow cairosvg) shared with build-styles.sh; eddsconv (Go).
# Run:  scripts/build-icons.sh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GUI="$ROOT/src/TraderX/datasets/gui"
ICONS_OUT="$ROOT/src/TraderX/datasets/icons"
SVG_DIR="$ICONS_OUT/svg"
TOOLS="$ROOT/../dayz-tools"
GEN="$TOOLS/styles_generator"
EDDSCONV="$TOOLS/edds-cli/eddsconv"
NAME="traderx_icons"
# Stable Enfusion asset GUID for the icon atlas (matches the existing imageset path
# so all `set:traderx_icons image:NAME` references keep resolving across rebuilds).
ICONS_GUID="0340C2EBFA081AA7"

VENV="$GEN/.venv"; PY="$VENV/bin/python"
if [ ! -x "$PY" ] || ! "$PY" -c "import cairosvg, PIL" 2>/dev/null; then
  echo "[0/4] bootstrapping python venv (pillow cairosvg)…"
  python3 -m venv "$VENV"
  "$PY" -m pip install -q --upgrade pip pillow cairosvg || {
    echo "ERROR: pip install failed. If cairosvg complains about libcairo: sudo apt install libcairo2"; exit 1; }
fi

echo "[1/4] svg -> sprite sheet png + imageset + dzml-icons.css"
"$PY" "$GEN/generate_icons.py" "$SVG_DIR" -o "$ICONS_OUT" \
  --cell 256 \
  --css "$GUI/dzml-icons.css" --name "$NAME" --guid "$ICONS_GUID"
[ -f "$ICONS_OUT/$NAME.png" ] || { echo "ERROR: sheet PNG not generated."; exit 1; }

echo "[2/4] png -> edds (game-accurate Go writer)"
"$EDDSCONV" conv "$ICONS_OUT/$NAME.png" "$ICONS_OUT/$NAME.edds"

echo "[3/4] write .edds.meta with stable GUID"
REL="TraderX/datasets/icons/${NAME}.edds"
cat > "$ICONS_OUT/${NAME}.edds.meta" <<META
MetaFileClass {
 Name "{${ICONS_GUID}}${REL}"
 Author "dmitri"
 ChangeDate 0
 Configurations {
  PNGResourceClass PC {
   SourceFile "${NAME}.png"
  }
  PNGResourceClass XBOX_ONE : PC {
  }
  PNGResourceClass PS4 : PC {
  }
 }
}
META

echo "[4/4] mirror dzml-icons.css to Banking + Garage preview dirs"
for d in Banking Garage; do
  [ -d "$GUI/$d" ] && cp "$GUI/dzml-icons.css" "$GUI/$d/dzml-icons.css"
done

echo "done. icons atlas + imageset + browser css regenerated from $SVG_DIR"
