# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Méthodologie TSL

Ce projet suit **TSL (Thin Spec Loop)**. À chaque session IA :
1. Lire `SCOPE.md` (story active, non-goals), `DECISIONS.md` (ADRs), `GLOSSARY.md` (termes exacts)
2. Utiliser le template `.ai/prompt-boilerplate.md` pour l'amorçage
3. Chaque commit doit référencer la story : `feat(US-XXX): description`
4. **"Done"** = test in-game vert + Dmitri validé en live

Stories dans `docs/stories/US-XXX-*.md`. Toute décision architecturale non documentée → ADR dans `DECISIONS.md`.

## Build & Development

**Build PBOs and launch server** (VS Code task: "Build And Start DayZ Server"):
```powershell
.\Build.ps1
```
Requires `C:\VSCodeForDayZModding\Configuration\Userconfig.ps1` with `BUILD_PBO_EXE_PATH` and `LAUNCH_AFTER_BUILD` set. The script packs every folder under `./src/` into a `.pbo` file placed in `./target/@Client/Addons/`.

**Other VS Code tasks** (all backed by PowerShell scripts):
- `LaunchDayZ.ps1` — launch client only
- `LaunchDayZServer.ps1` — launch server only
- `StopClientAndServer.ps1` — kill both processes
- `additionalCommands/ShowClientScriptlog.ps1` / `ShowServerScriptlog.ps1` — tail script logs
- `additionalCommands/ShowClientTraderXLogFile.ps1` / `ShowServerTraderXLogFile.ps1` — tail mod-specific logs

**Code quality helpers:**
```bash
python detect_redecl.py   # detect class re-declaration conflicts across PBOs
scripts/clean-build-styles.sh  # recompile CSS-like style sheets
```

No automated test runner — tests execute in-game via `TraderXDebugModeService` when `TraderXDebugSettings.json` has debug mode enabled.

## Architecture

### Language & Framework

- **Enforce Script** (`.c` files) — DayZ's C-like scripting language
- **DayZular** — MVC/service-registry framework (dep: `C:\DayZWorkspace\DayZular`)
- **DabsFramework** — lower-level DayZ MVC helpers
- **CF (Community Framework)** — event bus and shared utilities

### Layered Structure (DayZ convention)

```
3_Game/   — Domain layer: models, value objects, repositories, domain services
4_World/  — Application layer: services, UI pages, GUI prefabs, entity extensions
5_Mission/— Mission layer: server startup, DayZular service registration (TraderXAppInit.c)
```

All service singletons are registered in `5_Mission/TraderXAppInit.c` and resolved via DayZular's service locator.

### Source Packages (under `src/`)

| Package | Purpose |
|---|---|
| `TraderX` | Core trading system (main mod) |
| `TraderXBanking` | Banking, accounts, clans, ledger, credit cards |
| `TraderXCarLock` | Vehicle lock/unlock actions |
| `TraderXGarage` | Vehicle parking marketplace |
| `TraderPlusMoney` | In-game currency item definitions |
| `TraderPlusATM` | ATM entity and interaction |
| `TraderSign` | Visual trader location markers |

Each package maps to one PBO. Adding a new package = new folder under `src/`.

### Configuration System

Runtime config lives in `$profile/TraderX/TraderXConfig/`. Three formats auto-detected by `TraderXSourceFormatDetector`:
1. **Legacy** — individual JSON per product/category
2. **CSV** — spreadsheet source files under `Source/`
3. **Compiled** — single `Products_compiled.json` + `Categories_compiled.json`

`TraderXFormatMigrator` handles cross-format migration on load.

## DayZular Architecture

Three-layer design:
1. **UI Layer** (Components) — Data binding, event handlers. Components extend `DZlrComponentBase` with lifecycle hooks (OnInit, OnDestroy, OnChanges).
2. **Business Logic Layer** (Services) — Domain logic registered in singleton `DZlrServiceRegistry`. Services communicate via the event bus, not direct calls.
3. **Infrastructure Layer** — Event bus (13 mission/player lifecycle events), RPC system with middleware and rate limiting, DI registry.

