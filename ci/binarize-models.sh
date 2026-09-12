#!/usr/bin/env bash
# ONE-TIME (and whenever a model actually changes): binarize the MLOD .p3d
# sources to ODOL in place, so the regular build needs no Windows/Wine at all.
#
# Why: there is NO Linux p3d binarizer (Mikero = Windows-only; HEMTT binarize =
# Windows-only; armake's P3D conversion is incomplete). BI's binarize.exe is the
# only option, and it runs here via Proton. Once the .p3d are ODOL, the whole
# build becomes Linux-native and containerisable.
#
# Only 3 packages contain models (TraderPlusMoney 36, Keypads 1, TraderPlusATM 1)
# and they change rarely — so this is a occasional manual step, not per-build.
#
# SAME HOST REQUIREMENTS as the old Proton build (see ci/README.md):
#   Steam running, DISPLAY set, DayZ NOT running, paths under $HOME.
#
# Usage:
#   ./ci/binarize-models.sh            # binarize any MLOD p3d found
#   ./ci/binarize-models.sh --check    # report MLOD/ODOL counts, change nothing
set -euo pipefail

REPO_ROOT="${REPO_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
STEAM_ROOT="${STEAM_ROOT:-$HOME/.steam/steam}"
DAYZ_APPID="${DAYZ_APPID:-221100}"
DAYZ_TOOLS="${DAYZ_TOOLS:-$STEAM_ROOT/steamapps/common/DayZ Tools}"
BINARIZE="$DAYZ_TOOLS/Bin/Binarize/binarize.exe"
SRC="$REPO_ROOT/src"
# MLOD originals are preserved here so models stay editable after binarizing.
MLOD_BACKUP="${MLOD_BACKUP:-$REPO_ROOT/models-src}"

magic() { head -c4 "$1" 2>/dev/null; }

# --- report mode -------------------------------------------------------------
mlod=(); odol=()
while IFS= read -r f; do
  case "$(magic "$f")" in
    MLOD) mlod+=("$f") ;;
    ODOL) odol+=("$f") ;;
  esac
done < <(find "$SRC" -iname '*.p3d')

echo "p3d status: ${#odol[@]} ODOL (binarized), ${#mlod[@]} MLOD (need binarize)"
if [[ "${1:-}" == "--check" ]]; then
  printf '  MLOD: %s\n' "${mlod[@]#$SRC/}" 2>/dev/null || true
  exit 0
fi
[[ ${#mlod[@]} -eq 0 ]] && { echo "Nothing to do — all p3d already ODOL."; exit 0; }

# --- preflight ---------------------------------------------------------------
[[ -f "$BINARIZE" ]] || { echo "FATAL: binarize.exe not found at $BINARIZE" >&2; exit 1; }
command -v protontricks-launch >/dev/null || { echo "FATAL: protontricks-launch not found" >&2; exit 1; }
[[ -n "${DISPLAY:-}" ]] || echo "WARN: DISPLAY unset — binarize may hang." >&2
pgrep -x steam >/dev/null 2>&1 || echo "WARN: Steam not running — binarize may hang." >&2
if pgrep -f 'DayZ_x64\|DayZDiag' >/dev/null 2>&1; then
  echo "FATAL: DayZ is running — it shares the $DAYZ_APPID prefix. Close it first." >&2; exit 1
fi

towin() { printf 'Z:%s' "$(printf '%s' "$1" | sed 's#/#\\#g')"; }

echo "Binarizing ${#mlod[@]} model(s); MLOD originals -> $MLOD_BACKUP"
for f in "${mlod[@]}"; do
  rel="${f#$SRC/}"
  bak="$MLOD_BACKUP/$rel"
  mkdir -p "$(dirname "$bak")"
  [[ -f "$bak" ]] || cp "$f" "$bak"      # keep the editable source once

  tmp="$(mktemp -d "$HOME/.cache/dzbin.XXXXXX")"   # under $HOME for pressure-vessel
  echo "==> $rel"
  # Stage the INPUT into the temp dir too, not just the output. Wine's
  # `start.exe /exec` splits its command line on whitespace, so any space in the
  # repo path (e.g. "TraderX Projects") reaches binarize.exe as two separate
  # arguments and it sits forever on a path that does not exist. mktemp -d gives
  # a space-free directory, so copying in and out sidesteps it entirely.
  cp "$f" "$tmp/in.p3d"

  protontricks-launch --appid "$DAYZ_APPID" "$BINARIZE" \
      "$(towin "$tmp/in.p3d")" "$(towin "$tmp/out.p3d")" || true

  if [[ -s "$tmp/out.p3d" && "$(magic "$tmp/out.p3d")" == "ODOL" ]]; then
    mv "$tmp/out.p3d" "$f"
    echo "    ok -> ODOL"
  else
    echo "    FAILED (still MLOD) — see DayZ Tools/Bin/Logs/" >&2
    rm -rf "$tmp"; exit 1
  fi
  rm -rf "$tmp"
done

echo "Done. Commit the ODOL .p3d (and $MLOD_BACKUP/ for the editable sources)."
