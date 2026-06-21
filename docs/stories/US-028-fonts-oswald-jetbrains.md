# US-028 — Bascule polices : Oswald + JetBrains Mono

**Périmètre :** TraderX (core)
**Priorité :** P3

**Given** le système d'alias polices (US-026) pointant encore sur RobotoCondensed
**When** on bascule les alias vers les polices de CourseBarterTrader
**Then** TraderX utilise Oswald (texte/titres) + JetBrains Mono (chiffres) — typographie identique au barter

---

## Changements

1. **Fonts copiées** de CBT → `src/TraderX/datasets/fonts/` : `oswalt{12,14,16,22,28,48,58}.{fnt,edds}` + `Oswald-Regular.ttf` + `oswalt.xml` ; `jet-brains-mono*{,.fnt,.edds}` + `JetBrainsMono-Regular.ttf` + `jet-brains-mono.xml`. Aucune entrée `config.cpp` nécessaire (chargées par chemin, comme RobotoCondensed avant).
2. **Alias bascule** (`dzml.config.yaml`, **un seul fichier**) :
   - `heading` / `body` / `default` → `TraderX/datasets/fonts/oswalt` (Oswald)
   - `mono` → `TraderX/datasets/fonts/jet-brains-mono`
3. **Tag `mono`** sur les widgets numériques (prix, qty, stock, totaux, wallet, compteurs) : `dz-font-{heading,body}-N` → `dz-font-mono-N` sur `priceText, stockText, productPrice, productQty, totalAmount, moneyAmount, walletHeaderAmount, itemCountText, slotCount, sellPrice, buyPrice, quantityStr, PaymentAmount, …` (rewrite tag-scoped, n'affecte pas icônes/labels).

Résultat (layouts in-game, hors `.bundle`) : **0 ref RobotoCondensed**, tout en `oswalt*` + `jet-brains-mono*`. 45 layouts recompilés OK ; alias vérifiés (`dz-font-heading-28`→`oswalt28`, `dz-font-mono-14`→`jet-brains-mono14`).

## Re-skin futur

Changer toute la typo = éditer les 4 lignes `fonts:` de `dzml.config.yaml` + déposer les `.fnt/.edds` bakés + recompiler. Zéro édition de layout (objectif du plan d'unification atteint).

## Reste

- `RobotoCondensed*` toujours présents physiquement (référencés par les `.bundle.html`/`.layout` de preview, non chargés en jeu) — suppression possible après régénération des bundles (`dzml bundle`).
- Couverture `mono` : widgets numériques principaux taggés ; un widget numérique oublié reste en Oswald (ajout incrémental si repéré in-game).

## Tests

- **Pipeline** : `dzml compile` ✓ 45 layouts ; 0 Roboto in-game ; alias → oswalt/jet-brains vérifié.
- **Acceptance** : in-game.

## Étapes de validation (in-game)

1. **Build** : `.\Build.ps1`
2. **Vérifier** : titres/labels en Oswald (condensé), prix/qty/wallet/totaux en JetBrains Mono (chiffres tabulaires)
3. **Vérifier** : pas de glyphe manquant / texte vide (tailles bakées 12-28 OK)

## Done

- [ ] In-game : Oswald partout, chiffres en mono
- [ ] Pas de police manquante
- [ ] Dmitri validé in-game
