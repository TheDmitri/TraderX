# US-020 — Style re-skin : palette CourseBarterTrader (tokens only)

**Périmètre :** TraderX (core) + TraderXBanking (styles partagés)
**Priorité :** P2

**Given** l'UI TraderX en design Field-Ledger (amber/olive, fonds quasi-noirs)
**When** on retargete les tokens couleur vers ceux de CourseBarterTrader
**Then** l'UI adopte le look barter — accent or #C4A35A, vert #6B8E5A, fonds slate froid #0F1215/#1A1D21 — sans changer la structure des styles `TX*` ni l'atlas 9-slice

---

## Décision (session 2026-06-17)

**Scope = « align tokens only »** : on garde l'atlas `traderx_atlas` + les styles `TX*`, on ne réécrit QUE la palette (et, en option séparée, les polices). Constat : la palette TraderX était déjà proche (amber #D4A44C ≈ or #C4A35A, olive #7A9A4A ≈ vert #6B8E5A) ; le vrai delta visuel = les fonds (near-black #08090c → slate CBT #0F1215/#1A1D21).

### Mapping appliqué

| Rôle | Avant | Après (CBT) |
|---|---|---|
| Accent | amber `#D4A44C` | or `#C4A35A` |
| Action positive | olive `#7A9A4A` | vert `#6B8E5A` |
| Info | steel `#4A7AC7` | bleu `#5C8FC7` |
| Erreur | `#C93030` | `#C45050` |
| Fond racine/panel | `#08090C` / `#0A0B10` | `#0B0E12` / `#0F1215` |
| Header/surface/card | `#0C0E14` / `#10131B` | slate `#1A1D21` / `#15181D` |
| Elevated/hover/border | `#161A24` / `#1A1E28` | `#20242A` / `#2B3138` |

## Fichiers modifiés

| Fichier | Changement |
|---|---|
| `datasets/gui/_fixtures/field-ledger.html` | **Source unique** des tokens dz-styles — bloc réécrit (slate + or/vert/bleu CBT) |
| `datasets/styles/traderx_{styles.styles,ui.imageset,atlas.svg,atlas.png,atlas.edds,atlas.edds.meta}` | Régénérés via `scripts/build-styles.sh` |
| `datasets/gui/dzml-styles.css` | Régénéré (preview browser) |
| 14 `.html` + `.layout` | Remplacement inline `dz-c-D4A44C→C4A35A`, `dz-c-7A9A4A→6B8E5A`, recompilés |
| `scripts/3_Game/Constant/ViewStyles.c` | Tokens int alignés (UI_GREEN, + UI_GOLD, CATEGORY_*, TEXT_GRAY, NOTIFICATION_*) |

## Tests

- **Pipeline** : `build-styles.sh` ✓ (atlas 448×448 régénéré, edds game-readable ✓), `dzml compile` ✓ sur les 15 layouts touchés
- **Acceptance (source de vérité)** : Manuel in-game + preview browser

## Étapes de validation

1. **Preview browser** : ouvrir une page bundle → vérifier fonds slate + accents or/vert
2. **Build** : `.\Build.ps1` → in-game
3. **Vérifier** : header/cartes en slate CBT, onglet actif souligné or, boutons Buy verts / Accent or, focus input or
4. **Vérifier** : aucun contraste cassé (texte lisible sur les nouveaux fonds plus clairs)
5. **Vérifier** : toast US-019 cohérent avec la nouvelle palette

## Done

- [ ] Atlas + CSS régénérés, layouts recompilés
- [ ] ViewStyles.c aligné
- [ ] Preview browser OK
- [ ] In-game : look barter cohérent, pas de régression contraste
- [ ] Dmitri validé in-game

## Reste / hors-scope de cette story

- **Polices** (Oswald headers + JetBrains Mono chiffres) : ~2000 refs path-based `RobotoCondensedVF*` sur ~111 layouts → décision séparée (blanket rename vs split sémantique vs report).
- **Nettoyage** inline cruft : `dz-c-00FF00` (~80, debug), `dz-c-FF0000` (~3) — non touchés.
- TraderXBanking/Garage : partagent l'atlas, donc déjà reskinnés ; valider visuellement.
