#!/usr/bin/env bash
# Re-sign the built PBOs with BI's own DSSignFile — the authoritative signer for
# DayZ. Runs IN THE CONTAINER under plain Wine.
#
# WHY THIS EXISTS
#   ci/pack-linux.sh signs with armake2, which is Arma3-oriented. Its signatures
#   are rejected by DayZ, and neither armake2 nor HEMTT can validate a genuine BI
#   signature — so the breakage is invisible to tooling and only shows up as
#   servers refusing the mod. BI's DSSignFile is the only signer we can trust.
#
# WHY WINE AND NOT PROTON
#   DSSignFile.exe is a PE32+ x86-64 console app. It needs no Steam, no Proton,
#   no display, no registry setup — plain wine64 runs it. That is what lets this
#   step live in the same container as pack and publish instead of requiring a
#   host with Steam running.
#
# REQUIREMENTS
#   - wine64 (in ci/Dockerfile)
#   - DayZ Tools' Bin/DsUtils bind-mounted at /dayztools (or set DSUTILS_DIR).
#     NOT vendored: BI-licensed, must not ship inside a published image.
#
# Usage (from the repo root):
#   docker run --rm --user "$(id -u):$(id -g)" \
#     -v "$PWD:/repo" \
#     -v "$HOME/.steam/steam/steamapps/common/DayZ Tools/Bin/DsUtils:/dayztools:ro" \
#     -w /repo traderx-ci 'bash ci/sign-with-dayztools.sh'
set -euo pipefail

REPO_ROOT="${REPO_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
DSUTILS_DIR="${DSUTILS_DIR:-/dayztools}"
DSSIGN="$DSUTILS_DIR/DSSignFile.exe"

# Wine environment. The container runs as an arbitrary uid (--user), so the
# image's $HOME is not writable and /tmp itself is owned by root — wine refuses a
# HOME it does not own ("is not owned by you, refusing to create a configuration
# directory there"). Create our own, and give it an XDG_RUNTIME_DIR too or every
# call prints "XDG_RUNTIME_DIR is invalid or not set".
export HOME="${WINE_HOME:-/tmp/dzsign-home}"
export XDG_RUNTIME_DIR="$HOME/xdg"
export WINEPREFIX="${WINEPREFIX:-$HOME/.wine}"
export WINEDEBUG="${WINEDEBUG:--all}"
export WINELOADER="${WINELOADER:-/usr/lib/wine/wine64}"
mkdir -p "$HOME" "$XDG_RUNTIME_DIR"

# --- key: auto-detect, same rule as pack-linux.sh -----------------------------
if [[ -z "${PRIVATE_KEY:-}" ]]; then
  mapfile -t _priv < <(find "$REPO_ROOT/ci/keys" -maxdepth 1 -name '*.biprivatekey' 2>/dev/null | sort)
  case ${#_priv[@]} in
    1) PRIVATE_KEY="${_priv[0]}" ;;
    0) echo "FATAL: no *.biprivatekey in $REPO_ROOT/ci/keys (set PRIVATE_KEY=)" >&2; exit 1 ;;
    *) echo "FATAL: ${#_priv[@]} private keys — set PRIVATE_KEY= to pick one:" >&2
       printf '  %s\n' "${_priv[@]}" >&2; exit 1 ;;
  esac
fi

# --- mod dir: from build.yaml unless given ------------------------------------
if [[ -n "${1:-}" ]]; then
  MOD_DIR="$1"
else
  _mod=""
  command -v yq >/dev/null && _mod="$(yq -r '.mod_name // ""' "$REPO_ROOT/ci/build.yaml" 2>/dev/null || true)"
  [[ -n "$_mod" && "$_mod" != "null" ]] || _mod="@TraderXExperimental"
  MOD_DIR="$REPO_ROOT/target/$_mod"
fi
ADDONS="$MOD_DIR/addons"

[[ -x "$WINELOADER" ]] || { echo "FATAL: wine loader not found at $WINELOADER (rebuild the image)" >&2; exit 1; }
[[ -f "$DSSIGN" ]] || {
  echo "FATAL: DSSignFile.exe not found at $DSSIGN" >&2
  echo "       Bind-mount DayZ Tools' Bin/DsUtils there:" >&2
  echo "       -v \"\$HOME/.steam/steam/steamapps/common/DayZ Tools/Bin/DsUtils:/dayztools:ro\"" >&2
  exit 1; }
[[ -f "$PRIVATE_KEY" ]] || { echo "FATAL: missing $PRIVATE_KEY" >&2; exit 1; }
[[ -d "$ADDONS" ]] || { echo "FATAL: no addons dir at $ADDONS (build first)" >&2; exit 1; }

# The first call into a fresh prefix bootstraps it and is chatty. Nothing to do
# ahead of time - the noise is filtered per-call below.
[[ -d "$WINEPREFIX" ]] || echo "Initialising wine prefix at $WINEPREFIX ..."

n=0
for pbo in "$ADDONS"/*.pbo; do
  [[ -e "$pbo" ]] || continue
  rm -f "$pbo".*.bisign          # drop armake2's, DSSignFile's is the only one

  echo "==> signing $(basename "$pbo")"

  # Stage into a short, space-free dir and invoke with RELATIVE names. The repo
  # path contains a space ("TraderX Projects"); passing it through to a Windows
  # exe is a reliable source of argument-splitting bugs, and relative names in a
  # clean cwd sidestep the whole question.
  tmp="$(mktemp -d /tmp/dzsign.XXXXXX)"
  cp "$pbo" "$tmp/mod.pbo"
  cp "$PRIVATE_KEY" "$tmp/key.biprivatekey"

  ( cd "$tmp" && "$WINELOADER" "$DSSIGN" key.biprivatekey mod.pbo 2>&1 | grep -vi 'fixme\|rundll32\|created the configuration' ) || true

  # DSSignFile names the .bisign after the key's INTERNAL name, not the filename
  # we staged it under — signing "key.biprivatekey" still yields
  # "mod.pbo.C4BaseRaidbyDmitri.bisign". So glob for it rather than assuming.
  mapfile -t _sig < <(find "$tmp" -maxdepth 1 -name 'mod.pbo.*.bisign' 2>/dev/null)
  if [[ ${#_sig[@]} -ne 1 ]]; then
    echo "FATAL: expected 1 .bisign for $(basename "$pbo"), got ${#_sig[@]}" >&2
    ls -la "$tmp" >&2; rm -rf "$tmp"; exit 1
  fi
  # Keep DSSignFile's own naming — it encodes the key identity the server matches.
  signame="$(basename "${_sig[0]}")"          # mod.pbo.<keyname>.bisign
  mv "${_sig[0]}" "$(dirname "$pbo")/$(basename "$pbo").${signame#mod.pbo.}"
  rm -rf "$tmp"

  n=$((n+1))
done

[[ $n -gt 0 ]] || { echo "FATAL: no .pbo found in $ADDONS" >&2; exit 1; }

echo "----------------------------------------"
echo "Re-signed $n PBO(s) with DSSignFile (authoritative for DayZ)."
echo "Ship $MOD_DIR/keys/*.bikey to server owners."
