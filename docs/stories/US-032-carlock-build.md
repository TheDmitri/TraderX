# US-032 — TraderXCarLock : build complet (features CarLock + pattern TheLab + UI DayZular)

**Périmètre :** `src/TraderXCarLock` UNIQUEMENT (session parallèle isolée — à créer)
**Priorité :** P2 (implémente US-011)
**Dépendances :** aucune dure ; coordination légère avec US-031 Garage (lock véhicule)
**Préfixe de classe réservé :** `TXCL*` (collision cross-PBO + serveur spécifique — `detect_redecl.py`)

---

## Amorçage session (lire d'abord)

1. `CLAUDE.md` (TSL + DZML + gotchas Enforce)
2. Ce fichier
3. `docs/TraderX_Design_System.md` (tokens UI)
4. Les **3 sources de référence** ci-dessous (ne pas les modifier — sources read-only)

**Ne touche QUE `src/TraderXCarLock`.** Sessions parallèles : SafeZone, Garage.

---

## Contexte & intention

Construire `TraderXCarLock` = **features de l'original** `TraderPlusV1/src/CarLock` **+ pattern d'archi de** `TheLabCarMod/src/TL_CarLock` (version upgradée) **+ UI DayZular** + **mot de passe 8 chiffres** + **actions comme l'original**. Assez spécifique pour un serveur donné, sans gêner le joueur lambda.

Rien n'existe encore (`src/TraderXCarLock` et `wip/TraderXCarLock` absents).

### 3 sources

| Source | Chemin | Usage |
|---|---|---|
| FEATURES (original) | `TraderPlusV1/src/CarLock` (1 466 LOC) | quoi faire : lock/unlock, **mot de passe 8 chiffres (déjà le cas ici)**, lockpick (`CarLockPick`), alarme, persistance owner. Actions style original (continuous). |
| PATTERN (upgrade) | `TheLabCarMod/src/TL_CarLock` (2 944 LOC) | comment structurer : `CF_ModuleWorld`, composition `VehicleLockManager` + `VehicleLockComponent`, DB par joueur + cooldown, UI keypad grille + confirmation, `ActionSingleUseBase`. ⚠️ TL est en **4 chiffres** (`PASSWORD_DIGITS_LENGTH = 4`) → **on veut 8**. |
| PATTERN serveur | `TheLabCarMod/src/TL_CarLockSS` (361 LOC) | validation server-side + service cooldown (`TLCarLockAttemptService`, `EnterPasswordAttempt`). |

## Objectif (Given/When/Then)

**Given** un véhicule verrouillé par son propriétaire avec un mot de passe 8 chiffres
**When** un autre joueur ouvre le keypad et saisit un code (ou tente un lockpick)
**Then** déverrouillage seulement si code correct / owner / admin ; mauvais code → cooldown ; lockpick → alarme + temps configurable.

## Décisions d'archi (à figer en ADR `DECISIONS.md`)

- **Composition** (TL) plutôt qu'héritage : `TXCLVehicleLockManager` (wrapper CarScript/BoatScript) + `TXCLVehicleLockComponent` (état : owner, password, timestamp, sons). Cf `TheLabCarMod/.../Classes/VehicleLockManager.c` (402) + `.../Components/VehicleLockComponent.c` (339).
- **Module** : `CF_ModuleWorld` (`TXCLModule`) — hooks `OnInit`/`EnableInvokeConnect`/`EnableMissionStart`, DB par joueur au connect. Cf `TraderPlusCarLockModule.c` (144).
- **DB joueur + cooldown** : `TXCLPlayerDb` per-steamID JSON sous `$profile:TraderX/TraderXCarLock/Database/<steamID>.json` ; set de verrous débloqués + cooldowns. Cf `TLCarLockPlayerDb.c` (258). **Gotcha** : pas de `map<>` JSON → `array<ref>` (mémoire `dayz-jsonfileloader-no-map`).
- **Mot de passe** : entier en clair synchro réseau (compat), **longueur 8** via constante config `TXCLSettings.passwordLength = 8`. Validation longueur + digits-only.
- **Actions** : garder le **ressenti original** (lock/unlock/menu/lockpick). Choix interne single-use (TL) vs continuous (original) à documenter — recommandé : lock/unlock single-use, **lockpick continuous** (temps config-driven, full-body, déclenche alarme).

## Plan de fichiers (cible)

```
src/TraderXCarLock/
  config.cpp                      — CfgMods, widgetStyles/imageSets, sons (carAlarm/carLock/carUnlock), requiredAddons
  scripts/
    3_Game/                       — TXCLConstants, TXCLSettings (passwordLength=8, lockpick config), TXCLCooldownState, TXCLEnterPasswordResponse
    4_World/
      CarLockModule/TXCLModule.c  — CF_ModuleWorld + RPC server (EnterPasswordAttempt, GetCooldownState, LockingRequest)
      CarLockModule/TXCLPlayerDb.c
      Classes/TXCLVehicleLockManager.c
      Components/TXCLVehicleLockComponent.c
      Actions/                    — ActionTXCLLock, ActionTXCLUnlock, ActionTXCLLockMenu, ActionTXCLLockpick, ActionTXCLAttach/Detach
      Entities/CarScript.c, BoatScript.c, PlayerBase.c — délégation au manager
      GUI/TXCLLockMenu.c          — keypad DayZular, flow set/confirm, cooldown countdown
  datasets/gui/                   — tx_carlock_menu.html (source DZML) → .layout
  datasets/sounds/                — carAlarm.ogg, carLock.ogg, carUnlock.ogg (porter de TL_CarLock/datasets/sounds)
```

