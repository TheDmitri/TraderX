# Side packages: Banking, Garage, CarLock, SafeZone

All under `$profile:TraderX/`. Each settings file is built from code defaults, then the JSON is loaded over it:
missing keys keep defaults. Code paths relative to `src/`.

Units trap: Banking `transactionFeePercent` is a **fraction** (0.05 = 5 %). Garage `*Percent` fields
(`repairBodyPercent`, `premiumPercentPerDay`, `deductiblePercent`) are **real percents** (2.0 = 2 %).

Admin id trap: Garage `adminSteamIds` and SafeZone `adminSteamUIDs` need the exact steam64. CarLock `admins`
matches a substring of the 9-digit low part (a full steam64 works too).

---

## TraderXBanking: `TraderXBanking/TraderXBankingConfig.json`

Model `TraderXBanking/scripts/3_Game/Config/TXBSettings.c`.

```json
{
  "version": "1.0",
  "enableLogs": 1,
  "requireCreditCard": 0,
  "transactionFeePercent": 0.05,
  "defaultStartBalance": 0,
  "defaultMaxBalance": 1000000,
  "currencies": [
    { "className": "TraderX_Money_Ruble5000", "value": 5000 },
    { "className": "TraderX_Money_Ruble100", "value": 100 },
    { "className": "TraderX_Money_Ruble1_Coin", "value": 1 }
  ],
  "bankNpcs": []
}
```

| Field | Meaning |
|---|---|
| `enableLogs` | Per-transaction log lines in `TraderXBanking/Logs/` |
| `requireCreditCard` | Bank action and trader/garage bank payment need a `TXBCreditCard` item in hands. No item class with that name is defined in this repo: leave 0 unless one exists |
| `transactionFeePercent` | Fraction, charged **on top** for deposit and transfer (withdraw is free) |
| `defaultStartBalance` / `defaultMaxBalance` | Copied into **new** accounts only. Existing accounts keep their own `maxBalance` |
| `currencies` | Cash denominations accepted/paid. With TraderX loaded, all core currency classes are merged in at face value |
| `bankNpcs` | `[{className, position, orientation, name}]`. **Standalone only** (no TraderX): with TraderX the bank is the core trader `npcId: 126`. Must not be `null` |

Per-player data (server stopped before editing):
- `Accounts/Account_<steam64>.json`: `{version, playerUID, playerName, balance, maxBalance}`. The place to refund a
  player or raise one player's cap. Lowering `maxBalance` below `balance` truncates the balance.
- `Ledgers/Ledger_<steam64>.json`: history, cached and flushed every 30 s. Never edit live.
- `Contacts/`: cosmetic. `Vaults/Vault_player_<steam64>.json`: per-vault `capacity` (default 50) and
  `feePerSlotPerDay` (default 0) live **here**, not in the config. Fee is charged when the owner opens the vault.

Importers: TraderPlus banking config (only when `TraderXBankingConfig.json` is missing; sources
`TraderXBanking/TraderPlusBankingConfig.json` then `$profile:TraderPlus/TraderPlusConfig/TraderPlusBankingConfig.json`).
Expansion ATM balances are staged in `TraderXBanking/ExpansionImport/` and credited once per player at join
(file renamed `.imported`).

---

## TraderXGarage: `TraderXGarage/TraderXGarageConfig.json`

Model `TraderXGarage/scripts/3_Game/Config/TXGSettings.c`. Version `"2.0"` (a `"1.0"` file is upgraded).

