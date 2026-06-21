# TraderXBanking — Feature Matrix (2026-04-18)

Synthèse de l'état du module **TraderXBanking** après le câblage de la nouvelle UI 3-onglets. Document destiné aux contributeurs / handover.

---

## 1. Périmètre fonctionnel

TraderXBanking est un module bancaire indépendant greffé sur le stack TraderX. Il expose :

- **Un compte joueur persistant** (JSON par UID) avec `balance` et `maxBalance`.
- **Trois opérations atomiques** côté serveur : Dépôt, Retrait, Virement. Chaque opération passe par une file FIFO tickée à `QUEUE_INTERVAL` (1s).
- **Un grand livre (ledger) append-only** par joueur, rotation à 500 entrées, flush tampon (10 entrées OU 30s).
- **Un carnet de contacts** alimenté automatiquement par les virements sortants.
- **Un provider de paiement TraderX** (`TXBPaymentProvider`) — permet au Trader et au Garage de débiter/créditer via le compte bancaire du joueur et trace des entrées ledger cross-module (`TRADER_BUY`, `TRADER_SELL`, `GARAGE_FEE_IN`, `GARAGE_FEE_OUT`).
- **Une interaction monde** (`ActionTXBOpenBank`) sur les entités dont `NpcId == TXBConstants.BANK_ID (-2)`.

**Hors périmètre** (explicite) : coffres de clan, receipts papier, notifications push, système de prêt/intérêts, filtres d'export avancés.

---

## 2. Layouts et features par onglet

### 2.1 `BankingPageView.layout` — Shell

Rôle : hôte d'onglets. Ne porte aucune logique métier.

| Widget | Binding / Handler | État |
|---|---|---|
| `bankerName` | TextBinding `{{ bankerName }}` | ✅ |
| `bankerRole` | texte statique "BANKING · BANK TELLER" | ✅ |
| `tabAccount` / `tabTransfer` / `tabLedger` | NavButtonBinding → `SelectComponent(selector)` | ✅ |
| `tabLeave` (X) | ButtonBinding OnClick `OnClose` → `TXBUIService.CloseBankingMenu()` | ✅ |
| `subnavLeft` | TextBinding `{{ subnavContext }}` | ✅ statique |
| `onlinePill` | texte statique | ⚠ statique (jamais désactivé) |
| `feePill` | TextBinding `{{ feesText }}` | ✅ lu depuis settings |
| `tabContent` | ComponentWidgetBinding `{{ current_tab }}` | ✅ swap auto des tabs |

---

### 2.2 `AccountTabView.layout` — Onglet Account (`banking-account-tab`)

| Section | Widget | Binding / Handler | État |
|---|---|---|---|
| **Balance** | `bankAmountLabel` | TextBinding `{{ bankBalanceText }}` | ✅ |
| | `progressMaxLabel` | TextBinding `{{ maxBalanceText }}` | ✅ |
| | `progressFill` | `Widget.SetSize(ratio, 1)` via `UpdateProgressBar()` | ✅ |
| **Account ID** | `accountIdValue` | TextBinding `{{ playerIdText }}` | ✅ |
| | `copyIdBtn` | OnClick `OnCopyId` → `GetGame().CopyToClipboard(UID)` | ✅ |
| **Deposit** | `depositAmountInput` | EditBoxBinding two-way `{{ depositAmount }}`, OnChange `OnDepositAmountChanged` | ✅ |
| | `depositBtn` | OnClick `OnDeposit` → validation + `RequestDeposit` RPC | ✅ |
| | `depositHint` | TextBinding `{{ feesText }}` — preview "You will receive X (fee Y)" | ✅ dynamique |
| **Withdraw** | `withdrawAmountInput` | EditBoxBinding two-way `{{ withdrawAmount }}`, OnChange `OnWithdrawAmountChanged` | ✅ |
| | `withdrawBtn` | OnClick `OnWithdraw` → validation + `RequestWithdraw` RPC | ✅ |
| | `creditCardBody` | TextBinding `{{ statusText }}` | ✅ sert aussi de zone de feedback |
| **Stats 30j** | `statDepositsValue` | TextBinding `{{ statDepositsText }}` | ✅ calculé ledger client |
| | `statWithdrawsValue` | TextBinding `{{ statWithdrawalsText }}` | ✅ |
| | `statTransfersValue` | TextBinding `{{ statTransfersText }}` | ✅ |
| **Recent** | `recentTitle`, `recentEmpty` | statique | ⚠ jamais rempli — voir §4 |
| | `viewLedgerLink` | NavButtonBinding → `txb-ledger-tab` | ✅ |

Validations côté client (avant RPC) :
- `amount > 0`
- Deposit : `amount <= m_PlayerCash`, `m_BankBalance + afterFees <= m_MaxBalance`
- Withdraw : `amount <= m_BankBalance`
- Throttle 1s entre transactions

