# TraderX UI Rework — AI Handover

Document d'handover pour qu'une autre IA reprenne le travail en cours sans perdre de contexte.

---

## 1. Contexte du projet

**Mod DayZ TraderX** — système de trader, banking et garage en Enforce Script + DZML/DayZular pour l'UI.

- **Repo root** : `c:\DayZWorkspace\TraderX\`
- **Workspace** : `c:\DayZWorkspace\` (contient aussi `DayZular/`, `CourseBarterTrader/`, `RazBentleyWorkbench/` en référence, et `dayz-tools/` pour la toolchain DZML)
- **User** : Dmitri. Parle français.
- **Contrainte majeure** : **NE PAS TOUCHER** le dossier `src/TraderX/datasets/gui/v2/` (directive utilisateur explicite).

### Objectif en cours

Refaire l'UI du trader/banking/garage pour matcher un mockup design (dossier `mockup/`) avec :
1. **Field Ledger** — design system dark tactique style DayZ, palette ambre/olive/rouille
2. **DZML + DayZular** — framework UI compilé vers `.layout` pour DayZ
3. **Mock data en runtime** — pour pré-valider visuellement et fonctionnellement avant intégration finale

---

## 2. État actuel — ce qui existe

### 2.1 Mockup (référence visuelle HTML/CSS pure)

**Emplacement** : [src/TraderX/datasets/gui/mockup/](../src/TraderX/datasets/gui/mockup/)

C'est la **source de vérité visuelle** (HTML + CSS vanilla, rendu direct dans navigateur). À utiliser comme spec 1:1.

- `mockup/field-ledger.css` — design system complet en CSS
- `mockup/index.html` — landing avec liens vers toutes les pages
- `mockup/trader/{favorites,purchase,sell,catalog,customize}.html`
- `mockup/banking/{account,transfer,ledger}.html`
- `mockup/garage/my-vehicles.html`
- `mockup/components.html` — showcase de tous les tokens/patterns

### 2.2 Design system document

**Emplacement** : [docs/TraderX_Design_System.md](TraderX_Design_System.md)

Spec complète du "Field Ledger" : tokens couleurs (hex + float DayZ), typographie, spacing, composants, règles par module. À lire avant toute décision de design.

### 2.3 Trader v3 — nouvelle implémentation clean (EN COURS)

**Emplacement DZML** : [src/TraderX/datasets/gui/trader-v3/](../src/TraderX/datasets/gui/trader-v3/)
**Emplacement Enforce** : [src/TraderX/scripts/4_World/classes/TraderX/GUI/v3/](../src/TraderX/scripts/4_World/classes/TraderX/GUI/v3/)

**Status** : fonctionnel en preview navigateur + compile vers `.layout`. Pas encore testé en jeu.

C'est la **v3** — implémentation propre isolée pour valider visuellement et fonctionnellement avant de remplacer la v1 en production. Préfixe `tx3-` sur tous les selectors (le préfixe `v3-` était interprété comme un path par le runtime).

**Structure DZML (15 fichiers)** :
```
trader-v3/
├── dzml.config.yaml
├── dzml-runtime.js             ← runtime browser, copié + PATCHÉ avec PAGE_MAP tx3-*
├── dzml.css / dzml-styles.css   ← générés par pipeline
├── bg-preview.jpg               ← copié depuis gui/
├── TraderXV3MainView.{html,layout}       — shell (identity + 4 tabs + close + subnav)
├── Favorites/V3FavoritesView.{html,layout}
├── Purchase/V3PurchasePageView.{html,layout}
├── Sell/V3SellPageView.{html,layout}
├── Catalog/V3CatalogPageView.{html,layout}
├── Customize/V3CustomizePageView.{html,layout}  ← NOUVEAU — page customize
├── Prefab/
│   ├── ItemCard/{V3ItemCardView,V3ItemCardRowView,V3SellItemCardView}.{html,layout}
│   ├── CategoryCard/V3CategoryCardView.{html,layout}
│   ├── Checkout/{V3CheckoutRowView,V3CheckoutView}.{html,layout}
│   ├── Customize/{V3OptionCardView,V3BuildRowView}.{html,layout}  ← NOUVEAU
│   └── Navigation/V3SlotBarView.{html,layout}
```

**Structure Enforce (17 fichiers)** :
```
scripts/4_World/classes/TraderX/GUI/v3/
├── Mock/
│   ├── V3MockModels.c         — V3MockProduct, V3MockCategory, V3MockCartRow, V3MockSlot, V3MockOption, V3MockPreset, V3MockBuildRow
│   └── V3MockDataService.c    — singleton avec 17 produits, 7 catégories, 3+2 cart rows, 13 slots, wallet 14165, customize data KA-74 (21 options, 4 presets)
├── Components/
│   ├── V3TraderXMainComponent.c  (selector: tx3-trader-main)
│   ├── Pages/
│   │   ├── V3FavoritesPageComponent.c  (tx3-favorites-page)
│   │   ├── V3PurchasePageComponent.c   (tx3-purchase-page)
│   │   ├── V3SellPageComponent.c       (tx3-sell-page)
│   │   ├── V3CatalogPageComponent.c    (tx3-catalog-page)
│   │   └── V3CustomizePageComponent.c  (tx3-customize-page)  ← NOUVEAU
│   └── Prefabs/
│       ├── V3ItemCardComponent.c      (tx3-item-card)
│       ├── V3ItemCardRowComponent.c   (tx3-item-card-row)
│       ├── V3SellItemCardComponent.c  (tx3-sell-item-card)
│       ├── V3CategoryCardComponent.c  (tx3-category-card)
│       ├── V3CheckoutRowComponent.c   (tx3-checkout-row)
│       ├── V3CheckoutComponent.c      (tx3-checkout)
│       ├── V3SlotBarComponent.c       (tx3-slot-bar)
│       ├── V3OptionCardComponent.c    (tx3-option-card)     ← NOUVEAU
│       └── V3BuildRowComponent.c      (tx3-build-row)       ← NOUVEAU
```

### 2.4 TraderX v1 (runtime production) — modifié

**Emplacement** : [src/TraderX/datasets/gui/](../src/TraderX/datasets/gui/) (hors mockup/ et trader-v3/ et v2/)

Les fichiers v1 ont été **partiellement modifiés** avec le design Field Ledger (tokens TX*, quelques reskins ciblés). **À ne pas toucher davantage** — la v3 est l'approche propre qui remplacera v1 quand validée.

Modifications notables v1 déjà faites :
- `TraderXMainView.html` — style block TX* tokens Field Ledger
- `Favorites/FavoritesView.html` — reskin flat grid + nouveau `_favoriteProducts` binding
- `PurchasePageView.html` — filter pills + preview strip ajoutés
- `SellPageView.html` — slot bar déplacé en haut + labels uppercase
- `CatalogPageView.html` — full-width stacked sections + jump bar
- `CustomizePageVariant.html` — chip row Ammo/Mag
- 5 CategoryCard variants — fix `dz-rows-100` stretch bug
- Tous les `#tpm_*` translation keys remplacés par anglais littéral dans les 4 pages trader principales

