# Catalog formats (modes) and importers

Code: `src/TraderX/scripts/3_Game/Domain/Shared/Csv/**`,
`4_World/classes/TraderX/Services/TraderXConfigurationService.c`, boot in `4_World/classes/TraderX/Module/TraderXModule.c`.
The catalog = products + categories + admin presets. Everything else (traders, currencies, licenses...) is JSON in
every mode.

## Selector: `TraderXConfig/SourceConfig.json`

```json
{ "preferredFormat": "CSV", "autoMigrate": 0, "allowCompilation": 1 }
```

- Created at first boot when missing: **`CSV` + `autoMigrate 1` on a new server** (no `TraderXGeneralSettings.json`
  before boot), `LEGACY` on an existing server (it keeps the files it was edited with).
- `autoMigrate`: CSV/COMPILED only. When `Source/` is empty, export the per-file JSON catalog to `Source/` once, then
  the server sets it back to 0.
- `allowCompilation`: CSV only. 0 = do not recompile, keep using the current `Compiled/`.
- Unknown `preferredFormat` → LEGACY.

| Mode | Admin edits | Server reads |
|---|---|---|
| **CSV** (default for new servers, recommended) | `Source/*.csv` | `Source/` compiled to `Compiled/` at each boot, then `Compiled/` |
| LEGACY | `Products/prod_*_NNN.json`, `Categories/cat_*_NNN.json` | those files only (`Compiled/` is ignored, with a log warning) |
| COMPILED | `Compiled/Products_compiled.json`, `Categories_compiled.json` | `Compiled/` directly (never regenerated once present) |

## CSV format (`Source/`)

Three files, any names, as long as the name contains `product`, `categor` or `preset`. Read in alphabetical order;
a later file overrides rows of an earlier file with the same key (packs: `10_vanilla_products.csv`,
`50_mymods_products.csv`). A key twice in the **same** file is an error.

Spreadsheet rules (`TraderXCsvLoader.c`):
- Line 1 = header, column names in any order, case-insensitive; unknown columns ignored; `notes` free text.
- Delimiter: TAB (what the server writes), or `,` / `;` when the header has no TAB (Excel/LibreOffice "CSV",
  French Excel uses `;`). A cell holding the delimiter is quoted (`"a;b"`), `""` = literal quote. UTF-8 BOM ok.
- Lists (`attachments`, `variants`, `productKeys`, `licensesRequired`) are separated by `;`.
- Booleans `1/true/yes` or `0/false/no`. Decimal point or comma (`0.5` / `0,5`).
- A blank cell takes the default below (a missing column too).

### products.csv

```
productKey	className	buyPrice	sellPrice	maxStock	coefficient	buyQtyMode	buyQtyValue	sellQtyMode	sellQtyValue	destockCoefficient	stockBehaviorAtRestart	attachments	variants	notes
prod_akm_001	AKM	5000	2500	-1	1	FULL	0	FULL	0	0	0	prod_mag_akm_30rnd_001;prod_ak_suppressor_001
prod_water_bottle_001	WaterBottle	40	10	50	0.98	COEFFICIENT	0.5	NO_MATTER	0	0.1	1
prod_ammo_762x39_001	Ammo_762x39	2	1	-1	1	STATIC	20	STATIC	20	0	0
```

| Column | Default | Meaning |
|---|---|---|
| `productKey` | (required) | Product id. Any text without `/ \ :` (the `prod_<name>_NNN` rule is LEGACY only); keep the `prod_..._001` style for consistency. Stock file name `TraderXDatabase/Stock/<productKey>.json` |
| `className` | (required) | Item class, case-insensitive. Equal to a license name → sells that license |
| `buyPrice` / `sellPrice` | -1 | -1 = not buyable / not sellable; 0 = free |
| `maxStock` | -1 | -1 unlimited; > 0 limited stock |
| `coefficient` | 1 | Price decay per unit in stock: `price = base · coefficient^(stock-1)`. 1 = flat. Only with limited stock |
| `buyQtyMode` | FULL | EMPTY, FULL, COEFFICIENT, STATIC: quantity of the spawned item |
| `buyQtyValue` | 0 | COEFFICIENT: fraction (0.5) or percent (50) of max; STATIC: units (1-4095) |
| `sellQtyMode` | FULL | NO_MATTER (any quantity, price scaled), EMPTY, FULL, COEFFICIENT, STATIC: required quantity to sell |
| `sellQtyValue` | 0 | as buy (STATIC up to 8191) |
| `destockCoefficient` | 0 | Share of stock removed at each restart: 0.1 or 10 = 10 % |
| `stockBehaviorAtRestart` | 0 | 0 keep, 1 reset to maxStock, 2 random 0..max |
| `attachments` / `variants` | | productKeys |

### categories.csv

```
categoryKey	categoryName	icon	isVisible	licensesRequired	productKeys	notes
cat_assault_rifles_001	Assault Rifles		1		prod_akm_001;prod_ak74_001
cat_vehicles_001	Vehicles		1	Car License	prod_offroadhatchback_001
```
`categoryKey` is what traders list in `categoriesId`. `isVisible` default 1. Locked categories (licenses) stay
visible but greyed.

### presets.csv

