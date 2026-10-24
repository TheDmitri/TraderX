# Discord post: TraderX release (2026-10)

Discord caps a message at 2000 characters, so the post is split into 2 messages. Attach the screenshots to message 1 and the video to message 2.

---

## Message 1

# 🛒 TraderX: big update is live

**New look, Garage v2, banking, CSV catalog, DayZ 1.30, and near 100% TraderPlus compatibility.**

**🎨 New interface**
Trader, bank and garage fully redesigned: category sidebar, item tiles with "sells for", a cart column with totals, try-on preview, and attachment/preset customization. Works on 16:10 and ultrawide screens.

**🚗 Garage v2**
Park a vehicle and get it back exactly as it was (attachments, cargo, fluids, damage, lock). 3D preview, repair, sharing with friends, and insurance.

**🏦 Banking**
Player-to-player transfers, contacts, a full ledger, and you can pay at traders with your bank balance.

**📊 Catalog in a spreadsheet**
New servers start with `products.csv` / `categories.csv` / `presets.csv`. Edit them in Excel or Google Sheets and restart. Errors are reported with file and line number.

**⚙️ Simpler setup**
Needs **only CF** now. DabsFramework is no longer required and DayZular is built in. If you already run @DayZular, keep it and update it.

**🔒 Security**
Several exploits that let modified clients get items or money for free are fixed. **Update recommended.**

---

## Message 2

## 🔄 TraderPlus → TraderX: time to migrate

TraderX now imports a TraderPlus server **almost 100%**:
• config, traders, categories, stock, licences and bank accounts
• vehicles and **CarLock locks** (owner, code, locked state)
• the **cash players already hold**, bank cards, wrench and lockpick
• **vehicle receipts**: receipts players already have still deploy their car (parts, cargo, lock code)

Your TraderPlus files are never modified, and a `MigrationReport` explains what was imported.

👉 **If you're on TraderPlus, now is the right moment to switch over the coming weeks.** As always, **test on a test server first**: drop TraderX in, boot, check the report, then move your live server.

## 🤖 Faster fixes from now on
Builds, deployments and fixes are now AI-assisted. Expect **many more fixes, much faster**, and steady development. Report bugs in #bug-reports with your `TraderXLogs` and they'll get picked up quickly.

📦 **TraderX:** https://steamcommunity.com/sharedfiles/filedetails/?id=3560310973
🧪 **TraderX Experimental:** https://steamcommunity.com/sharedfiles/filedetails/?id=3760962576
📦 **DayZular** (optional, update it if you run it): https://steamcommunity.com/sharedfiles/filedetails/?id=3686232711
⚠️ Updating an existing server: **back up your profile folder first**. Your config, catalog and parked vehicles are kept. You can drop DabsFramework if nothing else needs it.

@everyone