Fichiers Enforce modifiés :
- `TraderXTradingService.GetFavoriteProducts()` (nouvelle méthode)
- `FavoritePageComponent._favoriteProducts` (nouveau binding)
- `PurchasePageComponent` (selected_product_* bindings, filter handlers, OnItemCardSelected)
- `ItemCardComponent` (`m_DisplayMode`, `SetDisplayMode(int)` — route vers row ou card layout)
- `UIEnums.c` (nouvelle enum `EItemCardDisplayMode`)

### 2.5 TraderXBanking — reskiné + ledger complet

**Emplacement** : [src/TraderXBanking/](../src/TraderXBanking/)

**État** : fonctionnel, pipeline compile 4/4 layouts. Backend complet, UI reskinée au Field Ledger.

Livraisons majeures :
- **Tokens TX*** — harmonisés (BankingPageView/AccountTabView/TransferTabView/LedgerTabView)
- **Ledger vertical slice complète** — domain (TXBLedgerEntry, TXBLedgerEntryType, TXBLedgerRepository bufferisé/trimmed), service (RecordLedgerEntry, RecordExternalLedger, SendLedgerToClient, Event_OnLedgerUpdated), RPC (RequestLedger/OnLedgerReceived), UI (TXBLedgerTabComponent + LedgerTabView + 3ème tab)
- **Transfer memo** — `TXBTransaction.note` + Param3 RPC thread + input widget
- **Contacts** — `TXBContactData` + auto-populate on transfer
- **Cross-module labeled payment** — `ITraderXPaymentProvider.DeductLabeled/CreditLabeled` + Garage hook via label `GARAGE_FEE_OUT`

