# Handoff — TraderXGarage rework (Modern Garage feature parity, in-world preview)

Written 2026-10-15. Audience: the agent who will implement the new TraderX garage.
Read this whole file, then `CLAUDE.md` at the repo root, before touching code.

## 1. Mission

Rebuild `src/TraderXGarage` into a garage that has **at least the feature set of "Modern Garage"**
(Steam Workshop 3783001329, author Rook1969) and a **better UI**, including an **in-world vehicle
preview**. Use **our** architecture: DayZular components and services, flat DTOs over RPC, DZML
layouts, and the TraderX provider contracts. The owner wants a garage that is clearly better than the
competition. "Done" means a green in-game test and Dmitri validating it live (TSL, see `CLAUDE.md`).

## 2. Hard constraint — clean room & simplicity (anti-bloat)

Modern Garage's licence forbids modification, redistribution and **derivative versions** of its code.
You may decompile it **only to study features and behaviour**. Then:

- Write a feature/behaviour description (section 6 deliverable), then design and code from that
  description. Do not copy, paraphrase or port its code, names, layouts, strings or file formats.
- Do not ship any of its files or assets. Do not reuse its class/RPC/config names (they are `MG_*`).
- Engine APIs (e.g. `CreateObjectEx(..., ECE_LOCAL ...)`, `LocationCreateLocalEntity`, `ItemPreviewWidget`) are fair game; they are
  DayZ's, not Modern Garage's.
- **Keep it simple and radically leaner than Modern Garage (~81k LOC → ~3.5k–4.5k LOC target):**
  - This is a DayZ mod, not an enterprise Java monolith. Do **not** over-engineer with multi-bank adapter layers, custom binary stream serializers, embedded self-test suites, or imperative 7,500-line menu classes.
  - Use **our** architecture and existing building blocks directly:
    - Payments go straight through `ITraderXPaymentProvider` (`TraderXProviderRegistry`) + our existing cash helper.
    - Locks & ownership go straight through `ITraderXCarLockProvider`.
    - Catalog & pricing reuse `TraderX` core (`TraderXProductRepository`) when loaded, with a simple config fallback when standalone.
    - Persistence stays clean JSON (`TXGGarageData` + `PlayerIndex.json`), never custom `.bin` files.
    - UI uses declarative **DZML + DayZular** (`DZlrComponentBase`, `TextBinding`, `Component_ForEach`), not manual widget trees.

## 3. Repository state (important — do not trample)

- Repo: `/home/dmitri/DayZWorkspace/TraderX Projects/TraderX`, branch **`migration-dayzular`**
  (the "experimental" TraderX, published as Workshop 3760962576; it is about to replace the stable
  item 3560310973). **Large uncommitted work is in the tree** (DayZ 1.30 fixes, migration work).
  Do not reset, stash or reformat unrelated files. Work on your own branch from the current tree once the
  owner has committed, or ask first.
- A second worktree `../TraderX-stable` (branch `fix/dayz-1.30-stable`) holds the stable 1.30 fix. Not yours.
- Migration converters (`src/TraderX/scripts/3_Game/ConvertTraderPlusV1`, `ConvertExternal`,
  `src/TraderXGarage/scripts/3_Game/Convert`) are being reworked in parallel. Coordinate before editing
  `ConvertTraderPlusGarageConfig.c` — it must keep importing TraderPlus garage config, and will gain
  TraderPlus garage **player data** import (vehicles + cargo, lowID→steam64 resolution).

## 4. Rules you must follow (each one has already cost real debugging time)

- `CLAUDE.md` → **DayZular RPC rules**: handler signature always `(PlayerIdentity sender, ParamsReadContext ctx)`;
  never send serialized JSON strings; send `ref array<ref DTO>`; DTOs flat (no live `EntityAI` in
  anything persisted or sent).
- `CLAUDE.md` → **DZML**: author `.html`, compile to `.layout` with the `dzml` CLI; never hand-edit `.layout`.
  Widget `data-name`s are a contract with the component's members. Use the Field-Ledger design tokens
  (`docs/TraderX_Design_System.md`).
- Enforce Script: no ternary; **never split a statement across lines**; class names globally unique
  (`TXG*` prefix).
- **DayZ 1.30 compiler** (stricter): no `new ref T`; no `ref` on method parameters; locals/params must not
  shadow vanilla globals (e.g. `_item`); no implicit base→derived downcasts (use `X.Cast`); missing
  `override` is an error; override parameter names must match the parent. One error cascades into fake
  `Bad type 'JsonFileLoader'`/`'Param1'` errors — always fix the **first** error.
- **1.30 `FindFile`** only lists paths relative to the server working directory. Never call `FindFile`
  directly; use `TraderXFileSystem.Find(...)` (core) or a package-local copy (see `TXCLFileSystem` in
  CarLock). Better: design storage that never needs directory listing (keep an index file).
