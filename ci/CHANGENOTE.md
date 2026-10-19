# Lines starting with '#' are ignored — everything else becomes the Steam
# Workshop change note for the next publish. Steam renders BBCode here, NOT
# Markdown: [b]bold[/b]  [i]italic[/i]  [list][*]item[/*][/list]  [url=..]..[/url]
#
# Edit this file, then run the publish command.

[b]DayZ 1.30 compatibility[/b]
[list]
[*]Compiles and runs on DayZ 1.30. Config files are found again even when FindFile can't list your -profiles folder (traders are loaded by reference from the general settings).
[*]Dependency change: DabsFramework is no longer required; TraderX now needs CF + DayZular.
[/list]

[b]Garage v2[/b]
[list]
[*]Park a vehicle and get it back exactly as it was: attachments, cargo, fluids, damage zones and CarLock.
[*]3D preview of your stored vehicles, repair/service at the garage, and sharing a vehicle with other players.
[*]Insurance: insure a vehicle, get a replacement if it is destroyed. The deductible is never lower than the vehicle's resale price.
[*]Every garage action is checked server-side: you must stand at a garage NPC.
[/list]

[b]Banking[/b]
[list]
[*]Player-to-player bank transfers, with contacts and a ledger of every bank movement.
[*]Trader purchases can use your bank balance when you're short on cash.
[/list]

[b]Painless migration[/b]
[list]
[*]Import from TraderPlus (config, bank accounts, licences, stock, garage), Expansion Market and Dr Jones Trader.
[*]Imports never overwrite an existing TraderX config and never touch the original mod's files. A MigrationReport_*.txt explains what was imported or skipped.
[*]CarLock data now lives in CF ModStorage; existing locks are kept.
[/list]

[b]Security fixes[/b]
[list]
[*]Fixed several exploits that let a modified client get items or money for free. Updating is strongly recommended.
[/list]

[b]Performance — no more freeze on join or opening a trader[/b]
[list]
[*]Large catalogs are no longer sent all at once on join. Trader products now load on-demand when you open a trader, then stay cached so reopening is instant.
[*]Fixed a multi-second stall (your inputs not registering) when joining a server or opening the first trader.
[*]New option [b]prewarmCatalogInSafeZone[/b] (on by default): entering a safezone quietly preloads that zone's traders in the background, so they open instantly.
[/list]

[b]Trading[/b]
[list]
[*]Buying with a full inventory now asks for confirmation before dropping the items on the ground (nothing is bought or charged if you cancel).
[*]A syncing indicator shows while a trader's catalog is still loading; "expand all" appears once it's ready.
[/list]

[b]Fixes[/b]
[list]
[*]HUD notifications now stay pinned to the right edge and no longer get cut off at 4K, and they grow to fit bulk-trade details instead of clipping them.
[*]Item card hover no longer flickers the description/checkmark.
[*]The category list scrollbar now actually scrolls.
[/list]

[b]Internal[/b]
[list]
[*]Logging module migrated to DayZular (fixes a load error); removed the DabsFramework UUID dependency.
[/list]