Détail complet dans `C:\Users\Brbap\.claude\projects\c--DayZWorkspace-TraderX\memory\project_banking_ledger_delivered.md`.

### 2.6 TraderXGarage — reskiné

**Emplacement** : [src/TraderXGarage/](../src/TraderXGarage/)

**État** : DZML pipeline activé (nouveau), 2/2 layouts compilent. UI reskinée au Field Ledger (unified view, park-in bar contextuel, cargo grid 40 slots).

Modifs clés :
- `GarageAppComponent.{html,layout}` — vue unifiée 3-col (NPCs sidebar / park-in bar + vehicles list / detail + gauges + cargo grid)
- `GarageVehicleCardComponent.{html,layout}` — dual progress health/fuel + inline park-out button
- `dzml.config.yaml` — nouveau
- Cross-module hook Garage→Banking pour ledger (via ITraderXPaymentProvider.DeductLabeled)
- `CargoSlotViewData.c` — nouveau view model + helper `GetSlotStyleToken`

### 2.7 Documents de référence

- [docs/TraderX_Design_System.md](TraderX_Design_System.md) — **LIRE EN PRIORITÉ** — design system complet
- [docs/TraderX_Banking_Garage_Rework_Assessment.md](TraderX_Banking_Garage_Rework_Assessment.md) — assessment initial du rework
- `C:\Users\Brbap\.claude\plans\favorite-card-have-the-nested-candle.md` — plan d'implémentation initial (partiellement exécuté)
- **Memory files** (mémoire persistante pour les IA) :
  - `project_banking_ledger_delivered.md` — détail banking/ledger
  - `project_agentic_toolchain.md` — info sur dayz-tools/dzml
  - `feedback_use_dayzular_bindings.md` — use DayZular bindings, pas de FindAnyWidget
  - `feedback_no_ternary.md` — pas de `?:` en Enforce, utiliser DayZular Ternary helpers

---

## 3. Toolchain — DZML pipeline

### 3.1 Comment compiler

```bash
cd <module>/datasets/layouts  # ou gui/, ou trader-v3/
node c:/DayZWorkspace/dayz-tools/dayz-layout-tools/dist/bin/dzml.js pipeline
```

Produit : `.layout` (DayZ runtime), `dzml.css` (browser preview), `dzml-styles.css` (custom styles), `*_atlas.edds/.png/.svg`, `*_inline_styles.json`.

### 3.2 Script clean-build

**Emplacement** : [scripts/clean-build-styles.sh](../scripts/clean-build-styles.sh)

Purge le cache + rebuild les 3 modules. **À utiliser quand on renomme/supprime des styles** — le pipeline n'écrase PAS le cache automatiquement, il accumule. Sans purge, des styles obsolètes persistent et les nouveaux peuvent manquer.

**Note** : le script ne couvre PAS encore `trader-v3/`. À rajouter si besoin.

### 3.3 MCP server DZML (non utilisé actuellement)

Existe à `c:/DayZWorkspace/dayz-tools/mcp-server/` mais n'est pas enregistré dans la session Claude Code. Exposerait : `dzml_compile`, `dzml_check`, `dzml_pipeline`, `dzml_decompile`, `dzml_lint`, `dzml_scaffold`. Pour l'activer, configurer le MCP dans `.mcp.json` ou settings Claude.

