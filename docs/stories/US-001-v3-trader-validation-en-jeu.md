# US-001 — V3 Trader : validation en jeu

**Périmètre :** TraderX (core)  
**Priorité :** P1

**Given** un joueur en jeu avec un serveur qui charge le PBO TraderX contenant les scripts v3  
**When** il interagit avec un NPC Trader (ou utilise une debug key configurée)  
**Then** le menu `V3TraderXMainComponent` s'ouvre avec les 5 pages (Favorites, Purchase, Sell, Catalog, Customize) fonctionnelles en mock data, sans crash

---

## Tests

- **Acceptance (source de vérité)** : Manuel in-game — voir étapes ci-dessous
- **Debug test** : TraderXDebugModeService si `runTestsOnStartup = true`
- **UT EnforceScript** : N/A (validation visuelle et fonctionnelle)

## Étapes de validation (in-game)

1. **Setup** : Ajouter les chemins v3 dans `config.cpp` de TraderX (`gameScriptModule` + `worldScriptModule` pour `scripts/4_World/classes/TraderX/GUI/v3/`)
2. **Setup** : Ajouter un hook d'ouverture (NPC Action ou debug key) appelant `ComponentMenu.ShowMenu("tx3-trader-main")`
3. **Build** : `.\Build.ps1` → lancer le serveur
4. **Action** : Se connecter, interagir avec le Trader NPC (ou appuyer sur la debug key)
5. **Vérifier** : Le menu s'ouvre sans crash, la page Purchase affiche les produits mock
6. **Vérifier** : Navigation entre les 5 onglets (Favorites / Purchase / Sell / Catalog / Customize) fonctionne
7. **Vérifier** : Ajout au checkout, qty stepper, clear checkout — aucun crash
8. **Vérifier** : Filter pills, expand/collapse catégories, recherche
9. **Vérifier** : Fermeture du menu sans crash

## Done

- [ ] Scripts v3 déclarés dans `config.cpp`
- [ ] Hook NPC/debug ouvre `V3TraderXMainComponent`
- [ ] 5 pages naviguables sans crash
- [ ] Interactions principales testées (checkout, search, expand)
- [ ] Dmitri validé in-game

## Notes

Voir `docs/AI_Handover.md §5` pour le détail des fichiers à wirer.  
Voir `DECISIONS.md D-003` pour le contexte de l'approche v3.
