# US-010 — TraderXGarage : validation backend complet

**Périmètre :** TraderXGarage  
**Priorité :** P1 (bloquant pour le module Garage)

**Given** un joueur en jeu sur un serveur avec TraderXGarage chargé et un NPC Garagiste configuré  
**When** il gare son véhicule, quitte le serveur, se reconnecte et demande la récupération  
**Then** le véhicule est restitué à la position de récupération avec son état intact (health, fuel, cargo, orientation)

---

## Tests

- **Acceptance (source de vérité)** : Manuel in-game — voir étapes ci-dessous
- **Debug test** : `TraderXVehicleTransactionTest` si applicable
- **UT EnforceScript** : `TraderXVehicleParkingService` — logique de snapshot/restore

## Étapes de validation (in-game)

### Park-in (garer)
1. **Setup** : Joueur conduit un véhicule chargé (cargo non vide) jusqu'au NPC Garagiste
2. **Action** : Interagir avec le NPC → sélectionner "Garer le véhicule"
3. **Vérifier** : Le véhicule disparaît de la map, son état est persisté (health, fuel, cargo snapshot)
4. **Vérifier** : Le ledger Banking affiche un débit `GARAGE_FEE_OUT` (si Banking actif)
5. **Vérifier** : Le menu Garage affiche le véhicule garé dans la liste

### Park-out (récupérer)
6. **Action** : Joueur revient au NPC Garagiste → sélectionner le véhicule → "Récupérer"
7. **Vérifier** : Le véhicule spawn à la position de récupération avec l'orientation correcte (roll trick)
8. **Vérifier** : Health, fuel, cargo sont identiques à l'état du park-in
9. **Vérifier** : Le véhicule est utilisable (pas bloqué en physique)

### Edge cases
10. **Vérifier** : Récupération impossible si position bloquée (log d'erreur, pas de crash)
11. **Vérifier** : Plusieurs véhicules garés simultanément (liste correcte, pas de confusion d'état)

## Done

- [ ] Park-in fonctionnel (snapshot + disparition véhicule)
- [ ] Park-out fonctionnel (restore fidèle + orientation)
- [ ] Débit `GARAGE_FEE_OUT` dans ledger Banking
- [ ] Liste véhicules garés correcte dans l'UI
- [ ] Edge cases vérifiés (position bloquée)
- [ ] Dmitri validé in-game

## Notes

L'UI Garage est déjà reskinée Field Ledger (DZML pipeline vert).  
L'orientation utilise le roll trick — voir `DECISIONS.md D-004` (non, c'est dans backlog fix) et `GLOSSARY.md`.  
Voir `docs/AI_Handover.md §2.6` pour l'état de TraderXGarage.
