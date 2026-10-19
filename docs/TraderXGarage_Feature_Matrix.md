# TraderXGarage — Feature Matrix & Lean Architecture Proposal (2026-10-15)

> **Deliverable 1 (No-Code Design)** per `docs/handoff/GARAGE_HANDOFF.md` §7.1.
> **Core Mandate:** Achieve full feature parity with *Modern Garage* (and superior Field-Ledger UI + in-world preview) in **~3,500–4,500 LOC** (vs. Modern Garage's **81,585 LOC**), using idiomatic **TraderX + DayZular + DZML** with zero Java-style over-engineering.

---

## 1. Why Modern Garage is 81,585 LOC — and How We Do It in ~4,000 LOC

| Area | Why Modern Garage is bloated (~81.5k LOC) | Our Lean TraderX Approach (~4k LOC) | Est. LOC |
|---|---|---|---|
| **Self-Tests** | ~15,000+ LOC of in-mod `*SelfTest.c` files shipped in the PBO | Zero shipped test bloat; validate via `dayz-autotest` harness & live in-game | **0** |
| **UI Layer** | `MG_Menu.c` (7,592 LOC) + `MG_CatalogEditor.c` (1,903 LOC) of manual `FindAnyWidget` / `SetText` / imperative DOM | Declarative **DZML (`.html` → `.layout`)** + `DZlrComponentBase` (`TextBinding`, `ButtonBinding`, `Component_ForEach`) | **~650** |
| **Cargo / State Persistence** | `MG_Cargo.c` + `MG_CargoScan.c` + `MG_NativeState.c` (~3,150 LOC) custom `.bin` binary stream + browser repair tool | Existing JSON `TXGItemSnapshot` tree extended with `array<ref TXGZoneHealth>` + 4 fluid floats + `ConfigIsExisting` skip on restore | **~300** |
| **Vehicle Lock & Ownership** | `MG_VehicleLock` + `MG_LockSync` + `MG_VehicleOwners` (~4,200 LOC) reinventing a lock mod inside the garage | Delegate directly to **`ITraderXCarLockProvider`** (`TraderXCarLock` PBO) + lightweight CF ModStorage owner stamp when standalone | **~200** |
| **Banking Adapters** | `MG_Funds` + `MG_Currency` + 4 external bank adapters (~2,200 LOC) | Direct call to **`ITraderXPaymentProvider`** (`TraderXBanking`) + existing 40-line `RemoveCashFromPlayer` / `AddCashToPlayer` fallback | **~120** |
| **Catalog & Dealer** | `MG_Catalog.c` (3,132 LOC) + custom price DB | Query **`TraderXProductRepository`** when TraderX core is loaded; simple optional `vehicles[]` price list in `TXGSettings` when standalone | **~350** |
| **In-World Preview** | `MG_Preview.c` + `MG_PreviewSlots.c` (~1,500 LOC) | Clean `TXGPreviewService` (~220 LOC): client-local `ECE_LOCAL` entity + `LocationCreateLocalEntity` for fitted parts + orbit/viewport framing | **~220** |
| **Core Garage + Repair + Recovery + Admin** | `MG_Garage` (3,242) + `MG_ServerRPC` (3,377) + `MG_Repair` (1,985) + `MG_Market` (3,900) | Focused `TXGGarageService`, `TXGRepairService`, `TXGRPCService` with flat DTOs | **~1,600** |

---

## 2. Feature Matrix (Modern Garage → TraderXGarage)

### Priority Legend
- **P0 — Core Parity & Differentiators (Phase 1):** Essential garage loop, exact vehicle fidelity, CarLock/Banking integration, in-world 3D preview with fitted parts, multi-NPC + aim-to-spawn option, Field-Ledger UI.
- **P1 — High-Value Features (Phase 2):** Per-category repair/service, wreck recovery (insurance replacement), vehicle sharing, condition-aware dealer integration, admin garage browser & impound.
- **P2 — Optional / Later (Phase 3):** Auto-impound idle vehicles (parking meter), in-game admin config tweak. *(Note: P2P player vehicle market is an explicit non-goal in `SCOPE.md` §2 unless requested later).*

| # | Feature | Priority | Modern Garage Behaviour (Studied) | Our Lean TraderX Design (Service / DTO / RPC / UI) | Depends On |
|---|---|---|---|---|---|
| **1** | **Owner & Lock-Checked Park-In** | **P0** | Checks vehicle ownership stamp and lock state; refuses occupied vehicles or someone else's locked car. | In `TXGGarageService.ParkIn`: check `!IsCarOccupied(veh)`. If `TraderXProviderRegistry.HasCarLock()`, verify `GetOwner(veh) == "" \|\| GetOwner(veh) == uid` (or unlocked + unowned) and persist `lockCode` / owner on `TXGVehicleData`. Support `Transport` (`CarScript`, `BoatScript`, 1.30 `MotorbikeScript`). | `ITraderXCarLockProvider` (optional, graceful fallback) |
| **2** | **Exact State Snapshot & Restore** | **P0** | Saves & restores all damage zones (`GetDamageZones` → `GetHealth01`/`SetHealth01`), all 4 fluids (`FUEL`, `OIL`, `COOLANT`, `BRAKE`, plus Boat/Motorbike fuel), recursive attachments (exact slot ID) & nested cargo quantities/ammo/foodstage/energy/wetness/liquid. | Extend `TXGVehicleData` (schema v2, auto-migrated from v1): add `ref array<ref TXGZoneHealth> zones` (`zoneName`, `health01`), `oilFraction`, `coolantFraction`, `brakeFraction`, and `ref array<string> attachmentClasses` (fast summary for preview without traversing cargo). On restore: iterate `zones` → `SetHealth01`, restore all 4 fluids (instead of topping up oil/coolant/brake for free), restore attachments + cargo recursively. | None (`TXGGarageService`) |
| **3** | **Dead-Class Tolerance & Boot Report** | **P0** | If a mod was removed, stored vehicles/cargo with missing classes don't brick retrieval; skipped items are logged and reported to the player, and listed at boot. | In `RestoreChildren`: check `GetGame().ConfigIsExisting("CfgVehicles " + c.item)` (or `CfgMagazines` / `CfgWeapons`). If missing, increment `skippedCount`, log `[TXG] Missing class skipped: <item>`, and continue. At server boot, scan indexed player JSONs via `TXGPlayerIndex.json` and log a concise warning summary if any stored vehicle or cargo class is missing. | `TXGPlayerIndex.json` (no `FindFile` needed on 1.30) |
| **4** | **1.30-Safe Storage Index & Versioned Migration** | **P0** | Keeps `PlayerIndex.txt` so it never relies on broken 1.30 `FindFile` directory listing. | Maintain `$profile:TraderX/TraderXGarageDatabase/PlayerIndex.json` (`array<string> playerUids` + `lastKnownNames`). Updated whenever a player joins or saves `Garage_<steam64>.json`. `TXGGarageData.version = 2` transparently upgrades v1 files (`zones` empty → falls back to `record.health`). | None |
| **5** | **Per-Player Limits (Stored + World)** | **P0** | Counts vehicles owned by the player in storage (and optionally in the world) against `maxVehicleStored`. | Enforce `maxVehicleStored` in `TXGGarageService.ParkIn` and `AddVehicle`. Optional setting `countWorldVehicles` in `TXGSettings` if server wants world-spawned owned cars counted too. | `TXGSettings` |
| **6** | **Multi-NPC Garages & Boat/Water Support** | **P0** | Multiple garage locations; supports cars, motorbikes (1.30), and boats (water depth check). | Fix `ActionTXGOpenGarage` so it accepts **any** NPC ID configured in `TXGSettings.npcs` (not hardcoded to `125`, while keeping `125` and `110+i` working). Remove hardcoded Chernarus sign spawn if map != Chernarus or when NPCs are managed by TraderX. Support `BoatScript` & `MotorbikeScript` in `FindNearbyVehicle` and `CreateVehicle`. | `ActionTXGOpenGarage`, `TXGNpcService` |
| **7** | **In-World 3D Vehicle Preview (with Fitted Parts)** | **P0** | Spawns a client-local (`ECE_LOCAL`) physics-disabled vehicle with its actual stored attachments (`LocationCreateLocalEntity` + `EEItemAttached`), framed in the UI viewport or at the parking spot, with mouse drag rotation & zoom. | **`TXGPreviewService` (client):** When a vehicle row is selected in `TXGGarageComponent`, spawn `ECE_LOCAL \| ECE_NOLIFETIME` entity, strip physics (`dBodyDestroy`, `DisableSimulation(true)`), attach every class in `TXGVehicleRowVM.attachments` via `CreateAttachmentEx` / `LocationCreateLocalEntity` + `EEItemAttached`. Support two modes toggleable in UI: **(A) Studio Viewport** (projected into the DZML preview frame with drag-orbit & scroll-zoom) and **(B) Spot Hologram** (placed at the NPC's first free `TXGParkSpot` so the player sees exactly where it will spawn). Destroyed immediately on close/deselect. | `TXGVehicleRowVM` (adds `array<string> attachments`, `array<int> attachmentHealthLevels`) |
| **8** | **Field-Ledger DZML UI Rework** | **P0** | Clean single-window UI showing vehicle list, condition breakdown, fitted attachment strip, preview, and action costs. | Rework `GaragePageView.html` + `GarageVehicleRow.html` using Field-Ledger tokens (`TX*`):<br>• Left column: stored vehicle list (`Component_ForEach`) with health badge, fuel bar, lock badge, hold/wreck tag.<br>• Center: live 3D preview (`TXGPreviewService`) + attachment slot strip (wheels, doors, battery, plug, radiator status icons) + "Preview at Spot" toggle.<br>• Right column: condition bars (Chassis, Engine, Fuel Tank, Fluids), Park-In / Retrieve / Service / Share buttons with clear fee display. | DZML pipeline (`dzml compile`) |
| **9** | **Per-Category Vehicle Repair (Service)** | **P1** | Repairs stored vehicle in 4 independent categories: **Fluids** (refill fuel/oil/coolant/brake), **Body** (chassis/zones), **Worn Parts** (attachments with `0 < health < 1`), **Ruined Parts** (replace ruined attachments). Priced via flat + % of vehicle value. | Add `RepairVehicle(player, vehicleId, categoryFlags)` to `TXGGarageService` (under 150 LOC!). Operates directly on the stored `TXGVehicleData` JSON (sets `zones[].health01 = 1.0`, `fuel/oil/coolant/brake = 1.0`, or attachment `health = max`) and charges via `ChargeFee(..., "GARAGE_REPAIR")`! Instant UI refresh. | `ITraderXPaymentProvider` |
| **10** | **Vehicle Insurance (Anti-Exploit Policy System, per `INSURANCE_SPEC.md`)** | **P1** | Modern Garage uses unconditional wreck recovery (`MG_Recovery`); TraderPlus paid cash collateral (exploitable). | **Replaced by `INSURANCE_SPEC.md`:**<br>• **No money payout ever** — claim yields a replacement `TXGVehicleData` in the owner's garage with `holdReason = "INSURANCE"` and `releaseFee = deductible`.<br>• **Server-only `EEKilled` trigger**; deletion/sale/cleanup voids the policy (`EEDelete` → `VOID DELETED`, trader sale → `VOID SOLD`).<br>• **CF ModStorage stamp** on vehicle (`policyId`, `ownerUid`, `stampState`, `noResaleUntil`).<br>• **Wreck stays lootable** in world (`stampState = CLAIMED`); replacement is a fresh vehicle (`CATALOG_PRESET` or `BARE`, no cargo) with `noResaleUntil` blocking trader resale.<br>• Optional `recoveryMode = "ALL_OWNED"` for admins wanting Modern-Garage-style deductible-only recovery. | `ITraderXCarLockProvider`, `ITraderXCatalogProvider`, `ITraderXGarageProvider` (`IsResaleBlocked`, `OnVehicleSold`), CF ModStorage |
| **11** | **Vehicle Sharing** | **P1** | Owner can grant named players permission to retrieve/park a shared vehicle. | Add `array<string> sharedWithUids` on `TXGVehicleData` (or garage-level `trustedUids` in `TXGGarageData`). One RPC `RequestShareAdd` / `RequestShareRemove`. When opening garage, also list vehicles shared with `sender` so friends/clanmates can park & retrieve them. | None (~120 LOC) |
| **12** | **Admin Garage Browser & Impound** | **P1** | Admins (listed in config `adminSteamIds`) can browse any player's garage from `PlayerIndex`, force-spawn, free-repair, unstick, or impound a nearby world vehicle with a reason & release fee. | Add an `ADMIN` button (visible only if `isAdmin` in `TXGConfigDTO`). Uses `PlayerIndex.json` to list players, loads target player's `TXGGarageData`, and lets admin **Impound Nearby Vehicle** (parks it into owner's garage with `holdReason` + `releaseFee`), **Release Hold**, **Repair Free**, or **Delete**. All actions logged with `[TXG AUDIT]`. | `TXGPlayerIndex.json` |
| **13** | **Condition-Aware Dealer Pricing** | **P1** | Selling a vehicle deducts needed repair costs + missing parts from the catalog sell price; refuses vehicles with cargo inside. | Connect with `TraderXVehicleTransactionService` (core) via `ITraderXGarageProvider` (or new `GetVehicleConditionQuote` method on the provider) so TraderX's existing vehicle trader can buy/sell directly into/from the player's garage with condition deductions! Avoids duplicating a second vehicle shop UI inside Garage. | `TraderX` core (`ITraderXGarageProvider`) |
| **14** | **Idle Auto-Impound (Parking Meter) & No-Retrieve Zones** | **P2** | Boot/periodic pass that auto-impounds owned vehicles left unused for `N` hours; zones where retrieval is blocked. | Simple boot check or low-frequency timer on server + optional `noRetrieveZones[]` check in `ParkOut`. | Optional |

---

## 3. Proposed File & Class Layout (`src/TraderXGarage`)

Keeping the existing structure and contracts (`ITraderXGarageProvider`, `TraderXGarageVehicleInfo`, `Garage_<steam64>.json` compatibility), we only add focused classes:

```text
src/TraderXGarage/
  scripts/
    3_Game/
      Constants/TXGConstants.c         — versions, RPC names, ledger labels, hold types
      Config/TXGSettings.c             — existing fields + repair/recovery rates + adminSteamIds
      Models/
        TXGGarageData.c                — versioned per-player JSON + v1->v2 migration + PlayerIndex update
        TXGVehicleData.c               — adds zones[], oil/coolant/brake, holdReason, releaseFee, sharedWith[]
        TXGItemSnapshot.c              — unchanged (recursive attachment + cargo snapshot)
        TXGParkSpot.c                  — unchanged
        TXGDtos.c                      — flat RPC DTOs (Config, VehicleRowVM, RepairRequest, ShareRequest, AdminDTO)
        TXGVehicleRowVM.c              — flat UI view-model (adds fuel/body/engine %, attachment list, hold fee)
    4_World/
      Actions/ActionTXGOpenGarage.c    — matches ANY npcId in TXGSettings.npcs (plus 125 default)
      Entities/                        — lightweight EEKilled hook on CarScript/BoatScript for P1 wreck recovery
      Providers/TXGGarageProvider.c    — implements ITraderXGarageProvider (unchanged contract)
      Services/
        TXGGarageService.c             — ParkIn (owner/lock check), ParkOut, exact snapshot/restore, dead-class skip
        TXGRepairService.c             — P1: quote & apply per-category repairs (Fluids, Body, Worn, Ruined)
        TXGPreviewService.c            — client-only ECE_LOCAL 3D vehicle + attachments preview & spot hologram
        TXGRPCService.c                — all server/client RPCs with rate limits & (PlayerIdentity, ParamsReadContext)
        TXGConfigService.c             — settings load/sync
        TXGNpcService.c                — standalone NPC spawn (map-aware default)
        TXGUIService.c                 — menu open/close
      Components/GUI/
        TXGGarageComponent.c           — main garage view controller (bindings + preview + repair/share/admin actions)
        TXGVehicleRowComponent.c       — vehicle list row component
        TXGSlotIconComponent.c         — attachment status chip in the details panel
```

---

## 4. Design Decisions Validated by Dmitri (2026-10-15)

1. **Retrieval Placement → NPC `parkSpots[]` ONLY (Validated):**
   - No "retrieve by aiming at the ground" (unrealistic / arcade). Vehicles always spawn at the interacted NPC's configured `parkSpots[]` (first free collision-checked spot).
   - The **in-world preview** (`ECE_LOCAL` with fitted attachments) will render in the UI viewport AND/OR on the NPC's target `parkSpot`.
2. **Dealer (Buy/Sell Vehicles) → Handled by TraderX Core (Validated):**
   - No duplicate vehicle shop/catalog inside `TraderXGarage`. Vehicle trading stays in `TraderX` core (`TraderXVehicleTransactionService`) and talks to Garage through `ITraderXGarageProvider`. Standalone `TraderXGarage` remains a pure garage (store, retrieve, preview, repair, share, admin/impound).
3. **Vehicle Insurance (replaces Wreck Recovery) → Spec Validated (`docs/handoff/INSURANCE_SPEC.md`):**
   - Follow `docs/handoff/INSURANCE_SPEC.md` strictly: policy-based, server-observed `EEKilled` only, CF ModStorage stamp on vehicle, replacement vehicle held in garage for deductible (never cash payout), wreck stays in world as `CLAIMED`, `noResaleUntil` blocks trader resale.
4. **Phase 1 Scope (P0) → GO Validated by Dmitri:**
   - Implement P0 now (Owner/CarLock park-in & round-trip, exact damage-zones + 4-fluids restore, dead-class tolerance + `PlayerIndex.json`, multi-NPC fix, in-world `ECE_LOCAL` preview with fitted attachments + spot hologram, and Field-Ledger DZML UI), followed by P1 (`INSURANCE_SPEC.md`, per-category repairs, sharing, admin/impound).
