# US-030 — TraderXSafeZone : refonte UI DayZular (Field Ledger)

**Périmètre :** `src/TraderXSafeZone` UNIQUEMENT (session parallèle isolée)
**Priorité :** P2
**Dépendances :** aucune (le backend SafeZone est déjà 100% fonctionnel)
**Préfixe de classe réservé :** `TXSZ*` (éviter collisions cross-PBO — voir `detect_redecl.py`)

---

## Amorçage session (lire d'abord)

1. `CLAUDE.md` (méthodo TSL + pipeline DZML + gotchas Enforce/UI)
2. Ce fichier (brief complet auto-suffisant)
3. `docs/TraderX_Design_System.md` (tokens couleur/typo Field Ledger — obligatoire)

**Ne touche QUE `src/TraderXSafeZone`.** Deux autres sessions tournent en parallèle sur `src/TraderXGarage` et `src/TraderXCarLock` — ne pas éditer hors de ton dossier ni les fichiers partagés racine.

---

## Contexte

Le mod SafeZone est **déjà complet et testé** (CF_ModuleWorld). Toute la logique de protection marche : damage off, melee block, fall damage, entity cleanup, AFK kick, admin bypass, influenza reset, détection de zone client (loop 1s). **Ne pas réécrire cette logique.**

Le seul point faible = **l'UI**, aujourd'hui minimale :
- `gui/SafeZoneNotification.layout` (36 LOC) — un seul `RichTextWidget` (NotifText) en haut-droite, texte orange sur fond noir semi-transparent, font etelkatextpro22. Affiche uniquement le compte à rebours de sortie.
- `scripts/4_World/SafeZoneNotificationUI.c` (87 LOC) — `UIScriptedMenu` brut (PAS DayZular). Charge le layout, fallback sur `NotificationSystem.AddNotificationExtended()` si layout absent.
- Toasts entrée/sortie de zone = `NotificationSystem` natif du jeu (icône `datasets/shield.paa`).

## Objectif (Given/When/Then)

**Given** un joueur qui entre, séjourne et sort d'une safezone
**When** chaque transition se produit (entrée / séjour / compte à rebours de sortie / sortie)
**Then** il voit une UI DayZular cohérente avec le design Field Ledger : toast d'entrée/sortie stylé + indicateur de statut persistant "EN ZONE SÛRE" + compte à rebours stylé — au lieu des widgets bruts actuels.

## Hooks d'intégration (ne PAS changer leur signature)

Brancher la nouvelle UI sur ces points existants dans `SafeZoneHandler.c` :
- `OnEnterSafeZone()` → afficher toast entrée + activer l'indicateur persistant
- `StartOnLeaving()` / `CountdownUpdate` → afficher/mettre à jour le compte à rebours
- `OnExitSafeZone()` → toast sortie + masquer l'indicateur

## Travail attendu

1. **Indicateur de statut persistant** (HUD discret, coin écran) : badge "ZONE SÛRE · <nom de zone>" avec icône shield, visible tant que `PlayerBase.IsInTXSafeZone()`. Style Field Ledger (panel TXCard/TXSurface, tokens).
2. **Toast entrée/sortie** stylé DayZ ular (remplacer `NotificationSystem` natif). Réutiliser si possible le toast service core US-019 (`docs/stories/US-019-*` — port CourseBarterTrader) si déjà présent dans le projet ; sinon composant autonome `TXSZ`.
3. **Compte à rebours de sortie** : remplacer `SafeZoneNotification.layout` brut par un composant DayZular (TextBinding `{{ }}`, `MarkForCheck()`, jamais `.SetText()`).
4. **Pipeline DZML** : authoring en `.html` sous `datasets/gui/` → `node ../dayz-tools/dayz-layout-tools/dist/bin/dzml.js compile <fichier>.html` → `.layout`. Lier les 3 CSS (`dzml.css`, `dzml-styles.css`, `dzml-icons.css`) dans le `.html`. Icône shield → ajouter en SVG dans le pipeline icônes si besoin (`src/.../icons/svg/`).

