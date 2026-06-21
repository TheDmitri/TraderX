# Patterns for DayZ Modding — Booklet Plan

## Positioning

**Title:** Patterns for DayZ Modding  
**Subtitle:** Professional patterns for Enforce Script, architecture, and multiplayer systems  
**Format:** PDF ebook, ~70–90 pages, code-heavy, black-on-white  
**Price:** $25–35 (Gumroad or Ko-fi)  
**Platform:** Own website + Gumroad  
**Audience:** Intermediate DayZ modders — they can write a class and attach an action, but their mods crash under load, leak memory, or don't scale past one feature  
**Not for:** Complete beginners who've never opened a .c file  

---

## What makes this different

Every tutorial in the DayZ space shows *what* to paste. This book explains *why* the code is shaped the way it is — the forces that push toward certain designs, and what breaks when you ignore them. Real examples drawn from production commission mods. No invented toy examples.

---

## Chapter Map

### Introduction — The Problem With Copy-Paste Modding
- Why most DayZ mods break after 3 features
- What a "pattern" is and isn't (not a recipe, a vocabulary)
- How to read this book (each pattern: problem → solution → code → gotchas → when not to use)
- Reference architecture used throughout: a banking mod on top of DayZular
- *~4 pages*

---

### Part I — Enforce Script Realities

Before patterns, the language constraints every modder hits.

**Chapter 1 — The Integer Enum**  
*Problem:* No native enum keyword. Switching on state becomes a mess of magic numbers.  
*Pattern:* Constant class with static int fields + static label/helper methods.  
*Source:* `TXBLedgerEntryType`, `TXBClanRole`  
*Gotcha:* Switch must have a return outside the switch body too — compiler won't catch missing paths.  
*~6 pages*

**Chapter 2 — No Ternary Operator**  
*Problem:* `condition ? a : b` crashes the compiler. Inline conditionals become verbose chains.  
*Pattern:* `DZTernaryString.If()` helpers; or extract to a single-purpose method.  
*Source:* DayZular helpers, TraderX usage throughout  
*Gotcha:* Never split a line of code across multiple lines — parser chokes silently.  
*~4 pages*

**Chapter 3 — Null Safety and Safe Casting**  
*Problem:* `Cast()` returns null on failure; dereferencing crashes the server with no useful log.  
*Pattern:* Always check after cast. Map lookups with `Find()`. Array bounds before index.  
*Source:* Every RPC handler and service in TraderXBanking  
*Gotcha:* `ref array` fields need explicit null-check before first insert.  
*~5 pages*

**Chapter 4 — The Constants Class**  
*Problem:* Magic strings and numbers scatter across files; changing one path breaks three systems.  
*Pattern:* One class, all static const fields, grouped by concern.  
*Source:* `TXBConstants`  
*Gotcha:* Class names are globally unique across all PBOs — name collisions silently win at load time.  
*~4 pages*

---

### Part II — Architecture

**Chapter 5 — The Layered Mod (3 / 4 / 5)**  
*Problem:* Putting everything in 4_World creates spaghetti; mission scripts leak into domain logic.  
*Pattern:* DayZ's own 3_Game / 4_World / 5_Mission convention applied to mod code. Domain models in 3_Game, services in 4_World, bootstrap in 5_Mission.  
*Source:* TraderX, TraderXBanking full structure  
*Gotcha:* 3_Game code cannot call 4_World code — dependency arrow points down only.  
*~6 pages*

**Chapter 6 — The Service Registry**  
*Problem:* Global variables for every system create invisible dependencies and load-order nightmares.  
*Pattern:* Service locator — register once in 5_Mission, resolve anywhere via `GetService()`. Lazy-init global accessor as syntactic sugar.  
*Source:* `DZlrServiceRegistry`, `GetTXBBankingService()`, `GetTXBConfigService()`  
*Gotcha:* Services resolved before `OnMissionStart` may be null — defer init to event.  
*~7 pages*

