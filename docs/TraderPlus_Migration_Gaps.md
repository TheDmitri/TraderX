# TraderPlus → TraderX: Migration Gaps & Feature Strategy

Companion doc to the config-converter work (filename fix + Garage/Banking/Vehicles converters).
Scope here = **features TraderPlus V1 has that TraderX lacks**, and how to build each one *better* in
TraderX's architecture (DayZular services + DTO/RPC + 3-format config) rather than porting V1 code 1:1.

Status legend: ❌ absent · ⚠️ partial / unverified · 🔁 replaced by different paradigm

---

## Converter work (the "painless migration" task — tracked separately)

Not feature gaps, just plumbing. Listed here for completeness.

| Item | Fix |
|---|---|
| **Filename mismatch** | Converter reads `TraderXPriceConfig.json` / `TraderXGeneralConfig.json`; real TraderPlus ships `TraderPlusPriceConfig.json` / `TraderPlusGeneralConfig.json`. Add filename fallback (check both) in `ConvertOldConfigToNew()`. |
| **Garage converter** | New `ConvertTraderPlusGarageConfig.c` → map NPCs/costs/timers/flags to `TraderXVehicleParkingConfig` + garage settings. |
| **Banking converter** | New `ConvertTraderPlusBankingConfig.c` → map `TransactionFees`, `DefaultStartCurrency`, `DefaultMaxCurrency`, `CurrenciesAccepted` to TraderXBanking settings. |
| **Vehicles converter** | New `ConvertTraderPlusVehiclesConfig.c` → `VehiclesParts[]` (VehicleName/Height/VehicleParts) into TraderX vehicle-parts equivalent. Needed by receipt/garage logic. |

Pattern for all three: copy `ConvertTraderPlusPriceConfig.c` shape — `old*Settings` parse class +
`ConvertToNewConfig()` + `MoveOldConfigToDeletedFolder()`. ~1–2 days total.

---

## Multi-source migration (TraderPlus, Dr Jones, Expansion Market)

TraderX now auto-imports catalogs from three external mods at server boot (server-only, before repos
load, in [TraderXModule.c](../src/TraderX/scripts/4_World/classes/TraderX/Module/TraderXModule.c)).

