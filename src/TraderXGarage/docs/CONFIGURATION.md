# TraderXGarage — Configuration

**One config file. One place. No second config anywhere.**

```
$profile:TraderX/TraderXGarage/TraderXGarageConfig.json
```

**One `npcs` entry = one garage.** Each garage has an attendant (the NPC you talk to) and one or
more **park spots** (where retrieved vehicles appear). The mod **spawns and tags its own
attendant** whether TraderX is loaded or not — you never touch TraderX's NPC/trader config.

## How it works (you don't configure this)

- **TraderX loaded:** the garage spawns its NPC and tags it with your `npcId` using TraderX's own
  NPC setup, but does **not** mark it as a trader — so talking to it opens the **garage**, never a
  trade menu.
- **Standalone (no TraderX):** the garage spawns the NPC and matches it by `className`.

Either way: everything lives in the file above.

## Fields

```jsonc
{
  "npcs": [
    {
      "npcId": 125,                      // 0..127, unique per garage. Ties the attendant to its spots.
      "className": "TraderPlusSign",     // the NPC/object spawned (survivor, sign, building...)
      "position": "11632.6 58.1 14658.4",
      "orientation": "250 0 0",
      "name": "Garage",
      "parkSpots": [                     // 1+ slots; first FREE one is used
        {
          "position": "11633.3 57.8 14650.1",
          "orientation": "250 0 0",
          "size": "6 3 12"               // collision box x y z. "0 0 0" = skip the check
        }
      ]
    }
  ],
  "parkInCost": 100,
  "parkOutCost": 100,
  "maxVehicleStored": 5,
  "saveVehicleCargo": true,
  "saveVehicleHealth": true,
  "saveVehicleFuel": true,
  "payWithBank": true                    // use the Banking ledger if present; else cash fallback
}
```

## Setup (same in both modes)

1. Edit a garage entry: `npcId`, `className`, `position`, `orientation`, `parkSpots`.
2. Restart the server. Read the `========= TraderXGarage =========` log block to confirm the mode
   and check for warnings.
3. Walk to the attendant → **Park In** / **Park Out**.

That's it. Nothing to add in TraderX's config.

## Multiple garages

Add more `npcs` entries, each with a **unique `npcId`** and its own `parkSpots`. Park-out always
spawns the vehicle at the garage you interacted with — never the wrong side of the map.

## Notes

- Don't *also* add a garage NPC inside TraderX's trader config — that would spawn a second,
  duplicate NPC. The garage handles its own.
- A `parkSpots` entry with `size` `"0 0 0"` skips the collision check (always considered free).

## Common warnings in the log

- `npcId out of range 0..127` — interaction will never match; pick 0–127.
- `empty className` — the NPC can't be spawned; set a class.
- `no parkSpots` — park-out fails; add at least one spot.
