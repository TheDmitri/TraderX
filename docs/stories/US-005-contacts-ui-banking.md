# US-005 — Contacts UI : carnet d'adresses Banking (TR-02)

**Périmètre :** TraderXBanking  
**Priorité :** P1

**Given** un joueur qui a effectué au moins un virement P2P (les contacts `TXBContactData` existent en JSON serveur)  
**When** il ouvre le menu Banking et navigue vers l'onglet Transfer  
**Then** il voit ses contacts récents et peut cliquer sur un contact pour pré-remplir automatiquement le champ destinataire du formulaire de virement

---

## Contexte technique

- Modèle `TXBContactData` : existe (`scripts/3_Game/Models/Domain/TXBContactData.c`)
- Persistence : `$profile:TraderXBanking/Contacts/Contacts_<uid>.json`
- Auto-population : déjà fait dans `TXBBankingService.ProcessTransfer()` (ligne 267)
- RPC pour syncer les contacts vers le client : **manquant**
- UI composant : **manquant**

## Scope technique

### `TXBRPCService.c` — ajouter

```
Client → Serveur : RequestContacts()
Serveur → Client : OnContactsReceived(contactsJson)
```

Handler serveur : charger `TXBContactData` du joueur → sérialiser → envoyer.

### `TXBTransferTabComponent.c` — ajouter

- `array<ref TXBContact> _contacts` (ComponentWidgetBinding + ForEach)
- `OnInit()` → appeler `RequestContacts()`
- Subscribe `Event_OnContactsReceived` → hydrate `_contacts` + `MarkForCheck()`
- `void OnContactSelected(string uid, string name)` : pré-remplir `destinationAccountId` avec `uid`

### Layout `TransferTabView.html` + `.layout`

- Ajouter section "Contacts récents" (liste horizontale ou verticale, max 5 affichés)
- Chaque contact : avatar initiales + nom + Account ID tronqué
- Click → `OnContactSelected`
- État vide : "No recent contacts" si `_contacts` vide

---

## Tests

- **Acceptance (source de vérité)** : Manuel in-game — voir étapes ci-dessous
- **UT EnforceScript** : N/A (UI only, backend existant)

## Étapes de validation (in-game)

1. **Setup** : Joueur A effectue un virement vers Joueur B (contact auto-créé)
2. **Action** : Joueur A ferme et rouvre Banking → onglet Transfer
3. **Vérifier** : Joueur B apparaît dans la liste des contacts récents (nom + ID tronqué)
4. **Action** : Joueur A clique sur le contact Joueur B
5. **Vérifier** : Le champ "Destinataire" est pré-rempli avec l'Account ID de Joueur B
6. **Vérifier** : Le formulaire reste utilisable (montant, memo, envoi)
7. **Vérifier** : Avec 0 contact, section affiche "No recent contacts" (pas de crash)
8. **Vérifier** : Pipeline DZML Banking recompile (4/4 layouts vert)

## Done

- [ ] RPC `RequestContacts` / `OnContactsReceived` implémenté
- [ ] Section contacts visible dans l'onglet Transfer
- [ ] Click contact pré-remplit le destinataire
- [ ] État vide géré
- [ ] Pipeline DZML Banking vert (4/4)
- [ ] Dmitri validé in-game
