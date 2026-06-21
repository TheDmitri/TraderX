# US-019 — Notification rework : toast HUD (port CourseBarterTrader)

**Périmètre :** TraderX (core)
**Priorité :** P2

**Given** un joueur en jeu qui réalise une transaction (achat / vente, simple ou bulk)
**When** la réponse serveur `ETraderXResponse.TRANSACTIONS` arrive
**Then** une carte toast unique glisse depuis la droite du HUD, themée Field-Ledger (accent or/bronze), tient 5 s en drainant sa barre de progression, puis ressort ; les notifications suivantes s'enfilent en FIFO avec un pill "+N QUEUED"

---

## Contexte

Remplace l'ancien système de notifications multi-cartes (stack MVVM `NotificationContainer` / `NotificationCard` + boucle `CallLater` 25 ms — anti-pattern) par un portage du toast de **CourseBarterTrader** (`BarterNotificationService` + `BarterToastCardComponent`).

Décisions (session 2026-06-17) :
- **Palette** : accents CBT « gold default » (or #C4A35A défaut, vert achat, ambre partiel, rouge erreur).
- **Ancien stack** : gardé **dormant** (non référencé) jusqu'à validation in-game, puis supprimé dans un commit cleanup.
- **Service** : nouveau service `TraderXToastService` extends `DayZularService`, enregistré dans le registry (pas de singleton `GetInstance`).

## Fichiers créés

| Fichier | Rôle |
|---|---|
| `scripts/3_Game/Constant/TraderXToastConstants.c` | Clés `TXNOTIF_*` + timings `TX_TOAST_*_MS` |
| `scripts/3_Game/Domain/Notification/Model/TraderXToastNotification.c` | Modèle (port `BarterNotification`) : type → icône / accent / kicker |
| `scripts/4_World/classes/TraderX/Services/Notification/TraderXToastService.c` | Runner FIFO client (state machine ENTER/HOLD/EXIT/GAP), `DayZularService` |
| `scripts/4_World/classes/TraderX/GUI/Toast/TraderXToastCardComponent.c` | Carte HUD `DZlrComponentBase` (port `BarterToastCardComponent`) |
| `datasets/gui/Notification/TraderXToastCard.html` (+ `.layout` compilé) | Carte 360×152, top-right, styles `TXCardElevated`/`TXSurface` |
| `datasets/icons/svg/{purchase,sale,info,error,warning}.svg` | Glyphs (4 portés de CBT + warning maison), atlas `traderx_icons` rebuild |

## Fichiers modifiés

| Fichier | Changement |
|---|---|
| `scripts/5_Mission/TraderXAppInit.c` | `registry.RegisterService(TraderXToastService, ...)` |
| `scripts/4_World/.../Transaction/TraderXTransactionNotifier.c` | `ShowTransactionNotification` route vers `GetTraderXToastService().ShowToast()` + `ResolveToastType()` (purchase/sale/info/warning/error) ; suppression de `GetNotificationDetails` |
| `scripts/4_World/.../Module/TraderXModule.c` | Suppression init de l'ancien `TraderXNotificationService` (dormant) |

## Tests

- **Acceptance (source de vérité)** : Manuel in-game — étapes ci-dessous
- **Pipeline** : `dzml check` ✓ + `dzml compile` ✓ (fait), atlas `build-icons.sh` ✓ (fait)

## Étapes de validation (in-game)

1. **Build** : `.\Build.ps1` → lancer le serveur + client
2. **Action** : Acheter un item → toast vert « TRADER · PURCHASED » glisse en haut-droite
3. **Action** : Vendre un item → toast or « TRADER · SOLD »
4. **Action** : Transaction qui échoue → toast rouge « TRADER · FAILED »
5. **Action** : Bulk avec succès+échecs → toast ambre « TRADER · PARTIAL »
6. **Vérifier** : barre de progression draine sur ~5 s, carte ressort par la droite
7. **Vérifier** : transactions rapides enchaînées → pill « +N QUEUED », passage FIFO sans chevauchement
8. **Vérifier** : icône correcte par type, glyph teinté à la couleur d'accent
9. **Vérifier** : aucun crash, aucune carte fantôme de l'ancien stack

## Done

- [ ] Build PBO sans erreur de compilation Enforce
- [ ] Les 5 types de toast s'affichent correctement (couleur + icône + kicker)
- [ ] Animation enter/hold/exit + FIFO + pill OK
- [ ] Ancien stack confirmé inactif (aucune régression visuelle)
- [ ] Cleanup : suppression de l'ancien stack notif (commit séparé après validation)
- [ ] Dmitri validé in-game

## Notes

Suite (stories séparées) : US-020 reskin tokens (or #C4A35A + Oswald/JetBrains Mono), US-021 footer promo DayZ Beans Launcher, US-022 migration services core (Currency/Transaction/Selection) → registry.
