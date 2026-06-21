# US-022 — Migration services core → registry DayZular

**Périmètre :** TraderX (core)
**Priorité :** P2

**Given** les services core encore en singletons `GetInstance()` lazy
**When** on les migre vers le `DZlrServiceRegistry` DayZular
**Then** Currency / Transaction / Selection deviennent des `DayZularService` résolus via le registry, sans casser les ~93 sites d'appel existants

---

## Décision (session 2026-06-17)

Scope = **Notif (US-019, déjà fait) + 3 core** : Currency, Transaction, Selection. Le reste des ~19 services reste en singleton jusqu'à une future story.

**Pattern (low-risk, zéro réécriture de call-site)** : chaque service `extends DayZularService` et garde son `GetInstance()`, mais le corps résout désormais depuis le registry (avec création+enregistrement de secours), mis en cache dans le champ `m_instance` existant. Les 93 `X.GetInstance()` continuent de marcher. Un accessor moderne `GetTraderXXxxService()` est ajouté pour le nouveau code.

```c
static X GetInstance()
{
    if (!m_instance)
    {
        m_instance = X.Cast(DZlrServiceRegistry.GetInstance().GetService(X));
        if (!m_instance) { m_instance = new X(); DZlrServiceRegistry.GetInstance().RegisterService(X, m_instance); }
    }
    return m_instance;
}
```

### Ordre / sécurité

- **Transaction** (ctor = 2 queues) et **Selection** (ctor = 1 map, UI cart) : sûrs à construire au bootstrap → **enregistrés dans `TraderXAppInit`**.
- **Currency** : le ctor serveur charge `TraderXCurrencyRepository.Load()` qui **doit** tourner APRÈS `TraderXConfigurationService.Initialize()` (appelé dans `TraderXModule.InitializeModule`). Donc **PAS** enregistré au bootstrap — il lazy-registre à la 1re `GetInstance()` (dans `InitRPCs`, après l'init config). Ordre préservé, zéro régression.
- Précédent : CourseBarterTrader enregistre `BarterCurrencyService`/`BarterTransactionService` dans son registry et fonctionne côté serveur → le registry DayZular est dispo client ET serveur.

## Fichiers modifiés

| Fichier | Changement |
|---|---|
| `Services/Product/TraderXSelectionService.c` | `: DayZularService` + GetInstance registry-backed + ctor dé-privatisé + accessor |
| `Services/Transaction/TraderXTransactionService.c` | `: DayZularService` + GetInstance registry-backed + accessor |
| `Services/Currency/TraderXCurrencyService.c` | `: DayZularService` + GetInstance registry-backed (lazy) + accessor |
| `5_Mission/TraderXAppInit.c` | Enregistre Transaction + Selection (Currency lazy, commenté) |

## Tests

- **Statique** : `new` direct des 3 services uniquement dans GetInstance + AppInit (vérifié grep) ; pas de double-instanciation (le registry est la source unique).
- **Acceptance** : Manuel in-game — pas de compilateur Enforce hors-jeu.

## Étapes de validation (in-game)

1. **Build** : `.\Build.ps1` → serveur + client
2. **Vérifier serveur** : transaction achat/vente traitée correctement (Transaction + Currency OK, pas de null)
3. **Vérifier** : devise affichée (wallet) correcte — Currency chargé après config
4. **Vérifier client** : sélection items / checkout / customize (Selection OK)
5. **Vérifier** : aucun crash au démarrage (ordre d'init), logs sans null-service

## Done

- [ ] Build serveur+client sans erreur Enforce
- [ ] Achat/vente OK (Transaction+Currency)
- [ ] Sélection/checkout OK (Selection)
- [ ] Pas de régression d'ordre d'init / crash
- [ ] Dmitri validé in-game

## Reste

- ~19 services restants en singleton (future story de migration progressive).