- **Vehicle persistence**: if you store anything on the vehicle entity, use **CF ModStorage**
  (`CF_OnStoreSave/CF_OnStoreLoad` with a `storageVersion` in the package `CfgMods`), never raw
  `ctx.Write` in `OnStoreSave`. See `TraderXCarLock` (`TXCLStorageMode.c`, `Entities/CarScript.c`) for the
  pattern and why. Never return `false` from a vehicle `OnStoreLoad` (it drops the vehicle).
- Two-PBO independence: `TraderXGarage` must work **standalone** (without TraderX core) and with it.
  Cross-module calls go through `TraderXProviderRegistry` ports, behind `#ifdef TraderX` where needed
  (see `TXBBankNpcService.c`).

## 5. Current TraderXGarage (what exists)

~2,340 lines, `src/TraderXGarage/scripts`:

| Area | Files |
|---|---|
| Models (3_Game) | `TXGGarageData` (per-player, `Garage_<steam64>.json`), `TXGVehicleData`, `TXGItemSnapshot` (cargo/attachment tree), `TXGParkSpot`, `TXGDtos`, `TXGVehicleRowVM` |
| Config | `TXGSettings` (`TraderX/TraderXGarage/TraderXGarageConfig.json`; costs, max stored, save cargo/health/fuel, payWithBank, npcs[] with parkSpots[]) — documented in `src/TraderXGarage/docs/CONFIGURATION.md` |
| Services (4_World) | `TXGGarageService` (park in/out, snapshot/restore), `TXGRPCService`, `TXGUIService`, `TXGNpcService` (spawns garage NPCs, standalone), `TXGConfigService` |
| Provider | `TXGGarageProvider` implements core port `ITraderXGarageProvider` (`src/TraderX/scripts/4_World/Integration/`) |
| UI | `Components/GUI/TXGGarageComponent`, `TXGVehicleRowComponent`; layouts `src/TraderXGarage/datasets/layouts/GaragePageView.{html,layout}`, `GarageVehicleRow.{html,layout}` |
| Actions | `ActionTXGOpenGarage` (opens on NPC id **125** only) |

Known defects to fix while you are there:
- Park-in takes any nearby empty vehicle: **no owner check, no lock check**; `lockCode` is stored as 0
  (`TXGGarageService.c` ~60-105). Integrate the CarLock port (`ITraderXCarLockProvider`).
- Only NPC id 125 opens a garage; a second garage NPC (`110+i` from the TraderPlus converter) can't be used.
- Default config spawns a `TraderPlusSign` at Chernarus coordinates on every map.
- Payment: `payWithBank` uses the Banking port (`TXBPaymentProvider` via `TraderXProviderRegistry`), falls
  back to cash with `TraderX_Money_*` defaults.

Contracts to **keep** (others depend on them): `ITraderXGarageProvider` (all methods), the
`TraderXGarageVehicleInfo` DTO, `Garage_<steam64>.json` readability (existing servers' stored vehicles must
still come back — add a version field and migrate, never break), NPC id 125 behaviour.
You may **add** contracts (new provider methods/ports, new DTOs) — document each in `DECISIONS.md` as an ADR.

## 6. Modern Garage — how to get it and what to study

Download (cached Steam session, no password needed):

```bash
S="/home/dmitri/DayZWorkspace/TraderX Projects/TraderX/ci/.steam-session"
HOME="$S" "$S/steamcmd/steamcmd.sh" +login cretidivada +workshop_download_item 221100 3783001329 +quit
# -> $S/Steam/steamapps/workshop/content/221100/3783001329/{addons/ModernGarage.pbo,docs/*}
```

Unpack (scripts are plain text `.c`, no obfuscation; config is `config.cpp`):

```bash
T="/home/dmitri/DayZWorkspace/TraderX Projects/dayz-autotest/tools"
python3 -I "$T/pbols.py" ModernGarage.pbo     # list entries + PBO header
python3 -I "$T/unpbo.py" ModernGarage.pbo /tmp/<your-scratch>/mg
```

Put the extracted files in **your scratch dir, not in the repo**. Read `docs/README.md`, `docs/Settings.sample.txt`,
`docs/admin-notes.md` first. ~135 script files, ~81k lines (a large share are `*SelfTest.c`).

Feature areas and where they live (for study only):