Key patterns:
- Components auto-register via `ComponentAutoRegister` static instances
- Property binding syntax: `{{ propertyName }}`, pipes: `{{ value | uppercase }}`
- Event binding in layout XML: `(click)="OnButtonClick"`
- Services subscribe to events in their constructor (explicit dependencies)
- Multi-mod service registration via `AppModule.Event_OnRegisterServices`
- Bootstrap flow: Game loads → AppModule.Bootstrap() → event bus + logger + pipes init → mods register services → lifecycle events fire

### UI System (DayZular bindings)

Layouts are `.layout` XML files under `datasets/gui/`. The design language is **"Field Ledger"** — dark tactical aesthetic using the token palette defined in `docs/TraderX_Design_System.md`. All new UI must use those tokens; no ad-hoc colors or padding.

**Binding rules — strictly enforced:**
- Text → `TextBinding` with `Binding_Expression "{{ property }}"` in layout; call `MarkForCheck()` on data change. Never `.SetText()`.
- Lists/grids → `ComponentWidgetBinding` + `Component_ForEach`. Never manual `AddChild`.
- Buttons → `ButtonBinding` with `OnClick` handler. Never override `OnClick` via widget matching.
- Navigation → `NavButtonBinding` with `TargetSelector`. Never manual page switching.

Reference projects for correct DayZular patterns: `C:\DayZWorkspace\CourseBarterTrader` and `C:\DayZWorkspace\RazBentleyWorkbench`.

### Enforce Script Gotchas

- **No ternary operator** — `?:` is not supported. Use DayZular helpers:
  - `DZTernaryInt.If(cond, t, f)`, `DZTernaryFloat.If(...)`, `DZTernaryString.If(...)`, etc.
  - Or global shorthands: `TernaryInt()`, `TernaryFloat()`, `TernaryString()`, …
- Class names must be globally unique across all loaded PBOs — run `detect_redecl.py` when adding new classes.
- `ScriptInvoker` is the event bus pattern used throughout; listeners attach/detach in `OnInit`/`OnDestroy`.
- switch default return is not enough in methods, we need to return a default case outside the switch too.
- You cannot split a line of code into multiple line.
- You don't have to use ref inside a method, it's auto ref !

## DZML Toolchain & UI Pipeline

**All TraderX UI is authored as HTML (DZML), compiled to DayZ `.layout` files.** You almost never hand-edit `.layout` — edit the `.html`, recompile. The Workbench is no longer the authoring tool; a browser preview replaces it.

### Where things live

| Thing | Path |
|---|---|
| DZML CLI (the tool) | `../dayz-tools/dayz-layout-tools/` (sibling of TraderX repo) |
| Built CLI entry | `../dayz-tools/dayz-layout-tools/dist/bin/dzml.js` |
| Class/attribute reference | `../dayz-tools/dayz-layout-tools/docs/DZML_CLASSES.md` |
| Source `.html` + compiled `.layout` | `src/TraderX/datasets/gui/**` (side by side, same basename) |
| Browser runtime | `src/TraderX/datasets/gui/{.,Banking,Garage}/dzml-runtime.js` |
| Styles atlas pipeline | `scripts/build-styles.sh` |
| Style/imageset/atlas assets | `src/TraderX/datasets/styles/` |

The CLI is TypeScript. After editing tool source, rebuild: `cd ../dayz-tools/dayz-layout-tools && node ./node_modules/typescript/bin/tsc` (output → `dist/`, note the `dist/src/...` layout). There is no global `dzml` on PATH — call `node <repo>/dist/bin/dzml.js <cmd>`.

### CLI commands