### 3.4 Preview navigateur

L'utilisateur lance un serveur Python local (ex: `python -m http.server 8092`) et ouvre les HTML dans le navigateur.

Assets nécessaires dans chaque dossier HTML :
- `dzml-runtime.js` (à copier depuis `gui/` — PAS généré par pipeline)
- `dzml.css` (généré)
- `dzml-styles.css` (généré)
- `bg-preview.jpg` (à copier depuis `gui/`)

### 3.5 dzml-runtime.js — mécanismes de résolution

Le runtime browser a DEUX systèmes parallèles :

| Système | Trigger | Résolution |
|---|---|---|
| `loadPage()` | `[data-component]` + `[data-nav-target]` buttons | Utilise `PAGE_MAP` hardcodé OU fallback kebab→PascalCase+`View.html` |
| `loadTemplates()` | `[data-template]` containers + `<template data-template-id data-src>` declarations | Utilise `data-src` explicite |

**Gotcha** : le PAGE_MAP dans `trader-v3/dzml-runtime.js` a été **patché manuellement** avec 12 entrées `tx3-*` car le fallback convention ne matche pas notre naming (`V3*View.html` dans subfolders). Si on ajoute de nouvelles pages V3, il faut aussi mettre à jour ce PAGE_MAP.

---

## 4. Patterns DayZular à respecter

**Impératif** (voir memory `feedback_use_dayzular_bindings.md`) :
- Jamais de `FindAnyWidget` pour data binding — toujours `TextBinding`, `ButtonBinding`, `ComponentWidgetBinding`, `NavButtonBinding`, `EditBoxBinding`
- Ternaires `?:` interdits en Enforce — utiliser `DZTernaryFloat.If(...)`, `DZTernaryString.If(...)`, `DZTernaryBool.If(...)`
- Signatures de fonctions sur **une seule ligne** en Enforce (pas de multi-line signatures)
- Pas de `for (int i = 0; ...)` inline avec déclaration — déclarer `i` hors de la boucle
- Pas d'inline comments après `;` en Enforce (`const int X = 1; // comment` → parse error)
- `ComponentAutoRegister` avec kebab-case selectors
- Static `ScriptInvoker` pour events inter-components (child→parent communication pattern)
- Unsubscribe invokers dans `OnDestroy`
- `MarkForCheck()` après mutation d'état pour trigger re-render

Projets de référence pour patterns : `c:/DayZWorkspace/CourseBarterTrader/` et `c:/DayZWorkspace/RazBentleyWorkbench/`.

---

## 5. Ce qui RESTE à faire

### Priorité HAUTE

1. **Tester V3 Trader en jeu** — jusqu'ici tout est validé en preview browser uniquement. Il faut :
   - Déclarer les scripts v3 dans `config.cpp` de TraderX (ajouter les chemins dans `gameScriptModule`/`worldScriptModule`)
   - Ajouter un hook (NPC action ou debug key) qui ouvre `V3TraderXMainComponent` via `ComponentMenu.ShowMenu()`
   - Charger en jeu → vérifier que les 4 pages fonctionnent avec mock data
   - Valider les interactions : favori toggle, ajout au panier, qty stepper, filter pills, category switch

2. **Décision approche V3 vs v1** — soit :
   - **Option A** : finaliser v3 avec tous les flows + remplacer v1 progressivement (selectors swap `traderx-*` ↔ `tx3-*`, puis bascule du menu opener)
   - **Option B** : back-porter les améliorations V3 dans v1 un par un (plus risqué — v1 a beaucoup de dépendances)

### Priorité MOYENNE

