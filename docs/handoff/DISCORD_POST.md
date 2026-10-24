# Discord post: TraderX release (2026-10-24)

Message 1 is for TraderX users. Message 2 is for TraderPlus users. Each is under Discord's 2000-character limit.
Attach the 6 screenshots from `release-screens/discord/` to message 1.

---

## Message 1 (TraderX users)

Hey everyone! 👋

I'm really excited to release the **new TraderX update**. It's the biggest one so far, it's **fully compatible with DayZ 1.30**, and it's live now on both TraderX and TraderX Experimental.

It's a big update, so there may be a few bugs. If you find one, tell me in #bug-reports (your `TraderXLogs` folder helps a lot) and I'll fix it quickly.

**Highlights**
- **Brand new interface** for the trader, bank and garage. It has a proper cart, buy and sell prices on every item, and weapon customization with attachments and presets.
- **Garage v2**: your vehicle comes back exactly as you parked it. You also get a 3D preview, servicing, insurance and vehicle sharing.
- **Banking**: transfers between players, a full history, and paying traders from your bank.
- **Spreadsheet catalog** for new servers (Excel or Google Sheets).
- **Security fixes** and **no more freeze** when you open a trader.

The full changelog is on the Workshop page: https://steamcommunity.com/sharedfiles/filedetails/?id=3560310973

**Server owners, please read before updating**
As some of you know, DabsFramework caused a few problems. TraderX **no longer needs it**. It now ships with our own framework (DayZular) built in, which makes it much easier to maintain.
1. Back up your profile folder. Your config, catalog and parked vehicles are kept.
2. Keep **CF + TraderX**. You can remove DabsFramework if no other mod uses it.
3. If you also load **@DayZular**, keep it updated (it was updated today too).

Enjoy, and if you need help, just ask! ❤️

---

## Message 2 (TraderPlus users)

**To everyone still on TraderPlus 👋**

I know a lot of you are still running TraderPlus, and that's totally fine with me. But I want you to know that **most of what was missing in TraderX is now there**:
- **Banking is back**: accounts, ATMs, bank cards, and transfers between players.
- **The garage is back**, and it's better: vehicles come back exactly as parked, with insurance and sharing.
- **Vehicle receipts are supported**: pack a car with the wrench and deploy it from the receipt. The receipts your players already hold still work.
- **A new CarLock**: your existing TraderPlus locks and 4-digit codes are kept.
- **Safe zones**, licences, and stock are all included.

**Migrating is easy.** TraderX imports your TraderPlus server for you: config, traders, stock, licences, bank accounts, vehicles and their locks, and even the TraderPlus money your players carry. Your TraderPlus files are never touched, and a report tells you exactly what was imported.

Running **Expansion Market** or **Dr Jones Trader** instead? TraderX can import those too: traders, products, prices, stock and NPCs.

So give it a try on a **test server**: add TraderX, boot it, read the report, and see how it feels. If it works for you, move your live server over in the coming weeks.

From now on I build, test and ship TraderX with the help of AI. That means many more fixes, delivered faster, and development that keeps moving.

TraderX: https://steamcommunity.com/sharedfiles/filedetails/?id=3560310973
TraderX Experimental (gets new builds first): https://steamcommunity.com/sharedfiles/filedetails/?id=3760962576

Questions about migrating? Ask here, I'm happy to help!

@everyone