- `dzml compile <file.html>` → writes `<file>.layout` next to it.
- `dzml pipeline` → full build: styles → CSS → check → compile (config-driven).
- `dzml check <file.html>` → lint + widget-name uniqueness + binding/style-ref validation.
- `dzml runtime [dir]` → (re)emit `dzml-runtime.js` into a dir. Re-run for the 3 dirs (`.`, `Banking`, `Garage`) after editing `src/runtime/preview-runtime.ts` + rebuilding.
- `dzml gallery [dir]` / `dzml manifest` / `dzml styles` — gallery preview, selector→html manifest, styles→CSS.
- `dzml tga2edds [target]` → convert `.tga` textures (bitmap-font pages, imageset atlases) → `.edds`. Pipeline per file: `.tga` --(python3 Pillow)--> temp `.png` --(`eddsconv` Go CLI)--> `.edds`, written beside the source. `target` is a dir (all `*.tga`) or a single `.tga`; `-r` recurses, `--delete` removes the source `.tga` after success, `--eddsconv <path>`/`$DZML_EDDSCONV` overrides the binary (auto-detects sibling `../edds-cli/eddsconv`). The game loads font textures as `.edds` and `.fnt` files reference the texture by basename (no extension), so emitting `<name>.edds` is enough — no `.fnt` edits. Requires python3+Pillow and the `eddsconv` binary built (see EDDS section). Verify with `eddsconv check *.edds`.

### DZML authoring (HTML → layout)

- Widget type: `data-dz="panel|text|button|image|grid|scroll|frame|richtext|editbox|progress|preview|playerpreview"`.
- `data-name` → widget name. **AutoLoadWidgets() in the component binds member widgets BY NAME** — the layout's `data-name` values are a hard contract with the `.c` component's widget members. Renaming one without the other silently breaks binding.
- Positioning via `class="dz-*"` utilities: `dz-xy-{x}-{y}`, `dz-wh-{w}-{h}` (numbers = %, `px` suffix = exact pixels, `full`), `dz-c-RRGGBB`/`dz-c-AA` (color — also the **text color for text/richtext widgets**), `dz-tc-*` (button label color only; NOT for text widgets), `dz-font-*`, `dz-align-*`, `dz-cols-N`/`dz-rows-N`/`dz-gap-N`/`dz-pad-N`, `dz-fit-v`, `dz-noptr` (ignorepointer), `dz-hidden`, `dz-clip`, `dz-scroll-v`, `dz-exact-text`, `dz-text-vc`/`dz-text-center`/etc. Full list: `DZML_CLASSES.md`.
- Bindings (data-* the parser recognizes): `data-bind="{{ prop }}"`, `data-bind-item`, `data-bind-player`, `data-two-way="1"`, `data-foreach`/`data-template`/`data-item` (lists), `data-component="{{ sel }}"` (ComponentWidgetBinding host), `data-nav-target` (NavButtonBinding), `data-on-click`/`data-on-dblclick`/`data-on-change`/`data-on-mouseenter`/`data-on-mouseleave`, `data-enable-rotation`/`data-enable-scale` (preview), `data-style`, `data-prop-style`, `data-disabled`.

### Styles atlas pipeline (Field-Ledger 9-slice)

The `TX*` styles (`TXContent`, `TXCard`, `TXSurface`, `TXListRow`, `TXInputField`, `TXBtnDefault`, `TXBtnIcon`, `TXProgressFill`, …) are 9-slice styles backed by one texture atlas. Source of truth: `src/TraderX/datasets/styles/` = `traderx_styles.styles` (XML) + `traderx_ui.imageset` + `traderx_atlas.{svg,png,edds}` + `traderx_atlas.edds.meta`.

