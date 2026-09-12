#!/usr/bin/env bash
# Publish the built @TraderX mod folder to the Steam Workshop via native Linux
# steamcmd. Reads creds from the environment (set locally on the runner box, or
# sourced from ~/.traderx-ci.env) + a per-run Guard code. Nothing is stored here.
set -euo pipefail

REPO_ROOT="${REPO_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
CONFIG="${CONFIG:-$REPO_ROOT/ci/build.yaml}"

# Same config as the build, so mod name / item id can't drift between them.
if [[ -f "$CONFIG" ]] && command -v yq >/dev/null; then
  MOD_NAME="${MOD_NAME:-$(yq -r '.mod_name // "@TraderX"' "$CONFIG")}"
  WORKSHOP_ITEM_ID="${WORKSHOP_ITEM_ID:-$(yq -r '.workshop_item_id // 0' "$CONFIG")}"
fi
MOD_NAME="${MOD_NAME:-@TraderX}"
CONTENT="${CONTENT:-$REPO_ROOT/target/$MOD_NAME}"
STEAMCMD_DIR="${STEAMCMD_DIR:-}"
DAYZ_APPID="${DAYZ_APPID:-221100}"

# --- change note ---------------------------------------------------------------
# Precedence: CHANGENOTE env  >  changenote_file (in build.yaml)  >  changenote
# (inline in build.yaml)  >  a dated fallback. Steam renders BBCode here
# ([b]bold[/b], [list][*]item[/list]), not Markdown.
if [[ -z "${CHANGENOTE:-}" ]] && [[ -f "$CONFIG" ]] && command -v yq >/dev/null; then
  _cf="$(yq -r '.changenote_file // ""' "$CONFIG")"
  # Absolute paths as-is; relative ones resolve against the repo root. Lets a
  # second mod keep its own note file outside this checkout.
  case "$_cf" in /*) _cfp="$_cf" ;; *) _cfp="$REPO_ROOT/$_cf" ;; esac
  if [[ -n "$_cf" && -f "$_cfp" ]]; then
    CHANGENOTE="$(cat "$_cfp")"
  else
    [[ -n "$_cf" ]] && echo "WARN: changenote_file '$_cf' not found — falling back." >&2
    CHANGENOTE="$(yq -r '.changenote // ""' "$CONFIG")"
  fi
fi
# Strip comment lines (so the note file can carry instructions) and trim blanks.
CHANGENOTE="$(printf '%s' "${CHANGENOTE:-}" | grep -v '^[[:space:]]*#' || true)"
CHANGENOTE="$(printf '%s' "$CHANGENOTE" | sed -e '/./,$!d' -e ':a' -e '/^\n*$/{$d;N;};/\n$/ba')"
[[ -n "$CHANGENOTE" ]] || CHANGENOTE="CI build $(date -u +%Y-%m-%d)"

: "${STEAM_USERNAME:?set STEAM_USERNAME}"
: "${WORKSHOP_ITEM_ID:?set WORKSHOP_ITEM_ID}"

# --- Steam Guard ---------------------------------------------------------------
# Three ways in, in order of preference:
#   1. CACHED SESSION  — after ONE successful interactive login, steamcmd stores a
#      refresh token under $HOME/Steam. Later runs need only the username, no
#      password and no Guard code, for weeks. Requires a PERSISTENT $HOME:
#         -v "$PWD/ci/.steam-session:/steamhome" -e HOME=/steamhome
#      Without that mount every `docker run --rm` is a cold login, which is why
#      the 30s code feels relentless.
#   2. STEAM_GUARD     — a one-shot code; needed for that first login (or after
#      the session expires). Nothing 2FA-related is stored.
#   3. STEAM_TFASEED   — TOTP shared secret, fully unattended. It is a PERMANENT
#      2FA bypass; keep it off any public repo (see ci/README.md).
guard=""
if [[ -n "${STEAM_GUARD:-}" ]]; then
  guard="$STEAM_GUARD"
elif [[ -n "${STEAM_TFASEED:-}" ]]; then
  command -v steamcmd-2fa >/dev/null || { echo "FATAL: steamcmd-2fa not found (use the ci/Dockerfile image)" >&2; exit 1; }
  guard="$(steamcmd-2fa "$STEAM_TFASEED")"
  [[ -n "$guard" ]] || { echo "FATAL: steamcmd-2fa produced no code — bad seed?" >&2; exit 1; }
fi

# Build the +login arguments. Omitting password/guard makes steamcmd fall back to
# the cached session; passing a stale guard code would fail, so only send one when
# we actually have it.
login_args=( "$STEAM_USERNAME" )
[[ -n "${STEAM_PASSWORD:-}" ]] && login_args+=( "$STEAM_PASSWORD" )
[[ -n "$guard" ]] && login_args+=( "$guard" )

if [[ ${#login_args[@]} -eq 1 ]]; then
  echo "Login: using cached session for $STEAM_USERNAME (no password/Guard supplied)."
  if [[ ! -d "$HOME/Steam" && ! -d "$HOME/.steam" ]]; then
    echo "WARN: no Steam session found under HOME=$HOME." >&2
    echo "      First run needs STEAM_PASSWORD + STEAM_GUARD, and a persistent" >&2
    echo "      HOME mount for the session to survive. See ci/README.md." >&2
  fi
fi

[[ -d "$CONTENT/addons" ]] || { echo "FATAL: no built mod at $CONTENT (run build-workshop.sh first)" >&2; exit 1; }
[[ -f "$CONTENT/meta.cpp" ]] || { echo "FATAL: $CONTENT/meta.cpp missing — Workshop items require it" >&2; exit 1; }
# meta.cpp's publishedid must agree with the id we upload to, or Steam updates
# one item while the shipped metadata points at another.
meta_id="$(grep -oP 'publishedid\s*=\s*\K[0-9]+' "$CONTENT/meta.cpp" || echo '')"
if [[ -n "$meta_id" && "$meta_id" != "0" && "$meta_id" != "$WORKSHOP_ITEM_ID" ]]; then
  echo "FATAL: meta.cpp publishedid ($meta_id) != WORKSHOP_ITEM_ID ($WORKSHOP_ITEM_ID)" >&2; exit 1
fi

# --- ensure steamcmd ---------------------------------------------------------
# Use the steamcmd baked into the image if present; else fetch one.
if [[ -z "$STEAMCMD_DIR" ]]; then
  if [[ -x /home/steam/steamcmd/steamcmd.sh ]]; then
    STEAMCMD_DIR=/home/steam/steamcmd
  else
    STEAMCMD_DIR="$HOME/steamcmd"
  fi
fi
STEAMCMD="$STEAMCMD_DIR/steamcmd.sh"
if [[ ! -x "$STEAMCMD" ]]; then
  echo "Installing steamcmd -> $STEAMCMD_DIR"
  mkdir -p "$STEAMCMD_DIR"
  curl -sSL https://steamcdn-a.akamaihd.net/client/installer/steamcmd_linux.tar.gz \
    | tar -xz -C "$STEAMCMD_DIR"
fi

# --- build the VDF -----------------------------------------------------------
VDF="$(mktemp --suffix=.vdf)"
trap 'rm -f "$VDF"' EXIT
# Steam does NOT interpret escape sequences in these fields - whatever bytes we
# send are what the Workshop page displays. Doubling backslashes printed literal
# "\\Data\\" on the page, and folding newlines into \n printed a literal "\n"
# between every line. So: keep real newlines and leave backslashes alone.
#
# A double quote CANNOT be escaped: this parser has no escape mode, so a `"` in
# the value terminates the VDF string early and corrupts the file
# ("KeyValues Error: got } in key"). It cannot be represented inside a "..."
# value at all, so we REPLACE it with a typographic quote (U+201D) that renders
# identically on the Workshop page and never breaks the parse.
vdf_str() { printf '%s' "$1" | sed -e 's/"/\xe2\x80\x9d/g'; }

note_esc="$(vdf_str "$CHANGENOTE")"
# A brand-new item (id 0) needs title/visibility; updates keep whatever the
# Workshop page already has, so we only send those fields when creating.
NEW_ITEM=0
extra=""
#
# UPDATE_METADATA=1 re-sends them for an EXISTING item too, which is how you fix
# a title or description without hand-editing the Workshop page. It overwrites
# whatever is there, including any edits made on the page itself - so it is
# opt-in rather than the default.
if [[ "$WORKSHOP_ITEM_ID" == "0" || "${UPDATE_METADATA:-0}" == "1" ]]; then
  [[ "$WORKSHOP_ITEM_ID" == "0" ]] && NEW_ITEM=1
  [[ "$NEW_ITEM" == "1" ]] || echo "UPDATE_METADATA=1 — re-sending title/description/visibility for item $WORKSHOP_ITEM_ID"
  title="$(yq -r '.title // ""' "$CONFIG" 2>/dev/null)"
  [[ -n "$title" && "$title" != "null" ]] || title="${MOD_NAME#@}"
  desc="$(yq -r '.description // ""' "$CONFIG" 2>/dev/null)"
  [[ "$desc" == "null" ]] && desc=""
  # Default to PRIVATE (2). A first upload should not go public by accident —
  # flip it on the Workshop page once you've checked it. 0=public 1=friends
  # 2=private 3=unlisted.
  vis="$(yq -r '.visibility // ""' "$CONFIG" 2>/dev/null)"
  [[ -n "$vis" && "$vis" != "null" ]] || vis=2
  preview="$(yq -r '.preview_image // ""' "$CONFIG" 2>/dev/null)"

  extra="  \"title\" \"$title\"
  \"visibility\" \"$vis\""
  [[ -n "$desc" ]] && extra="$extra
  \"description\" \"$(vdf_str "$desc")\""
  if [[ -n "$preview" && "$preview" != "null" ]]; then
    if [[ -f "$REPO_ROOT/$preview" ]]; then
      extra="$extra
  \"previewfile\" \"$REPO_ROOT/$preview\""
    else
      echo "WARN: preview_image '$preview' not found — skipping." >&2
    fi
  fi
fi

cat > "$VDF" <<EOF
"workshopitem"
{
  "appid" "$DAYZ_APPID"
  "publishedfileid" "$WORKSHOP_ITEM_ID"
  "contentfolder" "$CONTENT"
  "changenote" "$note_esc"
$extra
}
EOF

echo "--- change note ---"
printf '%s\n' "$CHANGENOTE"
echo "-------------------"

echo "Publishing $MOD_NAME (item $WORKSHOP_ITEM_ID) ..."
# steamcmd caches a refresh token after a successful login, so later runs can
# omit password/guard entirely (given a persistent HOME).
if [[ $NEW_ITEM -eq 1 ]]; then
  echo "Creating a NEW Workshop item (visibility=$vis, 2=private)."
fi

# DRY_RUN=1 shows the exact VDF and stops before contacting Steam.
if [[ -n "${DRY_RUN:-}" ]]; then
  echo "--- VDF (dry run, nothing uploaded) ---"
  cat "$VDF"
  echo "--------------------------------------"
  exit 0
fi

LOG="$(mktemp)"; trap 'rm -f "$VDF" "$LOG"' EXIT
set +e
"$STEAMCMD" +login "${login_args[@]}" \
  +workshop_build_item "$VDF" \
  +quit 2>&1 | tee "$LOG"
rc=${PIPESTATUS[0]}
set -e
[[ $rc -eq 0 ]] || { echo "FATAL: steamcmd exited $rc" >&2; exit $rc; }

if [[ $NEW_ITEM -eq 1 ]]; then
  # Recover the id Steam assigned; without persisting it, the NEXT publish would
  # create yet another duplicate item.
  new_id="$(grep -oiE 'published[ _]?file[ _]?id[^0-9]{0,10}([0-9]{6,})' "$LOG" \
            | grep -oE '[0-9]{6,}' | tail -1)"
  [[ -n "$new_id" ]] || new_id="$(grep -oE 'id[^0-9]{0,5}([0-9]{9,})' "$LOG" | grep -oE '[0-9]{9,}' | tail -1)"
  if [[ -n "$new_id" ]]; then
    echo ""
    echo "=================================================="
    echo " NEW Workshop item created: $new_id"
    echo "=================================================="
    if command -v yq >/dev/null && [[ -w "$CONFIG" ]]; then
      yq -i ".workshop_item_id = $new_id" "$CONFIG"
      echo "Wrote workshop_item_id: $new_id into $(basename "$CONFIG")."
    else
      echo "ACTION REQUIRED: set  workshop_item_id: $new_id  in $CONFIG"
    fi
    echo "Re-run the BUILD so meta.cpp carries the id, then publish again."
    echo "Item is currently visibility=$vis (2=private) — make it public on the"
    echo "Workshop page when you're ready."
    WORKSHOP_ITEM_ID="$new_id"
  else
    echo "WARN: could not detect the new item id in steamcmd output." >&2
    echo "      Find it on your Workshop page and set workshop_item_id manually," >&2
    echo "      or the next publish will create a duplicate item." >&2
  fi
fi
echo "Done. Verify at https://steamcommunity.com/sharedfiles/filedetails/?id=$WORKSHOP_ITEM_ID"
