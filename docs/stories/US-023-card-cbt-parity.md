# US-023 — Cartes & product card : parité visuelle CourseBarterTrader

**Périmètre :** TraderX (core)
**Priorité :** P2

**Given** les cartes TraderX au stroke épais et l'item card encore en styles legacy (DA_Smooth / Corny), hors design Field-Ledger
**When** on affine le stroke et on reskinne l'item card vers les styles TX
**Then** les cartes (et surtout la product card) ressemblent à celles du barter : bordure fine, fond slate, prix or, puits de preview — sensation de composants partagés entre les deux UI

---

## Constat

L'item card (`ItemCardMediumView` & variantes) utilisait des styles **non-TX** (`DA_Smooth_Small_10px`, `DA_Smooth_Small_5px`, `Corny_Square_Shadow_5px`, `DA_Smooth`) → empilement de 3 couches bordées = stroke épais, et palette hors Field-Ledger. C'était la cause principale de l'écart visuel vs CourseBarterTrader (dont la `BrowseCardComponent` est une surface unique propre).

## Changements

| Levier | Détail |
|---|---|
| **Stroke fin (global)** | `scripts/build-styles.sh` : `--stroke-width 1` (était 2). Atlas régénéré → toutes les cartes/boutons TX ont une bordure fine. |
| **Style anneau de sélection** | Nouveau token `TXCardSelected` (fond slate, **bordure or #C4A35A**) dans `_fixtures/field-ledger.html` — pour `itemCardOutline` (alpha togglé à la sélection). |
| **Tints item card** | `ViewStyles.c` : `ITEM_CARD_IDLE` → `#FEFEFE` (blanc, laisse voir le style), `ITEM_CARD_WARNING` → `#E6B8A0` (tint chaud léger). Avant ils étaient des fills sombres → auraient noirci un widget stylé (gotcha tint). |
| **Reskin variantes** | `TraderXPrefab/ItemCard/*.html` : `DA_Smooth_Small_10px→TXCard` (surface), `DA_Smooth_Small_5px→TXCardSelected` (anneau), `Corny_Square_Shadow_5px→TXSurface` (puits de preview), `DA_Smooth→TXProgressFill` (barre volume), `DA_Smooth10px_Colored→TXCardElevated`. |
| **Polish Medium** | prix + coin en or `#C4A35A`, nom en crème `#D8D2C0`, stock en gris muted `#9A9488` (hiérarchie type CBT). |

Contrat de widgets **inchangé** (mêmes `data-name` : itemCardOutline/itemCardContent/stockText/priceText/classNameText/volumeBar/favBtn/selected-unselected/healthImg…) → aucun binding `.c` cassé. Seuls styles/couleurs ont changé, pas la structure.

## Tests

- **Pipeline** : `build-styles.sh` ✓ (atlas 448×512, stroke 1px), `dzml check`+`compile` ✓ sur 5 variantes ItemCard.
- **Acceptance** : Manuel in-game + preview browser.

## Étapes de validation

1. **Preview browser** : grid d'items → bordure fine, fond slate, prix or, anneau or à la sélection
2. **Build** : `.\Build.ps1` → in-game
3. **Vérifier** : product card lisible, hiérarchie nom/prix/stock, état warning (stock bas) en tint chaud léger (pas noir)
4. **Vérifier** : sélection → anneau or ; hover/pressed OK
5. **Vérifier** : variantes Small/Large/Catalog/Tooltip cohérentes

## Done

- [ ] Stroke fin visible partout
- [ ] Product card proche de la BrowseCard CBT (slate + or + puits preview)
- [ ] Pas de carte noircie (tint idle/warning OK)
- [ ] Pas de régression binding/sélection/tooltip
- [ ] Dmitri validé in-game

## Reste / hors-scope

- **CategoryCard** (`DA_Squad`, `DA_Smooth10px_Colored`) non reskinnée — story suivante si besoin.
- Restructuration profonde façon CBT (pill condition, prix mono, barre temps) : itération future avec feedback in-game.
- Polices mono pour les chiffres : dépend de la story polices différée.
