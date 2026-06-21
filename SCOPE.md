# SCOPE — TraderX

**Statuts :** ✅ livré · 🟡 en cours · 🔴 bloqué · ⚪ à faire  
**Règle :** > 300 lignes → archiver agressivement.

---

## 1. Vision

TraderX est un écosystème de trading complet pour serveurs DayZ : achat/vente d'items via des NPCs Traders, système bancaire persistant (comptes, virements, ledger, vault), et marketplace de véhicules (Garage).  
Le système est modulaire (chaque module est un PBO indépendant), configurable par les admins sans code, et extensible via interfaces cross-module.  
Cible : serveurs communautaires DayZ voulant une économie structurée sans simulation automatique.

---

## 2. Non-goals

- Pas de simulation de marché (prix auto basé sur l'offre/demande globale entre joueurs)
- Pas de système d'enchères ou de marketplace joueur-à-joueur (hors Garage)
- Pas d'interface web ou app mobile d'administration
- Pas de multi-serveur / synchronisation inter-serveurs
- Pas de système de crédit / prêts entre joueurs
- Pas de factions distinctes du système de clans Banking
- Pas de système de crédit / prêts entre joueurs
- V3 Banking UI et V3 Garage UI = future (hors scope sprint actuel)

---

## 3. Carte des features

### TraderX (core)

- US-001 — V3 Trader : tester en jeu (config.cpp, hook NPC, 5 pages) 🟡
- US-002 — V3 Trader : décision approche V3 vs v1 + bascule opérationnelle ⚪
- US-003 — V1 Purchase : filtre par pills `active_filter` dans `ApplyFilter()` ⚪
- US-019 — Notification rework : toast HUD FIFO (port CourseBarterTrader) + service DayZular 🟡
- US-020 — Style re-skin : palette CourseBarterTrader (or/vert/slate, tokens only) 🟡
- US-021 — Footer promo DayZ Beans Launcher + réactivation bouton LAUNCHER 🟡
- US-022 — Migration services core (Currency/Transaction/Selection) → registry DayZular 🟡
- US-023 — Cartes & product card : parité CBT (stroke fin, item card reskin TX) 🟡
- US-024 — Reskin styles legacy (DA_*/Corny_*/TP_*) → TX sur ~14 layouts 🟡
- US-025 — Suppression TraderXViewStyles → styles + TraderXTheme (1% runtime) 🟡
- US-026 — Système d'alias polices (dzml.config.yaml heading/body/mono) 🟡
- US-027 — Consolidation binders → DZlr unique : 68 fichiers Dabs/notif-v1 supprimés, logique migrée 🟡
- US-028 — Bascule polices Oswald + JetBrains Mono (via alias dzml.config) 🟡
- US-004 — V3 Customize : wirer `OnPreviewCustomize` handler (bouton Purchase → Customize) ⚪
- ✅ Système achat/vente/checkout (transaction lock anti-duplication)
- ✅ Recherche (case-insensitive, 300ms debounce)
- ✅ Tri par prix (↑/↓)
- ✅ Expand/Collapse catégories
- ✅ Clear checkout button
- ✅ Rotation player preview
- ✅ Page Catalog
- ✅ Page Favorites
- ✅ Dynamic pricing (coefficient stock)
- ✅ Preset system (armes + attachments)
- ✅ Stock behavior (keep/reset/random au restart)
- ✅ Multi-format config (Legacy JSON / CSV / Compiled)
- ✅ V3 MainView + 5 pages (browser validé, mock data, pipeline vert)

### TraderXBanking

**Account**
- ✅ Solde + progression + dépôt + retrait — AC-01/02/03
- ✅ Historique 30 jours — AC-04
- ✅ Stats 30 jours (total dépôts/retraits/transferts) — AC-06
- US-006 — AC-05 : Formatage Account ID (76561198·XXXX·XXXX) ⚪ *(bouton copie fonctionne, formatage manquant)*

**Vault**
- ✅ Vault personnel (backend + UI + dépôt/retrait/take-all) — VA-01/03/04/05/08
- US-008 — VA-06 : Frais quotidiens vault (job auto serveur, débit compte) ⚪
- US-016 — VA-02/07 : Vault clan + sélecteur multi-propriétaire ⚪ *(dépend US-014)*

**Transfer**
- ✅ Virement P2P avec memo, validation destinataire, frais — TR-01/03/04
- US-005 — TR-02 : Contacts UI dans l'onglet Transfer 🟡
- US-017 — TR-05 : Transfert vers compte clan ⚪ *(dépend US-014)*

**Ledger**
- ✅ Grand livre complet, filtrage par type, séparateurs journaliers — LE-01/02/06
- US-018 — LE-03/04 : Recherche ledger + panel détail complet ⚪

**Clans** *(release 1)*
- US-014 — Clan backend : `TXBClanService` + `TXBClanRepository` + `TXBClanInviteRepository` ⚪
- US-015 — Clan RPC + wiring UI : création, invites, roster, banque clan ⚪ *(dépend US-014)*
- *(Modèles `TXBClanData`, `TXBClanInvite`, `TXBClanRole` et composants UI skeleton existants)*

**Home Dashboard**
- ✅ Hero card, raccourcis rapides, activité récente — HO-01/03/04
- US-019 — Home KPI complet : vault card, clan card, alertes, invites en attente ⚪ *(dépend US-014)*

**Infrastructure Banking**
- ✅ Contacts auto-ajoutés après virement (modèle `TXBContactData`)
- ✅ ITraderXPaymentProvider cross-module (Garage → Banking ledger)
- ✅ Field Ledger UI reskin (tokens TX* harmonisés)
- ✅ DZML pipeline Banking (4/4 layouts compilent)

### TraderXGarage

- US-010 — Garage : validation backend complet (parking, récupération véhicule) 🔴
- ✅ DZML pipeline Garage (2/2 layouts compilent)
- ✅ UI reskinée Field Ledger (vue unifiée, park-in bar contextuel, cargo grid 40 slots)
- ✅ Cross-module hook Garage → Banking (`GARAGE_FEE_OUT`)
- ✅ Vehicle orientation fix (roll trick physique)

### TraderXCarLock

- US-011 — CarLock : implémentation et validation en jeu ⚪
- ⚪ Actions lock/unlock véhicule

### Infrastructure & Toolchain

- US-012 — clean-build script : couvrir trader-v3/ + copie auto dzml-runtime.js ⚪
- US-013 — Traduction : ré-intégration des `#tpm_*` keys dans les 4 pages v1 ⚪
- ✅ DZML pipeline toolchain (dayz-tools)
- ✅ detect_redecl.py (détection conflits de classes cross-PBO)
- ✅ Build.ps1 (build PBOs + launch serveur)

---

## 4. Backlog priorisé

| Priorité | Story | Raison |
|---|---|---|
| P1 | US-014 Clan backend | Débloque US-015/016/017/019 — zéro backend actuellement |
| P1 | US-001 V3 Trader en jeu | Validation finale avant bascule sur v1 |
| P1 | US-005 Contacts UI Banking | Feature visible, modèle déjà existant |
| P1 | US-010 Garage backend | Bloquant pour le module Garage |
| P2 | US-015 Clan RPC + UI | Rend le système clan utilisable |
| P2 | US-016 Vault clan UI | Dépend de US-014 |
| P2 | US-017 Transfer vers clan | Dépend de US-014 |
| P2 | US-008 VA-06 Frais Vault | Feature Vault complète |
| P2 | US-018 Ledger recherche + détail | UX ledger incomplète |
| P2 | US-002 Décision V3 vs v1 | Unblocks tout le reste du Trader |
| P3 | US-019 Home KPI complet | Dépend de US-014 |
| P3 | US-004 V3 Customize wiring | Complète la page Customize |
| P3 | US-003 V1 ApplyFilter pills | UX filtre incomplète |
| P3 | US-006 AC-05 Formatage ID | QoL mineur |
| P3 | US-011 CarLock en jeu | Module à valider |
| P3 | US-012 clean-build trader-v3 | Toolchain confort |
| P3 | US-013 Traductions v1 | Localisation |

---

## 5. Bugs actifs

### Page Catalogue
- [ ] the search icon should be inside the search input field, like we need to make a component that have a border around the input field and the search icon inside it. 
- [ ] the item preview background could be darker perhaps
- [ ] the catalogue item card list could be improved : the item preview is too small, we need something a bit bigger
- [ ] the catalogue category with icon sort and icon collapse would need to be improved to have same color and feels connected
- [ ] the catalogue item card is a small list, it needs to look like a card like the item purchase card
- [ ] the button wth icon grid/list and text needs spacing between icon and icon, the button text needs to change based on the state. currently it's stuck at List
### Purchase Page
- [ ] the player preview container left border needs to be more spaced from the edge
- [ ] the left icon from the player preview needs to be changed to a different icon
- [ ] the search icon should be inside the search input field, like we need to make a component that have a border around the input field and the search
- [ ] the purchase item card favorite icon has an issue, the button needs to be empty so only the image deal with the save
- [ ] the purchase item card needs to have a better style with newly homemade icon made for the card and better contrast with preview, text and money
- [ ] sane goes for the purchase item card list
- [ ] the expand all buttom doesn't work anymore
### Customize page
- [ ] the presets, variants and attachments button aren't working properly nor it shows any of the grid card
- [ ] the item preview rotation and scale needs to be activated
- [ ] there is a all section on the top with a close button that is empty and no use.
### Favorites page
- [ ] the item card needs better rework like purchase and catalogue pages
- [ ] the checkout card could have better icon for the -, + and delete
- [ ] the search icon should be inside the search input field, like we need to make a component that have a border around the input field and the search icon inside it. 
### Sell page
- [ ] the checkout to pay and money in is fucked up, too minimized 
- [ ] the search icon should be inside the search input field, like we need to make a component that have a border around the input field and the search icon inside it. 
 
---

## 6. Archivé

### Features livrées

- ✅ Transaction lock anti-duplication (2026-02) — `TraderXTradingService`
- ✅ Search debounce 300ms (2026-02) — `PurchasePageViewController`
- ✅ Search case-insensitive (2026-02) — `CategoryCardViewController`
- ✅ Stock behavior random au restart (2026-02) — `TraderXConfigurationService.ApplyStockBehaviorAtRestart()`
- ✅ Notification sell/buy correcte (2026-02) — fix sérialisation `TraderXTransactionType`
- ✅ Tooltip persistence fix (2026-02) — `ItemCardViewController.DestroyAllTooltips()`
- ✅ Vehicle orientation (2026-02) — roll trick dans `TraderXVehicleFactory`
- ✅ Sort label "Price ↑/↓" (2026-02) — `stringtable.csv`
- ✅ Clear checkout button (2026-03) — `CheckoutViewController`
- ✅ Expand/Collapse categories (2026-03) — `CategoryCardViewController`
- ✅ Player preview rotation (2026-03) — `PlayerPreviewController` ignorepointer fix
- ✅ Catalog page (2026-03) — `CatalogPageViewController`
- ✅ Banking Ledger complet (2026-04-17) — domain + service + RPC + UI + filtrage par type
- ✅ Transfer memo (2026-04-17) — `TXBTransaction.note` + Param3 RPC
- ✅ Contacts auto-ajoutés (2026-04-17) — `TXBContactData`
- ✅ Cross-module labeled payment (2026-04-17) — `ITraderXPaymentProvider`
- ✅ Stats 30 jours Banking — AC-06 (US-007 archivé)
- ✅ Garage UI Field Ledger reskin (2026-04-18) — DZML pipeline vert
- ✅ V3 Trader (browser) — MainView + 5 pages + Customize + mock data (2026-04-18)

---

## 7. Tips

- command to open the workbench in linux from cmd: `"S:\common\DayZ Tools\Bin\Workbench\workbenchApp.exe" "P:\TraderX\Workbench\dayz.gproj"`
