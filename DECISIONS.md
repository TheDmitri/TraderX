# DECISIONS — TraderX

Architectural Decision Records (ADR) ultra-légers. Chaque décision active fait référence.  
Une décision annulée est taguée `[ANNULÉ]` — jamais supprimée. Une `D-XXX-bis` la remplace.

---

## D-001 — DayZular comme framework UI (2026-01-01)
- **Contexte** : Les classes de gestion de menu native à DayZ ne sont pas très optimisé pour des menus complexe, jusque là on a utilisé DabsFramework qui se base sur du MVC pattern pour l'UI. Or étant developpeur angular, je suis plus à l'aise avec le component base.
- **Choix** : DayZular (framework MVC + service-registry maison, dep : `c:\DayZWorkspace\DayZular`).
- **Raison** : Bindings déclaratifs (TextBinding, ButtonBinding, ForEach), cycle de vie composants (OnInit/OnDestroy), service locator centralisé — élimine la majorité des bugs de synchronisation UI.
- **Écarté** : DabsFramework (dépendance tier), UI Enforce natif pur (non maintenable à l'échelle).
- Voir `GLOSSARY.md` pour la liste des bindings.

## D-002 — Field Ledger comme design system (2026-02-01)
- **Contexte** : L'UI v1 était sans cohérence visuelle. Besoin d'un langage de design mémorisable pour l'IA et reproductible.
- **Choix** : "Field Ledger" — esthétique tactique sombre, palette ambre/olive/rouille, typographie Roboto Condensed.
- **Raison** : Cohérence avec l'univers DayZ (survie, austère), différenciation vs autres mods, palette réduite (4 couleurs sémantiques) = moins de décisions ad-hoc.
- **Écarté** : Design system générique (Material, Tailwind tokens) — trop loin de l'univers DayZ.
- Référence : `docs/TraderX_Design_System.md`. Tous les tokens sont `TX*`.
- **Addendum (US-020, 2026-06-17)** : palette retargetée sur CourseBarterTrader (accent or `#C4A35A`, vert `#6B8E5A`, fonds slate `#0F1215`/`#1A1D21`). Structure des styles `TX*` inchangée — seuls les tokens couleur (source : `_fixtures/field-ledger.html`) + `ViewStyles.c` ont bougé. Décision « align tokens only » : pas d'import de l'atlas CBT.

## D-003 — DZML v3 : implémentation UI isolée en HTML compilé vers .layout (2026-03-01)
- **Contexte** : La v1 avait été partiellement reskinée ad-hoc. Modifier en place créait des régressions. Besoin de valider visuellement dans un navigateur avant intégration jeu.
- **Choix** : Implémentation v3 dans `src/TraderX/datasets/gui/trader-v3/` avec mock data, compilée par `dzml pipeline` vers `.layout`.
- **Raison** : Isolation complète (pas de risque sur v1), preview browser rapide, mock data pour développement sans serveur DayZ.
- **Écarté** : Patch in-place de v1 (trop de dépendances enchevêtrées), retour à v1 pure (perd le benefit du design system).
- **Gotcha** : Sélectors `tx3-*` (pas `v3-*` — le préfixe `v3-` est interprété comme un path par le runtime DayZ). PAGE_MAP dans `dzml-runtime.js` est patché manuellement.

## D-004 — Transaction lock anti-duplication (2026-02-15)
- **Contexte** : Les joueurs pouvaient spammer le bouton d'achat/vente et déclencher plusieurs transactions simultanées, créant de la duplication d'items ou d'argent.
- **Choix** : Verrou centralisé dans `TraderXTradingService` (`LockTransaction()`/`UnlockTransaction()`/`IsTransactionPending()`). Vérifié à chaque point d'entrée de transaction avant `SendRPC`.
- **Raison** : Protection simple, single point of truth, pas de double-state côté client.
- **Écarté** : Lock côté serveur uniquement (latence réseau insuffisante), désactivation du bouton dans l'UI (contournable).
- Voir `backlog.md` pour le détail du fix.

## D-005 — Trois formats de configuration supportés (Legacy JSON / CSV / Compiled) (2025-12-01)
- **Contexte** : Les admins avaient des configs existantes en JSON individual (Legacy). Besoin d'un workflow tableur (CSV) et d'un format optimisé au chargement (Compiled).
- **Choix** : Auto-détection par `TraderXSourceFormatDetector` au chargement. Migration cross-format via `TraderXFormatMigrator`.
- **Raison** : Compatibilité ascendante totale, flexibilité admin, pas de migration obligatoire.
- **Écarté** : Format unique imposé (casse les configs existantes), migration automatique forcée (risque de perte de données).
- Référence : `CONFIGURATION_GUIDE.md`.

## D-006 — Architecture PBO-par-package (2025-11-01)
- **Contexte** : Les mods DayZ peuvent entrer en conflit si des classes sont redéclarées dans plusieurs PBOs.
- **Choix** : Un dossier `src/` = un PBO. Packages : TraderX, TraderXBanking, TraderXGarage, TraderXCarLock, TraderPlusMoney, TraderPlusATM, TraderSign.
- **Raison** : Isolation des modules, composabilité (les serveurs activent seulement les modules voulus), détection de conflits via `detect_redecl.py`.
- **Écarté** : Monolithe (un seul PBO) — empêche la modularité et les mises à jour partielles.

## D-007 — ITraderXPaymentProvider pour les paiements cross-module (2026-04-10)
- **Contexte** : TraderXGarage doit prélever des frais sur le compte Banking d'un joueur sans créer une dépendance directe entre les deux modules.
- **Choix** : Interface `ITraderXPaymentProvider` exposant `DeductLabeled(playerId, amount, label)` et `CreditLabeled(...)`. TraderXBanking implémente l'interface ; Garage l'appelle via le service locator.
- **Raison** : Découplage fort, traçabilité dans le ledger via le label (ex: `GARAGE_FEE_OUT`), extensible à d'autres modules.
- **Écarté** : Appel direct Banking→Garage (couplage fort), système de paiement séparé per-module (fragmentation du ledger).

## D-008 — Architecture DDD 3_Game / 4_World / 5_Mission (2025-11-01)
- **Contexte** : Convention DayZ pour les couches de scripts. TraderX l'adopte strictement.
- **Choix** : `3_Game` = domaine pur (models, value objects, repositories, services domaine). `4_World` = application (services app, UI, prefabs, extensions entités). `5_Mission` = démarrage + registration des services DayZular.
- **Raison** : Isolation des dépendances (3_Game ne dépend de rien), testabilité de la logique domaine, convention reconnue par la communauté DayZ.
- **Écarté** : Tout mettre dans `4_World` (couplage fort domaine/infra).

## D-009 — Pas de ternaire `?:` — DZTernary helpers (2025-11-01)
- **Contexte** : L'Enforce Script ne supporte pas l'opérateur ternaire `?:`. C'est une limitation du langage.
- **Choix** : Utiliser les helpers DayZular : `DZTernaryString.If(cond, a, b)`, `DZTernaryInt.If(...)`, `DZTernaryFloat.If(...)`, `DZTernaryBool.If(...)`.
- **Raison** : Contrainte du langage — pas de contournement possible.
- **Alternative** : Bloc `if/else` explicite (verbeux mais valide). Préférer DZTernary pour les expressions inline.

## D-010 — src_wip/ pour le travail en cours non packagé (2026-03-01)
- **Contexte** : La v3 du Trader et les modules en rework ne doivent pas interférer avec les PBOs actifs déployables.
- **Choix** : `src_wip/` héberge les scripts en cours de développement. Le script `Build.ps1` ne package que `src/`.
- **Raison** : Séparation claire entre "déployable maintenant" et "en cours de développement", évite de casser les serveurs live.
- **À noter** : Quand un module `src_wip/` est validé en jeu, il bascule vers `src/`.

## D-011 — v2/ UI off-limits (2026-02-01)
- **Contexte** : Le dossier `src/TraderX/datasets/gui/v2/` contient une UI intermédiaire.
- **Choix** : **Ne jamais modifier ce dossier.** Directive utilisateur explicite.
- **Raison** : Non documentée — à ne pas remettre en question.
