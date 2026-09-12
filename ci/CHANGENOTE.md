# Lines starting with '#' are ignored — everything else becomes the Steam
# Workshop change note for the next publish. Steam renders BBCode here, NOT
# Markdown: [b]bold[/b]  [i]italic[/i]  [list][*]item[/*][/list]  [url=..]..[/url]
#
# Edit this file, then run the publish command.

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
