#!/usr/bin/env bash
# Build TraderX Field-Ledger in-game style assets on LINUX (no Windows tools).
#   field-ledger.html ──stylesjson──► styles_traderx.json
#                       ──generate_styles.py──► .styles + .imageset + .svg
#                       ──cairosvg──► .png ──png2edds.py──► .edds
#
# Deps (one-time):  python3 -m pip install pillow cairosvg lz4
# Run:              scripts/build-styles.sh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GUI="$ROOT/src/TraderX/datasets/gui"
STYLES_OUT="$ROOT/src/TraderX/datasets/styles"
TOOLS="$ROOT/../dayz-tools"
DZ="$TOOLS/dayz-layout-tools/dist/bin/dzml.js"
GEN="$TOOLS/styles_generator"
EDDSCONV="$TOOLS/edds-cli/eddsconv"   # game-accurate EDDS writer (woozymasta/edds)
NAME="traderx"
# Stable Enfusion asset GUID for the atlas (written into .edds.meta + imageset path).
# Keep this fixed so references don't break across rebuilds.
ATLAS_GUID="B62CAA2543B1EC63"
SRC="$GUI/_fixtures/field-ledger.html"

# Self-contained Python env (pillow/cairosvg/lz4) so it runs regardless of the
# system python. cairosvg needs the system cairo lib (apt: libcairo2).
VENV="$GEN/.venv"; PY="$VENV/bin/python"
if [ ! -x "$PY" ] || ! "$PY" -c "import cairosvg, PIL" 2>/dev/null; then
  echo "[0/6] bootstrapping python venv (pillow cairosvg lz4)…"
  python3 -m venv "$VENV"
  "$PY" -m pip install -q --upgrade pip pillow cairosvg lz4 || {
    echo "ERROR: pip install failed. If cairosvg complains about libcairo: sudo apt install libcairo2"; exit 1; }
fi

# One token source, two atlases: TraderX's, and TraderXBanking's own copy (Banking can run
# without TraderX). Banking's copy is renamed TX* -> TXB* (PREFIX): two loaded .styles files
# declaring the same style names broke every TX style in game (verified 2026-10-23), so each
# mod keeps its own style namespace.
build_set() {
NAME="$1"; ATLAS_GUID="$2"; STYLES_OUT="$3"; REL="$4"; CSS="$5"; PREFIX="${6:-}"
echo "[1/6] dz-styles tokens -> $NAME.json"
node "$DZ" stylesjson "$SRC" --name "$NAME" --texture "{GUID}${REL}" -o "$GEN/${NAME}.json" >/dev/null

echo "[2-3/6] generate .styles + .imageset + .svg + .png (@2x, coords scaled to match)"
# --png drives PNG generation (cairosvg inside generate_styles) AND scales imageset
# coordinates by the same factor, so the atlas pixels and 9-slice rects stay aligned.
"$PY" "$GEN/generate_styles.py" "$GEN/${NAME}.json" -o "$STYLES_OUT" --png --scale 2 --stroke-width 1 --css "$CSS" >/dev/null
[ -f "$STYLES_OUT/${NAME}_atlas.png" ] || { echo "ERROR: PNG not generated (cairosvg missing/failed)."; exit 1; }

echo "[4/6] png -> edds (game-accurate Go writer)"
"$EDDSCONV" conv "$STYLES_OUT/${NAME}_atlas.png" "$STYLES_OUT/${NAME}_atlas.edds"

echo "[5/6] assign Enfusion GUID: write .edds.meta + patch imageset path"
cat > "$STYLES_OUT/${NAME}_atlas.edds.meta" <<META
MetaFileClass {
 Name "{${ATLAS_GUID}}${REL}"
 Author "dmitri"
 ChangeDate 0
 Configurations {
  PNGResourceClass PC {
   SourceFile "${NAME}_atlas.png"
  }
  PNGResourceClass XBOX_ONE : PC {
  }
  PNGResourceClass PS4 : PC {
  }
  PNGResourceClass LINUX : PC {
  }
 }
}
META
# generate_styles.py emits a literal {GUID} placeholder — replace with the real GUID.
sed -i "s|{GUID}|{${ATLAS_GUID}}|g" "$STYLES_OUT/${NAME}_ui.imageset"
if [ -n "$PREFIX" ]; then
  echo "      namespace styles: TX* -> ${PREFIX}*"
  sed -i "s/Name=\"TX/Name=\"${PREFIX}/g" "$STYLES_OUT/${NAME}_styles.styles"
  sed -i "s/ImageSetGroupClass TX\([A-Za-z]*\)_/ImageSetGroupClass ${PREFIX}\1_/; s/Name \"TX\([A-Za-z]*\)_/Name \"${PREFIX}\1_/" "$STYLES_OUT/${NAME}_ui.imageset"
  sed -i "s/data-style=\"TX/data-style=\"${PREFIX}/g; s/data-style='TX/data-style='${PREFIX}/g" "$CSS"
fi

echo "[6/6] validate edds is game-readable"
"$EDDSCONV" check "$STYLES_OUT/${NAME}_atlas.edds"
grep -o "{${ATLAS_GUID}}${REL}" "$STYLES_OUT/${NAME}_ui.imageset" >/dev/null && echo "imageset path GUID OK"
ls -la "$STYLES_OUT/${NAME}_styles.styles" "$STYLES_OUT/${NAME}_ui.imageset" "$STYLES_OUT/${NAME}_atlas.edds"
}

build_set traderx "$ATLAS_GUID" "$STYLES_OUT" "TraderX/datasets/styles/traderx_atlas.edds" "$GUI/dzml-styles.css"
build_set banking "5F5BAA874D85E4D0" "$ROOT/src/TraderXBanking/datasets/styles" "TraderXBanking/datasets/styles/banking_atlas.edds" "$ROOT/src/TraderXBanking/datasets/layouts/dzml-styles.css" TXB
