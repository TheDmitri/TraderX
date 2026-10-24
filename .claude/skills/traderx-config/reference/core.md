# Core TraderX config (package `src/TraderX`)

Paths relative to `$profile:TraderX/`. Code paths relative to `src/TraderX/scripts/`.
Path constants: `3_Game/Constant/Constants.c`. Boot order: `4_World/classes/TraderX/Module/TraderXModule.c:32-60`
(importers → `SourceConfig.json` → catalog load → general settings → presets → NPC spawn), 1 s after mission start.

## 1. `TraderXConfig/TraderXGeneralSettings.json`

Model `3_Game/Domain/Settings/Model/TraderXGeneralSettings.c`. Created with defaults only when missing.
Sent in full to every client on join (positions, licenses, serverID...): do not store secrets here.

| Field | Type | Default | Notes |
|---|---|---|---|
| `version` | string | `"1.0.2"` | Mismatch → file re-saved with current version (new fields get written then) |
| `serverID` | string | UUID if empty | Keys the client favorites/presets files. Changing it resets players' favorites and personal presets |
| `licenses` | `[{licenseId, licenseName, description}]` | Car License, Admin License | `licenseId` UUID auto-generated. References use `licenseName` |
| `acceptedStates` | `{acceptWorn, acceptDamaged, acceptBadlyDamaged, coefficientWorn, coefficientDamaged, coefficientBadlyDamaged}` | all accept = 1 | Accept flags gate **selling** only. Ruined is always refused. The 3 `coefficient*` are **unused**: sell price multipliers are hard-coded worn 0.8, damaged 0.6, badly 0.4 (`3_Game/Domain/Pricing/Model/TraderXItemState.c`). `null` → every state except ruined accepted |
| `traders` | `TraderXNpc[]` | Chernarus: garage 125 + ATM 126 | see §2 |
| `traderObjects` | `[{className, position, orientation}]` | 21 Chernarus decor objects | Static objects spawned at boot (tables, signs...) |
| `prewarmCatalogInSafeZone` | bool | 1 | Streams trader catalogs to the client when it enters a TraderXSafeZone |
| `showBeansWelcome` | bool | 1 | One-time launcher toast |

Non-Chernarus maps get no default objects and no garage/ATM NPCs.

## 2. Traders (`traders[]`, model `3_Game/Domain/Npc/Model/TraderXNpc.c`)

```json
{
  "npcId": 1,
  "className": "SurvivorM_Mirek",
  "givenName": "Boris",
  "role": "Weapons",
  "position": [11600.0, 58.5, 14670.0],
  "orientation": [62.0, 0.0, 0.0],
  "categoriesId": ["cat_assault_rifles_001"],
  "currenciesAccepted": ["EUR"],
  "licensesRequired": [],
  "loadouts": [
    { "className": "TShirt_Black", "quantity": -1, "slotName": "Body", "attachments": [] },
    { "className": "AKM", "quantity": -1, "slotName": "Hand", "attachments": [{ "className": "Mag_AKM_30Rnd", "quantity": -1 }] }
  ]
}
```

- `npcId`: unique int, key for RPCs and parking. **125 = Garage, 126 = Bank** (reserved). A trader with
  categories on 125/126 is moved to a free id 1-124 at boot (and its parking file renamed), with a log warning.
  0 works. Keep ids ≤ 127.
- `className`: a `PlayerBase` subclass (`SurvivorM_*`/`SurvivorF_*`) → human NPC wearing `loadouts`;
  a `BuildingBase` subclass (e.g. `TraderPlusSign`, `TraderPlus_BANK_ATM`) → static trader object.
- `givenName`/`role`: action text "Trade - givenName (role)" and menu header.
- `position` world coords; get Y from the terrain / an admin tool. `orientation` = `[yaw, pitch, roll]`.
- `categoriesId`: category ids (file names without `.json`). They define everything this trader trades:
  their products + those products' variants + attachments.
- `currenciesAccepted`: `currencyName`s from the currency file, **case-sensitive**. Empty = all types.
- `licensesRequired`: license names needed to open the trader at all.
- `loadouts[]`: `{className, quantity (-1 = default), slotName ("" inventory, "Hand" in hands, else
  attachment slot name), attachments:[{className, quantity}]}`.
- Bank terminal with TraderX loaded = a trader entry `npcId: 126`, `className: "TraderPlus_BANK_ATM"`,
  `categoriesId: []`. Garage attendants come from the Garage config, not from here (core 125 is auto-skipped
  when TraderXGarage is loaded).

## 3. Products: `TraderXConfig/Products/prod_<name>_NNN.json` (LEGACY)