3. **V3 Customize page** — ✅ LIVRÉ. DZML + Enforce + mock data + pipeline compilé.
   - `Customize/V3CustomizePageView.html` — 2 colonnes (options + build summary), preset bar, tab group (variant/attachment/magazine/ammo), options grid 6-col
   - `Prefab/Customize/V3OptionCardView.html` — option card avec checkmark
   - `Prefab/Customize/V3BuildRowView.html` — build row (marker + name + price)
   - `V3MockOption`, `V3MockPreset`, `V3MockBuildRow` — 3 nouveaux mock models
   - `V3MockDataService` — 21 options KA-74 (3 variants + 12 attachments + 4 magazines + 3 ammo), 4 presets (Stock/CQB/Urban/DMR)
   - PAGE_MAP patché avec `tx3-customize-page`, `tx3-option-card`, `tx3-build-row`
   - **À valider** : preview browser puis en jeu. Le bouton "Customize" du Purchase page ne navigue pas encore vers la page Customize (handler `OnPreviewCustomize` stub).

4. **V3 Banking + V3 Garage** — pas encore commencés. Suivre le même pattern que trader-v3/ :
   - Dossier `banking-v3/` + `garage-v3/`
   - Mock services (V3BankingMockService, V3GarageMockService)
   - Sélecteurs `txb3-*` et `txg3-*` (préfixes différents pour éviter collision et path-parsing)
   - Patch PAGE_MAP dans les dzml-runtime.js locaux

5. **Wiring runtime TraderX v1** — certaines améliorations v1 manquent des handlers runtime :
   - `PurchasePageComponent.ApplyFilter()` → pour l'instant ne filtre que par keyword, pas par filter pills active_filter
   - `ItemCardComponent.Event_OnItemCardSelectCallBack` → il faut vérifier qu'il est bien invoqué sur single-click
   - `ItemCardComponent.customize_flag` → à calculer dans UpdateUI selon `item.HasAttachments() || item.HasVariants()`

### Priorité BASSE

6. **V3 ItemCard avec vrai ItemPreviewWidget** — actuellement juste un label placeholder. À remplacer par le vrai widget DayZ 3D preview quand on utilise les vraies entités (pas le mock).

7. **Translation keys re-intégration** — les `#tpm_*` ont été retirées pour preview. Pour la localization runtime, on peut soit :
   - Wrapper chaque texte avec un component-level `SetText(Widget.TranslateString("#tpm_x"))` au OnInit
   - Ou laisser l'anglais en dur (acceptable si pas de besoin multi-langue)