```json
{
  "version": "2.0",
  "enableLogs": 1,
  "parkInCost": 100, "parkOutCost": 100, "maxVehicleStored": 5,
  "vehicleMustHaveLock": 0, "saveVehicleLock": 1, "saveVehicleCargo": 1,
  "saveVehicleHealth": 1, "saveVehicleFuel": 1, "payWithBank": 1,
  "parkRadius": 15.0,
  "enableRepairs": 1, "repairFluidCost": 100, "repairBodyCost": 300, "repairBodyPercent": 10,
  "repairWornPartCost": 150, "repairRuinedPartCost": 300,
  "impoundFee": 500, "adminSteamIds": [],
  "noRetrieveZones": [ { "name": "Trader", "position": [11620.0, 0.0, 14670.0], "radius": 150.0 } ],
  "insurance": {
    "enabled": 0, "recoveryMode": "INSURED_ONLY", "premiumPercentPerDay": 2.0,
    "minDays": 1, "maxDays": 30, "deductiblePercent": 25.0, "deductibleMin": 500,
    "claimCooldownHours": 24, "maxActivePoliciesPerPlayer": 3, "replacementNoResaleHours": 72,
    "replacementSpec": "CATALOG_PRESET", "holdCountsAgainstSlots": 0, "insurableClasses": []
  },
  "vehicleValues": [ { "className": "OffroadHatchback", "buyPrice": 20000, "sellPrice": 8000 } ],
  "npcs": [ {
    "npcId": 125, "className": "SurvivorM_Mirek", "name": "Garage",
    "position": [11632.63, 58.13, 14658.4], "orientation": [250.0, 0.0, 0.0],
    "parkSpots": [ { "position": [11633.36, 57.81, 14650.17], "orientation": [250.0, 0.0, 0.0], "size": [6.0, 3.0, 12.0] } ]
  } ],
  "currencies": [ { "className": "TraderX_Money_Ruble100", "value": 100 } ]
}
```

| Field | Meaning |
|---|---|
| `parkInCost` / `parkOutCost` | Fees (≤ 0 = free). Held vehicles use their `releaseFee` instead of `parkOutCost` |
| `maxVehicleStored` | Slots per player |
| `vehicleMustHaveLock` / `saveVehicleLock` | Need TraderX **and** TraderXCarLock loaded, otherwise no effect |
| `saveVehicleCargo/Health/Fuel` | What is restored on park-out |
| `payWithBank` | Try bank account first, then cash (core currencies with TraderX, else `currencies`) |
| `parkRadius` | Search radius around the **player** for the vehicle to park |
| `parkClaimDistance` | Unused (may appear in generated files) |
| `enableRepairs`, `repair*` | Repair menu costs; body = `repairBodyCost + ceil(value · repairBodyPercent %)` |
| `impoundFee`, `adminSteamIds` | Admins (exact steam64) can impound; release costs `impoundFee` |
| `noRetrieveZones` | `{name, position, radius}`, 2D: park-out refused inside |
| `insurance` | `enabled`; `recoveryMode` `NONE` / `INSURED_ONLY` / `ALL_OWNED`; premium = `ceil(value · pct % · days)`; deductible = `max(ceil(value · pct %), deductibleMin)`; `replacementSpec` `CATALOG_PRESET` / `BARE`; `insurableClasses` empty = any class with a known value |
| `vehicleValues` | Fallback value table used only when the TraderX catalog has no price for the class |
| `npcs` | Garage attendants, spawned by the garage itself (also with TraderX). `parkSpots[].size` `[0,0,0]` skips the collision check. Player must be within 12 m of the NPC |
| `currencies` | Standalone cash table; `npcs`/`currencies` must not be `null` |

Rules:
- One garage → `npcId: 125`. Several garages → other ids that are **not used by any core trader** (the client
  treats every configured garage id as a garage). The core trader list's own 125 entry is skipped automatically.
- A garage left at the default Chernarus coordinates is skipped on other maps: always set real coordinates.

Per-player data: `Database/Garage_<steam64>.json` (stored vehicles; with the server stopped you may delete an entry or
clear `holdReason`; never touch `nextVehicleId`), `Database/PlayerIndex.json` (delete to rebuild),
`Insurance/Policies_<steam64>.json` (stamped on vehicles, do not edit), `DeadClasses.txt` (boot report of stored
classes that no longer exist).

Importer: TraderPlus garage config + stored vehicles, only when `TraderXGarageConfig.json` is missing. Not mapped:
`VehicleMustHaveLock`, `SaveVehicleLock`, whitelist. Several TraderPlus garages get ids 110, 111... (check collisions).

---

## TraderXCarLock: `TraderXCarLock/TXCLSettings.json`

Model `TraderXCarLock/scripts/3_Game/TXCLSettings.c`. Whole package compiles out if the TraderPlus `CarLock` define
is present. The file is never re-saved: new fields must be added by hand.