| Source mod | Source path | Converter | Trigger / guard |
|---|---|---|---|
| **TraderPlus V1** | `$profile:TraderPlus\TraderPlusConfig\*` (or legacy `TraderX*Config.json`) | [ConvertTraderPlusV1/](../src/TraderX/scripts/3_Game/ConvertTraderPlusV1/) + Garage/Banking in their PBOs | runs if source present; archives source to `deleted\` |
| **Dr Jones Trader** | `$profile:Trader\TraderConfig.txt` | [ConvertDrJonesTrader.c](../src/TraderX/scripts/3_Game/ConvertExternal/ConvertDrJonesTrader.c) | one-shot sentinel `.migrated_drjones` |
| **Expansion Market** | `$profile:ExpansionMod\Market\*.json` | [ConvertExpansionMarket.c](../src/TraderX/scripts/3_Game/ConvertExternal/ConvertExpansionMarket.c) | one-shot sentinel `.migrated_expansion` |

External (Dr Jones / Expansion) converters are **non-destructive** — they never touch the source mod's
files (it may still be loaded); a sentinel in `TraderXConfig\` makes each run once. Delete the sentinel
to re-run.

### What converts cleanly (catalog = the painful 95%)
- **Categories + products + prices**: className, buy price, sell price, max stock, trade quantity.
- **Dr Jones currencies** (`<Currency>` lines → TraderX currency type).
- Dr Jones quantity codes (`*`/`M` → full stack, number → static qty, `W/S/V/VNK/K` → single).

### Known mapping limitations (all logged with counts at boot)
| Limitation | Source | Why / workaround |
|---|---|---|
| **Trader NPC positions not migrated** | Dr Jones + Expansion | Positions live in object/marker files (Dr Jones) or mission objects (Expansion), not the catalog. Migrated categories must be assigned to TraderX NPCs in `TraderXGeneralSettings.json` and placed in-world. |
| **Dynamic prices flattened** | Expansion | Expansion price = stock-curve between Min/Max. TraderX uses one buy price + coefficient → we take `MaxPriceThreshold` (exact when Min==Max). |
| **Attachments/variants dropped** | Expansion | `SpawnAttachments`/`Variants` are class names; TraderX wants product-id refs. Re-add via TraderX presets/attachments. |
| **Expansion currency worth not migrated** | Expansion | Lives in market settings, not category files → TraderX default currencies used. |

These are config-shape mismatches, not data loss in the painful part (the item/price catalog). NPC
placement is the one manual step after import.

## Feature gaps

### 1. Vehicle Insurance — ❌ absent

**TraderPlus had:** `TraderPlusInsuranceConfig.json` — per-vehicle `InsurancePriceCoefficient` +
`CollateralMoneyCoefficient`, authorized trader IDs, buy-policy + claim-on-destroy.

**Why port:** high-value RP/economy feature. Lost vehicle → payout instead of total loss. Strong money
sink (premiums) + sink-balancer (payouts). Pairs naturally with Garage + Banking already in TraderX.

**Better strategy in TraderX:**
- New package `TraderXInsurance` (one PBO, mirrors Garage layout).
- **Policy = ledger-backed, not a separate DB.** Reuse `TraderXBanking` ledger: premium = labeled
  debit (`INSURANCE_PREMIUM`), payout = labeled credit (`INSURANCE_CLAIM`). No new persistence layer.
- **Policy DTO** flat & serializable: `ownerKey, vehicleClassName, vehicleUid, premiumPaid, payoutValue,
  expiresAt`. Persist as `array<ref TXInsurancePolicy>` (NOT a map — see [[dayz-jsonfileloader-no-map]]).
- **Claim trigger** server-side: hook vehicle `EEKilled`/destroy → look up active policy by vehicleUid →
  credit owner via `ITraderXPaymentProvider` (already exists for Garage fees). Zero direct coupling.
- **Pricing** rides the existing product-pricing path: insurance is just a "product" whose price =
  `vehicleBuyPrice * InsurancePriceCoefficient`. Avoids a parallel pricing engine.
- **UI**: a tab in the Garage/Trader menu, not a new menu — buy/renew/view policies. Field-Ledger tokens.

**Effort:** medium (~1 week). Biggest unknown = reliable destroy hook + matching vehicleUid to policy.

---

### 2. Receipt-based vehicle delivery — 🔁 replaced, but collateral/loadout lost

**TraderPlus had:** buy car → spawns `Receipt` box holding `CarClassName`, `Attachments[]`,
`CollateralMoney`, `CarPassword`, `CarUniqueId`. Player deploys vehicle from box anywhere
(`ActionDeployVehicle`), with full attachment loadout + collateral refund on return.

**TraderX today:** Garage = **parking marketplace** (park in/out at NPC). Different paradigm — no
portable receipt, no collateral, no "buy car with exact attachment preset and deploy in field".

**Decision needed (yours):** do you want the receipt paradigm back, or is parking-marketplace the
intended replacement? They serve different playstyles:
- Receipt = mobile, deploy anywhere, attachment presets, collateral economy.
- Garage = fixed locations, simpler, no field-spawn abuse.

**Better strategy IF you keep receipts:**
- Don't resurrect the V1 `Receipt`/`NewReceipt`/`OldReceipt` triple. One DTO: `TXVehicleReceipt`
  (`vehicleClassName, ownerKey, attachments:array<ref TXAttachmentDTO>, collateral, lockCode, vehicleUid`).
- **Heavy attachment tree stays server-side** — wire only the lean DTO ([[dayzular-rpc-handler-signature]],
  CLAUDE.md RPC rules). Client never holds live `EntityAI`.
- Reuse TraderX **item-preset system** (already exists, attachment-aware) as the loadout source — a
  receipt is just "preset + collateral + spawn token". No second loadout codepath.
- Collateral = Banking ledger label `VEHICLE_COLLATERAL_HOLD` / `_REFUND`, same provider pattern.

**Effort:** medium-high if revived (~1.5 wk). **Recommend: confirm intent before building** — may be
deliberately dropped.

---

### 3. CE-based stock auto-calc — ⚠️ unverified in TraderX

**TraderPlus had:** `ConvertTraderConfigToTraderPlusWithStockBasedOnCE` + `EnableAutoCalculation` —
seed initial stock from the map's Central Economy `cfgeconomycore` nominal values, so trader stock
roughly tracks loot-table rarity instead of hand-typed maxStock.

**Strategy:** low priority, but cheap quality-of-life for server admins onboarding.
- Implement as a **one-shot config tool**, not runtime: a `TraderXStockSeeder` invoked on demand
  (admin flag) that reads CE nominal/min per className → writes `maxStock` into the source CSV/JSON.
- Keep it out of the hot path — stock at runtime stays config-driven; this only *populates* config.
- Fits the CSV format best (admin-editable column), then compile.

**Effort:** small (~2 days) if CE config reader exists; medium if not.

---

### 4. Per-condition buy coefficients — ⚠️ field exists, verify runtime

**TraderPlus had:** `AcceptedStates` — accept Worn/Damaged/BadlyDamaged, each with a price coefficient
(e.g. Worn ×0.75), `StoreOnlyToPristineState` toggle.

**TraderX today:** `acceptedStates` field IS carried through the General converter, but **unconfirmed
the runtime sell path applies the coefficient.** Likely a 1-line-of-logic gap, not a feature build.

**Strategy:**
- Verify in the sell-transaction service: does it read item health state → multiply by the matching
  state coefficient? If not, add it where final sell price is computed (server-side, single choke point).
- Enforce-script reminder: no ternary — use `DZTernaryFloat.If(...)` for state→coeff selection.

**Effort:** tiny (~half day) — mostly verification + one multiply.

---

## Prioritization

| # | Feature | Value | Effort | Build order |
|---|---|---|---|---|
| 4 | Condition buy coeff | med | tiny | **1st — verify/cheap** |
| 1 | Insurance | high | med | **2nd** |
| 3 | CE stock seeder | low-med | small | 3rd |
| 2 | Receipt vehicles | ? (decide) | med-high | **decide first, maybe drop** |

**Recommended sequence:** confirm #4 works → build Insurance (#1, the real gap) → CE seeder (#3) →
revisit receipts (#2) only if parking-marketplace proves insufficient in playtests.

## Cross-cutting principles (apply to all)

- **No new persistence engines.** Money flows → Banking ledger w/ labels. Policies/receipts →
  `array<ref DTO>` files, rebuild maps on load. Never persist a `map<>` or a live entity.
- **One PBO per feature** (`src/<Pkg>/`), registered in `5_Mission/TraderXAppInit.c` via DayZular.
- **Wire lean DTOs only** — heavy/recursive data stays server-side (CLAUDE.md RPC rules; handler sig
  always `(PlayerIdentity sender, ParamsReadContext ctx)`).
- **Reuse existing subsystems** — pricing, presets, payment provider, ledger — don't fork parallel paths.
- **UI = tabs in existing menus**, Field-Ledger tokens, DZML-authored. No new standalone menus unless
  unavoidable.
