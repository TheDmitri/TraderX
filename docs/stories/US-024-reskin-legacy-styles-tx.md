# US-024 — Reskin styles legacy → TX (parité CBT)

**Périmètre :** TraderX (core)
**Priorité :** P2

**Given** ~15 composants encore sur des styles legacy (`DA_*`, `Corny_*`, `TP_*`, `RoundedPanel_Colorable`) hors design Field-Ledger
**When** on remappe ces styles vers les styles `TX*` de l'atlas
**Then** toute l'UII (category cards, modals, preset/variant cards, combobox, player preview, payment resume, upgrade page) passe au look slate + bordure fine, cohérent avec CourseBarterTrader

---

## Pré-vérifs (sécurité)

- Tokens `CATEGORY_*` de `TraderXViewStyles` = **morts** (aucune ref hors ViewStyles.c) → pas de recolor à casser.
- Les panels « colorable » (`*_Colored`/`*_Colorable`) ne sont **pas** `SetColor`-és dans les binders (seulement `SetAlpha`) → remap vers styles TX sûr.
- Les vrais `SetColor` d'état (item card, stock, health, nav, slot, preset) sont **hors scope** → traités en US-025.

## Mapping appliqué (sed, source `.html` only, hors `.bundle.html` + hors `Notification/` dormant)

| Legacy | → TX |
|---|---|
| DA_Round1, DA_Smooth, DA_Smooth_Small_10px(_Stroke), DA_Squad, RoundedPanel_Colorable, TP_BackgroundStroke | `TXCard` |
| DA_Smooth_Small_5px, DA_Smooth5px_Colored, Corny_Square_Shadow_5px | `TXSurface` |
| DA_Smooth10px_Colored, DA_Squad1 | `TXCardElevated` |
| TP_CardBtn | `TXBtnDefault` |
| TP_PurchaseBtn | `TXBtnPrimary` |

**14 fichiers source** reskinnés. Recompilés : 51 layouts OK (les 2 "échecs" = `gallery.html` + `_fixtures/field-ledger.html`, qui ne sont pas des layouts).

Laissés volontairement : `DayZNormal` (style de texte vanilla, OK), `Corny_Rect_Side_*` (ancienne pile notif **dormante** → sera supprimée avec elle), `rover_sim_colorable` (recolor runtime légitime).

## Reste / suites

- **Bundles preview** (`*.bundle.html`) encore en legacy → régénérer via `dzml bundle` (artefacts, non bloquant).
- Quelques `DA_*` peuvent être sur des **boutons** (perte du feedback hover/pushed, clic OK) → affiner si visible in-game.
- US-025 : suppression `TraderXViewStyles` + `SetColor` d'état → styles/états.

## Tests

- **Pipeline** : `dzml compile` ✓ sur tous les layouts source.
- **Acceptance** : preview browser + in-game.

## Done

- [ ] Preview browser : plus de panneaux gris plats / styles legacy
- [ ] In-game : category cards, modals, preset/variant, combobox, player preview cohérents slate+or
- [ ] Pas de régression de clic/sélection (SetAlpha intact)
- [ ] Dmitri validé in-game
