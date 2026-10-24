# TraderX release handoff (2026-10-24)

Goal of the next session: ship the new TraderX quickly, without blockers.
Read this file first, then `ci/README.md` and `ci/CHANGENOTE.md`.

## Rules (from the owner)

- Answer the owner in French. The owner is TheDmitri.
- **Nothing is committed and nothing is published.** Branch `migration-dayzular`; the last commit is `ded6d7a`, with about 534 dirty or untracked files on top. The owner said "work on top of the uncommitted state". Ask before committing.
- **Never publish to the Workshop without the owner's explicit go-ahead.**
- Never `pkill -f` a pattern that appears in your own command line (it kills your shell). Build the pattern in pieces, e.g. `n="enf""Main"; pkill -x $n`.

## Workshop items

| Mod | Item | Note |
|---|---|---|
| DayZular | 3686232711 | ~8.5k subs, must be published FIRST |
| TraderX experimental | 3760962576 | `ci/build.yaml` (default target) |
| TraderX stable | 3560310973 | ~22k subs, `ci/build.stable.yaml`; `publish-workshop.sh` refuses it without `CONFIRM_STABLE=1` |
| TraderPlus (owner's other mod) | 2458896948 | not involved |

**Decision for the owner:** publish to experimental (3760962576) first, or straight to stable?
The stable→current migration was tested and passed (see "What is verified").

## Release order (mandatory)

TraderX ships its own copy of DayZular (`src/TraderXDayZular`, every file wrapped in `#ifndef DAYZULAR`), so admins need only CF + TraderX. The standalone DayZular must declare `defines[]={"DAYZULAR"}`. A server with the OLD DayZular (without the define) plus the new TraderX fails to compile with "Multiple declaration of class" (verified in game on 2026-10-23).

1. **DayZular**
   - Repo `../DayZular`. Three uncommitted files:
     - `config.cpp` (the `DAYZULAR` define)
     - `PropertyBindingImpl.c`
     - `WidgetBindings.c` (button text binding)
   - Commit and push after the owner's OK.
2. **Build DayZular.** A previous build exists at `TraderX/target/@DayZularDefine`.
   - ⚠️ **BLOCKER:** its `meta.cpp` says `publishedid = 3760962576`, which is the TraderX experimental id. It must be **3686232711**. Rebuild with `WORKSHOP_ITEM_ID=3686232711 MOD_NAME=@DayZular` (see `ci/README.md`, env overrides), or fix `meta.cpp`, and check before uploading. Otherwise DayZular overwrites the TraderX item.
3. **Publish DayZular** (owner's go-ahead). Wait until the Workshop has it.
4. **Regenerate the bundled copy:**
   ```
   python3 TraderX/ci/bundle-dayzular.py ../DayZular/src/DayZular
   ```
   - It was regenerated on 2026-10-24 from the current DayZular working tree.
   - Rerun it if DayZular changes before its publish. The copy must equal the PUBLISHED DayZular.
5. **Build TraderX:**
   ```
   cd TraderX && docker run --rm --user "$(id -u):$(id -g)" -v "$PWD:/repo" -v ~/depbo-tools-0.10.13:/opt/depbo-tools:ro -w /repo traderx-ci 'bash ci/pack-linux.sh'
   ```
   - Output goes to `target/@TraderXExperimental` (9 PBOs, 106 MB).
   - The build does NOT compile scripts. Only a server boot catches script errors.
6. **Test boots** (see "Smoke checks"), then publish TraderX with `ci/publish-workshop.sh`. The change note is `ci/CHANGENOTE.md` (BBCode, already up to date).

## Blockers / to check before release

### Must fix or decide
- [ ] **DayZular meta.cpp id** (above).
- [ ] **Signatures:** armake2 signatures were never proven valid. Test on a server with `verifySignatures=2`, or re-sign with `ci/sign-with-dayztools.sh`. Otherwise players get kicked on strict servers.
- [ ] **Garage default config spawns an NPC (id 125) at Chernarus coordinates** (Green Mountain) on EVERY updated server, including non-Chernarus maps. It should be off by default, or only created on fresh installs.
- [ ] **SafeZone placeholder:** `src/TraderXSafeZone/scripts/3_Game/SafeZoneSettings.c:52` inserts a default zone named "Test" at 0,0,0 r=10. Check it doesn't ship as a live zone.
- [ ] **In-game UI layout bugs** seen in `release-screens/in-game/` (2026-10-23, Purchase/Favorites):
  - cart footer: "TOTAL" overlaps "Cash after", and the big "€ 0" overlaps "€ 3,500";
  - item tiles: price overlaps "sells for €…", and long prices are clipped ("€ 1,0").

  These may predate the width-drift fix (`scripts/fluid-layouts.py`). Re-screenshot in game at 16:10 and 16:9 and fix whatever remains.
- [ ] **No in-game client test yet** for:
  - receipts (pack with the wrench, deploy with the receipt in hands, receipt name "Reçu de véhicule : …", notifications);
  - 4-digit CarLock code entry;
  - the look of the TraderPlus credit card.

  Earlier in-game joins also failed with "Login timed out (WaitAuthPlayerLoginState)". `DayZServer/steamclient.so` was replaced by the `~/.steam/sdk64` copy (backup `steamclient.so.bak-2026-05-08`).

### Should check (not blocking)
- [ ] Vehicle fuel reads 0 after a reload in every setup, including stable. Probably a test artifact; unverified.
- [ ] The default currency file uses Dollar. The owner wanted EUR?
- [ ] The stable `CONFIGURATION_GUIDE.md` is outdated: CSV is now the default for new servers, plus the receipts settings.
- [ ] Unused fonts in the PBO (size).
- [ ] The `TraderXQAAutoOpen.c` QA hook must not ship. It is NOT in `src/` (checked 2026-10-24); keep it that way.
- [ ] Owner wishes queued earlier: auto-expand favorites on the dashboard, 3D preview background = layout colour, add "fatfly" to the credits.

## What is verified (server-side, 1.30)

- **Stable TraderX (Workshop) → current:** 1008 products, 51 categories, 6 presets identical. Cars keep txId and attachments. Works with or without Dabs. Rollback to stable OK. Switching to CSV is identical.
- **TraderPlus → TraderX:**
  - config: independent verifier 6439 OK / 2 DIFF (a TraderPlus lowercase-class bug) / 4 with no TraderX equivalent;
  - TP vehicles and CarLock locks: 0 "Scripted variables corrupted";
  - TP cash (`TraderPlus_Money_*`), bank cards, wrench and lockpick are kept;
  - receipts: TP receipts deploy (parts, cargo, lock code); pack refused for a non-owner; duplicated receipt refused; survive restarts.
- **Fresh install:** CSV catalog by default; `ConfigReport.log` reports CSV errors with file and line.
- **Security fixes:** bank vault and safezone cleanup RPCs.
- **In game (client), 2026-10-23:** CF+TraderX OK; CF+new DayZular+TraderX OK; style drift fixed (Banking styles namespaced `TXB*`).

## Smoke checks before publish

Use the test rigs in the DayZServer dir: `txmig_serverDZ.cfg` (stable→current) and `tpmig_serverDZ.cfg` (TraderPlus→TraderX). Then:

1. Boot CF + TraderX alone: 0 script errors.
2. Boot CF + DayZular (the one just published) + TraderX: 0 script errors.
3. Client join: open a trader, buy and sell, then bank, garage, receipt actions.

Take screenshots with Steam F12 (`xdotool windowactivate <DayZ win>; xdotool key F12`). They land in `~/.steam/steam/userdata/87209913/760/remote/221100/screenshots`.

## Screens of the new interface (Nasdara redesign)

`docs/handoff/release-screens/`:
- `emulator/` has 23 PNGs rendered by enforce2ts (`node scripts/screens.ts out/screens` in `../enforce2ts`):
  - trader pages: home, catalog, purchase (+empty cart, list), customize (+presets, variants, save dialog), sell, favorites;
  - bank pages: bank, account, ledger, transfer, vault;
  - garage pages: garage, insurance, share;
  - other: toast, loading, promo.

  These are a TS simulation: fonts and layout are close to the game but not identical.
- `in-game/` has 8 Steam screenshots from 2026-10-23 QA, taken while fixing style drift. Some are from before the fixes. Compare them with the emulator shots and re-shoot the final state.

Design source (owner called it "perfect"): https://claude.ai/artifact/X8aVoCe23Dyzmojvgs8b7G

## Useful paths

- Change note: `ci/CHANGENOTE.md`. CI docs: `ci/README.md`.
- TraderPlus migration gaps: `docs/TraderPlus_Migration_Gaps.md`. Garage: `docs/handoff/GARAGE_HANDOFF.md`, `INSURANCE_SPEC.md`.
- Config skill: `.claude/skills/traderx-config` (lint: `scripts/txconf.py`).
- Server: `~/.steam/steam/steamapps/common/DayZServer`. Mod symlinks:
  - `@tx_TXE` → `target/@TraderXExperimental`
  - `@tx_TXWS` = stable workshop
  - `@tx_TPWS` = TraderPlus
  - `@tx_Dabs`
  - `@tx_DZLnew` → `target/@DayZularDefine`
- QA client+server: `../dayz-autotest/qa/start_qa.sh` (env `MODS_SERVER` / `MODS_CLIENT`). Start the client via `steam -applaunch 221100 -nolauncher`.
