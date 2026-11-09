# Stable change note (BBCode). '#' lines are ignored.
[b]Crash fixes[/b]
[list]
[*]Clicking the X in the trader, bank, garage or licence window no longer closes the game. Esc and X now behave the same.
[*]Fixed a random crash when buying with a full inventory and answering the "not enough space" confirmation.
[/list]

[b]Banking[/b]
[list]
[*]The bank now uses the currencies set in TraderXBankingConfig.json on the player's side too. Before, the bank window counted cash in Rubles only, so players with another currency appeared to have no money.
[*]When TraderX is loaded, bank objects are configured in TraderXGeneralSettings.json. The server log now says so if bankNpcs is filled in the banking config.
[/list]

[b]Vehicle purchases[/b]
[list]
[*]Buying a vehicle no longer fails with "No parking available" when the trader has no parking file (TraderXConfig/TraderXVehicleParking/parkings_<npcId>.json) or all its spots are taken: the vehicle spawns on a free spot near the trader.
[*]A parking spot with size 0 0 0 skips the collision check, like garage parking spots.
[*]Vehicles can be bought anywhere on maps larger than Chernarus (Deer Isle and others).
[*]Boats and motorbikes bought from a trader now come with fuel.
[/list]

[b]Motorbikes (DayZ 1.30)[/b]
[list]
[*]Motorbikes can be locked, unlocked and lockpicked with CarLock (lockpicking uses the car settings).
[*]Motorbikes can be parked in the garage with insurance and sharing.
[*]Motorbikes can be sold to traders (last driver only, engine off).
[/list]
