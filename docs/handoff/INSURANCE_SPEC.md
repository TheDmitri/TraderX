# Spec — Vehicle insurance in TraderXGarage (replaces Feature Matrix #10 "Wreck Recovery")

Written 2026-10-15 for the agent implementing `docs/TraderXGarage_Feature_Matrix.md`. Read
`docs/handoff/GARAGE_HANDOFF.md` first (rules: DayZular RPC, DZML, DayZ 1.30, CF ModStorage, clean room).
Dmitri validates scope before you code. This spec supersedes the "Wreck Recovery" row: recovery
happens **only through an insurance policy** unless an admin opts into the Modern-Garage-like mode.

## 1. Why TraderPlus insurance was exploited (what we must not repeat)

TraderPlus paid **money** (collateral) when a player claimed, and the claim was decided from client-side
data: the server never checked that the car was really destroyed, and a car could be sold at another
trader/mod (outside TraderPlus' control) while its insurance stayed claimable. Result: players insured a
car, sold it elsewhere, then claimed — money from nothing.

## 2. Principles (the anti-exploit rules — every one is mandatory)

1. **No money payout, ever.** A claim gives back a **vehicle** (in the owner's garage), never currency.
2. **Only the server decides a claim**, and only on a server-observed destruction (`EEKilled` on the
   server). Disappearance, deletion by another mod, CE cleanup without a kill, or a sale → the policy is
   **voided**, not claimed.
3. **The policy is bound to one physical vehicle** through a persistent id stored on the entity with
   **CF ModStorage** (never raw `OnStoreSave`). A vehicle without our stamp is uninsured.
4. **Only the owner can insure, and only at a garage** (vehicle parked or being retrieved). Owner = the
   garage record owner; when TraderXCarLock is present the lock owner must match too.
5. **Value comes from the server catalog**, never from the client: TraderX product buy price of the class
   (core), else `TXGSettings.vehicleValues` (standalone), else the vehicle is not insurable.
6. **A replacement vehicle cannot be turned into money**: it carries a `noResaleUntil` stamp; TraderX
   vehicle trading refuses to buy it (or buys at 0) until it expires.
7. **One claim per incident, no double objects**: the wreck stays in the world as a normal wreck (lootable,
   PvP value preserved); the replacement is a **fresh** vehicle built from the coverage spec — no cargo,
   no copy of the wreck's parts. The wreck's stamp is set to `CLAIMED` so it can never claim again.
8. Every policy event writes one greppable log line and a bank ledger entry (premium, deductible).

## 3. Player flow

1. **Subscribe** (garage UI, on a stored vehicle): shows coverage value, premium for N days, deductible,
   rules. Pays the premium (bank via `TXBPaymentProvider`, else cash). Policy `ACTIVE` until `expiresAt`.
   Extending = paying more days. Selling or scrapping the vehicle voids it (no refund).
2. **Drive normally.** The vehicle carries `policyId` + `ownerUid` in its TXG ModStorage block.
3. **Vehicle destroyed** (server `EEKilled`): if the stamp's policy is `ACTIVE`, not expired, owner matches,
   cooldown elapsed → policy goes `CLAIMED`, a `TXGVehicleData` with `holdReason = "INSURANCE"` and
   `releaseFee = deductible` is added to the owner's garage (does **not** count against slot limits until
   released, configurable), wreck stamp → `CLAIMED`. Owner gets a notification if online, else on login.
4. **Release** (garage UI): pay the deductible → the hold becomes a normal stored vehicle (fresh, full
   health, coverage-spec parts, no cargo) with `noResaleUntil = now + replacementNoResaleHours`. The
   policy ends (`CLAIMED`); the player may buy a new policy for the replacement.

## 4. Data model

### 4.1 Server record (authoritative)
`$profile:TraderX/TraderXGarage/Insurance/Policies_<steam64>.json`, listed in the existing
`PlayerIndex.json` (no `FindFile`).

```c
class TXGInsurancePolicy        // flat, JSON-persisted, never sent whole over RPC
{
    int    version = 1;
    string policyId;            // UUID, also stamped on the vehicle
    string ownerUid;            // steam64
    string vehicleClassName;    // lowercase
    int    coveredValue;        // catalog price at signing (server)
    int    premiumPaid;         // total paid, for the ledger/report
    int    startedAt;           // unix seconds (server clock)
    int    expiresAt;
    int    status;              // ETXGPolicyStatus
    int    claimedAt;
    string voidReason;          // "SOLD", "DELETED", "OWNER_CHANGED", "ADMIN", "EXPIRED"
}
enum ETXGPolicyStatus { ACTIVE, EXPIRED, CLAIMED, VOID }
```

### 4.2 Vehicle stamp (CF ModStorage, key = the TXG `CfgMods` class, add `storageVersion`)
`policyId` (string), `ownerUid` (string), `stampState` (int: `NONE`, `INSURED`, `CLAIMED`),
`noResaleUntil` (int). Pattern to copy: `TraderXCarLock` `CF_OnStoreSave/CF_OnStoreLoad`
(`src/TraderXCarLock/scripts/4_World/Entities/CarScript.c`). Never return `false` from `OnStoreLoad`.

### 4.3 Settings (`TXGSettings`, new block, everything off by default)
```json
"insurance": {
  "enabled": false,
  "recoveryMode": "INSURED_ONLY",          // NONE | INSURED_ONLY | ALL_OWNED (Modern-Garage-like, deductible only)
  "premiumPercentPerDay": 2.0,             // of coveredValue
  "minDays": 1, "maxDays": 30,
  "deductiblePercent": 25.0, "deductibleMin": 500,
  "claimCooldownHours": 24,                // per owner
  "maxActivePoliciesPerPlayer": 3,
  "replacementNoResaleHours": 72,
  "replacementSpec": "CATALOG_PRESET",     // CATALOG_PRESET (TraderX product preset/attachments) | BARE
  "holdCountsAgainstSlots": false,
  "insurableClasses": []                   // empty = every class that has a catalog value
}
```
Economic guard (enforced, see DECISIONS D-013): the deductible is raised to the vehicle sell price at the trader when lower,
for every insurable class, so destroy-and-claim can never beat selling.

## 5. Contracts

Keep `ITraderXGarageProvider` intact; **add** (document each as an ADR in `DECISIONS.md`):

| Port | Method | Implemented by | Used by |
|---|---|---|---|
| `ITraderXGarageProvider` (+) | `bool IsResaleBlocked(EntityAI vehicle)` | TraderXGarage | TraderX core `TraderXVehicleTransactionService` before buying a vehicle from a player |
| `ITraderXGarageProvider` (+) | `void OnVehicleSold(EntityAI vehicle, string sellerUid)` | TraderXGarage (voids policy, reason `SOLD`) | TraderX core after a vehicle sale |
| new `ITraderXCatalogProvider` | `int GetVehicleValue(string className)` (−1 = unknown) | TraderX core (product buy price) | TraderXGarage (coverage value, deductible) |

Standalone (no TraderX core): `IsResaleBlocked` is simply unused, value comes from
`TXGSettings.vehicleValues`. Vehicles sold through **other** mods disappear without `EEKilled` → voided
by rule 2.

## 6. Server hooks (TXG package, `4_World/Entities`, wrap in the usual standalone guards)

- `CarScript/BoatScript.EEKilled(Object killer)` (server only): read stamp → `TXGInsuranceService.OnVehicleKilled(this)`.
- `CarScript/BoatScript.EEDelete(EntityAI parent)` (server only): if stamp `INSURED` and not killed and not
  being parked by our own `TXGGarageService` (set a transient `m_TXG_GarageRemoval` flag before deleting
  on park-in) → `VoidPolicy(policyId, "DELETED")`.
- Park-in: policy stays `ACTIVE`, stamp is captured in `TXGVehicleData` (add `policyId`, `noResaleUntil`)
  and re-applied on retrieval. A vehicle retrieved by someone else (sharing/admin give) keeps the policy
  owner; `OWNER_CHANGED` voids it.
- Expiry: checked lazily (on claim, on garage open, at boot), no timers.

## 7. Services, RPC, UI

- `TXGInsuranceService` (server): `Quote`, `Subscribe`, `Extend`, `OnVehicleKilled`, `VoidPolicy`,
  `ReleaseClaim`, `ListPolicies(uid)`, boot validation of settings.
- RPCs (rate-limited, `(PlayerIdentity sender, ParamsReadContext ctx)`): `RequestInsuranceQuote(vehicleId, days)`,
  `RequestInsuranceSubscribe(vehicleId, days)`, `RequestInsuranceRelease(vehicleId)`; responses send flat
  `TXGInsuranceQuoteDTO {vehicleId, coveredValue, premium, deductible, days, expiresAt, status}`.
- UI (Field-Ledger, DZML): insurance badge on the row (`ACTIVE n days`, `CLAIM — pay X to release`),
  a panel in the details column with quote/subscribe/extend and the rules in plain words, release button
  with fee. All strings in the stringtable.

## 8. Migration (TraderPlus)

TraderPlus `Account_*.json` `Insurances{carUniqueId: "Type,collateral"}` cannot be mapped to vehicles
(TraderPlus ids are raw data on vehicle saves). Do not convert policies. Proposed (Dmitri to confirm):
**refund the collateral** into the player's TraderX bank balance and log it in
`MigrationReport_TraderPlus.txt`. The migration converter owner (other session) will implement it;
expose nothing for it.

## 9. Test scenarios (all must pass in game on 1.30 before "done")

1. Insure, destroy (explosion / ram / ruin) → hold appears, pay deductible → fresh vehicle, no cargo.
2. Destroy again within cooldown → no claim, log line.
3. Insure, sell at TraderX vehicle trader → policy `VOID SOLD`; destroying the next car does nothing.
4. Insure, delete with admin tool / let another mod remove it → `VOID DELETED`, no hold.
5. Release a replacement, try to sell it at a trader → refused until `noResaleUntil`.
6. Policy expired → destruction → no claim.
7. Non-owner tries to insure / claim → refused.
8. Server restart between kill and login → hold still there, notification on login.
9. Park-in and retrieve an insured vehicle → policy kept, stamp restored, no `VOID DELETED`.
10. CarLock absent / TraderX core absent (standalone) → still works with settings values.
11. Wreck remains lootable; its stamp is `CLAIMED`; repairing/salvaging it can't claim again.
12. Config with `deductible < sell price` → boot CONFIG warning naming the classes.