---

### 2.3 `TransferTabView.layout` — Onglet Transfer (`banking-transfer-tab`)

| Section | Widget | Binding / Handler | État |
|---|---|---|---|
| **Form** | `transferDestInput` | EditBoxBinding two-way `{{ destinationAccountId }}` | ✅ |
| | `transferAmountInput` | EditBoxBinding two-way `{{ transferAmount }}`, OnChange `OnTransferAmountChanged` | ✅ |
| | `currencyValue` | texte statique "Roubles" | ⚠ statique |
| | `transferNoteInput` | EditBoxBinding two-way `{{ transferNote }}` | ✅ |
| **Summary** | `summaryAmountValue` | TextBinding `{{ transferAmount }}` | ✅ |
| | `summaryFeeValue` | TextBinding `{{ transferFeesDisplay }}` | ✅ live |
| | `summaryReceivesValue` | TextBinding `{{ transferAmount }}` | ✅ (recipient reçoit montant plein) |
| | `summaryTotalValue` | TextBinding `{{ transferBalanceText }}` — total = amount + fee | ✅ live |
| **Actions** | `resetBtn` | OnClick `OnReset` → `ResetForm()` | ✅ |
| | `transferBtn` | OnClick `OnTransfer` → validation + `RequestTransfer` RPC | ✅ |
| **Feedback** | `finalAlert` | statique "TRANSFERS ARE FINAL" | ✅ |
| | `transferStatusPanel` | `Widget.Show()` + TextBinding `{{ transferStatusText }}` | ✅ |
| **Contacts** | `contactsTitle`, `contactsSubtitle`, `contactsEmpty` | statique | ⚠ empty state permanent — voir §4 |
| | `contactsSearchInput` | EditBoxBinding two-way `{{ contactsSearch }}` | ⚠ lié mais non consommé |
| **Clan** | `clanCard` | texte statique "Coming soon" | ⚠ statique (volontaire, hors scope) |

Validations côté client :
- `amount > 0`
- `destUID.Length() >= 16`
- `destUID != m_PlayerUID`
- `amount + fees <= m_BankBalance`
- Throttle 1s

---

### 2.4 `LedgerTabView.layout` — Onglet Ledger (`txb-ledger-tab`)

| Section | Widget | Binding / Handler | État |
|---|---|---|---|
| **Filtres** | `ledgerSearchInput` | EditBoxBinding two-way `{{ ledgerSearch }}` | ⚠ lié mais non consommé — voir §4 |
| | `filterAllBtn` | OnClick `OnFilterAll` | ✅ |
| | `filterDepositBtn` | OnClick `OnFilterDeposit` | ✅ |
| | `filterWithdrawBtn` | OnClick `OnFilterWithdraw` | ✅ |
| | `filterTransferBtn` | OnClick `OnFilterTransfer` — match `TRANSFER_IN`+`TRANSFER_OUT` | ✅ |
| | `filterTraderBtn` | OnClick `OnFilterTrader` — match `TRADER_BUY`+`TRADER_SELL` | ✅ |
| | `exportCsvBtn` | aucun handler | ❌ inerte — voir §4 |
| **Table** | `tableHeader` | statique (DATE · TYPE · AMOUNT · BALANCE) | ✅ |
| | `dayGroup` | statique "TODAY · 2026-04-18" | ⚠ jamais mis à jour |
| | `ledger_list` (ScrollWidget → GridSpacer) | ComponentWidgetBinding ForEach `{{ _visibleEntries }}` template `txb-ledger-row` item `_inputEntry` | ✅ |
| | `ledgerEmpty` | statique "No transactions yet" | ⚠ jamais masqué |
| **Row** (`LedgerRowView.layout`) | `rowClickBtn` | OnClick `OnRowClicked` → `Event_OnLedgerRowClicked(id)` | ✅ |
| | `rowDate`, `rowType`, `rowAmount`, `rowBalance` | TextBinding | ✅ |
| **Détail** | `detailAmount` | TextBinding `{{ selected_amount }}` | ✅ |
| | `detailSubtitle` | TextBinding `{{ selected_type }}` | ✅ |
| | `detailId`, `detailDate`, `detailType`, `detailParty`, `detailFee`, `detailBalance`, `detailNote` | TextBinding `{{ selected_* }}` | ✅ |
| | `confirmedAlert` | statique | ⚠ toujours visible |
| | `copyTxIdBtn`, `receiptBtn` | aucun handler | ❌ inertes — voir §4 |

Stats dérivées calculées en parallèle (`ComputeStats`) : `stat_deposits`, `stat_withdrawals`, `stat_transfers` (non-bindés sur cette vue — consommés par AccountTab via son propre calcul).