```json
{
  "version": "1.0",
  "carLockClassName": "CarLock",
  "passwordLength": 8,
  "initCooldownTime": 10,
  "admins": [],
  "lockpickingSettings": {
    "carConfig":  { "lockpickClassName": "CarLockPick",  "lockpickTime": 60.0,  "lockpickChance": 0.33, "lockpickDamagePerAttempt": 0.34, "consumeLockpickOnSuccess": 0, "consumeLockpickOnFailure": 0 },
    "boatConfig": { "lockpickClassName": "BoatLockPick", "lockpickTime": 45.0,  "lockpickChance": 0.40, "lockpickDamagePerAttempt": 0.25, "consumeLockpickOnSuccess": 0, "consumeLockpickOnFailure": 0 },
    "heliConfig": { "lockpickClassName": "HeliLockPick", "lockpickTime": 120.0, "lockpickChance": 0.15, "lockpickDamagePerAttempt": 0.50, "consumeLockpickOnSuccess": 0, "consumeLockpickOnFailure": 0 }
  }
}
```

- `carLockClassName`: lock item (defined in `src/Keypads/config.cpp` as `CarLock`).
- `passwordLength`: not enforced, the UI is always 8 digits.
- `initCooldownTime`: seconds of lockout after a wrong code, doubled at each further failure.
- `admins`: bypass codes.
- Lockpick family: BoatScript → boat; CarScript with "heli" in the type → heli; else car. `lockpickChance` 0-1;
  `lockpickTime` seconds. **No `CarLockPick`/`BoatLockPick`/`HeliLockPick` item is defined in this repo**: point
  them at real item classes of the server (e.g. `Lockpick`).
- Never delete or change `storage_mode.txt`. `Database/<steam64>.json` is runtime state (`_isAdmin` recomputed).
- Lock data itself is stored on the vehicle (CF ModStorage), not in JSON.

---

## TraderXSafeZone: `TraderXSafeZone/TXSafeZoneSettings.json`

Model `TraderXSafeZone/scripts/3_Game/SafeZoneSettings.c` + `SafeZoneLocation.c`. Requires TraderX core. A legacy
`TraderXConfig/TraderXTXSafeZoneSettings.json` is moved here once.

```json
{
  "version": "1.0.0",
  "notificationTimer": 10,
  "notificationTitle": "SafeZone",
  "msgOnEnteringZone": "You entered the safezone!",
  "msgOnLeavingZone": "You have left the safezone!",
  "leaveZoneCountdown": 30,
  "msgOnLeavingZoneCountdown": "You are leaving the safezone in %1 seconds",
  "adminSteamUIDs": [],
  "enableAfkDisconnect": 0,
  "kickAfterDelay": 300,
  "cleanupTimer": 1200,
  "objectsToDelete": [],
  "locations": [
    { "id": 1, "isActive": 1, "name": "Krasnostav Trader", "position": [11620.0, 0.0, 14670.0],
      "radius": 150, "isEntitiesCleanActive": 1, "allowedAnimals": [] }
  ]
}
```

- First boot writes a placeholder zone `"Test"` at `[0,0,0]` r10 and admin `76561198000000000`: **replace both**.
- Zones are 2D circles (Y ignored), `radius` int metres, `isActive` 0 disables.
- Inside: no damage, no melee, some actions disabled (admins exempt). `leaveZoneCountdown` seconds of grace after exit.
- `isEntitiesCleanActive`: zombies and animals not in `allowedAnimals` are deleted in the zone.
- `cleanupTimer` seconds between sweeps deleting `objectsToDelete` classes (exact type names) in zones; ≤ 0 = off.
- `enableAfkDisconnect` + `kickAfterDelay` seconds: AFK kick inside zones.
- The whole file (including admin list) is sent to every client.
- With core `prewarmCatalogInSafeZone: 1`, entering a zone preloads nearby trader catalogs: put each trader
  inside a zone.

---

## No config: TraderPlusATM, TraderPlusMoney, Keypads

Item/class definitions only: `TraderPlus_BANK_ATM` (bank terminal object), `TraderX_Money_*` notes and coins,
`CarLock` item.