**Chapter 7 — The Event Bus**  
*Problem:* Services that call each other directly form cycles and can't be tested or replaced independently.  
*Pattern:* `ScriptInvoker` as a typed pub/sub channel. Services emit, components subscribe. No direct references between services.  
*Source:* `DayZularEvents`, `TXBBankingService.Event_OnAccountUpdated`  
*Gotcha:* Always `Remove()` in `OnDestroy` — dangling listeners cause hard-to-trace crashes after reconnect.  
*~7 pages*

---

### Part III — Client-Server Communication

**Chapter 8 — The Server/Client Guard**  
*Problem:* Running server-side code on the client (or vice versa) causes silent failures or crashes.  
*Pattern:* `IsServer()` / `IsClient()` guards at the top of every method that touches the network. Single-player compatibility with `!IsMultiplayer()`.  
*Source:* Every service constructor and `OnUpdate` in TraderXBanking  
*Gotcha:* Forgetting single-player case locks players out of features in offline testing.  
*~5 pages*

**Chapter 9 — The RPC Endpoint**  
*Problem:* Raw `RPCSingleParam` calls have no namespacing, no error handling, and collide across mods.  
*Pattern:* Namespace + endpoint name registration. Typed `Param1/2/3<T>` structs for payloads. Always validate `ctx.Read()` before trusting data.  
*Source:* `DayZularRPCManager`, TraderXBanking 5 endpoints  
*Gotcha:* Never trust client-sent amounts — validate on server against stored state.  
*~8 pages*

**Chapter 10 — Rate Limiting and Middleware**  
*Problem:* A client spamming an RPC can crash the server or duplicate transactions.  
*Pattern:* `SetRateLimit(namespace, endpoint, maxCalls, windowSeconds)`. Middleware chain for auth, logging, and blocking.  
*Source:* `DayZularMiddleware`, `DayZularAuthMiddleware`  
*Gotcha:* Rate limits are per-player per-endpoint — global locks are wrong for multiplayer.  
*~5 pages*

---

### Part IV — Data and Persistence

**Chapter 11 — JSON Save / Load with Versioning**  
*Problem:* Player data lost on restart. Config format changes break existing installs.  
*Pattern:* `JsonFileLoader<T>.JsonSaveFile()` / `JsonLoadFile()`. Version field + auto-migrate on mismatch. Create-if-missing with defaults.  
*Source:* `TXBAccountData`, `TXBSettings`, `TXBLedgerFile`  
*Gotcha:* `MakeDirectory()` must be called before first write — missing dir silently drops the file.  
*~7 pages*

**Chapter 12 — The Config Service**  
*Problem:* Server settings loaded in ten different places; client doesn't know what the server allows.  
*Pattern:* One service owns the config object. Settings distributed to clients via RPC on `OnPlayerJoined`. Client reads only from the service, never disk.  
*Source:* `TXBConfigService`, `TXBSettings` distribution flow  
*Gotcha:* Config must be sent after the player is ready, not just joined — use `OnPlayerReady`.  
*~5 pages*

**Chapter 13 — Buffered Writes (Write Coalescing)**  
*Problem:* Writing to disk on every transaction tanks server performance under load.  
*Pattern:* Dirty counter per key + periodic flush on `OnUpdate`. Flush early when dirty count crosses threshold. In-memory cache avoids repeated reads.  
*Source:* `TXBLedgerRepository` — flush threshold 10, flush interval 30s, max entries 500  
*Gotcha:* Cache must be invalidated on player disconnect — otherwise stale data survives reconnect.  
*~6 pages*

---

### Part V — UI Patterns

Three UI systems exist in the DayZ modding ecosystem. This part covers all three — not to push readers toward one, but to make them fluent in reading and writing any of them. Each chapter follows the same axis: how a layout loads, how data gets in, how clicks get out, how lists are built.

**Chapter 14 — The Three UI Systems (Overview)**  
A single reference table comparing Vanilla, Dabs MVC, and DayZular on six axes: layout loading, data-to-widget flow, event handling, list rendering, reusability, debugging difficulty. The chapter's job is orientation, not recommendation — readers should know which system they're looking at within 30 seconds of opening an unfamiliar .c file.  
*Source:* Side-by-side excerpts from all three  
*~4 pages*

---

