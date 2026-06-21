# US-026 — Système d'alias de polices DayZular

**Périmètre :** TraderX (core) + outil dzml
**Priorité :** P3

**Given** ~375 refs de police **path-based** (`dz-font-TraderX/datasets/fonts/RobotoCondensedVF{N}`) + quelques numériques (qui tombaient sur etelka)
**When** on introduit des **alias sémantiques** via `dzml.config.yaml`
**Then** changer la typo de toute l'UI = éditer **1 fichier** (les alias), sans toucher aux layouts

---

## Changements

1. **Outil** (`dayz-tools/dayz-layout-tools/src/commands/compile.ts`) : recherche **ascendante** de `dzml.config.yaml` (`findConfigUpward`) — un seul config couvre tous les sous-dossiers de layouts (avant : cherché seulement à côté du fichier). Rebuild `tsc` → `dist/`.
2. **Config** : `src/TraderX/datasets/gui/dzml.config.yaml` avec alias `heading` / `body` / `mono` / `default` → préfixe `TraderX/datasets/fonts/RobotoCondensedVF` (le compilo append la taille).
3. **Layouts** : `dz-font-…RobotoCondensedVF{N}` et `dz-font-{N}` → `dz-font-{alias}-{size}` :
   - 28/24 → `heading-28`, 22/20 → `heading-22`, 48 → `heading-48`
   - 16 → `body-16`, 14 → `body-14`, 12/11/10/9/13 → `body-12`
   - Tailles cassées **VF20/VF24** (fnt absent) snappées vers 22/28 (baked) — fix de bug au passage.
   - Numériques (qui résolvaient en **etelka**, incohérent) ramenés sur Roboto via alias.

Résultat : `dz-font-body-14` → `font "TraderX/datasets/fonts/RobotoCondensedVF14"` (vérifié, y compris sous-dossiers profonds). 51 layouts recompilés OK, sortie identique à avant (même police), mais désormais **aliasée**.

## Bénéfice (prépare US-028)

Basculer vers Oswald/JetBrains = éditer `dzml.config.yaml` :
```yaml
heading: TraderX/datasets/fonts/oswalt
mono:    TraderX/datasets/fonts/jet-brains-mono
```
+ copier les fonts bakées + recompiler. Zéro édition de layout.

## Reste

- **`mono`** défini mais pas encore assigné aux widgets numériques (prix/qty/compteurs) — passe manuelle en US-028 pour que les chiffres prennent JetBrains.
- 6 refs numériques résiduelles probablement dans des `.bundle.html` (artefacts) — non bloquant.
- Banking/Garage : ajouter leur propre `dzml.config.yaml` (ou un partagé) le moment venu.

## Tests

- **Pipeline** : `dzml compile` ✓ 51 layouts ; alias → bon path vérifié (racine + sous-dossier).
- **Acceptance** : in-game (police identique = pas de régression visuelle attendue).

## Done

- [ ] In-game : textes rendus normalement (police inchangée)
- [ ] Dmitri validé in-game