The runtime model of a product. In CSV format the same data is one row of `Source/products.csv` with the bitfields
split into readable columns (`reference/formats.md`); the compiled file holds exactly these fields + `productId`.

Model `TraderXJsonProduct` (`3_Game/Domain/Product/Model/TraderXProduct.c:1-18`). **The file name is the id.**
Name rule (`ValueObject/TraderXProductId.c`): starts `prod_`, middle part `[A-Za-z0-9_]` (other chars are
sanitized with a warning), ends `_NNN` with NNN 001-999. Generated ids: `prod_<lowercased class, ' ' '-' '.'→'_'>_001`.

```json
{ "className": "AKM", "coefficient": 1.0, "maxStock": -1, "tradeQuantity": 18,
  "buyPrice": 5000, "sellPrice": 2500, "stockSettings": 0, "attachments": [], "variants": [] }
```

| Field | Meaning |
|---|---|
| `className` | Item class (case-insensitive). If it equals a `licenseName`, the product sells that license |
| `buyPrice` | Price the player pays. `-1` = cannot be bought |
| `sellPrice` | Price the player receives. `-1` = cannot be sold, `0` = taken for free |
| `maxStock` | `-1` unlimited. `>0` caps stock: buying needs stock, selling refused when stock+qty > max |
| `coefficient` | **Price** decay per unit in stock: `price = base · coef^(stock-1)` (min 1). Ignored when `maxStock = -1` or `coef = 1.0`. `1.0` = flat price. Expansion import uses `(min/max)^(1/(maxStock-1))` |
| `tradeQuantity` | Packed bitfield, see §3.1 |
| `stockSettings` | Packed bitfield, see §3.2 |
| `attachments` | Product ids offered in Customize (each has its own price/stock) |
| `variants` | Product ids shown as variants (own price/stock; need not be in a category) |

### 3.1 `tradeQuantity` (`4_World/classes/TraderX/Utilities/TraderXTradeQuantity.c`)

`value = sellMode | buyMode<<3 | (sellQty & 0x1FFF)<<6 | buyQty<<19`

| | Sell mode (bits 0-2) | Buy mode (bits 3-5) |
|---|---|---|
| 0 | NO_MATTER: any amount, price scaled by the item's current quantity | EMPTY: item spawned empty |
| 1 | EMPTY | — |
| 2 | FULL: item must be full | FULL: spawned full |
| 3 | COEFFICIENT: qty = `sellQty`% of max (0-100) | COEFFICIENT: `buyQty`% of max |
| 4 | STATIC: exactly `sellQty` units | STATIC: exactly `buyQty` units |

Common values: `0` (buy empty / sell any), `18` (FULL/FULL), `524388` (STATIC 1 / STATIC 1).
Items without quantity (weapons, clothes) ignore it. Use `scripts/txconf.py tq` / `tq-decode`.

### 3.2 `stockSettings` (`TraderXProduct.c:68-72,213-227`)

`value = destockPct (0-100) | behavior<<7`

- `behavior` at restart: 0 keep stock, 1 reset to `maxStock`, 2 random 0..max (3 = keep).
- After that, if `destockPct > 0`: `stock = round(stock · (1 - destockPct/100))`, **every restart**.
- Example: reset to max then -10 % → `10 | 1<<7 = 138`. Use `txconf.py ss --behavior 1 --destock 10`.

### 3.3 Stock: `TraderXDatabase/Stock/<productId>.json`

`{ "productId": "prod_ak74_001", "stock": 10 }`. Created on first use (at `maxStock` if > 0). Cached at boot
and saved on each change: edit only with the server stopped. Delete the file to re-seed it.

## 4. Categories: `TraderXConfig/Categories/cat_<name>_NNN.json` (LEGACY; CSV: a row of `Source/categories.csv`)

Model `3_Game/Domain/Category/Model/TraderXCategory.c`. File name = id (rule like products, prefix `cat_`).
Category files are **re-saved by the server at each boot** (formatting normalized).

```json
{ "isVisible": 1, "icon": "", "categoryName": "Assault Rifles",
  "licensesRequired": [], "productIds": ["prod_akm_001", "prod_ak74_001"] }
```

- `isVisible` 0 hides the category everywhere.
- `licensesRequired`: license names, all required. The category stays visible, greyed and locked; buying is
  refused server-side.
- `icon`: image path (optional).
- A product may be in several categories.

## 5. Currencies: `TraderXConfig/TraderXCurrencySettings.json`

```json
{ "version": "1.0.2", "currencyTypes": [
  { "currencyName": "EUR", "currencies": [
    { "className": "TraderX_Money_Euro100", "value": 100 },
    { "className": "TraderX_Money_Euro10",  "value": 10 },
    { "className": "TraderX_Money_Euro1",   "value": 1 } ] } ] }
```

