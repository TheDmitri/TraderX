# TraderXBanking - Specification Document

> **Version:** 1.0  
> **Date:** 2026-04-19  
> **Status:** Architecture & Implementation Planning

TraderXBanking est un mod DayZ simulant un système bancaire complet avec comptes joueurs, vaults pour items, système de clans avec banque partagée, et historique des transactions.

---

## Table of Contents

1. [Domaines & Features](#1-domaines--features)
2. [Architecture Technique](#2-architecture-technique)
3. [Modèles de Données](#3-modèles-de-données)
4. [User Flows par Domaine](#4-user-flows-par-domaine)
5. [Plan d'Implémentation](#5-plan-dimplémentation)
6. [Intégrations Externes](#6-intégrations-externes)

---

## 1. Domaines & Features

### 1.1 Domaine: Compte Bancaire Personnel

**Objectif:** Gestion du compte bancaire individuel du joueur.

| Feature | Description | Status |
|---------|-------------|--------|
| **AC-01** Visualiser solde et capacité | Dashboard avec balance, limite max (default 50k₽), progression | ✅ Backend OK, UI OK |
| **AC-02** Déposer cash | Conversion cash inventaire → solde bancaire (avec frais %) | ✅ Backend OK, UI OK |
| **AC-03** Retirer cash | Conversion solde bancaire → cash inventaire (requiert carte ATM) | ✅ Backend OK, UI OK |
| **AC-04** Historique personnel | Liste des transactions 30 derniers jours dans l'onglet Account | ✅ Backend OK, UI OK |
| **AC-05** Copier Account ID | Bouton copie de l'ID Steam formaté (76561198·XXXX·XXXX) | 🔄 À implémenter |
| **AC-06** Stats 30 jours | Total dépôts/retraits/transferts sur 30j | 🔄 À implémenter |

**Règles métier:**
- Frais de transaction: configurable (default 2%)
- Solde max: configurable par compte (default 50,000₽)
- Retrait requiert une `TXBCreditCard` dans l'inventaire (si et seulement si l'admin à config la carte)
- Cash = items physiques (Roubles|US|EUR|GBP) dans l'inventaire du joueur à définir par l'admin la monnaie principale.

---

### 1.2 Domaine: Vault (Coffre-fort)

**Objectif:** Stockage sécurisé d'items avec snapshots complets.

| Feature | Description | Status |
|---------|-------------|--------|
| **VA-01** Vault personnel | Coffre individuel avec capacité limitée (default 100 slots) | ✅ Backend OK, UI OK |
| **VA-02** Vault clan | Coffre partagé au clan, accessible selon rôle | ✅ Backend OK, UI en cours |
| **VA-03** Déposer item | Snapshot complet de l'item (health, qty, attachments, cargo) | ✅ Backend OK |
| **VA-04** Retirer item | Recréation fidèle de l'item avec tout son état | ✅ Backend OK |
| **VA-05** Grille de slots | Visualisation 6×N des items stockés avec health indicator | ✅ UI OK |
| **VA-06** Frais quotidiens | X₽ par slot occupé, débité auto du compte bancaire | 🔄 À implémenter |
| **VA-07** Sélecteur propriétaire | Switch entre vault perso et vaults des clans du joueur | 🔄 À implémenter |
| **VA-08** Take all | Retirer tous les items d'un coup | ✅ Backend OK |

**Règles métier:**
- Snapshot récursif: health, quantity, température, wetness, énergie, ammo, liquid type
- Frais: 3₽/slot/jour (configurable), collecté toutes les 24h
- Si solde insuffisant: items gelés (pas de retrait possible jusqu'à paiement)
- Limite de 50 entrées d'activité conservées

---

### 1.3 Domaine: Transferts P2P

**Objectif:** Transfert de fonds entre joueurs.

| Feature | Description | Status |
|---------|-------------|--------|
| **TR-01** Transfert à un joueur | Envoi à un Account ID avec note | ✅ Backend OK, UI OK |
| **TR-02** Carnet d'adresses | Historique des destinataires récents | 🔄 Modèle existe, UI manque |
| **TR-03** Validation destinataire | Vérification existence compte avant envoi | ✅ Backend OK |
| **TR-04** Frais de transfert | % prélevé sur l'expéditeur | ✅ Backend OK |
| **TR-05** Transfert vers clan | Dépôt direct sur le compte clan | 🔄 À implémenter |

**Règles métier:**
- Frais: 2% (configurable) payés par l'expéditeur
- Montant reçu = montant envoyé (frais en plus)
- Contact auto-ajouté après transfert
- Double écriture: TRANSFER_OUT (expéditeur) + TRANSFER_IN (destinataire)

---

### 1.4 Domaine: Clans

**Objectif:** Gestion de groupes avec banque et vault partagés.

| Feature | Description | Status |
|---------|-------------|--------|
| **CL-01** Créer un clan | Nom, tag (3-5 chars), motto, max members, dépôt initial | 🔄 À implémenter |
| **CL-02** Rejoindre un clan | Par invitation ou code d'invitation | 🔄 À implémenter |
| **CL-03** Invitations | Système d'invite avec expiration (7j) | 🔄 À implémenter |
| **CL-04** Rôles | Owner > Officer > Member (permissions différenciées) | 🔄 Modèle existe, logique manque |
| **CL-05** Gestion roster | Liste membres avec contribution 30j | 🔄 À implémenter |
| **CL-06** Promouvoir/Rétrograder | Officer+ peut gérer les rôles | 🔄 À implémenter |
| **CL-07** Expulser | Officer+ peut kick (sauf Owner) | 🔄 À implémenter |
| **CL-08** Quitter le clan | Sortie volontaire, transfert ownership si Owner | 🔄 À implémenter |
| **CL-09** Clan bank | Dépôt/retrait de fonds (Officer+ pour retrait) | 🔄 À implémenter |
| **CL-10** Clan vault | Stockage items partagé | ✅ Backend OK |
| **CL-11** Log d'activité | Historique des actions clan | 🔄 À implémenter |
| **CL-12** Classement contributions | Membres triés par contribution 30j | 🔄 À implémenter |

**Permissions par rôle:**
| Action | Owner | Officer | Member |
|--------|-------|---------|--------|
| Déposer bank/vault | ✅ | ✅ | ✅ |
| Retirer bank | ✅ | ✅ | ❌ |
| Retirer vault | ✅ | ✅ | ✅ |
| Inviter joueurs | ✅ | ✅ | ❌ |
| Accepter/refuser invites | ✅ | ✅ | ❌ |
| Promouvoir/rétrograder | ✅ | ✅ (jusqu'à Officer) | ❌ |
| Expulser | ✅ | ✅ (non-Officer) | ❌ |
| Modifier settings | ✅ | ❌ | ❌ |
| Dissoudre clan | ✅ | ❌ | ❌ |

---

### 1.5 Domaine: Ledger (Grand Livre)

**Objectif:** Historique complet et traçable de toutes les transactions.

| Feature | Description | Status |
|---------|-------------|--------|
| **LE-01** Liste chronologique | Toutes les transactions avec pagination | ✅ Backend OK |
| **LE-02** Filtrage | Par type (dépôt/retrait/transfert/trader/fees) | 🔄 À implémenter |
| **LE-03** Recherche | Par montant, contrepartie, note | 🔄 À implémenter |
| **LE-04** Détail transaction | Vue détaillée d'une transaction sélectionnée | 🔄 À implémenter |
| **LE-05** Export CSV | Génération fichier export | 🔄 À implémenter |
| **LE-06** Séparateurs journaliers | Grouping par date visuelle | ✅ UI OK |

**Types d'entrées:**
```
DEPOSIT        = Cash → Banque
WITHDRAW       = Banque → Cash
TRANSFER_OUT   = Envoi à un joueur
TRANSFER_IN    = Réception d'un joueur
TRADER_SELL    = Vente au trader (externe)
TRADER_BUY     = Achat au trader (externe)
GARAGE_FEE     = Frais garage (externe)
CLAN_DEPOSIT   = Dépôt sur compte clan
CLAN_WITHDRAW  = Retrait du compte clan
VAULT_FEE      = Frais vault
```

---

### 1.6 Domaine: Home Dashboard

**Objectif:** Vue d'ensemble et accès rapide.

| Feature | Description | Status |
|---------|-------------|--------|
| **HO-01** Hero card | Message de bienvenue + meta (Account ID, Session) | ✅ UI OK |
| **HO-02** KPI tiles | Balance, Vault, Clan, Stats 30j | ✅ UI OK |
| **HO-03** Raccourcis rapides | Boutons vers actions fréquentes | ✅ UI OK |
| **HO-04** Activité récente | 6 dernières transactions | ✅ Backend OK |
| **HO-05** Invitations en attente | Pending clan invites avec accept/decline | 🔄 À implémenter |
| **HO-06** Alertes | Vault fee due, notifications importantes | 🔄 À implémenter |

---

## 2. Architecture Technique

### 2.1 Stack & Frameworks

```
UI Layer:      DZML (DayZ Markup Language) → .layout
Components:    DayZular Framework (DZlrComponentBase)
Services:      DayZularService (Server/Client sync via RPC)
Models:        JSON-serializable classes (3_Game)
Persistence:   JSON files sur disque server
Currency:      Items physiques (Roubles) + Compte virtuel
```

### 2.2 Structure des Packages

```
scripts/
├── 3_Game/           # Models, Constants, Config (client+server)
│   ├── Config/
│   ├── Constants/
│   ├── Models/       # Data classes (Account, Clan, Vault, Ledger)
│   └── Repository/     # Persistence helpers
├── 4_World/
│   ├── Actions/      # UserActions pour ATM/NPC
│   ├── Components/GUI/   # DZlr components par feature
│   │   ├── Account/
│   │   ├── Clan/
│   │   ├── Ledger/
│   │   ├── Transfer/
│   │   ├── Vault/
│   │   ├── TXBBankingComponent.c      # Shell principal
│   │   └── TXBHomeTabComponent.c       # Dashboard
│   ├── Entities/     # ATM, CreditCard items
│   ├── Providers/    # Payment provider pour TraderX
│   └── Services/     # Business logic server-side
│       ├── TXBBankingService.c      # Comptes & transactions
│       ├── TXBVaultService.c        # Vault deposit/withdraw
│       ├── TXBClanService.c         # Gestion clans (À créer)
│       ├── TXBRPCService.c          # RPC handlers
│       └── TXBUIService.c           # Menu management
└── 5_Mission/        # Mission gameplay
```

### 2.3 Flux de Données

```
Client (UI)          RPC                    Server (Services)
─────────────────────────────────────────────────────────────
Component.Init  ──→ RequestAccount ──→  LoadAccount()
                    OnAccountReceived ←  SendAccountToClient()

Deposit Request ──→ RequestDeposit ──→  ProcessDeposit()
                    OnAccountReceived ←  RecordLedgerEntry()

Vault Request   ──→ RequestVault ───→  LoadVault()
                    OnVaultReceived  ←  SendVaultToClient()

Transfer Req    ──→ RequestTransfer ─→  ProcessTransfer()
                    OnAccountReceived ←  (double écriture)
```

---

## 3. Modèles de Données

### 3.1 TXBAccountData (existe)

```c
string version;       // "1.0"
string playerUID;     // Steam ID
string playerName;    // Nom affiché
int balance;          // Solde actuel (roubles)
int maxBalance;       // Limite max (default 50000)
```

**Fichier:** `$profile:TraderXBanking/DB/Account_<UID>.json`

### 3.2 TXBVaultData (existe)

```c
string ownerKey;          // "player:<UID>" ou "clan:<clanId>"
string ownerType;         // "player" | "clan"
int capacity;             // Max slots
int slotsUsed;
int feePerSlotPerDay;     // Default 3
int lastAccrualUtc;       // Dernier prélèvement frais
array<TXBVaultItem> items;
array<TXBVaultActivityEntry> activity;
```

**Fichier:** `$profile:TraderXBanking/Vaults/Vault_<ownerKey>.json`

### 3.3 TXBClanData (existe, partiel)

```c
string clanId;            // UUID généré
string name;
string tag;               // 3-5 chars
string motto;
int foundedUtc;
int maxMembers;           // Default 20
string ownerUid;
int bankBalance;          // Solde clan
int vaultCapacity;        // Default 500
int vaultUsed;
array<TXBClanMember> members;
array<TXBClanActivityEntry> activity;
```

**Fichier:** `$profile:TraderXBanking/Clans/<clanId>.json`

### 3.4 TXBClanMember

```c
string uid;
string name;
int role;                 // 0=Member, 1=Officer, 2=Owner
int joinedUtc;
int contribution30d;        // Total dépôts 30j
// UI helpers (non persistés)
bool canPromote;
bool canKick;
```

### 3.5 TXBClanInvite (À créer)

```c
string inviteId;          // UUID
string clanId;
string clanName;
string clanTag;
string invitedByUid;
string invitedByName;
string targetUid;         // Joueur invité
int createdUtc;
int expiresUtc;           // 7 jours
string inviteCode;        // Code optionnel (24h)
```

**Fichier:** `$profile:TraderXBanking/Invites/<inviteId>.json`

### 3.6 TXBContactData (existe)

```c
array<TXBContactEntry> entries;
// Chaque entrée: uid, name, lastTransferUtc, transferCount
```

### 3.7 TXBLedgerEntry (existe)

```c
string id;                // TX-XXXX-XXXX-XXXX
int timestampUtc;         // YYYYMMDDhhmmss format
int type;                 // TXBLedgerEntryType
string counterpartyName;
string counterpartyUid;
int amount;
int fee;
int balanceAfter;
string location;
string note;
```

**Fichier:** `$profile:TraderXBanking/Ledgers/<UID>.json`

---

## 4. User Flows par Domaine

### 4.1 Flow: Déposer du Cash

```
1. Joueur ouvre menu Banking (via NPC ou ATM)
2. Navigation → Onglet "Account"
3. Saisie du montant dans le champ "Deposit"
4. Clic bouton "Deposit"
   → Validation: cash suffisant dans inventaire?
   → Validation: solde+montant ≤ maxBalance?
5. Client envoie RPC RequestDeposit
6. Server:
   - Vérifie cash dans inventaire (GetPlayerMoney)
   - Calcule frais: fee = montant × feePercent
   - Net = montant - fee
   - Retire items cash de l'inventaire (RemoveMoneyFromPlayer)
   - Crédite le compte (+Net)
   - Crée LedgerEntry DEPOSIT
   - Envoie compte mis à jour au client
7. UI update: solde affiché mis à jour
```

### 4.2 Flow: Transfert P2P

```
1. Onglet "Transfer"
2. Saisie Account ID du destinataire
3. Validation client: existe-t-il? (optionnel)
4. Saisie montant + note
5. Affichage récapitulatif:
   - Montant: X
   - Frais: X × 2%
   - Total débité: X + frais
   - Montant reçu: X
6. Clic "Send transfer"
7. RPC RequestTransfer
8. Server:
   - Vérifie solde suffisant (balance ≥ total)
   - Vérifie compte destinataire existe
   - Débite expéditeur (-total)
   - Crédite destinataire (+montant)
   - Crée LedgerEntry TRANSFER_OUT (expéditeur)
   - Crée LedgerEntry TRANSFER_IN (destinataire)
   - Ajoute contact automatique
   - Notifie les deux clients (si online)
9. Confirmation UI
```

### 4.3 Flow: Déposer un Item au Vault

```
1. Onglet "Vault"
2. Sélection du vault (perso ou clan) via dropdown
3. Grid affichée avec slots occupés/vides
4. Clic bouton "Deposit from inventory"
5. Modal/Panel listing items de l'inventaire
6. Sélection item
7. RPC RequestVaultDeposit(ownerKey, className)
8. Server:
   - Charge le vault
   - Vérifie capacité non atteinte
   - Trouve l'item dans l'inventaire
   - Capture snapshot complet (CaptureItemData récursif)
   - Supprime l'item physique
   - Ajoute TXBVaultItem au vault
   - Crée activity entry
   - Sauvegarde vault
9. RPC OnVaultReceived avec vault mis à jour
10. Grid refresh
```

### 4.4 Flow: Créer un Clan

```
1. Onglet "Clan" (sans clan actif)
2. Affichage mode "Create or Join"
3. Formulaire création:
   - Nom (unique)
   - Tag (3-5 chars, unique)
   - Motto (optionnel)
   - Max members (5-50)
   - Dépôt initial (min 5,000₽)
4. Validation:
   - Nom/tag disponibles?
   - Solde suffisant pour frais+dépôt initial?
5. RPC RequestCreateClan(params)
6. Server:
   - Vérifie unicité nom/tag
   - Vérifie solde (frais création + dépôt initial)
   - Débite le compte
   - Crée TXBClanData
   - Crée vault clan (capacity initial = maxMembers × 10)
   - Ajoute créateur comme Owner
   - Crée LedgerEntry CLAN_DEPOSIT
7. Redirection vers clan actif
```

### 4.5 Flow: Accepter une Invitation Clan

```
1. Dashboard ou Clan tab affiche "Pending invites"
2. Liste avec nom clan, tag, invité par, membres, expiration
3. Clic "Accept" sur une invite
4. RPC RequestAcceptInvite(inviteId)
5. Server:
   - Vérifie invite existe et non expirée
   - Vérifie joueur non déjà dans un clan
   - Vérifie capacité clan non atteinte
   - Ajoute membre au clan (role=Member)
   - Supprime l'invite
   - Crée activity entry "X joined clan"
6. Client: refresh affichage clan
```

### 4.6 Flow: Collection Frais Vault (Auto)

```
[Job server-side exécuté périodiquement]

1. Pour chaque vault existant:
   a. Calcule slots utilisés
   b. Frais = slots × feePerSlotPerDay
   c. Détermine compte à débiter:
      - Si vault player: compte du joueur
      - Si vault clan: compte clan
   d. Si solde suffisant:
      - Débite le compte
      - Met à jour lastAccrualUtc
      - Crée LedgerEntry VAULT_FEE
      - Crée activity entry
   e. Sinon:
      - Marque vault "frozen"
      - Notification au propriétaire
      - Les items ne peuvent plus être retirés
```

---

## 5. Plan d'Implémentation

### Phase 1: Core Bancaire (Complet)

**Status:** ✅ Terminé
- TXBBankingService avec queue de transactions
- Deposit/Withdraw/Transfer avec frais
- Ledger persistence
- Account sync RPC

### Phase 2: Vault (Core Complet)

**Status:** ✅ Backend terminé, UI en cours

**Tâches restantes:**
- [ ] TXBVaultFeeJob: Systeme de prélèvement automatique
- [ ] Gestion "vault frozen" quand solde insuffisant
- [ ] Sélecteur multi-vaults (player + clans)
- [ ] UI: Item grid avec interactions complètes

### Phase 3: Carnet d'Adresses

**Status:** 🔄 À démarrer

**Modèle existant:** TXBContactData

**Tâches:**
- [ ] TXBContactService: CRUD contacts
- [ ] UI Transfer: Intégration contacts récents
- [ ] RPC: Sync contacts client-serveur

### Phase 4: Système de Clans (À créer)

**Status:** 🔄 Prioritaire

**Nouveaux fichiers à créer:**

| Fichier | Description |
|---------|-------------|
| `TXBClanService.c` | Business logic clans |
| `TXBClanRepository.c` | Persistence clans |
| `TXBClanInviteRepository.c` | Persistence invites |
| `TXBClanRPCService.c` | Handlers RPC clans |

**Components UI à créer:**

| Fichier | Description |
|---------|-------------|
| `TXBClanTabComponent.c` | Onglet Clan (switch InClan/NoClan) |
| `TXBClanCreateComponent.c` | Formulaire création clan |
| `TXBClanInClanComponent.c` | Dashboard clan (roster, bank, vault) |
| `TXBClanMemberRowComponent.c` | Ligne membre dans roster |
| `TXBClanInviteRowComponent.c` | Ligne invitation pending |
| `TXBClanActivityRowComponent.c` | Ligne activité clan |

**Tâches détaillées:**

1. **Service & Persistence (2-3j)**
   - [ ] TXBClanService avec méthodes: Create, Join, Leave, Invite, Kick, Promote
   - [ ] TXBClanRepository: Load/Save clans
   - [ ] TXBClanInviteRepository: Gestion cycle de vie invites
   - [ ] Validation permissions selon rôles

2. **RPC & Sync (1-2j)**
   - [ ] TXBClanRPCService: handlers pour tous les cas
   - [ ] RPC: GetMyClans, GetPendingInvites, CreateClan, AcceptInvite, etc.
   - [ ] Events: Event_OnClanUpdated, Event_OnInviteReceived

3. **UI Components (3-4j)**
   - [ ] TXBClanTabComponent: router entre NoClan/InClan views
   - [ ] TXBClanCreateComponent: formulaire avec validation
   - [ ] TXBClanInClanComponent: tabs Overview/Roster/Bank/Activity
   - [ ] Roster avec permissions dynamiques (boutons selon rôle)

4. **Intégration Vault (1-2j)**
   - [ ] Sélecteur vault: perso + clans du joueur
   - [ ] Permissions retrait selon rôle clan

### Phase 5: Ledger Avancé

**Status:** 🔄 UI de base OK, features avancées manquantes

**Tâches:**
- [ ] Filtrage par type (boutons toggle)
- [ ] Recherche texte (contrepartie, montant)
- [ ] Panel détail transaction (clic sur ligne)
- [ ] Export CSV (génération côté client)

### Phase 6: Home Dashboard Complet

**Status:** 🔄 Mockup OK, données mocked

**Tâches:**
- [ ] TXBHomeTabComponent: bind avec vraies données
- [ ] KPI tiles: compteurs dynamiques
- [ ] Pending invites: intégration avec TXBClanService
- [ ] Alertes: notifications temps réel

### Phase 7: Polishing & Intégrations

**Tâches:**
- [ ] CreditCard item: requirement pour retrait
- [ ] ATM Entity: interaction physique
- [ ] NPC Banker: dialogue + ouverture menu
- [ ] Sonfx et feedback visuel
- [ ] Documentation utilisateur

---

## 6. Intégrations Externes

### 6.1 TraderX (Trader)

**Point d'intégration:** TXBPaymentProvider

**Flux:**
```
Vente chez trader:
  Trader → TXBPaymentProvider.OnTraderSell()
         → TXBBankingService.RecordExternalLedger()
         → LedgerEntry de type TRADER_SELL créé

Achat chez trader:
  Trader → TXBPaymentProvider.OnTraderBuy()
         → Vérification solde bancaire disponible
         → Débit + LedgerEntry TRADER_BUY
```

### 6.2 Garage (Future)

**Point d'intégration:** TXBPaymentProvider

**Flux:**
```
Frais garage:
  GarageService → TXBPaymentProvider.OnGarageFee()
              → TXBBankingService.RecordExternalLedger()
              → LedgerEntry GARAGE_FEE
```

---

## Annexes

### A. Constantes Clés

```c
// File: 3_Game/Constants/TXBConstants.c
const string DB_DIR = "$profile:TraderXBanking/DB/";
const string VAULT_DIR = "$profile:TraderXBanking/Vaults/";
const string CLAN_DIR = "$profile:TraderXBanking/Clans/";
const string INVITE_DIR = "$profile:TraderXBanking/Invites/";
const string LEDGER_DIR = "$profile:TraderXBanking/Ledgers/";

const int QUEUE_INTERVAL = 1;           // Secondes
const int VAULT_DEFAULT_CAPACITY = 100;
const int VAULT_DEFAULT_FEE_PER_DAY = 3;
const int CLAN_DEFAULT_CAPACITY = 500;
const int INVITE_EXPIRY_DAYS = 7;
const int INVITE_CODE_EXPIRY_HOURS = 24;

const int TRANSACTION_DEPOSIT = 0;
const int TRANSACTION_WITHDRAW = 1;
const int TRANSACTION_TRANSFER = 2;
```

### B. RPC Namespace

```c
// TXBConstants.RPC_NAMESPACE = "TraderXBanking"

// Client → Server
RequestAccount
RequestDeposit
RequestWithdraw
RequestTransfer
RequestVault
RequestVaultDeposit
RequestVaultWithdraw
RequestVaultTakeAll
RequestCreateClan
RequestJoinClan
RequestAcceptInvite
RequestDeclineInvite
RequestClanDeposit
RequestClanWithdraw
RequestLedger

// Server → Client
OnAccountReceived
OnLedgerReceived
OnVaultReceived
OnClanReceived
OnInviteReceived
OnNotification
```

### C. URLs Documentation

- **Workflow DZML:** `C:\DayZWorkspace\dayz-tools\.windsurf\workflows\dzml-ui-mockup.md`
- **Validator:** `C:\DayZWorkspace\dayz-tools\dzml_validator.py`
- **Mockups HTML:** `C:\DayZWorkspace\TraderX\src\TraderXBanking\docs\banking\`

---

## Résumé d'Implémentation

| Domaine | Backend | UI | Priorité |
|---------|---------|-----|----------|
| Compte Bancaire | ✅ 100% | ✅ 100% | - |
| Vault (perso) | ✅ 100% | 🔄 80% | P2 |
| Transferts P2P | ✅ 100% | ✅ 100% | - |
| Ledger | ✅ 100% | 🔄 70% | P3 |
| Contacts | 🔄 50% | ❌ 0% | P4 |
| Clans | ❌ 0% | ❌ 0% | **P1** |
| Home Dashboard | 🔄 30% | ✅ 90% | P2 |
| Vault Fees (auto) | ❌ 0% | - | P2 |

**Prochaine étape recommandée:** Implémenter TXBClanService + Components Clan (Phase 4)
 

