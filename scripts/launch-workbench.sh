#!/usr/bin/env bash
# Launch DayZ Tools Workbench (Proton) and open the TraderX project.
# Routes through the Steam Linux Runtime (pressure-vessel) — required for Proton
# Experimental, otherwise fontconfig/locale libs break ("out of memory" errors).
#
# The .gproj is copied to the P: drive ROOT as a REAL file (not opened through the
# P:\TraderX symlink) — Wine canonicalizes symlinks off the workdrive, which makes
# Workbench report "can't find game project file". Addon paths inside the gproj are
# workdrive-relative (TraderX/, DayZular/, TraderX_GUI/) and resolve via the symlinks
# that live in the P: root.
set -euo pipefail

STEAM="$HOME/.steam/debian-installation"
RUNTIME="$STEAM/steamapps/common/SteamLinuxRuntime_4/_v2-entry-point"
PROTON="$STEAM/steamapps/common/Proton - Experimental/proton"
WORKBENCH="$STEAM/steamapps/common/DayZ Tools/Bin/Workbench/workbenchApp.exe"

PDRIVE="$STEAM/steamapps/compatdata/221100/pfx/drive_c/PDrive"
SRC_GPROJ="$HOME/DayZWorkspace/TraderX Projects/TraderX/src/TraderX/Workbench/dayz.gproj"

# Refresh real gproj at P: root so edits in src are picked up.
cp -f "$SRC_GPROJ" "$PDRIVE/dayz.gproj"
GPROJ='P:\dayz.gproj'

# Kill any stale tools session — only one wineserver per prefix.
pkill -9 -f 'DayZToolsLauncher' 2>/dev/null || true
pkill -9 -f 'workbenchApp'      2>/dev/null || true

export STEAM_COMPAT_DATA_PATH="$STEAM/steamapps/compatdata/830640"
export STEAM_COMPAT_CLIENT_INSTALL_PATH="$STEAM"
export STEAM_COMPAT_APP_ID=830640
export SteamAppId=830640

exec "$RUNTIME" --verb=waitforexitandrun -- \
	"$PROTON" waitforexitandrun "$WORKBENCH" "$GPROJ"
