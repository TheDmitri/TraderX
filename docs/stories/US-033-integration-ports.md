# US-033 — TraderX : couche d'intégration (ports providers cross-mod)

**Périmètre :** `src/TraderX` (core) UNIQUEMENT — fichier(s) d'intégration partagé(s)
**Priorité :** P1 — **prérequis** des providers Garage/CarLock/SafeZone/Banking
**Préfixe de classe réservé :** `ITraderX*` (interfaces) + `TraderXProviderRegistry` + `TraderX*Info` (DTO)

> ⚠️ **À FAIRE EN PREMIER**, par le coordinateur (pas en parallèle). Les 3 sessions modules (US-030/031/032) implémentent ensuite *leur* provider dans *leur* dossier, en référençant ces interfaces. Tant que ce fichier n'existe pas, `TXBPaymentProvider` (Banking) est **dormant** : il référence `ITraderXPaymentProvider` + `TraderXProviderRegistry` qui ne sont **définis nulle part** aujourd'hui.

---

## Objectif

Un point d'entrée stable et unique pour que **des mods externes** (ou nos propres modules) se branchent sur TraderX :
- payer avec le compte bancaire,
- garer / sortir / ajouter un véhicule au garage d'un joueur,
- poser / retirer un carlock (8 chiffres) sur un véhicule acheté,
- interroger / enregistrer une safezone.

**Principe** : un **registry singleton** dans le core expose des **interfaces** ; chaque module enregistre son implémentation à l'init (comme Banking le fait déjà). Les mods externes appellent `TraderXProviderRegistry.GetInstance().GetXProvider()` sans dépendre du PBO du module.

**Règle de découplage (cruciale)** : les signatures n'utilisent **que des types vanilla** (`Man`, `EntityAI`, `string`, `int`, `vector`, `bool`) + DTO du core. Jamais de type interne d'un module → un mod externe compile sans hard-dep. Pour le contexte ledger, passer une **string catégorie** (ex. `"GARAGE_FEE_OUT"`) comme `DeductLabeled` (déjà en place).

## Emplacement

```
src/TraderX/scripts/4_World/Integration/
  TraderXProviderRegistry.c     — singleton + getters/setters des 4 providers
  ITraderXPaymentProvider.c     — (formalise l'existant utilisé par Banking)
  ITraderXGarageProvider.c
  ITraderXCarLockProvider.c
  ITraderXSafeZoneProvider.c
  TraderXIntegrationDTO.c        — TraderXGarageVehicleInfo, etc.
```

> Enforce n'a pas de mot-clé `interface` fiable cross-version → on définit les "interfaces" comme **classes de base à méthodes virtuelles** renvoyant des défauts. `class TXBPaymentProvider : ITraderXPaymentProvider` fonctionne déjà ainsi.

## Squelettes (contrat — à implémenter tel quel)

### Registry

```c
class TraderXProviderRegistry
{
    protected static ref TraderXProviderRegistry s_Instance;

    protected ref ITraderXPaymentProvider  m_Payment;
    protected ref ITraderXGarageProvider   m_Garage;
    protected ref ITraderXCarLockProvider  m_CarLock;
    protected ref ITraderXSafeZoneProvider m_SafeZone;

    static TraderXProviderRegistry GetInstance()
    {
        if (!s_Instance) s_Instance = new TraderXProviderRegistry();
        return s_Instance;
    }

    void RegisterBankingProvider(ITraderXPaymentProvider p)  { m_Payment = p; }
    void RegisterGarageProvider(ITraderXGarageProvider p)    { m_Garage = p; }
    void RegisterCarLockProvider(ITraderXCarLockProvider p)  { m_CarLock = p; }
    void RegisterSafeZoneProvider(ITraderXSafeZoneProvider p){ m_SafeZone = p; }

    ITraderXPaymentProvider  GetBankingProvider()  { return m_Payment; }
    ITraderXGarageProvider   GetGarageProvider()   { return m_Garage; }
    ITraderXCarLockProvider  GetCarLockProvider()  { return m_CarLock; }
    ITraderXSafeZoneProvider GetSafeZoneProvider() { return m_SafeZone; }

    bool HasBanking()  { return m_Payment  != null; }
    bool HasGarage()   { return m_Garage   != null; }
    bool HasCarLock()  { return m_CarLock  != null; }
    bool HasSafeZone() { return m_SafeZone != null; }
}
```

### ITraderXPaymentProvider (formalise l'existant — voir `TXBPaymentProvider.c`)

