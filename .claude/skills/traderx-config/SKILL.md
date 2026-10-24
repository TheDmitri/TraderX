---
name: traderx-config
description: Create, edit, migrate, audit or debug a TraderX server configuration (traders/NPCs, products, categories, currencies, licenses, presets, parking, Banking, Garage, CarLock, SafeZone, LEGACY/CSV/COMPILED formats, TraderPlus/Expansion/DrJones imports). Use whenever a task touches files under $profile:TraderX/ or asks "how do I configure X in TraderX".
---

# TraderX server configuration

Everything lives under the server profile folder (`-profiles=...`), in `$profile:TraderX/`.
All facts here were read from the code (2026-10-23, TraderX `1.0.2`). When in doubt, the code wins:
paths are relative to `src/TraderX/scripts/` unless another package is named.

Reference files (read the one you need, not all):

| File | Read it for |
|---|---|
| `reference/core.md` | General settings, traders/NPCs, products, categories, currencies, licenses, presets, stock, parking, translations, logging, debug |
| `reference/packages.md` | TraderXBanking, TraderXGarage (+ insurance), TraderXCarLock, TraderXSafeZone |
| `reference/formats.md` | CSV / LEGACY / COMPILED modes, CSV columns and defaults, compile errors, switching format, importers (TraderPlus, Expansion, DrJones) |
| `scripts/txconf.py` | Lint a whole profile (any format), encode/decode bitfields, generate ids |

## Golden rules

1. **Server stopped, then edit, then restart.** Nothing is hot-reloaded except `TraderXLogs/Config/LoggingSettings.json`.
   Stock, ledger and account caches overwrite files edited while the server runs.
2. **Check the catalog format first:** `TraderXConfig/SourceConfig.json` → `preferredFormat`.
   - `CSV` (default on new servers, recommended): edit `Source/products.csv`, `categories.csv`, `presets.csv`
     (spreadsheet rows, readable quantity modes). Never edit `Compiled/` or `Products/` then: they are
     regenerated / not read.
   - `LEGACY`: edit `Products/prod_<name>_NNN.json` + `Categories/cat_<name>_NNN.json` (one file per entry,
     the **file name is the id**, `NNN` = 001-999; a badly named file is skipped: `Invalid product ID ...`).
   - `COMPILED`: edit `Compiled/*_compiled.json` (rare).
   - To move an existing server to CSV: `"preferredFormat": "CSV", "autoMigrate": 1`, restart. Details,
     columns and defaults: `reference/formats.md`.
3. **After a CSV change, read `TraderXLogs/Config/ConfigReport.log`.** An ERROR refuses the compile and the
   server keeps the last good catalog, so "my change did nothing" usually means an error is listed there.
4. **Ids are references, check them.** Trader `categoriesId` → category ids (CSV `categoryKey` / LEGACY file
   name); category products, product `attachments`/`variants`, preset attachments → product ids; `currenciesAccepted` → `currencyName`
   (case-sensitive); `licensesRequired` → `licenseName` (case-insensitive). A dangling id fails silently
   (empty trader, missing item). Run `scripts/txconf.py lint` after every change.
5. **npcId 125 = Garage, 126 = Bank.** Never give a trading NPC (one with categories) these ids; the server
   moves it to a free id at boot. Garage attendant ids must not collide with core trader ids either.
6. **Bitfields (LEGACY/COMPILED only):** `tradeQuantity` and `stockSettings` are packed ints. Never
   hand-compute them: `python3 scripts/txconf.py tq --buy FULL --sell FULL` → `18`. CSV uses the plain columns.
7. **JSON as DayZ writes it:** bools are `0`/`1` (true/false also parse), vectors are `[x, y, z]` arrays,
   missing keys keep their code default, unknown keys are ignored. Keep files valid JSON (no comments,
   no trailing commas): a parse failure usually means "defaults used" and, for several files, the defaults
   are written back over yours.
8. Never touch generated or per-player state unless the task is explicitly a data fix:
   `TraderXDatabase/`, Banking `Accounts/Ledgers/Vaults/Contacts`, Garage `Database/Insurance`,
   CarLock `Database/` and `storage_mode.txt`, `.migrated_*` sentinels.

## Folder map (server)