```
presetKey	productKey	presetName	isDefault	attachments	notes
preset_akm_tactical	prod_akm_001	Tactical	1	prod_mag_akm_30rnd_001;prod_ak_suppressor_001
```
Compiled into `TraderXPresets/presets_<productKey>.json` (overwrites that file). `isDefault` 1 marks the default;
none → the first preset of the product. Products without preset rows keep their existing preset JSON file.

### Compile, errors, backup

At each boot (`TraderXConfigCompiler.c`): load + merge → validate → write `Compiled/` + presets → copy to `Backup/`.
Report: `TraderXLogs/Config/ConfigReport.log`, `[ERROR] products.csv (line 12, column 'buyQtyMode'): ...`.

- **ERROR** (compile refused, the last good `Backup/` catalog is restored and used): duplicate key in one file,
  unknown qty mode word, key containing `/ \ :`, `stockBehaviorAtRestart` not 0-2.
- **WARNING** (compiled as written, like the server does today): unknown product/attachment/variant reference
  (ignored at runtime), quantity out of range (clamped), price -1/-1, coefficient ≤ 0 with limited stock,
  empty className or category name.
- Always read the report after a restart; `txconf.py lint` checks the same rules offline.

## Switching an existing server

- **LEGACY → CSV**: set `"preferredFormat": "CSV", "autoMigrate": 1`, restart. The loaded catalog (including the
  1.30 by-reference fallback) and its admin presets are exported to `Source/products.csv`, `categories.csv`,
  `presets.csv`, compiled, loaded. `Products/` and `Categories/` stay as a backup with a `_NOT_USED_SEE_Source.txt`.
  Verified identical on the QA catalog (1013 products, 53 categories, 7 presets).
- **CSV → LEGACY**: set `"preferredFormat": "LEGACY"`. The old `Products/`/`Categories/` files are used again
  (edits made in CSV since are not in them).
- **Prices in old CSV files** written by TraderX before this rewrite are wrong (mis-encoded quantities, coefficient
  lost). Do not reuse them: re-export with `autoMigrate` from the JSON files.

## Importers (all server-side, at boot, before the catalog load, they write per-file JSON)

Read `TraderXConfig/MigrationReport_<src>.txt` after any import. Sentinels: `TraderXConfig/.migrated_<traderplus|drjones|expansion>`.
How the import reaches the catalog:
- New server (CSV): imported JSON → exported once to `Source/products.csv` etc.
- CSV server that already has `Source/`: exported to `Source/00_imported_*.csv`, the lowest-priority pack: the
  admin's own rows win, new products/categories are added. Review it, then move what you keep into your files.
- LEGACY: used directly. COMPILED: not used (log warning).

### TraderPlus V1 (`3_Game/ConvertTraderPlusV1/`)
- Runs only if a TraderPlus source exists, no `.migrated_traderplus`, **and no `TraderXGeneralSettings.json`**
  (fresh TraderX install only).
- Sources, in order: `TraderXConfig/TraderX{Price,General,Vehicles}Config.json`, `TraderXConfig/TraderPlus*Config.json`,
  `$profile:TraderPlus/TraderPlusConfig/TraderPlus*Config.json` (+ IDs config).
- Converts products/categories, licenses, acceptedStates, currencies (grouped by class-name: Dollar→USD, Euro→EUR,
  Ruble→RUB, DIM, else UNKNOWN), traders (TraderPlus id -2 → 126 bank, -3 → 125 garage, -4 skipped),
  player bank accounts, licenses and stock. Files taken from `TraderXConfig/` move to `TraderXConfig/deleted/`;
  files in the TraderPlus folder stay.
- Re-run: delete `.migrated_traderplus` **and** `TraderXGeneralSettings.json` (back them up).
- Banking, Garage, SafeZone each import their own TraderPlus config when their TraderX config file is missing.

### Dr Jones Trader (`ConvertExternal/ConvertDrJonesTrader.c`)
- Runs if `$profile:Trader/TraderConfig.txt` exists and no `.migrated_drjones`.
- Adds products/categories only if new, merges currency, **appends** NPCs to general settings, creates parking
  files if absent, merges safezones. Sentinel written only on full success (retries otherwise).

### Expansion Market (`ConvertExternal/ConvertExpansionMarket.c`)
- Runs if `$profile:ExpansionMod/Market/*.json` is listable and no `.migrated_expansion`.
- Reads markets, traders, `$mission:expansion/settings/MarketSettings.json`, trader zones, `.map` NPCs, ATM files.
- `coefficient = (MinPrice/MaxPrice)^(1/(maxStock-1))`, `maxStock = MaxStockThreshold`, currency type `EXPANSION`,
  ATM balances staged for Banking. Never overwrites; NPC ids skip 125/126.
- Importer-created general settings have `acceptedStates: null` (everything but ruined sellable): set it explicitly.

### DayZ 1.30 note
`FindFile` ignores `$profile:` on 1.30; TraderX maps it to the `-profiles` folder. If that folder is **outside** the
server directory, listing fails: `Source/` then only finds the standard names (`products.csv`, `categories.csv`,
`presets.csv`, `*_migrated.csv`, `00_imported_*.csv`), LEGACY loads by reference from the traders, and importers
that need listing retry each boot. Keep `-profiles` inside the server folder.