**Chapter 15 — Vanilla UI (Raw Widget Management)**  
*Problem:* Every beginner starts here. Knowing it well prevents cargo-culting its patterns into framework code.  
*Pattern:* `UIScriptedMenu` + `ScriptedWidgetEventHandler`. `CreateWidgets()` loads the layout. `FindAnyWidget()` + explicit cast for every widget you touch. `SetText()` / `SetCurrent()` for updates. `OnClick()` switch statement for events. `CallLater()` for polling updates.  
*Source:* `LaptopMenu.c` (Armx_Vault_Scripts), `BankHeistUI.c`  
*When to use:* Simple one-screen menus with few widgets and no dynamic lists. Rapid prototyping. No framework dependency wanted.  
*Gotchas:*
- Every `FindAnyWidget()` call is a runtime string lookup — typo = silent null crash
- `CallLater()` timers survive component death if not explicitly removed
- No change detection: update everything or track dirty state yourself
- OnClick switch grows unbounded as UI complexity rises  
*~8 pages*

---

**Chapter 16 — Dabs Framework MVC (Property Binding)**  
*Problem:* Vanilla UI scales badly past 5 interactive widgets. Dabs introduces a binding layer without requiring a full component architecture.  
*Pattern:* Inherit `ViewController`. Declare public properties on the controller class. In the layout, mark widgets with `scriptclass "ViewBinding"` and set `Binding_Name` or `Relay_Command`. Call `NotifyPropertyChanged("propertyName")` after mutations. Dynamic lists use `ObservableCollection<T>` — changes propagate automatically.  
*Source:* `DabsFramework/Scripts/3_Game/DabsFramework/MVC/` — `ViewController.c`, `ViewBinding.c`, `ScriptedViewBase.c`  
*When to use:* Mid-complexity UIs where team already uses DabsFramework. Clear MVC separation wanted without a full component system.  
*Gotchas:*
- `NotifyPropertyChanged()` must be called manually — no auto-detection of mutation
- Property names in code must exactly match `Binding_Name` in layout — no compile-time check
- Type Converter auto-generation is opaque; binding failures are silent at edit time
- `ObservableCollection` carries more memory overhead than a plain array  
*~8 pages*

---

**Chapter 17 — DayZular Component Lifecycle**  
*Problem:* Dabs MVC still couples UI logic to a single controller class. As UI grows into tabs, pages, and reusable rows, a component composition model is needed.  
*Pattern:* Inherit `DZlrComponentBase`. Register with `ComponentAutoRegister("selector-name", MyComponent)`. Declare layout path in `RegisterMetadata()`. Subscribe to service events in `OnInit()`, unsubscribe in `OnDestroy()`. Call `MarkForCheck()` to signal that bindings need re-evaluation.  
*Source:* `TXBAccountTabComponent.c`, `TXBLedgerRowComponent.c`, `DayZular/Core/ComponentBase.c`  
*When to use:* Complex multi-tab UIs, reusable row/card components, any UI that needs to survive data changes from a service event.  
*Gotchas:*
- `ComponentAutoRegister` static field must exist or the component is never registered
- `OnDestroy` must remove both `ScriptInvoker` listeners and `CallLater` entries
- Components instantiated by selector at runtime — typo in selector = silent failure  
*~7 pages*

---

**Chapter 18 — DayZular Declarative Bindings**  
*Problem:* Even with lifecycle hooks, manually syncing properties to widgets re-introduces the Vanilla problem.  
*Pattern:* Declare public string/int/array properties on the component. In the layout, attach binding scriptclasses: `TextBinding` with `{{ property }}`, `ButtonBinding` with `OnClick "MethodName"`, `ComponentWidgetBinding` with `Component_ForEach "{{ arrayProperty }}"` for lists. Use built-in pipes (`| currency`, `| uppercase`, `| truncate:20`) or define custom ones extending `DZPipe`. For tab navigation, `NavButtonBinding` with `TargetSelector`.  
*Source:* `AccountTabView.layout`, `PropertyBinding.c`, `TXBPipes.c`, `ForEachBinding.c`, `NavButtonBinding.c`  
*When to use:* Anytime you're already inside a DayZular component.  
*Gotchas:*
- `MarkForCheck()` is not automatic — forgetting it means stale UI even after data changes
- `ForEach` rebuilds the entire list on any array change — not suitable for lists over ~100 items
- Custom pipes must be registered in `PipeRegistry` before first use
- `{{ property }}` binding expressions are runtime-evaluated strings — no compile-time validation  
*~8 pages*