- Build/regenerate: `scripts/build-styles.sh` (Linux). Pipeline: stylesjson → `generate_styles.py` (SVG→PNG via **cairosvg**, `--scale 2`) → **`eddsconv`** Go CLI (PNG→EDDS, Enfusion LZ4) → write `.edds.meta` + patch `{GUID}` → `eddsconv check`.
- EDDS: do **NOT** use the old `png2edds.py` (emits bad `0x59` LZ4 flags rejected by the game). Use the Go CLI at `../dayz-tools/edds-cli/` (`eddsconv conv|export|check`), built on `woozymasta/edds`. Build it: `cd ../dayz-tools/edds-cli && env -u GOPATH -u GOTOOLCHAIN GOFLAGS=-mod=mod go build -o eddsconv .`
- **Enfusion `{GUID}`**: Workbench stamps a 16-hex asset GUID into `.edds.meta` and references it as `{GUID}path` in imageset/styles. Our atlas GUID is `B62CAA2543B1EC63`. `build-styles.sh` writes the meta + sed-patches the imageset.
- Registered in `src/TraderX/config.cpp` under CfgMods/TraderX: `widgetStyles.files[]` += `traderx_styles.styles`; `imageSets.files[]` += `traderx_ui.imageset`. New `.styles`/`.imageset` must be added there or the game won't load them.

### Browser preview CSS (three stylesheets)

The browser preview renders the Field-Ledger look from **CSS**, not the atlas. Pages must link all three, in order:
- `dzml.css` — `dz-*` utility classes (position/size/color/font). Generated by `dzml css`.
- `dzml-styles.css` — `[data-style="TX*"]` → background/border (the 9-slice styles approximated as CSS). Generated by `dzml previewcss` from the `<style type="dz-styles">` tokens in `_fixtures/field-ledger.html`.
- `dzml-icons.css` — `[data-image="set:traderx_icons image:NAME"]` → a CSS `mask-image` (inline SVG data-URI). The widget's `dz-c-*` background shows **through** the mask, so `dz-c-*` tints the icon exactly like the in-game widget color. Generated by `scripts/build-icons.sh`.

If a page links only `dzml.css`, you get a **white background and flat gray panels** (no TX styles) and **plain colored squares** instead of icons. All source pages link the three; `dzml bundle` **inlines** any local `<link>`ed CSS into the `.bundle.html` so `file://` renders standalone (sibling `.css` no longer required). Subpages in subdirs load as fragments — their `<head>` is ignored; the entry page's CSS applies.

### Bundle navigation + #loc strings

`dzml bundle <entry.html>` makes a self-contained navigable preview: it inlines every manifest component as `<template data-page="selector">`, inlines local CSS, and injects `window.__DZML_{MANIFEST,NAVMAP,STRINGS}__` (read from `components.manifest.json` + `_fixtures/{navmap,strings}.json`). The runtime navigates via those inline templates (no fetch).

