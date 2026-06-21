# US-014 — Clan Banking : backend complet

**Périmètre :** TraderXBanking  
**Priorité :** P1 — débloque US-015, US-016, US-017, US-019

**Given** un admin avec TraderXBanking chargé  
**When** un joueur crée un clan via l'UI Banking  
**Then** le clan est persisté serveur (JSON), le fondateur devient Owner, un compte bancaire clan est créé avec le dépôt initial, et les autres joueurs peuvent être invités

---

## Scope technique

Créer les 3 fichiers manquants qui constituent le backbone clan :

### `TXBClanService.c` — `scripts/4_World/Services/`

Méthodes à implémenter :
```
CreateClan(ownerUID, name, tag, motto, maxMembers, initialDeposit) → bool
DisbandClan(clanId, requesterUID) → bool
InvitePlayer(senderUID, targetUID, clanId) → TXBClanInvite
AcceptInvite(playerUID, inviteId) → bool
DeclineInvite(playerUID, inviteId) → bool
JoinByCode(playerUID, code) → bool
LeaveClan(playerUID, clanId) → bool
KickMember(requesterUID, targetUID, clanId) → bool
PromoteMember(requesterUID, targetUID, clanId, newRole) → bool
DepositToClanBank(playerUID, clanId, amount) → bool
WithdrawFromClanBank(playerUID, clanId, amount) → bool
GetPlayerClanId(playerUID) → string
GetClan(clanId) → TXBClanData
```

Règles métier :
- Frais de création = `TXBSettings.clanFounderFee` (débité du compte joueur)
- Dépôt initial obligatoire ≥ `TXBSettings.clanMinInitialDeposit`
- Retrait banque clan : rôle OFFICER+ uniquement
- Invite : rôle OFFICER+ uniquement
- Promote : OWNER peut tout ; OFFICER peut promouvoir jusqu'à OFFICER
- Kick : impossible de kicker un rôle ≥ au sien
- Dissolution : OWNER uniquement, transfère solde clan au Owner
- Quitter clan quand Owner : doit transférer ownership d'abord

### `TXBClanRepository.c` — `scripts/3_Game/Repository/`

```
LoadClan(clanId) → TXBClanData
SaveClan(data)
DeleteClan(clanId)
ClanExists(clanId) → bool
FindClanByPlayerUID(playerUID) → string   ← index inversé (joueur → clan)
```

Persistence : `$profile:TraderXBanking/Clans/Clan_<clanId>.json`  
Index inversé : `$profile:TraderXBanking/Clans/PlayerIndex_<uid>.json`

### `TXBClanInviteRepository.c` — `scripts/3_Game/Repository/`

```
SaveInvite(invite)
LoadInvite(inviteId) → TXBClanInvite
DeleteInvite(inviteId)
FindIncomingInvites(playerUID) → array<TXBClanInvite>
FindOutgoingInvites(clanId) → array<TXBClanInvite>
PurgeExpiredInvites()   ← appelé au startup
```

Persistence : `$profile:TraderXBanking/Clans/Invite_<inviteId>.json`  
Expiration : 7 jours (`TXBConstants.CLAN_INVITE_EXPIRY_DAYS`)

### Intégration dans `TXBAppModule.c`

Enregistrer `TXBClanService` dans le service registry (pattern identique à `TXBBankingService`).  
Appeler `TXBClanInviteRepository.PurgeExpiredInvites()` au startup serveur.

---

## Tests

- **Acceptance (source de vérité)** : Manuel in-game — voir étapes ci-dessous
- **UT EnforceScript** : Logique de permissions (PromoteMember, KickMember) si isolable

## Étapes de validation (in-game)

1. **Setup** : Joueur A avec compte Banking et solde suffisant
2. **Action** : Joueur A crée un clan "TestClan" avec dépôt initial 1000₽
3. **Vérifier** : `Clan_<id>.json` créé sur disque, Joueur A = OWNER, compte clan = 1000₽
4. **Vérifier** : Compte Joueur A débité (frais fondation + dépôt initial)
5. **Action** : Joueur A invite Joueur B → Joueur B accepte
6. **Vérifier** : Joueur B apparaît dans roster avec rôle MEMBER
7. **Action** : Joueur A promeut Joueur B en OFFICER
8. **Vérifier** : Rôle OFFICER enregistré
9. **Action** : Joueur B dépose 500₽ dans la banque clan
10. **Vérifier** : Solde clan = 1500₽, ledger Banking enregistre `TRANSFER_OUT` pour Joueur B

## Done

- [ ] `TXBClanService.c` créé et fonctionnel
- [ ] `TXBClanRepository.c` créé, JSON persisté
- [ ] `TXBClanInviteRepository.c` créé, purge expiration OK
- [ ] Service enregistré dans `TXBAppModule`
- [ ] Règles de permissions respectées (can't kick/withdraw sans rôle)
- [ ] Dmitri validé in-game