---

## 3. Flux principaux

**Ouverture :**
```
Player interact NPC (NpcId = -2)
  → ActionTXBOpenBank.ActionCondition() (+ check credit card si requireCreditCard)
  → TXBUIService.OpenBankingMenu()
  → new TXBBankingComponent().InitComponent(null)
  → RequestAccount RPC → server → OnAccountReceived → SetClientAccount → Event_OnAccountUpdated
  → ComponentWidgetBinding mount banking-account-tab (default)
```

**Dépôt / Retrait :**
```
User clicks → TXBAccountTabComponent.OnDeposit/OnWithdraw
  → Validation client → RequestDeposit/Withdraw RPC
  → Server enqueue → ProcessDeposit/Withdraw (next tick)
  → SendAccountToClient + RecordLedgerEntry
  → Client: Event_OnAccountUpdated + Event_OnLedgerUpdated
```

**Virement :** idem mais `RequestTransfer`, 2 entrées ledger créées (TRANSFER_OUT côté sender, TRANSFER_IN côté recipient), contact sender auto-enregistré.

**Fermeture :**
```
Click X → TXBBankingComponent.OnClose()  OU  ESC → MissionGameplay.OnKeyRelease
  → TXBUIService.CloseBankingMenu()
  → Component.Destroy() (cascade ChildComponents)
  → Restore player controls + hide cursor
```

**Cross-module paiement :**
```
Trader/Garage → TraderXProviderRegistry.GetBankingProvider()
  → TXBPaymentProvider.Deduct/Credit/DeductLabeled
  → TXBBankingService.RecordExternalLedger(TRADER_BUY | GARAGE_FEE_OUT | …)
```

---

## 4. Features non câblées (scope MVP, `styleguide: inerte sans crash`)

### 4.1 Ledger
- **`exportCsvBtn`** : bouton présent, aucun `ButtonBinding`. Implémentation prévue : sérialiser `_visibleEntries` en CSV sous `$profile:TraderXBanking/Exports/ledger_<UID>_<stamp>.csv`.
- **`copyTxIdBtn`** : aucun handler. Devra copier `selected_id` via `CopyToClipboard`.
- **`receiptBtn`** : aucun handler. Devra afficher une modale récapitulative ou pousser une notification TraderX.
- **`ledgerSearchInput`** : binding two-way OK mais aucun `OnChange` handler ni filtrage dans `ApplyFilter()`. Plug : ajouter un `OnSearchChanged()` qui filtre par `counterpartyName` / `note` / `amount.ToString()`.
- **Pills de filtre — état visuel actif** : pas de highlight de la pill actuellement sélectionnée. `active_filter` est maintenu mais pas reflété visuellement.
- **`dayGroup`** : label "TODAY" statique, jamais recalculé. Peut être supprimé ou bindé sur une date dynamique.
- **`ledgerEmpty`** : jamais masqué quand des entrées sont présentes. `Widget.Show(_visibleEntries.Count() == 0)` à ajouter dans `ApplyFilter()`.

### 4.2 Transfer
- **Liste de contacts (`contactsCard`)** : empty state permanent. Aucun ForEach template, aucune source de données client.
  - Backend requis : côté serveur `TXBContactData` existe déjà. Ajouter un RPC `RequestContacts` + `OnContactsReceived` sur le même modèle que `RequestLedger`, et un cache client dans `TXBBankingService`.
  - Front requis : `TXBContactRowComponent` + layout row, ForEach sur un `array<ref TXBContact>` dans `TXBTransferTabComponent`.
  - Click contact → pré-remplit `destinationAccountId`.
- **`contactsSearchInput`** : binding présent, pas de filtrage.
- **Clan Vault** : carte "Coming soon" — hors scope, garder tel quel.

### 4.3 Account
- **`recentCard`** ("RECENT TRANSACTIONS") : jamais rempli. Soit afficher un mini-ForEach des 5 dernières entrées ledger, soit supprimer la carte. Le bouton `viewLedgerLink` fonctionne déjà (nav vers ledger).

### 4.4 Shell / UX globale
- **`onlinePill`** : toujours "● ONLINE" — pas de ping serveur. Peut rester ainsi.
- **Premier render avant sync** : `TXBConfigService` instancie un `TXBSettings` par défaut (`transactionFeePercent = 0.05`), donc `feesText` affiche 5% tant que `OnSettingsReceived` n'est pas arrivé. Acceptable en pratique car la RPC arrive quasi instantanément post-spawn.

---

## 5. Dépendances module

