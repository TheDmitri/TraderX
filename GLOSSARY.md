# GLOSSARY — TraderX

Vocabulaire partagé humain + IA. Utiliser les termes exacts de ce fichier dans le code, les commits, les stories et les sessions IA.

---

## Moteur DayZ

- **Enforce Script** — Langage de script de DayZ (syntaxe C-like). Pas de ternaire `?:`, pas de multi-ligne de signature, pas d'inline comment après `;`.
- **PBO** — Pack Binary Object. Conteneur de mod DayZ. Un package = un PBO. (`TraderX.pbo`, `TraderXBanking.pbo`, etc.)
- **NPC** — Non-Player Character. Entité en jeu incarnant un Trader, un Banker ou un Garagiste.
- **Action** — Interaction du menu contextuel (clic droit sur l'NPC) permettant d'ouvrir un menu.
- **RPC** — Remote Procedure Call. Communication client ↔ serveur via `SendRPC` / handler `OnRPC`.
- **ScriptInvoker** — Pattern d'event bus en Enforce. `static ref ScriptInvoker Event_X = new ScriptInvoker()`. Listeners insérés/retirés dans `OnInit`/`OnDestroy`.
- **3_Game** — Couche domaine (models, value objects, repositories, services domaine).
- **4_World** — Couche application (services app, pages UI, prefabs GUI, extensions entités).
- **5_Mission** — Couche mission (démarrage serveur, enregistrement des services DayZular).

---

## Framework DayZular

- **DayZular** — Framework MVC + service-registry pour DayZ. Remplace DabsFramework pour la logique UI et les services.
- **DabsFramework** — Ancienne couche MVC bas niveau. Toujours présente comme dépendance, mais n'est plus le pattern de référence pour les nouveaux composants.
- **DZlrComponentBase** — Classe de base de tout composant DayZular. Hooks : `OnInit`, `OnDestroy`, `OnChanges`.
- **ComponentAutoRegister** — Instance statique qui enregistre automatiquement un composant via son selector kebab-case.
- **AppModule** — Point d'entrée bootstrap DayZular. `AppModule.Bootstrap()` initialise le bus d'events, le logger et les pipes.
- **DZlrServiceRegistry** — Locator de services singleton. Tous les services sont enregistrés dans `5_Mission/TraderXAppInit.c`.
- **MarkForCheck()** — Déclenche le re-render d'un composant après mutation d'état.
- **TextBinding** — Binding déclaratif de texte (`Binding_Expression "{{ prop }}"` dans le layout). Jamais `.SetText()`.
- **ButtonBinding** — Binding déclaratif de bouton (`OnClick` handler). Jamais override manuel de `OnClick`.
- **ComponentWidgetBinding + Component_ForEach** — Binding déclaratif de liste/grille. Jamais `AddChild()` manuel.
- **NavButtonBinding** — Navigation entre pages/tabs (`TargetSelector`). Jamais switching de page manuel.
- **EditBoxBinding** — Binding déclaratif d'un champ de saisie.
- **DZTernary\<T\>** — Helper ternaire pour Enforce. Utiliser `DZTernaryString.If(cond, a, b)`, `DZTernaryInt.If(...)`, `DZTernaryFloat.If(...)`, etc. Le `?:` est interdit.

---

## Toolchain DZML

- **DZML** — DayZ Markup Language. Compilateur HTML → `.layout` DayZ runtime, développé dans `c:\DayZWorkspace\dayz-tools\`.
- **.layout** — Fichier de layout runtime DayZ (format XML propriétaire). Généré par DZML depuis les `.html`.
- **dzml.config.yaml** — Configuration du pipeline DZML pour un module.
- **dzml-runtime.js** — Runtime browser pour preview HTML. À copier manuellement dans chaque dossier HTML, pas généré par le pipeline.
- **PAGE_MAP** — Table de navigation manuelle dans `dzml-runtime.js`. À mettre à jour manuellement quand une nouvelle page est ajoutée.
- **Pipeline** — Commande `dzml pipeline` : compile tous les `.html` → `.layout` + génère CSS + atlas.
- **Atlas** — Fichier `.edds`/`.png`/`.svg` groupant les assets graphiques. Généré par le pipeline.
- **clean-build** — Script `scripts/clean-build-styles.sh` : purge le cache DZML + rebuild Banking + Garage (pas encore trader-v3). À utiliser après renommage/suppression de styles.

---

## Modules TraderX

- **TX** — Module principal TraderX (système de trading).
- **TXB** — Module TraderXBanking (comptes, virements, ledger, clans, cartes).
- **TXG** — Module TraderXGarage (marketplace véhicules, parking).
- **TXCL** — Module TraderXCarLock (verrouillage/déverrouillage de véhicules).
- **v3 / tx3-\*** — Nouvelle implémentation UI DZML du Trader. Sélectors `tx3-*` (pas `v3-*` — le préfixe `v3-` est interprété comme un chemin par le runtime).
- **src_wip/** — Dossier de travail en cours. Les scripts dedans ne sont pas packagés dans les PBOs actifs. Remplace `src/` quand validé.

---

## Domaine métier TraderX

- **Stock** — Quantité disponible d'un produit chez un Trader.
- **Stock behavior** — Comportement du stock au redémarrage du serveur : `0` = conserver, `1` = réinitialiser au max, `2` = aléatoire.
- **Destock coefficient** — Coefficient (0-1) appliqué au stock max pour réduire la quantité disponible initiale.
- **Transaction** — Opération d'achat ou de vente entre un joueur et un Trader NPC.
- **Transaction lock** — Verrou centralisé dans `TraderXTradingService` empêchant les transactions en double dues à des clics rapides.
- **Preset** — Configuration sauvegardée d'un item avec ses attachments (arme + accessoires).
- **Attachment** — Item attaché à un autre (accessoire d'arme, etc.).
- **Variant** — Version alternative d'un item (couleur, taille, etc.).
- **Loadout** — Pack d'équipement pré-configuré vendu en une seule transaction.
- **License** — Prérequis spécifique à un Trader pour accéder à certains produits.
- **Dynamic pricing** — Prix calculé dynamiquement selon le stock (coefficient).
- **Checkout** — Panier d'achat (liste d'items sélectionnés avant confirmation).
- **Category** — Groupe de produits dans l'interface du Trader.

---

## Domaine métier TraderXBanking

- **Account** — Compte bancaire d'un joueur. Solde persistant côté serveur.
- **Vault** — Stockage sécurisé d'items via le Banking (non implémenté en v1).
- **Ledger** — Grand livre des transactions : historique complet des mouvements de compte.
- **LedgerEntry** — Entrée individuelle dans le ledger (montant, type, label, horodatage).
- **Transfer** — Virement de compte à compte entre joueurs.
- **Contact** — Destinataire sauvegardé dans le carnet d'adresses, auto-ajouté après un premier virement.
- **Memo / Note** — Texte libre associé à un virement (`TXBTransaction.note`).
- **Clan** — Groupe de joueurs partageant des ressources bancaires (compte commun, prévu P2).
- **ITraderXPaymentProvider** — Interface cross-module exposant `DeductLabeled`/`CreditLabeled` pour que Garage et d'autres modules utilisent Banking comme provider de paiement.
- **GARAGE_FEE_OUT** — Label de paiement utilisé par Garage dans le ledger Banking.

---

## Design System

- **Field Ledger** — Langage de design TraderX : esthétique tactique sombre, typographie condensée, palette ambre/olive/rouille. Référence : `docs/TraderX_Design_System.md`.
- **Token TX\*** — Variable CSS custom property du design system (ex : `--tx-accent`, `--tx-positive`, `--tx-destructive`).
- **accent-amber** — Couleur d'accentuation principale (#F59E0B). Pour les actions primaires.
- **olive-positive** — Couleur de succès/validation (vert olive). Pour les états positifs.
- **rust-destructive** — Couleur d'action destructive (rouille). Pour les suppressions, alertes critiques.
- **blueish-steel** — Couleur informationnelle (bleu acier). Pour les états neutres/info.
- **Mockup** — Source de vérité visuelle HTML/CSS dans `src/TraderX/datasets/gui/mockup/`. Rendu direct dans navigateur.

---

## Termes à bannir

Ces termes sont interdits — utiliser les équivalents ci-dessus.

| Terme banni | Utiliser à la place |
|---|---|
| `v2-` (préfixe selector) | `tx3-*` pour les nouveaux composants v3 |
| `wallet` | `account balance` ou `player money` |
| `seller` | `NPC` ou `Trader` |
| `store` | `Trader` (NPC) ou `stock` (quantité) |
| `shop` | `Trader` |
| `owner` (d'un véhicule garé) | `player` |
| `SetText()` | `TextBinding` + `MarkForCheck()` |
| `AddChild()` | `ComponentWidgetBinding` + `Component_ForEach` |
| `FindAnyWidget()` | Binding DayZular déclaratif correspondant |
| `?:` (ternaire) | `DZTernary<T>.If(cond, a, b)` |
