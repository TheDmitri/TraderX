# US-016 — Vault clan + sélecteur multi-propriétaire

**Périmètre :** TraderXBanking  
**Priorité :** P2 — dépend US-014

**Given** un joueur membre d'un clan (US-014 livré)  
**When** il navigue vers l'onglet Vault dans Banking  
**Then** il voit un sélecteur "Personal / NomDuClan" et peut basculer entre son vault personnel et le vault clan, avec les permissions liées à son rôle

---

## Scope technique

### `TXBVaultTabComponent.c` — compléter `BuildOwnerList()`

```c
void BuildOwnerList()
{
    _availableOwners.Clear();
    // 1. Personal vault (toujours présent)
    TXBVaultOwner personal = new TXBVaultOwner();
    personal.ownerKey = "player:" + m_PlayerUID;
    personal.displayName = "Personal Vault";
    _availableOwners.Insert(personal);

    // 2. Clan vault si joueur membre d'un clan
    string clanId = TXBClanService.GetInstance().GetPlayerClanId(m_PlayerUID);
    if (clanId != "")
    {
        TXBClanData clan = TXBClanService.GetInstance().GetClan(clanId);
        if (clan)
        {
            TXBVaultOwner clanOwner = new TXBVaultOwner();
            clanOwner.ownerKey = "clan:" + clanId;
            clanOwner.displayName = clan.name + " Vault";
            _availableOwners.Insert(clanOwner);
        }
    }
    MarkForCheck();
}
```

### Permissions vault clan

- **Dépôt** : tous les membres peuvent déposer
- **Retrait** : OFFICER+ uniquement (`clan.GetMemberRole(playerUID) >= OFFICER`)
- **Take All** : OWNER uniquement
- Afficher/masquer les boutons selon le rôle courant

### `TXBVaultRPCService.c` — transmettre `ownerKey` avec les requêtes

Le `ownerKey` est déjà un paramètre dans les RPC existants. Vérifier que :
- `RequestVaultData(ownerKey)` est appelé avec le bon owner après `OnSwitchOwner()`
- Les handlers serveur vérifient les permissions clan avant d'exécuter dépôt/retrait

---

## Tests

## Étapes de validation (in-game)

1. **Given** Joueur A = OFFICER d'un clan, Joueur B = MEMBER du même clan
2. **Joueur A** : ouvre Vault → sélecteur affiche "Personal Vault" + "NomDuClan Vault"
3. **Joueur B** : ouvre Vault → sélecteur identique visible
4. **Joueur A** : sélectionne vault clan → dépose un item → item visible dans le vault clan
5. **Joueur B** : sélectionne vault clan → voit l'item → tente de le retirer
6. **Vérifier** : Joueur B ne peut pas retirer (MEMBER) — bouton retraite masqué ou désactivé
7. **Joueur A** : retire l'item du vault clan → item dans son inventaire
8. **Vérifier** : Vault clan vide, vault personnel A inchangé

## Done

- [ ] Sélecteur affiche personal + clan vault si joueur est membre
- [ ] Sélection bascule le vault affiché (données correctes)
- [ ] Permissions respectées (dépôt all / retrait OFFICER+ / take-all OWNER)
- [ ] Dmitri validé in-game