- **Strings:** `_fixtures/strings.json` is generated from the real `src/TraderX/languagecore/stringtable.csv` via `dzml strings <csv> -o src/TraderX/datasets/gui/_fixtures/strings.json`. Any `#tpm_*` key not in the CSV stays literal in the preview — add the missing key to the stringtable (it's needed in-game too), regenerate, re-bundle. Re-run after editing the stringtable.
- **Nav targets:** top-bar buttons use `data-on-click="OnXClick"`; `_fixtures/navmap.json` maps handler→selector for the preview. The page host (`data-component="{{ sel }}"`) takes `data-default-page="tx-..."` to pick the boot page (else the runtime falls back to the first manifest entry, which may be a card). `data-default-page` is runtime-only — the `.layout` compiler ignores it.
- **Order gotcha (tool):** `bundle.ts` must replace the `<script src=dzml-runtime.js>` tag **before** inlining CSS — `dzml.css`'s header comment contains an example `<script>…dzml-runtime.js…</script>`, so inlining first makes the regex match inside the stylesheet and trap the whole payload in `<style>`. Verify a bundle with `grep -n '<style\|</style>\|<body' file.bundle.html` — globals/templates must be **after** `</style>`/inside `<body>`.

### Icon pipeline (single SVG source → browser + game)

`src/TraderX/datasets/icons/svg/*.svg` is the **single source of truth** for `traderx_icons`: monochrome line glyphs (64×64 viewBox, white stroke so the widget color tints them). One file per `image:NAME` referenced in the layouts.

`scripts/build-icons.sh` (calls `dayz-tools/styles_generator/generate_icons.py`) renders the SVGs → one sprite sheet PNG (8 cols, alphabetical = stable Pos) → `traderx_icons.{png,imageset,edds,edds.meta}` (atlas GUID `0340C2EBFA081AA7`) **and** emits `dzml-icons.css` for the browser, mirrored into Banking/Garage dirs. Add a new icon = drop `NAME.svg` in `svg/`, add `<img data-image="set:traderx_icons image:NAME">` in the layout, rerun `build-icons.sh`. The `.imageset` only contains the icons present in `svg/` — category icons loaded via dynamic `category.icon` paths are separate and unaffected.

### Critical gotchas (UI)

- **Text color on text widgets = `dz-c`, NOT `dz-tc`.** For `data-dz="text"`/`"richtext"`/`"multitext"` widgets, set the text color with `dz-c-RRGGBB` (the widget `color`). `dz-tc-*` (`textcolor`) does **not** color a text widget's glyphs in-game. Use `dz-tc-*` only for the label color of `data-dz="button"`. So: text/richtext → `dz-c`; button label → `dz-tc`.
- **Styled widget colors are TINTS, not fills.** A widget's `color` *multiplies* its `data-style`. A dark `dz-c-*` on a styled widget → black UI. **The compiler forces `color 0.9961 0.9961 0.9961 1` (FEFEFE/255 white) on any widget with `data-style`** (in `dzml-parser.ts`) so the style's baked colors show. Corollary: to draw a **solid dark fill** (e.g. a darker preview background), use a *plain* panel with `dz-c-*` and **no** `data-style` — otherwise the dark color is overwritten with white.
- **Widget-name contract** (see above): card/component layouts must use the exact `data-name`s the `.c` component declares. Known contracts: ItemCard → `itemCardOutline, itemCardContent, stockText, volumeBar, favBtn, selectedIcon/unselectedIcon, favIcon/unfavIcon, healthImg, itemCardMainBtn, volumeBtn`; CategoryCard → `categoryCardPanel, categoryIcon, CategoryName, dropdownParentPanel, croissantIcon/decroissantIcon, itemCountText, licensePanel, itemCardGridList/itemCardGridTiles`; CheckoutCard → `btnPlus, btnMinus, removeBtn`; PlayerSlot → `btnNav, imgBtn, imgLock`; ViewToggle → `viewToggleBtn, gridIcon, listIcon, viewLabel`.
- **List/grid view convention**: two sibling foreach grids named `*GridList` and `*GridTiles`. In-game `TraderXViewModeService` + `TraderXViewModeBinder.Apply()` show one / hide the other; the `tx-view-toggle` component toggles globally so no per-page logic. The browser runtime mirrors this by display-toggling `[data-name$="GridList"]`/`[data-name$="GridTiles"]`.
- **No nested `<button>`** — the HTML parser flattens children to siblings. Build a background button + overlay labels/icons as siblings instead.
- **Browser preview vs in-game** are two different runtimes. The browser (`dzml-runtime.js`) interprets `data-*` directly and is for visual iteration; in-game uses the compiled `.layout` + EnforceScript components. A bug can exist in one and not the other (e.g. `data-on-click="OnViewToggleClick"` works in-game via the component but needs explicit handling in `preview-runtime.ts`). Subpage `.html` in subdirs load as fetched fragments — their `<script src="dzml-runtime.js">` does not re-execute; the root page's runtime drives them.
- After changing tool source or runtime: rebuild tsc → re-emit runtime (3 dirs) → recompile affected `.html`. After changing a `.html`: just `dzml compile`. **Neither builds the PBO** — `Build.ps1` + in-game test is still a separate, user-run step (no Enforce compiler here).

## Key Files

- `docs/TraderX_Design_System.md` — canonical color tokens, typography, spacing rules for all UI work
- `CONFIGURATION_GUIDE.md` — full server configuration reference
- `changelog.md` / `backlog.md` — recent fixes and open issues
- `docs/AI_Handover.md` — AI session handover notes