| Feature | Files to read |
|---|---|
| Store/retrieve anywhere via keybind, per-player limits counting vehicles in the world | `MG_ActionStoreVehicle`, `MG_Store`, `MG_Restore`, `MG_Garage`, `MG_Slot*`, `MG_Counters` |
| Exact restore: cargo, attachments, fuel/fluids, every damage zone; survives removed mods (dead classes reported at boot) | `MG_Cargo`, `MG_CargoScan`, `MG_ItemNode`, `MG_NativeState`, `MG_Floats`, `MG_DeadClasses`, `MG_Manifest` |
| **In-world preview** before retrieving | `5_Mission/MG_Preview.c`, `MG_PreviewSlots.c` (client-local entity via `ECE_LOCAL`, local attachments via `LocationCreateLocalEntity`, camera framing), `MG_RestingHeight`, `MG_SpawnFinder` |
| Dealer buy/sell with condition-aware prices (repair cost + missing parts deducted), refuses vehicles with gear inside | `MG_Price`, `MG_PartCondition`, `MG_PartFinder`, `MG_Offers`, `MG_DealerSale*`, `MG_PartsTrade` |
| Repairs priced per category (fluids/body/worn/ruined) | `MG_Repair`, `MG_RepairQuote`, `5_Mission/MG_ServiceScreen` |
| Wreck recovery with deductible (insurance-like) | `MG_Recovery`, `MG_Transit` |
| Player market (sellers paid offline, crash-recoverable journals) | `MG_Market*`, `MG_MarketJournal` |
| Vehicle lock, unauthorized drivers stopped, sharing with named players, ownership after leaving the garage | `MG_VehicleLock*`, `MG_Sharing`, `MG_VehicleOwners`, `MG_SlotOwners`, `Adapters/MG_OwnershipAdapter` |
| Admin: any player's garage, impound with reason/fee, parking meter, no-retrieve zones, zone capture in game, in-game settings & price catalog editor | `MG_AdminGarage`, `MG_Impound`, `MG_ParkingMeter`, `MG_Capture`, `MG_Roster`, `5_Mission/MG_SettingsScreen`, `MG_CatalogEditor`, `MG_PlayersScreen`, `MG_ImpoundScreen` |
| Payment adapters (cash, several banking mods) | `Adapters/*BankAdapter`, `MG_Funds`, `MG_Currency`, `MG_Money*` |
| Per-client language, message overrides | `MG_Msg`, `MG_Text` |
| Boats on water, motorbikes (1.30) | `MG_SpawnFinder`, `MG_RestingHeight` |

## 7. Target design (proposal — refine, then record as ADRs)

1. **First deliverable (no code):** `docs/TraderXGarage_Feature_Matrix.md` — every Modern Garage feature →
   our design (service/DTO/RPC/UI) → priority (P0 parity, P1 differentiator, P2 later) → depends on
   (CarLock / Banking / core). Get Dmitri's go before coding.
2. **P0:**
   - Owner-checked park-in, integrated with CarLock.
   - Exact restore (all damage zones, fluids, nested cargo).
   - Dead-class tolerance with a boot report.
   - Per-player limits.
   - Multiple garage NPCs.
   - **In-world preview** (client-local entity at the retrieval spot, orbit camera, attachments shown, no server entity until confirmed).
   - New DZML UI: list, details panel, preview, costs.
3. **P1:**
   - Dealer with condition-aware pricing, priced through the TraderX product catalog when core is present.
   - Repairs per category.
   - Wreck recovery: this is the **insurance replacement**. The vehicle record stays server-side, so it can't be sold outside our jurisdiction. Agree the rules with Dmitri first.
   - Sharing.
   - Admin garage browser and impound.
4. **P2:** player market, parking meter, in-game catalog/settings editor.
5. Architecture:
   - `TXG*` services registered in `TXGAppModule`.
   - One RPC service with rate limits.
   - Flat DTOs (`TXGVehicleSummaryDTO`, `TXGVehicleDetailDTO`, `TXGQuoteDTO`…).
   - Server-only snapshot trees; keep `TXGItemSnapshot` server-side, never sent whole.
   - Storage index file per player, so no directory listing is needed.
   - Versioned `Garage_<steam64>.json` with migration.

## 8. Build and test

- Build (Linux):
  `cd TraderX && ~/DayZWorkspace/DayZWorkspaceLinux/.venv/bin/python -m dayz_mod_workspace.cli --mod-dir . build --no-launch`
  → `target/@client/addons/*.pbo`.
  **Run the build in its own shell command:** the tool kills processes whose command line mentions DayZ
  server names, including your own shell if that line contains them.
- Server: `~/.steam/steam/steamapps/common/DayZServer` (1.30.164078).
  - Test config `tx_serverDZ.cfg`, mission `dayzOfflineTP.chernarusplus` (vanilla init.c), relative profile (`-profiles=jp_<name>`).
  - Mods via symlinks in the server dir: `@tx_CF` (Workshop CF), `@tx_DZL` (DayZular), `@tx_TXE` → `TraderX/target/@client`.
  - The server process is named `enfMain`; stop it by PID, not `pkill -f`.
- Headless conversion/boot harness: `dayz-autotest/tools/convert-run.sh <name> <source-profile-dir> [extra-mods]`.
  It boots, stops cleanly and copies the resulting profile. Edit the paths at the top for your scratch dir.
- Client test: `steam -applaunch 221100 "-mod=@1559212036;@<DayZular>;@<yourmod>" -nolauncher -connect=127.0.0.1 -port=2302`.
  - `-mod` must come first in that argument list.
  - Mod folders are symlinks inside the DayZ client dir.
  - DayZ must have **no Steam launch-option wrapper**: it broke Steam auth (`WaitAuthPlayerLoginState`).
  - Close any previous DayZ instance first (`AppError_16`).
- Logs: `<profile>/script_*.log`, `<profile>/*.RPT`, `<profile>/TraderX/TraderXGarage/Logs/`.

## 9. Ask Dmitri before

Changing any existing contract or file format, deleting features, publishing anything, or committing on
`migration-dayzular`.
