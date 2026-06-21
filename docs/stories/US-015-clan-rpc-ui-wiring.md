# US-015 — Clan Banking : RPC + wiring des composants UI

**Périmètre :** TraderXBanking  
**Priorité :** P2 — dépend US-014  

**Given** un joueur avec le backend clan disponible (US-014)  
**When** il navigue vers l'onglet Clan dans Banking  
**Then** il peut créer un clan, inviter des joueurs, gérer le roster, déposer/retirer de la banque clan — le tout via l'UI existante (layouts déjà créés)

---

## Scope technique

Les layouts et composants skeleton existent déjà. Il faut uniquement :
1. Créer `TXBClanRPCService.c` avec tous les handlers RPC
2. Wirer les handlers dans les composants UI existants

### `TXBClanRPCService.c` — `scripts/4_World/Services/`

**Requests client → serveur :**
```
RequestCreateClan(name, tag, motto, maxMembers, initialDeposit)
RequestInvitePlayer(targetUID)
RequestJoinByCode(code)
RequestAcceptInvite(inviteId)
RequestDeclineInvite(inviteId)
RequestLeaveClan()
RequestKickMember(targetUID)
RequestPromoteMember(targetUID, newRole)
RequestClanDeposit(amount)
RequestClanWithdraw(amount)
RequestClanData()          ← sync initial
RequestIncomingInvites()   ← sync initial
```

**Callbacks serveur → client :**
```
OnClanDataReceived(clanJson)
OnIncomingInvitesReceived(invitesJson)
OnClanActionResult(success, message)
```

### Wiring `TXBClanCreateTabComponent.c`

- `void OnFoundClan()` → `RequestCreateClan(...)` avec les fields existants
- `void OnJoinByCode()` → `RequestJoinByCode(inviteCode)`
- `void OnAcceptInvite(inviteId)` → `RequestAcceptInvite(inviteId)`
- `void OnDeclineInvite(inviteId)` → `RequestDeclineInvite(inviteId)`
- Subscribe `Event_OnIncomingInvitesReceived` → hydrate `_incomingInvites`
- Lire `TXBSettings.clanFounderFee` pour afficher `founderFeeText`

### Wiring `TXBClanInClanComponent.c`

- `void OnClanDeposit()` → `RequestClanDeposit(clanBankDepositAmount)`
- `void OnClanWithdraw()` → `RequestClanWithdraw(clanBankWithdrawAmount)`
- `void OnInvitePlayer()` → `RequestInvitePlayer(inviteTargetUID)`
- `void OnLeaveClan()` → `RequestLeaveClan()`
- Subscribe `Event_OnClanDataReceived` → hydrate tous les champs (balance, members, activity)
- Subscribe `TXBClanMemberRowComponent.Event_OnActionClicked` → router promote/kick

### Wiring `TXBClanTabComponent.c`

- `OnInit()` → `RequestClanData()` + `RequestIncomingInvites()`
- Subscribe `Event_OnClanDataReceived` : si clan null → afficher "create", sinon afficher "inclan"

---

## Tests

- **Acceptance (source de vérité)** : Manuel in-game — voir étapes ci-dessous
- **Debug test** : TraderXDebugModeService si applicable

## Étapes de validation (in-game)

1. **Given** Joueur sans clan → ouvre Banking → onglet Clan
2. **Vérifier** : Vue "Créer un clan" affichée avec calcul frais en temps réel
3. **Action** : Remplir le formulaire → cliquer "Créer"
4. **Vérifier** : Vue bascule sur "Dans le clan" avec les données correctes
5. **Action** : Inviter un autre joueur (saisir son UID)
6. **Vérifier** : Invité reçoit la notification côté client (si connecté)
7. **Action** : Joueur invité accepte l'invitation
8. **Vérifier** : Roster mis à jour (MEMBER visible)
9. **Action** : Déposer 200₽ dans la banque clan
10. **Vérifier** : Solde clan mis à jour, ledger Banking enregistre la transaction

## Done

- [ ] `TXBClanRPCService.c` créé (12 requests, 3 callbacks)
- [ ] `TXBClanTabComponent` route create/inclan selon état
- [ ] Création clan fonctionnelle end-to-end
- [ ] Invite/accept/decline fonctionnel
- [ ] Deposit/withdraw banque clan fonctionnel
- [ ] Promote/kick roster fonctionnel
- [ ] Dmitri validé in-game
