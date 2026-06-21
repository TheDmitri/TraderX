# US-021 — Footer promo DayZ Beans Launcher

**Périmètre :** TraderX (core)
**Priorité :** P3

**Given** la vue principale du trader (TraderXMainView)
**When** elle s'affiche
**Then** une barre footer discrète en bas affiche « Sponsored by · [logo] · DayZ Beans Launcher » ; cliquer la barre (ou le bouton LAUNCHER du header) ouvre la page de téléchargement du launcher

---

## Contexte

Repris de CourseBarterTrader (qui ships déjà l'asset `dzb_launcher_logo`). CBT charge le logo **par chemin direct** (pas d'imageset) → aucune entrée `config.cpp` nécessaire. CBT affiche aussi une modale promo one-shot ; ici on se limite au footer demandé + on réactive le bouton LAUNCHER déjà présent (mais mort) du header.

Découverte : le header avait déjà un bouton `LauncherBtn` avec `data-on-click="OnLauncherClick"` **sans** `OnLauncherExecute` côté controller → bouton inopérant. US-021 ajoute le handler, ce qui réactive le bouton ET sert le footer (même handler).

## Fichiers

| Fichier | Changement |
|---|---|
| `datasets/images/dzb_launcher_logo.{png,edds,edds.meta}` | **Copiés** de CBT (1536×1024, chargé par chemin direct) |
| `datasets/gui/TraderXMainView.html` (+ `.layout`) | `MainPageContent` réduit `100-89`→`100-84` ; ajout panel `dzbFooter` (bouton plein cliquable `dzbFooterBtn` + overlays noptr : prefix « Sponsored by », logo `dzbLogo`, texte or « DayZ Beans Launcher ») |
| `scripts/4_World/classes/TraderX/GUI/TraderXMainViewController.c` | Ajout `OnLauncherExecute(ButtonCommandArgs)` → `GetGame().OpenURL("steam://openurl/https://dayzbeanslauncher.com/download?ref=SO-5882")` |

## Détails techniques

- **Click** : `data-on-click="OnLauncherClick"` → convention DabsFramework `On<X>Click` → méthode `On<X>Execute`. Bouton de fond + overlays `dz-noptr` (le clic traverse vers le bouton, cf. gotcha « pas de bouton imbriqué »).
- **Logo** : `data-image="TraderX/datasets/images/dzb_launcher_logo.edds"` (chemin direct, pas `set:`). 39×26px (ratio 3:2 du source 1536×1024), teinte `#FFFFFFCC`.
- **Palette** : texte or `#C4A35A` (cohérent US-020). Footer fond `TXSurface` slate.

## Tests

- **Pipeline** : `dzml check` + `dzml compile` ✓ (footer + bindings + image path présents dans le `.layout`)
- **Acceptance** : Manuel in-game

## Étapes de validation (in-game)

1. **Build** : `.\Build.ps1` → in-game, ouvrir le trader
2. **Vérifier** : footer visible en bas, logo + textes lisibles, fond slate
3. **Vérifier** : clic sur le footer ouvre l'overlay Steam vers dayzbeanslauncher.com
4. **Vérifier** : clic sur le bouton LAUNCHER du header fait la même chose
5. **Vérifier** : le contenu des pages n'est pas tronqué (hauteur réduite à 84%)

## Done

- [ ] Footer affiché, logo chargé (pas de carré manquant)
- [ ] Clic footer + clic LAUNCHER ouvrent l'URL
- [ ] Pas de régression de layout sur les 5 pages
- [ ] Dmitri validé in-game

## Notes

- Preview browser : le logo `.edds` ne s'affiche pas (pas de support edds navigateur) ; le `.png` est présent si besoin d'un fallback preview plus tard.
- Optionnel futur : modale promo one-shot façon CBT (`BeansPromoModalComponent`).