- Denominations sorted high→low at load. Change is paid largest note first; a sale is refused if it would need
  more than 1000 stacks.
- Money item classes: `src/TraderPlusMoney/config.cpp` (`TraderX_Money_Euro*`, `_Dollar*`, `_Ruble*`, ...).
- Known default bug: the generated default puts Dollar notes in the EUR type and leaves USD empty: rewrite it.
- Traders reference `currencyName` (case-sensitive). Banking/Garage merge every class from here into their
  own cash table at face value (one unit, no exchange rate).

## 6. Licenses

- Defined in general settings `licenses[]`.
- Granted only by **buying a product whose `className` equals the `licenseName`** (or TraderPlus import).
- Ownership file `TraderXDatabase/PlayerLicenses/playerLicense_<steam64>.json`:
  `{ "steamId": "...", "playerName": "...", "licenses": [{ "licenseId": "...", "licenseName": "Car License", "description": "" }] }`.
  Grant/revoke by hand = edit this file with the server stopped. Matched by name, case-insensitive.
- Used by trader `licensesRequired` (open trader) and category `licensesRequired` (buy in category).

## 7. Admin presets: `TraderXPresets/presets_<productId>.json`

In CSV format these files are generated from `Source/presets.csv` for every product that has preset rows.

```json
{ "productId": "prod_ak74_001", "defaultPresetId": "p1",
  "presets": [ { "presetName": "Tactical", "presetId": "p1", "productId": "prod_ak74_001",
                 "attachments": ["prod_mag_ak74_30rnd_001", "prod_ak_suppressor_001"] } ] }
```

- Keep the file name and inner `productId` identical (the repository keys by one or the other).
- `defaultPresetId` must match a preset, else no default.
- `attachments` are product ids (must exist and be in that product's `attachments`).
- Sent to all clients on join. Players' own presets are client-side (`presets_<serverID>.json`), not here.

## 8. Vehicle parking: `TraderXConfig/TraderXVehicleParking/`

`parkings_<npcId>.json` for every trader that sells vehicles (class `IsKindOf CarScript/BoatScript`):

```json
{ "traderId": "1", "positions": [
  { "position": [11615.0, 59.0, 14690.0], "rotation": [50.0, 0.0, 0.0], "size": [3.0, 2.0, 6.0] } ] }
```

- Map key comes from the **file name**; `traderId` should equal it. World coordinates.
- The key is **`rotation`**: `orientation` is silently ignored (rotation 0).
- `size` = collision box checked before spawning; the first free spot is used.
- Auto-generated default spots are near world origin: always replace them.
- `parking_config.json`: `{ "whitelistedObjects": ["Land_Grass", "ClutterCutter", ...] }`, substring match on
  objects allowed to overlap a spot.
- Vehicles are always bought one at a time.

## 9. `TraderXConfig/TraderXDynamicTranslationSettings.json`

Server-side transaction messages (`3_Game/Domain/Translation/Model/TraderXDynamicTranslationSettings.c`):
`version` + 35 strings with `%1`/`%2` placeholders (`successfullyBought`, `insufficientFunds`,
`insufficientStock`, `traderStockFull`, `purchaseFailedPrefix`, ...). Missing keys keep English defaults.
Some messages are hard-coded ("Ruined items cannot be sold", "Item condition is not accepted").

## 10. Logging: `TraderXLogs/Config/LoggingSettings.json`

`{ "logLevel": 0, "refreshRateInSeconds": 60 }`: 0 Debug, 1 Info, 2 Warn, 3 Error. **Hot-reloaded** every
`refreshRateInSeconds` and pushed to clients. Logs: `TraderXLogs/Logs/TraderX_<date>.log` (server and client each).
CSV compile report: `TraderXLogs/Config/ConfigReport.log` (file + line of every error/warning).

## 11. `TraderXConfig/TraderXDebugSettings.json`

Only `enableDebugMode` + `authorizedAdmins` (steam64 list) matter: they capture those admins' transactions into
`TraderXConfig/DebugTestCases/`. The `run*Tests` flags do nothing (suites commented out). Keep `enableDebugMode: 0`
in production.

## 12. Client-side files (player machine, informative)

`$profile:TraderX/TraderXFavorites\Fav_<serverID>.json`, `TraderXPresets/presets_<serverID>.json`, client logs.

## 13. What the client receives

On join: whole general settings, all categories, currencies, own licenses, all admin presets, log level.
On trader open: that trader's products + stock (chunks of 100). Server-only: `SourceConfig.json`, CSV/compiled
sources, debug settings, translations, parking files, stock files, other players' data.
The client computes prices and the server rejects any mismatch, so config/stock must be loaded consistently
(always restart after edits).
