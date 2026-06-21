# US-025 — Suppression de `TraderXViewStyles` → styles + `TraderXTheme` minimal

**Périmètre :** TraderX (core)
**Priorité :** P2

**Given** des couleurs codées en dur éparpillées (`TraderXViewStyles` + `SetColor` littéraux)
**When** on supprime `TraderXViewStyles` et on ne garde en code que le strict runtime
**Then** ~99% des couleurs viennent des styles atlas ; le reste tient dans un seul fichier `TraderXTheme` (1% runtime), facile à re-skinner

---

## Ce qui a été fait

1. **Supprimé `ViewStyles.c`** (`TraderXViewStyles`). Tokens **morts** retirés : `UI_GREEN/GOLD/BLACK`, `CATEGORY_*`, `TEXT_IDLE`, `TEXT_GRAY`, `NOTIFICATION_WARNING` (aucune ref).
2. **Créé `3_Game/Constant/TraderXTheme.c`** — seules les couleurs appliquées au **runtime par état** (qu'aucun style statique ne peut exprimer) :
   - `CARD_IDLE`/`CARD_WARNING` (tint item card)
   - `TEXT_OK`/`TEXT_WARN` (dispo/indispo tooltip)
   - `NAV_ACTIVE`(or)/`NAV_INACTIVE`, `SLOT_ACTIVE`(rouge)/`SLOT_INACTIVE`
   - `STOCK_LOW`, `PRESET_DEFAULT`/`PRESET_NORMAL`, `NOTIF_SUCCESS`/`NOTIF_ERROR`
   - Alignées palette CBT (or #C4A35A / vert #6B8E5A / rouge #C45050).
3. **Repointé** les 6 tokens vivants + 8 `SetColor`/`SetTextColor` littéraux → `TraderXTheme.*`. Les **deux** piles de binders (Dabs `*ViewController` + DZlr `*Component`) ont été mises à jour (nav, slot, preset, item card, stock).
4. **Aligné** au passage : nav actif olive `#afbc38` → or `#C4A35A` ; stock-bas rouge pur `#ff0000` → `#C45050`.

## Hors scope (reste en code, légitime)

- **Dégradé de santé** (`GetHealthFromLevel` dans `TraderXCoreUtils`) : calcul runtime par état de condition, garde ses RGB.
- Couleurs inline `dz-c` dans les layouts (~442) : déjà au niveau layout/atlas, pas du code — re-skinnables via tokens/atlas.

## Tests

- **Statique** : 0 ref `TraderXViewStyles` restante (hors commentaire doc). `TraderXTheme` utilisé par 12 fichiers. Littéraux ciblés tous remplacés.
- **Acceptance** : in-game (pas de compilateur Enforce hors-jeu).

## Étapes de validation (in-game)

1. **Build** : `.\Build.ps1`
2. **Vérifier** : onglet actif = label or, inactif = gris ; item card warning (stock atteint) = tint chaud (pas noir) ; stock bas = texte rouge ; slot joueur actif = rouge ; preset par défaut = marqueur rouge ; tooltip dispo/indispo = vert/brun
3. **Vérifier** : aucune erreur de compilation Enforce (classe `TraderXViewStyles` supprimée — vérifier qu'aucun PBO externe ne la référence)

## Done

- [ ] Build sans erreur (classe supprimée)
- [ ] États de couleur corrects in-game
- [ ] Dmitri validé in-game

## Note

Cible finale (plan §6) : re-skin complet = éditer `field-ledger.html` (atlas) + `dzml.config.yaml` (polices) + `TraderXTheme.c` (1% runtime). Trois fichiers, zéro chasse au `SetColor`.