## Port public (provider cross-mod) — dépend de [US-033](US-033-integration-ports.md)

En plus de l'UI, exposer la safezone aux mods externes via `ITraderXSafeZoneProvider` (core, US-033). Mirror du pattern Banking.

1. Créer `scripts/4_World/Providers/TXSZSafeZoneProvider.c` :
   ```c
   #ifdef TraderX
   class TXSZSafeZoneProvider : ITraderXSafeZoneProvider
   {
       override bool   IsInSafeZone(Man player) { /* PlayerBase.IsInTXSafeZone() */ }
       override bool   IsPositionInSafeZone(vector pos) { /* distance 2D à chaque zone */ }
       override string GetZoneNameAt(vector pos) { ... }
       override int    RegisterDynamicZone(string name, vector pos, float radius) { /* zone runtime, retourne id */ }
       override bool   UnregisterDynamicZone(int zoneId) { ... }
   }
   #endif
   ```
2. Déléguer au handler/module existant (`SafeZoneHandler` / `SafeZoneModule`). `IsPositionInSafeZone` réutilise le check distance 2D déjà codé.
3. `RegisterDynamicZone` : ajouter une zone en mémoire (non persistée) à la liste runtime — utile pour des event-mods (zone temporaire). Optionnel pour le MVP UI mais inclure le stub.
4. Enregistrer dans `SafeZoneModule` (ou AppModule), sous `#ifdef TraderX` :
   `TraderXProviderRegistry.GetInstance().RegisterSafeZoneProvider(new TXSZSafeZoneProvider());`

Cas d'usage cible : un mod externe vérifie `IsInSafeZone(player)` avant d'autoriser une action PvP, ou pose une zone sûre temporaire pendant un event.

## Non-goals

- Ne PAS modifier la logique de protection (actions désactivées, cleanup, AFK, damage).
- Ne PAS changer le schéma de config JSON (`TraderXTXSafeZoneSettings.json`) ni les RPC.
- Ne PAS migrer le CF_ModuleWorld vers un autre framework — garder CF, ajouter juste la couche UI DayZular par-dessus.

## Gotchas (Enforce + UI)

- Texte : couleur via `dz-c-RRGGBB` (PAS `dz-tc`, qui ne colore que les labels de bouton). Voir mémoire `dzml-text-color-dz-c-not-dz-tc`.
- Pas d'opérateur ternaire en Enforce → `TernaryString()` / `DZTernaryString.If(...)`.
- Noms de classe uniques cross-PBO → préfixe `TXSZ`. Lancer `python detect_redecl.py` avant build.
- `MarkForCheck()` après chaque changement de donnée bindée ; jamais `.SetText()`.
- DZML : éditer `.html` puis recompiler — ne jamais éditer le `.layout` à la main.

## Done

- [ ] Indicateur persistant "zone sûre" stylé Field Ledger, apparaît/disparaît avec le statut
- [ ] Toast entrée/sortie DayZular (plus de `NotificationSystem` brut)
- [ ] Compte à rebours via composant DayZular + TextBinding
- [ ] Layouts compilent (pipeline DZML vert) + 3 CSS liés en preview
- [ ] Logique de protection intacte (regression test in-game : entrée/sortie zone OK)
- [ ] `detect_redecl.py` propre
- [ ] Build.ps1 + Dmitri validé in-game

## Références fichiers

| Quoi | Chemin |
|---|---|
| UI actuelle (à remplacer) | `src/TraderXSafeZone/gui/SafeZoneNotification.layout`, `scripts/4_World/SafeZoneNotificationUI.c` |
| Hooks UI | `src/TraderXSafeZone/scripts/4_World/SafeZoneModule/SafeZoneHandler.c` |
| Config schema | `src/TraderXSafeZone/scripts/3_Game/SafeZoneSettings.c` |
| Icône | `src/TraderXSafeZone/datasets/shield.paa` |
| Référence style DayZular | Banking (`src/TraderXBanking/datasets/layouts/*.html`) + `docs/TraderX_Design_System.md` |