```
$profile:TraderX/
  TraderXConfig/
    SourceConfig.json                  catalog format selector: CSV | LEGACY | COMPILED
    TraderXGeneralSettings.json        traders (NPCs), licenses, accepted item states, decor objects
    TraderXCurrencySettings.json       currency types and denominations
    TraderXDynamicTranslationSettings.json   server transaction messages
    TraderXDebugSettings.json
    Source/products.csv, categories.csv, presets.csv   CSV catalog (+ any *product*/*categor*/*preset* pack)
    Compiled/ Backup/                  generated from Source/ at each boot (CSV) - do not edit
    Products/prod_*_NNN.json           LEGACY catalog, one per product
    Categories/cat_*_NNN.json          LEGACY catalog, one per category
    TraderXVehicleParking/parkings_<npcId>.json, parking_config.json
  TraderXDatabase/Stock/<productId>.json, PlayerLicenses/playerLicense_<steam64>.json   runtime state
  TraderXPresets/presets_<productId>.json   admin presets (Customize page)
  TraderXLogs/Config/LoggingSettings.json, TraderXLogs/Logs/*.log, TraderXLogs/Config/ConfigReport.log
  TraderXBanking/TraderXBankingConfig.json   (+ Accounts/ Ledgers/ Contacts/ Vaults/ Logs/)
  TraderXGarage/TraderXGarageConfig.json     (+ Database/ Insurance/ DeadClasses.txt Logs/)
  TraderXCarLock/TXCLSettings.json           (+ Database/ storage_mode.txt Logs/)
  TraderXSafeZone/TXSafeZoneSettings.json    (+ Logs/)
```

Every config file is created with defaults on first boot when missing. The fastest way to get a correct
skeleton is: start the server once with the mods, stop it, then edit the generated files.

## Typical tasks

**Add a trader.** Append to `traders[]` in `TraderXGeneralSettings.json`: unique `npcId` (not 125/126, not a
garage id), `className` (a `SurvivorX_*` → human NPC, a building class like `TraderPlusSign` → static
trader), `position`, `orientation` (`[yaw, 0, 0]`), `categoriesId`, `currenciesAccepted` (empty = all),
optional `licensesRequired`, `loadouts`. Vehicle trader: also create
`TraderXVehicleParking/parkings_<npcId>.json` (key is `rotation`, not `orientation`). See `reference/core.md`.

**Add products to a trader.** CSV: one row per item in `Source/products.csv` (`productKey` like
`prod_<class>_001`), add the keys to a row of `categories.csv` (`productKeys`, `;`-separated), put that
`categoryKey` in the trader's `categoriesId`. LEGACY: one `Products/prod_<class>_001.json` per item
(`txconf.py newid` gives the name), listed in a category file's `productIds`.

**Change prices / stock.** `buyPrice`/`sellPrice` (`-1` = not buyable / not sellable), `maxStock`
(`-1` unlimited), `coefficient` (price decay per unit in stock, `1` = flat), restart behaviour + destock
(CSV columns `stockBehaviorAtRestart`, `destockCoefficient`; LEGACY `stockSettings` bitfield). Current stock is
runtime state in `TraderXDatabase/Stock/<productId>.json` (delete one to re-seed it).

**Sell a license.** Define it in `licenses[]` of general settings, then create a product whose `className`
equals the `licenseName`. Gate a category or a trader with `licensesRequired: ["<licenseName>"]`.

**Bank / garage / safezone / car lock settings.** `reference/packages.md`.

**Import from TraderPlus / Expansion / DrJones.** Automatic at boot under strict conditions; see
`reference/formats.md` → Importers. Always read `TraderXConfig/MigrationReport_<src>.txt` afterwards.

**Debug "trader is empty / item missing / can't sell".** Run the linter, then grep the newest
`TraderXLogs/Logs/TraderX_*.log` for `Invalid product ID`, `not found`, `Error`, and (CSV) read
`ConfigReport.log`. Check, in this order: right format edited (see rule 2)?; compile errors?; ids referenced exist?; `acceptedStates` (all `0` means only
pristine items can be sold); `currenciesAccepted` spelling; `licensesRequired`; `maxStock` full (selling
refused when stock would exceed max).

## Lint before handing back

```
python3 .claude/skills/traderx-config/scripts/txconf.py lint <profileDir>
```
`<profileDir>` is the folder that contains `TraderX/` (the `-profiles=` folder). It reads the catalog in the
format the server uses (CSV parsed exactly like the server: delimiters, quotes, defaults, pack merge) and checks
JSON validity, file names, every cross-reference, reserved npcIds, parking keys, garage/core id collisions and known
dead settings. Fix every `ERROR`; read every `WARN`.

QA server profile on this machine: `~/.steam/steam/steamapps/common/DayZServer/jp_txqa/`.