8. **Clean-build script couvre trader-v3/** — ajouter une 4ème section dans `scripts/clean-build-styles.sh` + copier automatiquement `dzml-runtime.js` et `bg-preview.jpg` après rebuild.

9. **Polish final** : translations audit, doc changelog, final pipeline.

---

## 6. Gotchas importants à connaître

### 6.1 EnforceScript quirks

- `for (int i = 0; i < X; i++)` peut parfois ne pas compiler selon le contexte. Si erreur E107 → déclarer `i` avant.
- Pas de `?:`. Utiliser `DZTernary*.If(cond, a, b)`.
- Les default parameters peuvent poser souci si > 1. Mieux : paramètres explicites partout.
- Inline comments après `;` font parse-error parfois.
- `static ref ScriptInvoker` pour events inter-components.

### 6.2 DZML pipeline

- Le cache ne se purge PAS automatiquement. Si rename/delete de styles → lancer `clean-build-styles.sh`.
- `dz-c-FE` = couleur quasi-blanche (byte = 254). Pour transparent : `dz-c-00000000`. Pour alpha : `dz-c-RRGGBBAA`.
- `dz-c-` avec panel sans `data-style` → background rendu selon cette couleur (si `FE` → blanc visible, gros bug visuel).
- `data-component` = runtime DayZ only. `data-template` = browser preview only. Utiliser les 2 ensemble pour double-compat.

### 6.3 V3 specifics

- Selectors `tx3-*` (pas `v3-*` — préfixe `v3-` causait le runtime à chercher dans dossier `V3/`).
- PAGE_MAP dans `trader-v3/dzml-runtime.js` est MANUELLEMENT patché. Si nouvelle page V3 → ajouter entry ou ça fera 404.
- `V3MockDataService` est lazy-init via `GetInstance()`. Pas besoin de l'initialiser manuellement.

### 6.4 Modules indépendants

- Trader / Banking / Garage sont 3 menus INDÉPENDANTS. Pas de cross-module navigation (pas de tabs TraderX↔Banking↔Garage dans le shell). Chaque module a son propre NPC qui ouvre son propre shell.

### 6.5 v2 folder

- **OFF LIMITS** — directive utilisateur explicite. Ne pas modifier.

---

## 7. Mémoire persistante Claude Code

L'utilisateur utilise le système de mémoire auto de Claude Code.

**Emplacement** : `C:\Users\Brbap\.claude\projects\c--DayZWorkspace-TraderX\memory\`

**Fichiers pertinents** :
- `MEMORY.md` — index
- `feedback_no_ternary.md` — pas de `?:`
- `feedback_use_dayzular_bindings.md` — bindings framework
- `project_agentic_toolchain.md` — dayz-tools / dzml
- `project_banking_ledger_delivered.md` — banking ledger détail

**Convention** : écrire de nouvelles memory files pour les décisions durables, update `MEMORY.md` avec un bullet par fichier.

---

## 8. Commandes utiles

```bash
# Build pipeline pour un module
cd c:/DayZWorkspace/TraderX/src/<MODULE>/datasets/layouts
node c:/DayZWorkspace/dayz-tools/dayz-layout-tools/dist/bin/dzml.js pipeline

# Clean-rebuild tous les modules (v1 + Banking + Garage ; v3 pas encore)
bash c:/DayZWorkspace/TraderX/scripts/clean-build-styles.sh

# Preview browser — lancer le python server depuis trader-v3/
cd c:/DayZWorkspace/TraderX/src/TraderX/datasets/gui/trader-v3
python -m http.server 8092
# puis ouvrir http://localhost:8092/TraderXV3MainView.html

# Compile un seul layout
node c:/DayZWorkspace/dayz-tools/dayz-layout-tools/dist/bin/dzml.js compile <file>.html

# Check un layout (lint + binding + style refs)
node c:/DayZWorkspace/dayz-tools/dayz-layout-tools/dist/bin/dzml.js check <file>.html
```

---

## 9. Fichier à lire EN PREMIER pour prendre le relais

1. Ce document (`docs/AI_Handover.md`)
2. [docs/TraderX_Design_System.md](TraderX_Design_System.md) — comprendre le design system
3. [src/TraderX/datasets/gui/mockup/trader/favorites.html](../src/TraderX/datasets/gui/mockup/trader/favorites.html) — voir le mockup cible
4. [src/TraderX/datasets/gui/trader-v3/TraderXV3MainView.html](../src/TraderX/datasets/gui/trader-v3/TraderXV3MainView.html) — voir la v3 actuelle
5. [src/TraderX/scripts/4_World/classes/TraderX/GUI/v3/Mock/V3MockDataService.c](../src/TraderX/scripts/4_World/classes/TraderX/GUI/v3/Mock/V3MockDataService.c) — comprendre le mock data pattern

---

## 10. Contact état — ce que j'ai compris du user

- **Approche préférée** : chantiers bien bornés, livrables concrets, pas de sur-abstraction
- **Frustration** quand je patchouille plusieurs fichiers au lieu de faire propre. Préfère reconstruction isolée (d'où la v3).
- **Toujours valider visuellement** — il travaille beaucoup en preview browser pour voir le résultat avant de tester en jeu.
- **Framework first** — utiliser DayZular correctement (bindings, invokers, ComponentBase lifecycle). Jamais de workaround à la main.
- Parle français mais accepte les commits/commentaires en anglais (pour cohérence avec la codebase).

---

**Dernier commit de contexte** : 2026-04-18 — Trader v3 livré en preview browser + pipelines vert + PAGE_MAP patché pour tx3-* selectors. Customize V3 ajouté (DZML + Enforce + mock data, pipeline 15/15 compilé). Prochaine étape recommandée : validation preview browser Customize → validation en jeu.
