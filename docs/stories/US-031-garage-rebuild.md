# US-031 — TraderXGarage : build complet (port + UI DayZular)

**Périmètre :** `src/TraderXGarage` UNIQUEMENT (session parallèle isolée — le dossier n'existe pas encore, à créer)
**Priorité :** P1 (bloquant module Garage — implémente US-010)
**Dépendances :** lit l'interface `ITraderXPaymentProvider` de TraderXBanking (ne PAS éditer Banking)
**Préfixe de classe réservé :** `TXG*` (éviter collisions cross-PBO — `detect_redecl.py`)

---

## Amorçage session (lire d'abord)

1. `CLAUDE.md` (TSL + pipeline DZML + archi DayZular + gotchas Enforce)
2. Ce fichier
3. `docs/stories/US-010-garage-backend-validation.md` (les étapes de validation in-game = critère d'acceptation)
4. `docs/TraderX_Design_System.md` (tokens UI)
5. **Le mod Banking comme modèle d'architecture** : `src/TraderXBanking/scripts/5_Mission/TXBAppModule.c` + `scripts/4_World/Services/*` (service registry, RPC service, composants GUI). **Garage doit suivre ce même pattern.**

**Ne touche QUE `src/TraderXGarage`.** Deux autres sessions tournent sur SafeZone et CarLock.

---

## Contexte

`src/TraderXGarage` et `wip/TraderXGarage` **n'existent pas**. Il faut **construire le module from scratch**, en :
- **portant la logique métier** de l'original `TraderPlusV1/src/Garage` (2 527 LOC, vieux `UIScriptedMenu` + helpers statiques — fonctionnel mais ancien style)
- **adoptant l'architecture DayZular** de TraderXBanking (service registry, composants `DZlrComponentBase`, RPC service, payment provider cross-module)

Des **remnants UI DayZular** existent déjà (markup seul, non câblé) : `original_version_unchanged/TraderX/src/TraderX/datasets/gui/Garage/GaragePageView.{html,layout}` — point de départ visuel possible.

## Objectif (Given/When/Then)

**Given** un joueur sur un serveur avec TraderXGarage chargé + NPC Garagiste configuré
**When** il gare un véhicule, se déconnecte/reconnecte, puis le récupère
**Then** le véhicule est restitué fidèlement (health, fuel, cargo, attachments, orientation) avec débit `GARAGE_FEE_OUT` dans le ledger Banking.

## Pièces réutilisables de l'original (à porter)

| Fichier source | Rôle | Réutilisable |
|---|---|---|
| `TraderPlusV1/src/Garage/.../Helpers/GarageHelpers.c` (571) | `CreateVehicle()` (179, spawn+restore attach/cargo/lock), `GetGarageItemData()` (77, snapshot récursif), `RestoreCargo()` (64), `ConvertHealthToHealthLevel()`, `RemoveMoneyFromPlayer()` | 🟢 cœur logique — porter |
| `.../Config/GarageData.c` (176) | modèles `VehiculeData`, `ItemsCargo`, `GarageItemData`, persistance JSON | 🟢 porter (schéma) |
| `.../Config/GarageSettings.c` (92) | config (fees, locks, flags save cargo/fuel/health, max slots, NPC array) | 🟢 porter |
| `.../Core/GarageSystemServer.c` (493) | 3 RPC : `GarageRequest`, `ParkInRequest`, `ParkOutRequest` (validation, persist, spawn) | 🟡 adapter au RPC service DayZular |
| `.../Action/ActionGarageMenu.c` (85) | interaction NPC (`TraderID == -3`) | 🟡 adapter au système d'action/NPC TraderX |
| `.../Core/GarageMenu.c` (323) | UI vieux `UIScriptedMenu` | 🔴 remplacer par composant DayZular |

## Architecture cible (calquée sur Banking)

```
src/TraderXGarage/
  config.cpp                       — CfgMods, widgetStyles/imageSets, requiredAddons
  scripts/
    3_Game/Models/                 — TXGVehicleData, TXGItemSnapshot, TXGGarageData (JSON), TXGSettings
    4_World/Services/
      TXGGarageService.c           — logique park-in/out, snapshot/restore (port GarageHelpers)
      TXGRPCService.c              — handlers RPC (GarageRequest/ParkIn/ParkOut) + rate limiting
      TXGConfigService.c           — load config $profile:TraderX/TraderXConfig/
      TXGNpcService.c              — spawn NPC garagiste (mode standalone)
    4_World/Components/GUI/         — composants DZlrComponentBase + .html/.layout (DZML)
    4_World/Actions/               — ActionTXGOpenGarage (interaction NPC)
    5_Mission/TXGAppModule.c       — enregistrement services (registry DayZular)
  datasets/layouts/                — *.html (source) + *.layout (compilé)
  datasets/styles/                 — réutiliser tokens/atlas TX si possible
```

## Intégration Banking (fees)

- Implémenter le débit via `ITraderXPaymentProvider` (déjà utilisé par Banking pour `GARAGE_FEE_OUT`/`GARAGE_FEE_IN`). **Lire** l'interface dans TraderXBanking ; **ne pas éditer** Banking.
- Fallback cash si Banking absent (`#ifdef`), comme l'original (`RemoveMoneyFromPlayer` abstrait bank vs cash).

## Persistance

- JSON sous `$profile:TraderX/TraderXGarageDatabase/Garage_<steamID>.json` (cohérent avec convention TraderX).
- **Gotcha** : `JsonFileLoader` ne sérialise pas `map<>` (mémoire `dayz-jsonfileloader-no-map`) → persister en `array<ref>` et reconstruire la map au load.

## Décisions à prendre (documenter en ADR dans `DECISIONS.md`)

- Modèle handler-based porté vs service DayZular pur réactif → **recommandé : service DayZular** (cohérence projet).
- Gestion lock véhicule : l'original supporte 3 systèmes (DayZ CarLock / MuchCarKey / TraderPlus keys via `#ifdef`). Coordonner avec la session **TraderXCarLock** (US-032) : idéalement déléguer au futur `TXGVehicleLock`/manager. Pour le MVP, supporter au minimum DayZ CarLock natif.

## Port public (provider cross-mod) — dépend de [US-033](US-033-integration-ports.md)

Exposer le garage aux mods externes via `ITraderXGarageProvider` (défini dans le core par US-033). Mirror exact du pattern Banking (`src/TraderXBanking/scripts/4_World/Providers/TXBPaymentProvider.c` + register dans `TXBAppModule.c`).

1. Créer `scripts/4_World/Providers/TXGGarageProvider.c` :
   ```c
   #ifdef TraderX
   class TXGGarageProvider : ITraderXGarageProvider
   {
       override bool AddVehicle(string playerUid, EntityAI vehicle) { /* snapshot + store + delete world */ }
       override bool AddVehicleByType(string playerUid, string className) { /* create fresh + store */ }
       override bool RemoveVehicle(string playerUid, string vehicleId) { /* drop, no respawn */ }
       override EntityAI RetrieveVehicle(string playerUid, string vehicleId, vector pos, vector orientation) { /* spawn + restore */ }
       override int  GetStoredCount(string playerUid) { ... }
       override int  GetMaxSlots() { ... }
       override bool HasFreeSlot(string playerUid) { ... }
       override array<ref TraderXGarageVehicleInfo> ListVehicles(string playerUid) { ... }
   }
   #endif
   ```
2. Déléguer toute la logique à `TXGGarageService` (le provider est une façade fine, comme `TXBPaymentProvider` délègue à `TXBBankingService`).
3. Enregistrer dans `TXGAppModule.c`, sous `#ifdef TraderX` :
   `TraderXProviderRegistry.GetInstance().RegisterGarageProvider(new TXGGarageProvider());`
4. Tous les appels sont **serveur**. `AddVehicle` doit faire le même snapshot que le park-in UI (réutiliser le service).

Cas d'usage cible : un trader-mod achète un véhicule → `AddVehicleByType(uid, className)` le range direct, ou `AddVehicle(uid, spawnedVeh)` range un véhicule déjà spawné.

## Non-goals

- Pas de marketplace véhicule joueur-à-joueur (hors scope, cf SCOPE §2).
- Pas d'assurance véhicule pour le MVP (feature séparée).
- Ne pas implémenter le clan/garage partagé.

## Gotchas (Enforce + UI)

- Préfixe `TXG`, `detect_redecl.py` propre (3 PBOs se buildent en parallèle).
- DZML : éditer `.html` → `dzml compile` → `.layout`. Listes/grids → `ComponentWidgetBinding` + `Component_ForEach` (jamais `AddChild`). ForEach veut `array<ref Object>` (mémoire `dayzular-foreach-navbutton-gotchas`).
- Texte couleur `dz-c` pas `dz-tc`. Pas de ternaire (`TernaryX()`).
- Orientation véhicule au spawn = roll trick (déjà connu projet — voir `GLOSSARY.md` / fix orientation Banking-Garage).
- `MarkForCheck()` sur data change, jamais `.SetText()`.

## Done (= US-010)

- [ ] Park-in : snapshot (health/fuel/cargo/attachments) + disparition véhicule
- [ ] Park-out : restore fidèle + orientation correcte + véhicule utilisable
- [ ] Débit `GARAGE_FEE_OUT` dans ledger Banking (provider)
- [ ] Liste véhicules garés correcte dans l'UI DayZular
- [ ] Max slots respecté + edge case position bloquée (pas de crash)
- [ ] Persistance multi-véhicule OK après reconnexion
- [ ] Pipeline DZML vert + `detect_redecl.py` propre
- [ ] Build.ps1 + Dmitri validé in-game
