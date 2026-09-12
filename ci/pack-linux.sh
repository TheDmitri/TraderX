#!/usr/bin/env bash
# Pure-Linux PBO build: stage -> rapify+pack (armake2) -> sign -> assemble @TraderX.
# Runs inside the ci/Dockerfile image (or natively if armake2 + rsync + yq are on PATH).
# No Wine, no DayZ Tools — requires the .p3d to be pre-binarized (ci/binarize-models.sh).
#
# Config: ci/build.yaml (env vars override). Packages listed under `skip:` are
# NOT rebuilt and their prebuilt .pbo in the output are left untouched, so you
# can hand-place release-ready PBOs there and only rebuild what changed.
#
# Usage:
#   pack-linux.sh              # incremental: replace built packages only
#   pack-linux.sh --clean      # wipe the output folder first
set -euo pipefail

CLEAN=0
[[ "${1:-}" == "--clean" ]] && CLEAN=1

REPO_ROOT="${REPO_ROOT:-$(pwd)}"
CONFIG="${CONFIG:-$REPO_ROOT/ci/build.yaml}"
# src_dir lets one checkout build several mods (or a mod whose sources live
# somewhere other than ./src). Absolute paths are used as-is.
SRC_DIR="${SRC_DIR:-}"
if [[ -z "$SRC_DIR" && -f "$CONFIG" ]] && command -v yq >/dev/null; then
  SRC_DIR="$(yq -r '.src_dir // "src"' "$CONFIG")"