---

### Part VI — Advanced Patterns

**Chapter 19 — The Transaction Queue**  
*Problem:* Two simultaneous transfer requests can both read the same balance and produce a dupe.  
*Pattern:* FIFO queue processed one item per tick. Enqueue on receive, dequeue on successful commit.  
*Source:* `TXBTransactionQueue`, `TXBBankingService.OnUpdate`  
*Gotcha:* Queue must survive disconnect — if player leaves mid-queue, roll back or complete server-side.  
*~5 pages*

**Chapter 20 — Role-Based Access Control**  
*Problem:* Checking raw role integers scattered everywhere; adding a new permission means grep-and-pray.  
*Pattern:* Role constants class with `CanX(role)` helper methods. All permission logic in one file.  
*Source:* `TXBClanRole` — Member / Officer / Owner  
*Gotcha:* Always validate role server-side even if client already checked — client data is untrusted.  
*~4 pages*

**Chapter 21 — Conditional Compilation for Mod Interop**  
*Problem:* Your mod has an optional integration with another mod — hard dependency breaks standalone installs.  
*Pattern:* `#ifdef ModName` guards around integration code. Standalone fallback outside the ifdef.  
*Source:* `ActionTXBOpenBank` — TraderX integration optional  
*Gotcha:* Test both paths — ifdef guards are easy to forget and break standalone silently.  
*~4 pages*

---

### Closing — A Reference Architecture

Walk through the full shape of a well-structured mod end-to-end: file layout, service registration, first RPC round-trip, data load/save cycle, UI component wired to a service event. One concrete mod, all patterns applied.  
*~6 pages*

---

## Production Notes

### Per-chapter structure (enforced)
1. **Problem** — one paragraph, what breaks without this pattern
2. **Pattern** — the shape of the solution
3. **Code** — real excerpts from production mods, minimally cleaned
4. **Gotchas** — 2–3 bullets on what trips people up
5. **When not to use** — one paragraph

### Source projects to draw from
- `C:\DayZWorkspace\TraderX\src\TraderXBanking\` — primary source (richest patterns)
- `C:\DayZWorkspace\DayZular\` — framework patterns
- `C:\DayZWorkspace\TraderX\src\TraderX\` — config system, UI patterns
- Tutorial projects in `C:\Users\Brbap\OneDrive\Documents\DayZWorkspace\Projects\` — for simpler/isolated examples

### What to strip from examples
- Remove banking-specific domain names where possible (rename `TXBAccountData` → `PlayerData` etc.)
- Keep real file structure references so readers can cross-check
- No invented examples — every snippet must trace to a real file

### Avoid
- Explaining DayZ basics (player spawning, item classes) — link to wiki
- Framework installation — link to DayZular repo
- Server setup — not the audience

---

## Estimated Length

| Section | Pages |
|---|---|
| Intro | 4 |
| Part I — Language (Ch 1–4) | 19 |
| Part II — Architecture (Ch 5–7) | 20 |
| Part III — Communication (Ch 8–10) | 18 |
| Part IV — Data (Ch 11–13) | 18 |
| Part V — UI (Ch 14–18) | 35 |
| Part VI — Advanced (Ch 19–21) | 13 |
| Closing | 6 |
| **Total** | **~133 pages** |

---

## Delivery Milestones

| Milestone | Content |
|---|---|
| Draft 1 | Parts I + II (language + architecture) — foundational, testable as standalone |
| Draft 2 | Parts III + IV (networking + data) — the core of what people get wrong |
| Draft 3 | Parts V + VI + Closing (UI + advanced + reference arch) |
| Final | Edit pass, code cleanup, PDF export |

---

## Open Questions

- Include DayZular setup as an appendix, or assume readers already have it?
- Sell with or without companion source repo on GitHub?
- Tiered pricing: base PDF vs PDF + source repo access?