| Fournisseur | Consommateur | Interface |
|---|---|---|
| TraderX (registry) | TraderXBanking | `TraderXProviderRegistry.RegisterBankingProvider(TXBPaymentProvider)` — s'enregistre à l'init |
| TraderXBanking | TraderX (trader UI) | `ITraderXPaymentProvider` → `Deduct/Credit/GetBalance/DeductLabeled` |
| TraderXBanking | TraderXGarage | Idem (fees garage) |
| DayZular | TraderXBanking | `DZlrComponentBase`, `DayZularService`, `DayZularRPCManager`, bindings (Text/EditBox/Button/Nav/ComponentWidget) |

---

## 6. Fichiers clés (références rapides)

**Layouts (runtime) :**
- [BankingPageView.layout](../src/TraderXBanking/datasets/layouts/BankingPageView.layout)
- [AccountTabView.layout](../src/TraderXBanking/datasets/layouts/AccountTabView.layout)
- [TransferTabView.layout](../src/TraderXBanking/datasets/layouts/TransferTabView.layout)
- [LedgerTabView.layout](../src/TraderXBanking/datasets/layouts/LedgerTabView.layout)
- [LedgerRowView.layout](../src/TraderXBanking/datasets/layouts/LedgerRowView.layout)

**Components :**
- [TXBBankingComponent.c](../src/TraderXBanking/scripts/4_World/Components/GUI/TXBBankingComponent.c)
- [TXBAccountTabComponent.c](../src/TraderXBanking/scripts/4_World/Components/GUI/TXBAccountTabComponent.c)
- [TXBTransferTabComponent.c](../src/TraderXBanking/scripts/4_World/Components/GUI/TXBTransferTabComponent.c)
- [TXBLedgerTabComponent.c](../src/TraderXBanking/scripts/4_World/Components/GUI/TXBLedgerTabComponent.c)
- [TXBLedgerRowComponent.c](../src/TraderXBanking/scripts/4_World/Components/GUI/TXBLedgerRowComponent.c)

**Services :**
- [TXBBankingService.c](../src/TraderXBanking/scripts/4_World/Services/TXBBankingService.c) — file FIFO, processing, ledger server-side
- [TXBRPCService.c](../src/TraderXBanking/scripts/4_World/Services/TXBRPCService.c) — 5 RPCs serveur + 3 RPCs client, rate-limits
- [TXBConfigService.c](../src/TraderXBanking/scripts/4_World/Services/TXBConfigService.c)
- [TXBUIService.c](../src/TraderXBanking/scripts/4_World/Services/TXBUIService.c)

**Modèles :**
- [TXBAccountData.c](../src/TraderXBanking/scripts/3_Game/Models/TXBAccountData.c)
- [TXBLedgerEntry.c](../src/TraderXBanking/scripts/3_Game/Models/TXBLedgerEntry.c) — timestamp YYYYMMDDhhmmss int
- [TXBLedgerEntryType.c](../src/TraderXBanking/scripts/3_Game/Models/TXBLedgerEntryType.c)
- [TXBContactData.c](../src/TraderXBanking/scripts/3_Game/Models/TXBContactData.c)
- [TXBTransaction.c](../src/TraderXBanking/scripts/3_Game/Models/TXBTransaction.c)

**Providers / Actions / Repository :**
- [TXBPaymentProvider.c](../src/TraderXBanking/scripts/4_World/Providers/TXBPaymentProvider.c)
- [ActionTXBOpenBank.c](../src/TraderXBanking/scripts/4_World/Actions/ActionTXBOpenBank.c)
- [TXBLedgerRepository.c](../src/TraderXBanking/scripts/3_Game/Repository/TXBLedgerRepository.c)

---

## 7. État `done / not done` — récap

| Feature | État |
|---|---|
| Ouvrir/fermer menu (action + ESC + X) | ✅ |
| Swap onglets Account / Transfer / Ledger | ✅ |
| Affichage balance + progress dynamique | ✅ |
| Copie Account ID | ✅ |
| Dépôt cash → bank | ✅ |
| Retrait bank → cash | ✅ |
| Preview fees live (deposit) | ✅ |
| Preview fees live (transfer) | ✅ |
| Virement entre comptes | ✅ |
| Reset form transfer | ✅ |
| Status panel transfer | ✅ |
| Throttle anti-spam (1s) | ✅ |
| Rate-limit RPC serveur (5/s, 2/s ledger) | ✅ |
| Ledger table (ForEach rows) | ✅ |
| Clic row → détail | ✅ |
| Filtres par type | ✅ |
| Stats 30j (Account) | ✅ |
| Provider paiement Trader + Garage | ✅ |
| Ledger cross-module (TRADER_*, GARAGE_*) | ✅ |
| Contacts : collecte serveur | ✅ |
| Contacts : affichage client | ❌ |
| Recherche ledger | ❌ |
| Export CSV | ❌ |
| Copy Tx ID / Receipt | ❌ |
| Recent transactions preview (Account) | ❌ |
| Clan Vault | ❌ (hors scope) |
