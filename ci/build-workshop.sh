#!/usr/bin/env bash
# Headless Linux build: pack + binarize + sign every folder under src/ into a PBO
# using DayZ Tools' AddonBuilder.exe, then assemble @TraderX/.
#
# AddonBuilder is a .NET app that only runs correctly under **Proton** in the
# **DayZ (221100) prefix** — that's where dotnet48/vcrun/gdiplus were installed
# (see ~/DayZWorkspace/dayz-tools-linux-setup.sh). We drive it via
# `protontricks-launch --appid 221100`, NOT plain wine.
#
# HARD REQUIREMENTS on the runner box (see ci/README.md):
#   * Steam client running (AddonBuilder inits the Steam API on startup).
#   * A usable X display (DISPLAY set) — Proton/pressure-vessel needs it.
#   * DayZ itself NOT running — the game and AddonBuilder share the 221100
#     prefix and will contend if launched together.
#   * All input/output paths under $HOME — pressure-vessel bind-mounts $HOME
#     but NOT arbitrary /tmp, so a /tmp output silently disappears.
#
# Secrets are NOT read here.
set -euo pipefail

# --- config (env-overridable) ------------------------------------------------
REPO_ROOT="${REPO_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
STEAM_ROOT="${STEAM_ROOT:-$HOME/.steam/steam}"
DAYZ_APPID="${DAYZ_APPID:-221100}"     # prefix that has .NET installed
DAYZ_TOOLS="${DAYZ_TOOLS:-$STEAM_ROOT/steamapps/common/DayZ Tools}"
PRIVATE_KEY="${PRIVATE_KEY:-$REPO_ROOT/ci/keys/TraderX.biprivatekey}"   # gitignored
BIKEY="${BIKEY:-$REPO_ROOT/ci/keys/TraderX.bikey}"                       # public, committed
MOD_NAME="${MOD_NAME:-@TraderX}"
BUILD_EXCLUDE="${BUILD_EXCLUDE:-DayZular}"   # space/comma list of src/ folders to skip
# Workshop item id baked into meta.cpp. 0 = Publisher/steamcmd creates a NEW item.
WORKSHOP_ITEM_ID="${WORKSHOP_ITEM_ID:-0}"
MOD_VERSION="${MOD_VERSION:-}"               # optional, informational only

ADDONBUILDER="$DAYZ_TOOLS/Bin/AddonBuilder/AddonBuilder.exe"

# --- preflight ---------------------------------------------------------------
for p in "$ADDONBUILDER" "$PRIVATE_KEY" "$BIKEY"; do
  [[ -e "$p" ]] || { echo "FATAL: missing required path: $p" >&2; exit 1; }
done
command -v protontricks-launch >/dev/null || { echo "FATAL: protontricks-launch not found (install protontricks)" >&2; exit 1; }
[[ -n "${DISPLAY:-}" ]] || echo "WARN: DISPLAY unset — AddonBuilder may hang. Run with a display or xvfb-run." >&2
pgrep -x steam >/dev/null 2>&1 || echo "WARN: Steam client not detected — AddonBuilder inits Steam API and may hang." >&2

SRC="$REPO_ROOT/src"
OUT="$REPO_ROOT/target/$MOD_NAME"          # under $HOME -> visible inside pressure-vessel
# Lowercase addons/ + keys/ — matches the layout that actually published (and
# case-sensitive Linux servers care).
ADDON_OUT="$OUT/addons"
KEY_OUT="$OUT/keys"
case "$OUT" in "$HOME"/*) : ;; *) echo "FATAL: output ($OUT) must be under \$HOME for pressure-vessel" >&2; exit 1;; esac
rm -rf "$OUT"
mkdir -p "$ADDON_OUT" "$KEY_OUT"
cp "$BIKEY" "$KEY_OUT/"

# Linux path -> Wine path (Z:\...). The 221100 prefix maps z: -> /.
towin() { printf 'Z:%s' "$(printf '%s' "$1" | sed 's#/#\\#g')"; }

excl="$(echo "$BUILD_EXCLUDE" | tr ',' ' ')"
is_excluded() { for e in $excl; do [[ "$1" == "$e" ]] && return 0; done; return 1; }

proj_w="$(towin "$SRC")"
key_w="$(towin "$PRIVATE_KEY")"
out_w="$(towin "$ADDON_OUT")"

built=0
for dir in "$SRC"/*/; do
  name="$(basename "$dir")"
  if is_excluded "$name"; then echo "-- skip $name (excluded)"; continue; fi
  echo "==> packing $name"
  # AddonBuilder <sourceDir> <destDir> -project=<root> -sign=<privkey> -clear
  #   -project fixes the $prefix class root; -sign binarizes AND signs in one pass.
  protontricks-launch --appid "$DAYZ_APPID" "$ADDONBUILDER" \
      "$(towin "${dir%/}")" "$out_w" \
      "-project=$proj_w" \
      "-sign=$key_w" \
      -clear
  # AddonBuilder's exit code is unreliable through Proton; verify the PBO landed.
  [[ -f "$ADDON_OUT/$name.pbo" ]] || { echo "FATAL: $name.pbo not produced (see DayZ Tools/Bin/Logs/AddonBuilder.rpt)" >&2; exit 1; }
  built=$((built+1))
done

# --- meta.cpp -----------------------------------------------------------------
# REQUIRED for a Workshop mod: identifies the item. Mirrors the layout of the
# folder that was published manually (protocol/publishedid/name/timestamp).
# timestamp: Swarm documents .NET DateTime.UtcNow.Ticks, and that's what we emit.
# NOTE: values written by DayZ Tools Publisher on this box are ~8x larger
# (e.g. 5250877833240181827 vs ticks ~6.4e17), so Publisher uses a different
# scheme. `publishedid` is the field that actually identifies the item; the
# timestamp appears to be informational. If the Workshop ever rejects/misorders
# an upload, copy the timestamp scheme from a Publisher-generated meta.cpp.
mod_bare="${MOD_NAME#@}"
ticks=$(( ( $(date -u +%s) + 62135596800 ) * 10000000 ))
cat > "$OUT/meta.cpp" <<EOF
protocol = 1;
publishedid = $WORKSHOP_ITEM_ID;
name = "$mod_bare";
timestamp = $ticks;
EOF
[[ -n "$MOD_VERSION" ]] && echo "  version: $MOD_VERSION"

echo "----------------------------------------"
echo "Built $built PBO(s) -> $OUT"
echo "meta.cpp: publishedid=$WORKSHOP_ITEM_ID name=$mod_bare"
ls -1 "$ADDON_OUT"/*.pbo | sed 's/^/  /'
echo "Signatures:"
ls -1 "$ADDON_OUT"/*.bisign 2>/dev/null | sed 's/^/  /' || echo "  (none — check -sign / key)"