fi
SRC_DIR="${SRC_DIR:-src}"
case "$SRC_DIR" in /*) SRC="$SRC_DIR" ;; *) SRC="$REPO_ROOT/$SRC_DIR" ;; esac
# Keys: use PRIVATE_KEY/BIKEY if given, else auto-detect a single pair in ci/keys
# (the key can be named anything — the .bisign is named after it either way).
KEYDIR="${KEYDIR:-$REPO_ROOT/ci/keys}"
if [[ -z "${PRIVATE_KEY:-}" ]]; then
  mapfile -t _priv < <(find "$KEYDIR" -maxdepth 1 -name '*.biprivatekey' 2>/dev/null | sort)
  case ${#_priv[@]} in
    1) PRIVATE_KEY="${_priv[0]}" ;;
    0) echo "FATAL: no *.biprivatekey in $KEYDIR (set PRIVATE_KEY=)" >&2; exit 1 ;;
    *) echo "FATAL: ${#_priv[@]} private keys in $KEYDIR — set PRIVATE_KEY= to pick one:" >&2
       printf '  %s\n' "${_priv[@]}" >&2; exit 1 ;;
  esac
fi
if [[ -z "${BIKEY:-}" ]]; then
  BIKEY="${PRIVATE_KEY%.biprivatekey}.bikey"     # matching public key
fi

# --- packer selection --------------------------------------------------------
# Preferred: Mikero's depbo-tools `makepbo` — DayZ-aware (-D), and it generates
# texHeaders.bin like AddonBuilder does. armake2 does NOT emit texHeaders.bin.
# Fallback: armake2 (Arma3-oriented) if depbo-tools isn't available.
DEPBO_ROOT="${DEPBO_ROOT:-/opt/depbo-tools}"
if [[ -x "$DEPBO_ROOT/bin/makepbo" ]]; then
  export LD_LIBRARY_PATH="$DEPBO_ROOT/lib:${LD_LIBRARY_PATH:-}"
  export PATH="$DEPBO_ROOT/bin:$PATH"
fi
# PACKER=armake2 forces the fallback. makepbo is preferred: it emits
# texHeaders.bin and, with the $PBOPREFIX$ written below, the correct DayZ
# header extensions too.
if [[ -n "${PACKER:-}" ]]; then
  :
elif command -v makepbo >/dev/null; then PACKER=makepbo; else PACKER=armake2; fi

for t in armake2 rsync yq; do command -v "$t" >/dev/null || { echo "FATAL: $t not found" >&2; exit 1; }; done
echo "Packer: $PACKER    (signing: armake2)"
[[ -f "$CONFIG" ]] || { echo "FATAL: config not found: $CONFIG" >&2; exit 1; }
[[ -d "$SRC" ]] || { echo "FATAL: source dir not found: $SRC (set src_dir in $(basename "$CONFIG"))" >&2; exit 1; }
for p in "$PRIVATE_KEY" "$BIKEY"; do [[ -f "$p" ]] || { echo "FATAL: missing $p" >&2; exit 1; }; done

# --- config (env wins over yaml) ---------------------------------------------
MOD_NAME="${MOD_NAME:-$(yq -r '.mod_name // "@TraderX"' "$CONFIG")}"
WORKSHOP_ITEM_ID="${WORKSHOP_ITEM_ID:-$(yq -r '.workshop_item_id // 0' "$CONFIG")}"

# Two distinct kinds of "don't build this":
#   skip:    keep a hand-placed prebuilt .pbo in the output; warn if it's absent.
#   exclude: never part of this mod at all (external deps, third-party, separately
#            published packages) — absence is correct, so no warning.
mapfile -t SKIP < <(yq -r '.skip[]' "$CONFIG")
# BUILD_SKIP env (comma/space list) replaces the yaml list entirely.
if [[ -n "${BUILD_SKIP:-}" ]]; then
  mapfile -t SKIP < <(echo "$BUILD_SKIP" | tr ',' '\n' | tr ' ' '\n' | sed '/^$/d')
fi
mapfile -t EXCLUDE_PKGS < <(yq -r '.exclude_packages[]' "$CONFIG")
mapfile -t EXCLUDES < <(yq -r '.exclude_files[]' "$CONFIG")
[[ ${#EXCLUDES[@]} -eq 0 ]] && EXCLUDES=('*.html' '*.css' '*.js' '*.bisign' '*.bikey' '*.biprivatekey')

OUT="$REPO_ROOT/target/$MOD_NAME"
ADDON_OUT="$OUT/addons"
KEY_OUT="$OUT/keys"
STAGE="${STAGE:-$REPO_ROOT/target/.stage}"

[[ $CLEAN -eq 1 ]] && { echo "--clean: wiping $OUT"; rm -rf "$OUT"; }
rm -rf "$STAGE"
mkdir -p "$ADDON_OUT" "$KEY_OUT" "$STAGE"
cp "$BIKEY" "$KEY_OUT/"

rsync_args=()
for e in "${EXCLUDES[@]}"; do rsync_args+=( --exclude="$e" ); done

is_skipped() { local n="$1"; for s in "${SKIP[@]}"; do [[ "$n" == "$s" ]] && return 0; done; return 1; }
is_excluded_pkg() { local n="$1"; for s in "${EXCLUDE_PKGS[@]}"; do [[ "$n" == "$s" ]] && return 0; done; return 1; }

# Guard: a package that is a symlink pointing OUTSIDE the bind-mount resolves to
# nothing inside the container, so `$SRC/*/` silently drops it and the mod ships
# incomplete with no error. (src/DayZular is exactly this.) Catch it explicitly:
# tolerable when the package is excluded/skipped, fatal otherwise.
for entry in "$SRC"/*; do
  [[ -e "$entry" || -L "$entry" ]] || continue
  ename="$(basename "$entry")"
  [[ -f "$entry" ]] && continue                     # stray files (e.g. .zip)
  if [[ ! -d "$entry" ]]; then
    if is_excluded_pkg "$ename" || is_skipped "$ename"; then
      echo "-- note: '$ename' is an unresolvable symlink here (excluded anyway)"
    else
      echo "FATAL: '$ename' is a symlink that does not resolve inside this" >&2
      echo "       environment ($(readlink "$entry" 2>/dev/null))." >&2
      echo "       It would be silently omitted from the mod. Either mount its" >&2
      echo "       target into the container, or list it under exclude_packages:." >&2
      exit 1
    fi
  fi
done

built=0; preserved=(); missing=(); excluded=()
for dir in "$SRC"/*/; do
  name="$(basename "$dir")"

  if is_excluded_pkg "$name"; then
    echo "-- exclude $name (not part of this mod)"
    excluded+=("$name")
    continue
  fi

  if is_skipped "$name"; then
    if compgen -G "$ADDON_OUT/$name.pbo" >/dev/null; then
      echo "-- skip $name (kept prebuilt $(basename "$ADDON_OUT/$name.pbo"))"
      preserved+=("$name")
    else
      echo "-- skip $name (NO prebuilt pbo in output)"
      missing+=("$name")
    fi
    continue
  fi

  echo "==> $name"

  # Refuse to ship un-binarized models — they'd load as broken in-game.
  mlod=0
  while IFS= read -r f; do [[ "$(head -c4 "$f")" == "MLOD" ]] && mlod=$((mlod+1)); done \
    < <(find "$dir" -iname '*.p3d')
  if [[ $mlod -gt 0 ]]; then
    echo "FATAL: $name has $mlod un-binarized (MLOD) .p3d." >&2
    echo "       Run ci/binarize-models.sh, or add '$name' to skip: in $(basename "$CONFIG")." >&2
    exit 1
  fi

  rsync -a "${rsync_args[@]}" "$dir" "$STAGE/$name/"

  # Drop this package's previous artifacts only (never touch other packages').
  rm -f "$ADDON_OUT/$name.pbo" "$ADDON_OUT/$name.pbo."*.bisign

  if [[ "$PACKER" == "makepbo" ]]; then
    # Rapify the config OURSELVES first, then pack with -B (don't binarise).
    #
    # Why: makepbo runs an Arma-centric dependency check while binarising
    # config.cpp. DayZ references sounds without an extension
    # ("TraderX\datasets\sounds\coins" -> coins.ogg) but makepbo looks for
    # Arma's .wss and aborts with "DePbo:missing file(s)" even though the .ogg
    # files are present. -B alone does NOT skip the check — the config has to be
    # gone by then. armake2's rapify does no dependency resolution, so this
    # produces the same config.bin without the false positive.
    if [[ -f "$STAGE/$name/config.cpp" ]]; then
      armake2 rapify -f "$STAGE/$name/config.cpp" "$STAGE/$name/config.bin"
      rm -f "$STAGE/$name/config.cpp"
    fi
    # Same story for .rvmat: makepbo compiles them and runs the same missing-file
    # check, which aborts on any texture the material references from OUTSIDE the
    # mod. A material pointing at a vanilla texture ("dz\data\data\env_land_co.paa")
    # is perfectly valid — that file lives in the game, not in our PBO — but the
    # check cannot see it and fails the build. Rapify them ourselves so makepbo
    # has nothing left to resolve.
    while IFS= read -r -d '' rv; do
      armake2 rapify -f "$rv" "$rv.bin" && mv -f "$rv.bin" "$rv"
    done < <(find "$STAGE/$name" -type f -iname '*.rvmat' -print0)
    # makepbo has no command-line flag for PBO header extensions, but it DOES
    # read them from a $PBOPREFIX$ in the source folder. Without this the output
    # lacks "product=dayz ugc" and carries makepbo's own datestamp as "version"
    # — DayZ uses both to recognise a PBO as a DayZ mod addon, and AddonBuilder /
    # FileBank write product=dayz ugc + version=2. The file is consumed by the
    # packer, not packed into the archive.
    printf 'prefix=%s\nproduct=dayz ugc\nversion=2\n' "$name" > "$STAGE/$name/\$PBOPREFIX\$"

    # -P no pause, -D DayZ mode, -B no binarise, -@= prefix. Sources are
    # pre-filtered by rsync, so makepbo's own -X list isn't needed.
    # makepbo also emits texHeaders.bin, which armake2 does not.
    makepbo -P -D -B "-@=$name" "$STAGE/$name" "$ADDON_OUT/$name.pbo"
  else
    # -e header extensions mirror what DayZ Tools' FileBank writes.
    armake2 build -f \
      -e "product=dayz ugc" -e "version=2" -e "prefix=$name" \
      "$STAGE/$name" "$ADDON_OUT/$name.pbo"
  fi
  # `build -k` does not emit the .bisign — sign as a separate step (v3 default).
  armake2 sign -f "$PRIVATE_KEY" "$ADDON_OUT/$name.pbo"

  [[ -s "$ADDON_OUT/$name.pbo" ]] || { echo "FATAL: $name.pbo empty" >&2; exit 1; }
  built=$((built+1))
done
rm -rf "$STAGE"

# meta.cpp identifies the Workshop item — required.
mod_bare="${MOD_NAME#@}"
ticks=$(( ( $(date -u +%s) + 62135596800 ) * 10000000 ))
cat > "$OUT/meta.cpp" <<EOF
protocol = 1;
publishedid = $WORKSHOP_ITEM_ID;
name = "$mod_bare";
timestamp = $ticks;
EOF

echo "----------------------------------------"
echo "Rebuilt : $built package(s)"
[[ ${#preserved[@]} -gt 0 ]] && echo "Preserved: ${preserved[*]}"
[[ ${#excluded[@]} -gt 0 ]] && echo "Excluded : ${excluded[*]} (dependencies / not shipped)"
echo "Output  : $OUT  (publishedid=$WORKSHOP_ITEM_ID)"
ls -1 "$ADDON_OUT"/*.pbo 2>/dev/null | sed 's#.*/#  #'

if [[ ${#missing[@]} -gt 0 ]]; then
  echo ""
  echo "WARNING: skipped but no .pbo in output: ${missing[*]}" >&2
  echo "         The mod will ship incomplete. Place their prebuilt .pbo in" >&2
  echo "         $ADDON_OUT/ or remove them from skip:." >&2
fi

# Self-check: a .bisign must EXIST for every PBO. Its cryptographic validity is
# deliberately NOT treated as pass/fail here:
#   armake2 and HEMTT are Arma3-oriented and both reject DayZ's own
#   AddonBuilder/DSSignFile-produced signatures ("Hash 3 doesn't match" /
#   "Signature does not match"), so their verdict is not authoritative for DayZ.
# The only real test is a DayZ server with verifySignatures=2. See ci/README.md.
fail=0
for pbo in "$ADDON_OUT"/*.pbo; do
  [[ -e "$pbo" ]] || continue
  if ! compgen -G "$pbo.*.bisign" >/dev/null; then
    echo "  !! MISSING signature: $(basename "$pbo")" >&2; fail=1
  fi
done
if [[ $fail -ne 0 ]]; then
  echo "FATAL: some PBOs have no signature." >&2; exit 1
fi
echo "All PBOs have a .bisign."
echo "NOTE: signature validity is not machine-checkable on Linux (Arma3 tools"
echo "      disagree with DayZ's format). If servers run verifySignatures=2,"
echo "      re-sign with BI's own tool: ci/sign-with-dayztools.sh"
