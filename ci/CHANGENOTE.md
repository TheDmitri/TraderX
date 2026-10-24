# Lines starting with '#' are ignored — everything else becomes the Steam
# Workshop change note for the next publish. Steam renders BBCode here, NOT
# Markdown: [b]bold[/b]  [i]italic[/i]  [list][*]item[/*][/list]  [url=..]..[/url]
#
# Edit this file, then run the publish command.

[b]New look: Nasdara[/b]
[list]
[*]Trader, bank and garage redesigned with the DayZ Beans Launcher's Nasdara style: warm dark surfaces, ember accents, new fonts and icons.
[*]Purchase: category sidebar with counts, product tiles or rows with price and "sells for", a cart column with totals, and a try-on preview.
[*]Customize: attachment slots with thumbnails and prices, presets and variants tabs, configuration total.
[*]The interface keeps its proportions on 16:10 and other non-16:9 screens.
[*]Garage is fully translated into the 13 game languages.
[/list]

[b]DayZ 1.30 compatibility[/b]
[list]
[*]Compiles and runs on DayZ 1.30. Config files are found again even when FindFile can't list your -profiles folder (traders are loaded by reference from the general settings).
[*]Dependency change: TraderX now needs only CF. DabsFramework is no longer required, and DayZular is included in TraderX. If your server already loads @DayZular, keep it (update it to the latest version): TraderX then uses it instead of its own copy.
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
[*]TraderPlus: vehicles locked with TraderPlus CarLock keep their owner, code and locked state, and world vehicles load without "Scripted variables corrupted" warnings.
[*]TraderPlus: trade quantities follow what TraderPlus really did (above the item maximum = full, magazines traded empty, exact amounts), sell ratios keep TraderPlus' rounding, and traders that accept only some notes still accept only those.
[*]TraderPlus: the cash players hold is kept (TraderPlus_Money_* notes and coins are defined by TraderX), and so are TraderPlus bank cards, packing wrenches and car lockpicks. Unlocking also accepts the 4-digit codes of TraderPlus locks.
[*]Vehicle receipts are back, compatible with TraderPlus: receipts players already hold keep their vehicle (parts, cargo, lock code) and can be taken out with the receipt in hands. Pack a vehicle with a wrench: owner of its lock only, empty vehicle unless allowed, near a trader if configured. The vehicle state stays on the server, so a duplicated receipt cannot bring a vehicle twice, and fluids are kept as they were (no free refill). On by default only on servers imported from TraderPlus (TraderXGarageConfig.json "receipts").
[*]TraderPlus: the garage keeps "vehicle must have a lock" and "save vehicle lock", the car lockpick chance is kept, categories with special characters in their name are imported, and TraderPlus sign traders appear again.
[/list]

[b]Catalog in a spreadsheet (CSV)[/b]
[list]
[*]New servers now start with the catalog in TraderXConfig/Source/: products.csv, categories.csv, presets.csv. Edit them in Excel, LibreOffice or Google Sheets (save as TAB-separated or comma CSV), restart, done. Quantity modes are written in plain words (FULL, STATIC 1, COEFFICIENT 0.5) instead of packed numbers.
[*]Existing servers keep their Products/ and Categories/ JSON files. To switch, set "preferredFormat": "CSV" and "autoMigrate": 1 in TraderXConfig/SourceConfig.json and restart: your catalog, presets included, is exported to Source/ once (checked identical on a 1000-product catalog).
[*]Mistakes in the CSV are listed with file and line in TraderXLogs/Config/ConfigReport.log. A broken file never empties your traders: the last good catalog is kept.
[*]The CSV/compiled pipeline was rewritten: the old one mis-encoded quantities and stock settings, lost the price coefficient and could leave traders empty. A leftover Compiled/ folder no longer overrides the JSON files of a server in LEGACY mode.
[/list]

[b]Security fixes[/b]
[list]
[*]Fixed several exploits that let a modified client get items or money for free. Updating is strongly recommended.
[*]Bank vault: a modified client could open, empty or fill another player's vault. The server now only serves your own vault.
[*]Safezone cleanup: a modified client could delete zombies and animals anywhere on the map. The server now cleans only the safezone you really stand in.
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
