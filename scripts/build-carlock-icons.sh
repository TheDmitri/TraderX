#!/usr/bin/env bash
# Build TraderXCarLock's OWN icon atlas (independent of TraderX's traderx_icons).
#   src/TraderXCarLock/datasets/icons/svg/*.svg ──► traderxcarlock_icons.{png,imageset,edds,edds.meta}
#                                              ──► datasets/gui/dzml-icons.css (browser preview)
# Deps: styles_generator venv (pillow cairosvg) + eddsconv (Go), shared with build-icons.sh.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GUI="$ROOT/src/TraderXCarLock/datasets/gui"
ICONS_OUT="$ROOT/src/TraderXCarLock/datasets/icons"
SVG_DIR="$ICONS_OUT/svg"
TOOLS="$ROOT/../dayz-tools"
GEN="$TOOLS/styles_generator"
EDDSCONV="$TOOLS/edds-cli/eddsconv"
NAME="traderxcarlock_icons"
REL="TraderXCarLock/datasets/icons/${NAME}.edds"
# Stable Enfusion asset GUID for THIS atlas (distinct from traderx_icons 0340C2EBFA081AA7).
ICONS_GUID="0C1A2B3C4D5E6F70"

VENV="$GEN/.venv"; PY="$VENV/bin/python"
if [ ! -x "$PY" ] || ! "$PY" -c "import cairosvg, PIL" 2>/dev/null; then
  echo "[0/3] bootstrapping python venv (pillow cairosvg)…"
  python3 -m venv "$VENV"
  "$PY" -m pip install -q --upgrade pip pillow cairosvg
fi

echo "[1/3] svg -> sprite png + imageset + dzml-icons.css"
"$PY" "$GEN/generate_icons.py" "$SVG_DIR" -o "$ICONS_OUT" \
  --cell 256 \
  --css "$GUI/dzml-icons.css" --name "$NAME" --guid "$ICONS_GUID"
# generate_icons.py hardcodes the TraderX path prefix — repoint it to this mod.
sed -i "s#TraderX/datasets/icons/${NAME}.edds#${REL}#g" "$ICONS_OUT/${NAME}.imageset"

echo "[2/3] png -> edds"
"$EDDSCONV" conv "$ICONS_OUT/$NAME.png" "$ICONS_OUT/$NAME.edds"

echo "[3/3] write .edds.meta with stable GUID"
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

echo "done. $NAME atlas + imageset + browser css generated from $SVG_DIR"
