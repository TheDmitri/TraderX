# US-027 — Consolidation des binders → DZlr unique (+ cleanup notif v1)

**Périmètre :** TraderX (core)
**Priorité :** P1

**Given** deux piles d'UI coexistantes (Dabs v1 `*ViewController`/`*View` + DZlr `*Component`) avec du code vivant encore couplé à la v1
**When** on déplace la logique restante vers DZlr puis on supprime toute la pile Dabs
**Then** TraderX n'a plus **qu'une seule pile UI (DZlr/DayZular)** — plus simple à maintenir et à re-skinner

---

## Étape 1 — Découplage (logique migrée vers DZlr)

Les 3 couplages vivant→Dabs ont été résolus. Bonus : la logique DZlr existait déjà, les consommateurs pointaient juste la mauvaise pile (donc le slider→checkout était **cassé** en jeu, maintenant **réparé**).

| Avant (Dabs) | Après (DZlr) | Fichier |
|---|---|---|
| `SellPageViewController.Event_OnSliderQuantityChangedCallBack` | `SellPageComponent.Event_…` | `CheckoutComponent.c` (×2) |
| `SellPageViewController.IsManualSell()` | `SellPageComponent.IsManualSell()` | `SellTooltipComponent.c` |
| `CheckoutViewController.GetInstance().GetCheckoutItems()` (`CheckoutCardView`) | `CheckoutComponent.GetInstance().GetCheckoutItems()` (`TraderXProduct`) | `CustomizeStateService.c` |

Ajout : `CheckoutComponent.GetCheckoutItems()` (renvoie les `TraderXProduct` du panier). `TraderXAppComponent.OnLauncherClick` corrigé (URL `?ref=SO-5882`).

## Étape 2 — Suppression de la pile morte (68 fichiers)

- **63 fichiers Dabs** : toutes les classes `: ScriptView` / `: ViewController` / `: ScriptViewTemplate<…>` / `extends ViewController` (pages v1, item/category/checkout/preset/variant cards v1, tooltips v1, modals v1, navigation v1, combobox v1, player preview v1, payment resume v1, `TraderXMainView(Controller)`).
- **Ancienne pile notification** (dormante depuis US-019) : `TraderXNotificationService.c`, `NotificationContainer/Card/DetailsComponent.c`, `TraderXNotificationItem.c`, contrôleurs notif, + layouts orphelins `TraderXNotification*Card` / `Parent` / `DetailedCard`.

**Vérifications (substitut au compilateur, absent hors-jeu) :**
- Toutes les classes Dabs cross-référencées → **0 ref de code vivant** (exclus : noms apparaissant dans des chemins `*.layout` partagés, et commentaires).
- Aucun `enum`/classe helper défini dans un fichier Dabs (rien que des sous-classes de vues Dabs).
- Après suppression : `grep` des 82 noms supprimés sur tout le code restant → **0 ref pendante**. `grep` Dabs (`:`/`extends`) → **0**.

Les `.layout` restent (réutilisés par les composants DZlr via `templatePath`, ex. `SellPage/SellPageView.layout` ← `SellPageComponent`).

## Pile vivante (rappel)

`TraderXUIService` → `TraderXAppComponent` (`tx-app`, rend `TraderXMainView.layout`) → pages DZlr `tx-favorite/purchase/sell/catalog-page`, cards/tooltips/modals DZlr (`*Component`), services. Notifications = `TraderXToastService` + `TraderXToastCardComponent`.

## Tests

- **Statique** : 0 Dabs restant, 0 ref pendante (vérifié).
- **Acceptance** : in-game — **indispensable** (pas de compilateur Enforce ici ; risque résiduel = une ref non-classe manquée).

## Étapes de validation (in-game)

1. **Build** : `.\Build.ps1` — doit compiler sans erreur (sinon : ref pendante à corriger, `git` pour revert).
2. Ouvrir le trader : 5 pages, achat/vente, **slider qty de vente → mise à jour checkout** (le fix), customize (attachments via checkout), tooltips, modals, presets, player preview.
3. Toast notifications OK.

## Done

- [x] 3 couplages migrés vers DZlr
- [x] 68 fichiers Dabs/notif-v1 supprimés, 0 ref pendante
- [x] Slider→checkout réparé ; URL launcher corrigée
- [ ] **Build PBO sans erreur Enforce** — in-game
- [ ] Parcours complet validé — in-game
- [ ] Dmitri validé in-game