## UI keypad (DayZular, 8 chiffres)

- Réécrire le menu en DayZular (composant `DZlrComponentBase`), inspiré du layout grille `TheLabCarMod/.../datasets/gui/tl_carLockMenu.layout` (596) : EditBox code + pavé 1-9/0 + ENTER + CLR + ligne message.
- **Flow** : owner sans mot de passe → mode SET (saisie → confirmation → match → RPC). Sinon → mode ENTER (saisie → RPC validation). Mauvais code → afficher cooldown countdown (`Update()` 1s).
- **8 chiffres** : adapter la constante de longueur partout (UI + validation serveur), ne pas garder le 4 de TL.
- Authoring `.html` → `node ../dayz-tools/dayz-layout-tools/dist/bin/dzml.js compile` → `.layout`. Boutons → `ButtonBinding` + `OnClick` (jamais override OnClick par widget matching). Lier les 3 CSS en preview.

## Sons & alarme

- Porter `carAlarm.ogg / carLock.ogg / carUnlock.ogg` depuis `TheLabCarMod/src/TL_CarLock/datasets/sounds/`, déclarer en `CfgSoundShaders`/`CfgSoundSets` dans `config.cpp` (préfixe `TXCL_`).
- Lockpick démarre l'alarme (rayon ~500m, comme les deux sources).

## Port public (provider cross-mod) — dépend de [US-033](US-033-integration-ports.md)

Exposer le carlock aux mods externes via `ITraderXCarLockProvider` (core, US-033). Mirror du pattern Banking.

1. Créer `scripts/4_World/Providers/TXCLLockProvider.c` :
   ```c
   #ifdef TraderX
   class TXCLLockProvider : ITraderXCarLockProvider
   {
       override bool AddCarLock(EntityAI vehicle, string ownerUid) { /* attach lock + owner, no pw */ }
       override bool AddCarLockWithPassword(EntityAI vehicle, string ownerUid, string password) { /* valide 8 digits puis pose */ }
       override bool RemoveCarLock(EntityAI vehicle) { ... }
       override bool IsLocked(EntityAI vehicle) { ... }
       override bool SetOwner(EntityAI vehicle, string ownerUid) { ... }
       override string GetOwner(EntityAI vehicle) { ... }
       override int  GetPasswordLength() { return 8; }
   }
   #endif
   ```
2. Déléguer à `TXCLVehicleLockManager` / `TXCLVehicleLockComponent` (façade fine).
3. `AddCarLockWithPassword` valide la longueur (8) + digits-only avant de poser ; retourne false sinon.
4. Enregistrer dans `TXCLModule` (ou un AppModule dédié), sous `#ifdef TraderX` :
   `TraderXProviderRegistry.GetInstance().RegisterCarLockProvider(new TXCLLockProvider());`
5. Appels **serveur**.

Cas d'usage cible : un trader-mod achète un véhicule → `AddCarLockWithPassword(veh, buyerUid, code)` pose un verrou 8 chiffres au nom de l'acheteur dès le spawn.

## Non-goals

- Pas de hashing du mot de passe (compat réseau, comme les deux sources).
- Pas d'intégration trader/économie (verrou seul).
- Support BoatScript = optionnel (nice-to-have via le manager), pas requis MVP.

## Gotchas (Enforce + UI)

- **Noms uniques** : préfixe `TXCL` strict — ne PAS réutiliser `CarLock`/`TLCarLock`/`TraderPlusCarLock` (collision avec mods existants chargés). `detect_redecl.py` propre.
- Pas de ternaire (`TernaryInt()`/`DZTernaryInt.If`).
- `switch` dans une méthode : prévoir un `return` par défaut HORS du switch aussi.
- Une ligne de code ne se découpe pas sur plusieurs lignes (Enforce).
- DZML : éditer `.html`, recompiler ; texte couleur `dz-c` pas `dz-tc`.
- Right-click lève aussi OnClick → gérer centralement (mémoire `dayz-rclick-also-raises-onclick`).

## Done (= US-011)

- [ ] Actions lock / unlock / menu / lockpick / attach-detach fonctionnelles (ressenti original)
- [ ] Mot de passe **8 chiffres** : set + confirmation + validation serveur
- [ ] Keypad DayZular (pavé grille) avec cooldown countdown
- [ ] Cooldown sur mauvais code (DB par joueur persistée)
- [ ] Lockpick : temps config-driven + alarme + full-body
- [ ] Persistance owner/verrou après reconnexion
- [ ] Sons lock/unlock/alarme branchés
- [ ] Pipeline DZML vert + `detect_redecl.py` propre
- [ ] Build.ps1 + Dmitri validé in-game