```c
class ITraderXPaymentProvider
{
    int  GetBalance(Man player) { return 0; }
    int  GetClientBalance() { return 0; }
    bool Deduct(Man player, int amount) { return false; }
    void Credit(Man player, int amount) { }
    bool DeductLabeled(Man player, int amount, string category, string counterpartyName) { return false; }
    void CreditLabeled(Man player, int amount, string category, string counterpartyName) { }
    bool IsAvailable(Man player) { return false; }
    string GetDisplayName() { return ""; }
    void RequestBalanceRefresh() { }
}
```

### ITraderXGarageProvider

```c
class ITraderXGarageProvider
{
    // Serveur. Snapshot l'état (health/fuel/cargo/attachments) puis retire l'entité du monde.
    bool AddVehicle(string playerUid, EntityAI vehicle) { return false; }
    // Crée un véhicule neuf de ce type et le range direct (ex. véhicule acheté non spawné).
    bool AddVehicleByType(string playerUid, string className) { return false; }
    // Retire du garage sans respawn (vendu/détruit).
    bool RemoveVehicle(string playerUid, string vehicleId) { return false; }
    // Respawn le véhicule garé à pos/orientation, restaure l'état. Retourne l'entité (ou null).
    EntityAI RetrieveVehicle(string playerUid, string vehicleId, vector pos, vector orientation) { return null; }
    int  GetStoredCount(string playerUid) { return 0; }
    int  GetMaxSlots() { return 0; }
    bool HasFreeSlot(string playerUid) { return false; }
    array<ref TraderXGarageVehicleInfo> ListVehicles(string playerUid) { return null; }
}
```

### ITraderXCarLockProvider

```c
class ITraderXCarLockProvider
{
    // Serveur. Pose un carlock, owner = ownerUid, sans mot de passe.
    bool AddCarLock(EntityAI vehicle, string ownerUid) { return false; }
    // Pose + mot de passe 8 chiffres (string de 8 digits). False si longueur/format invalide.
    bool AddCarLockWithPassword(EntityAI vehicle, string ownerUid, string password) { return false; }
    bool RemoveCarLock(EntityAI vehicle) { return false; }
    bool IsLocked(EntityAI vehicle) { return false; }
    bool SetOwner(EntityAI vehicle, string ownerUid) { return false; }
    string GetOwner(EntityAI vehicle) { return ""; }
    int  GetPasswordLength() { return 8; }
}
```

### ITraderXSafeZoneProvider

```c
class ITraderXSafeZoneProvider
{
    bool   IsInSafeZone(Man player) { return false; }
    bool   IsPositionInSafeZone(vector pos) { return false; }
    string GetZoneNameAt(vector pos) { return ""; }
    // Zone dynamique runtime (ex. event mod). Retourne un id zone (>0) ou -1.
    int    RegisterDynamicZone(string name, vector pos, float radius) { return -1; }
    bool   UnregisterDynamicZone(int zoneId) { return false; }
}
```

### DTO

```c
class TraderXGarageVehicleInfo
{
    string vehicleId;
    string className;
    string displayName;
    int    healthLevel;   // 0..4 (PRISTINE..RUINED)
}
```

## Exemple d'usage (mod externe — acheter un véhicule)

```c
TraderXProviderRegistry reg = TraderXProviderRegistry.GetInstance();
string uid = player.GetIdentity().GetPlainId();

// 1. Payer via banque (catégorie ledger custom)
if (reg.HasBanking() && reg.GetBankingProvider().DeductLabeled(player, price, "TRADER_BUY", "VehicleShop"))
{
    // 2. Spawner le véhicule acheté
    EntityAI veh = EntityAI.Cast(GetGame().CreateObjectEx(className, spawnPos, ECE_PLACE_ON_SURFACE));

    // 3. Poser un carlock 8 chiffres au nom de l'acheteur (si dispo)
    if (reg.HasCarLock())
        reg.GetCarLockProvider().AddCarLockWithPassword(veh, uid, chosenPassword);

    // 4. Ou directement le garer (si dispo + slot libre)
    if (reg.HasGarage() && reg.GetGarageProvider().HasFreeSlot(uid))
        reg.GetGarageProvider().AddVehicle(uid, veh);
}
```

## Non-goals

- Pas de RPC dans les interfaces : ce sont des appels **serveur** in-process (le mod externe orchestre côté serveur). Si un mod tiers veut piloter côté client, il passe par ses propres RPC puis appelle le provider serveur.
- Pas de versionning d'API formel pour le MVP (ajouter un `int GetApiVersion()` au registry si besoin plus tard).

## Done

- [ ] `TraderXProviderRegistry` + 4 interfaces + DTO compilent dans le core
- [ ] Banking `TXBPaymentProvider` n'est plus dormant (`#ifdef TraderX` satisfait, s'enregistre)
- [ ] `detect_redecl.py` propre
- [ ] Les 3 modules peuvent référencer leurs interfaces respectives
