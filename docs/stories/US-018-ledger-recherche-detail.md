# US-018 — Ledger : recherche + panel détail complet

**Périmètre :** TraderXBanking  
**Priorité :** P2

**Given** un joueur sur l'onglet Ledger avec des transactions existantes  
**When** il saisit un terme dans le champ de recherche ou clique sur une ligne  
**Then** la liste se filtre en temps réel (par montant, contrepartie, note) ET un panel détail s'affiche avec toutes les infos de la transaction sélectionnée

---

## Scope technique

### LE-03 : Recherche ledger (`TXBLedgerTabComponent.c`)

Ajouter :
- Binding `searchText` (EditBoxBinding sur le champ existant dans `LedgerTabView.layout`)
- `void OnSearchChanged()` dans `PropertyChanged()` — déclenche `ApplyFilter()` avec debounce 200ms
- Dans `ApplyFilter()` : si `searchText` non vide, filtrer `m_AllEntries` par substring sur :
  - `entry.counterpartyName`
  - `entry.note`
  - `Math.AbsInt(entry.amount).ToString()` (recherche par montant)
- Combiner avec le filtre de type existant (AND logique)

### LE-04 : Panel détail (`TXBLedgerTabComponent.c`)

L'infrastructure existe (`selected_*` fields, `OnRowClicked`, `SelectEntry`). Compléter :
- `void SelectEntry(string entryId)` : peupler tous les fields `selected_*` depuis l'entry trouvée
- Vérifier que `selected_visible` bascule bien le panel (binding layout OK)
- Vérifier le bouton "fermer détail" → remet `selected_visible = false`
- Champs à afficher : date/heure, type (libellé lisible), montant, frais, solde après, contrepartie, note

---

## Tests

- **Acceptance (source de vérité)** : Manuel in-game — voir étapes ci-dessous

## Étapes de validation (in-game)

### Recherche
1. **Given** Joueur avec ≥ 10 transactions dans le ledger
2. **Action** : Saisir "Dmitri" dans le champ recherche
3. **Vérifier** : Seules les lignes avec "Dmitri" en contrepartie s'affichent (filtre temps réel)
4. **Action** : Saisir "500" dans le champ recherche
5. **Vérifier** : Seules les transactions de 500₽ (±) s'affichent
6. **Action** : Vider le champ recherche
7. **Vérifier** : Liste complète restaurée

### Détail
8. **Action** : Cliquer sur une ligne de transaction
9. **Vérifier** : Panel détail s'affiche avec date/heure, type, montant, frais, solde après, contrepartie, note
10. **Action** : Cliquer "fermer" ou cliquer ailleurs
11. **Vérifier** : Panel détail se masque sans crash

## Done

- [ ] Recherche temps réel (debounce 200ms) sur contrepartie + note + montant
- [ ] Recherche combinée avec filtre type (AND)
- [ ] `SelectEntry()` peuple tous les `selected_*` fields
- [ ] Panel détail visible/caché via `selected_visible`
- [ ] Dmitri validé in-game
