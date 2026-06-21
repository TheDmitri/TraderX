# TraderX Design System — "Field Ledger"

Design language partagé par **TraderX**, **TraderXBanking** et **TraderXGarage**.

Toute nouvelle interface de ces trois modules doit s'y conformer. Aucune couleur, police ou padding n'est décidé au cas par cas : tout passe par les tokens définis ici.

---

## 1. Philosophy

### 1.1 Mood

> *Le registre du quartier-maître dans une caserne abandonnée de Tchernarus. Pages d'un carnet de comptes militaire, éclairées par une lampe tempête. Pas de design web, pas de glassmorphism, pas de "dashboard" — une interface d'inventaire tactique.*

Deux références visuelles fusionnées :
1. **HUD tactique DayZ** — menu inventaire officiel, marketplace natif : palette noire, accents ambre, icônes monochromes, hiérarchie par typographie plutôt que par couleur.
2. **Journal de terrain / registre soviétique** — carnets de stock, fiches de dotation, bordereaux. Densité d'information, alignements stricts, absence de décoration gratuite.

### 1.2 Trois principes directeurs

1. **Pas de couleur gratuite.** Chaque teinte porte une sémantique stricte (stock / action / danger / info). Le reste du monde est gris. Si une couleur n'appartient pas à la palette, elle n'entre pas dans l'interface.
2. **Densité d'information > respiration aérée.** DayZ récompense le regard qui scanne vite. On privilégie les lignes de tableau serrées, les marges de 8-12 px, les ellipses plutôt que les retours à la ligne. Une UX "web app" avec ses `padding: 32px` et ses cartes respirantes est hors-sujet.
3. **Pas de skeuomorphisme lourd.** Pas de textures canvas, pas de papier vieilli, pas de tampons rouges. L'atmosphère naît de la palette, de la typographie et du contraste — jamais des bitmaps décoratifs.

### 1.3 Ce qu'on évite

| Anti-pattern | Pourquoi |
|---|---|
| Rouge vif `#ff0000` ou `#c9302c` en accent principal | Générique "Bootstrap web app" — rien de DayZ. |
| Vert néon type notification Discord | Brise l'atmosphère post-apo, évoque un SaaS. |
| Coins arrondis `radius > 3` | Les équipements militaires ont des angles droits. Les cartes plastifiées ont 2 px de radius maximum. |
| Gradients verticaux forts | Sauf pour simuler un léger éclairage du dessus sur les surfaces (déjà présent, discret). |
| Ombres portées (`box-shadow`) | Inutilisables en DayZ layout, et cassent le style plat. |
| Blanc pur `#ffffff` en texte | Trop brillant. On utilise l'ivoire `#d8d2c0`. |
| Icônes colorées ou multi-teintes | Toutes les icônes sont monochromes (ivoire ou ambre). |

---

## 2. Color Tokens

### 2.1 Palette de base (surfaces et bordures)

Teintes chargées en bleu-gris très froid — elles évitent le "pur noir" qui donne un aspect OLED/app mobile, et posent une atmosphère de métal oxydé.

| Token | Hex | DayZ float `r g b a` | Usage |
|---|---|---|---|
| `--bg-base` | `#08090c` | `0.031 0.035 0.047 1` | Fond de page, derrière tout |
| `--bg-surface` | `#0c0e14` | `0.047 0.055 0.078 1` | Header, sidebar, row de liste |
| `--bg-card` | `#10131b` | `0.063 0.075 0.106 1` | Cartes produit, panels |
| `--bg-elevated` | `#161a24` | `0.086 0.102 0.141 1` | Carte sélectionnée, modal, hover fort |
| `--bg-hover` | `#1a1e28` | `0.102 0.118 0.157 1` | État hover générique |
| `--bg-pressed` | `#06070a` | `0.024 0.027 0.039 1` | État actif/pushed |
| `--border-soft` | `#1a1e28` | `0.102 0.118 0.157 1` | Séparateurs discrets, card border |
| `--border-default` | `#252a36` | `0.145 0.165 0.212 1` | Bordure standard input/bouton |
| `--border-strong` | `#3a4050` | `0.227 0.251 0.314 1` | Bordure focus, emphase |

### 2.2 Couleurs sémantiques

**Une couleur = un sens.** Toute utilisation hors-contexte est un bug de design.

| Token | Hex | Float | Sémantique — **et uniquement ça** |
|---|---|---|---|
| `--accent-amber` | `#d4a44c` | `0.831 0.643 0.298 1` | **Stock, prix, sélection, accent primaire**. L'ambre est la "valeur" au sens large : ce qui a une quantité, un coût, ou ce que l'utilisateur a choisi. |
| `--accent-amber-dim` | `#8a6a30` | `0.541 0.416 0.188 1` | Ambre pressé/désactivé |
| `--action-olive` | `#7a9a4a` | `0.478 0.604 0.290 1` | **Action positive d'engagement** : Buy, Deposit, Park-In, Confirm. Vert olive militaire, pas vert pomme. |
| `--action-olive-deep` | `#4a6a2a` | `0.290 0.416 0.165 1` | Olive pressé |
| `--action-rust` | `#b0482a` | `0.690 0.282 0.165 1` | **Action destructive/inverse** : Sell, Withdraw, Park-Out, Remove from cart. Rouille oxydée, pas rouge signalisation. |
| `--action-rust-deep` | `#7a2a18` | `0.478 0.165 0.094 1` | Rouille pressé |
| `--info-steel` | `#4a7ac7` | `0.290 0.478 0.780 1` | **Information / transfert** : bloc info, transfert bancaire, solde entrant. Bleu acier froid. |
| `--warn-brass` | `#d4a44c` | idem ambre | **Avertissement** : stock faible, prix élevé, attention. Même teinte que l'ambre — c'est cohérent, un warning c'est une valeur qui demande attention. |
| `--danger-blood` | `#8b0000` | `0.545 0 0 1` | **Fermeture / quitter** — bouton Leave/Close uniquement. Pas pour les actions trader. |
| `--status-error` | `#c93030` | `0.788 0.188 0.188 1` | **Erreur système / transaction refusée**. Usage strictement limité aux messages d'erreur. |

### 2.3 Texte

Jamais de blanc pur. Jamais de gris trop sombre qui disparaît.

| Token | Hex | Float | Usage |
|---|---|---|---|
| `--text-ivory` | `#d8d2c0` | `0.847 0.824 0.753 1` | Texte principal — ivoire usé |
| `--text-ash` | `#9a9488` | `0.604 0.580 0.533 1` | Texte secondaire — labels, métadonnées |
| `--text-dust` | `#6a6560` | `0.416 0.396 0.376 1` | Texte tertiaire — placeholder, désactivé, hint |
| `--text-amber` | `#d4a44c` | idem ambre | Texte valorisé — prix, quantité, nom du trader sélectionné |
| `--text-olive` | `#9ab05a` | `0.604 0.690 0.353 1` | Texte de confirmation (solde positif, transaction réussie) |
| `--text-rust` | `#c8664a` | `0.784 0.400 0.290 1` | Texte d'alerte (solde négatif, stock à 0) |

### 2.4 Règle d'or sur l'ambre

L'ambre `#d4a44c` est l'accent qui porte l'identité. Il apparaît **partout** où l'utilisateur doit savoir "c'est ici que ça se passe" :
- Onglet actif (bordure inférieure 3 px)
- Ligne sélectionnée (bordure gauche 3 px)
- Prix affichés
- Chiffres de stock
- Nom du trader en cours
- Barre de progression
- Input en focus (bordure)

Cette cohérence est **non négociable**. L'utilisateur apprend en 5 secondes que "ambre = ce qui compte ici".

---

## 3. Typography

### 3.1 Fonts embarquées

TraderX utilise **Roboto Condensed Variable Font**, déjà packagé dans `datasets/fonts/`. Aucune autre police.

Raison : condensée → densité d'information ; variable → toutes les graisses à partir d'un seul fichier ; géométrique mais pas froide → raccord militaire/inventaire.

### 3.2 Échelle typographique

| Token | Taille | Graisse | Font DayZ | Usage |
|---|---|---|---|---|
| `--font-display` | 28 px | 600 | `RobotoCondensedVF28` | Nom du trader, titre de modal |
| `--font-heading` | 20 px | 600 | `RobotoCondensedVF20` | Titre de section, "SHOPPING CART" |
| `--font-subheading` | 16 px | 500 | `RobotoCondensedVF16` | Titre de carte, total |
| `--font-body` | 14 px | 400 | `RobotoCondensedVF14` | Texte courant, libellés boutons, onglets |
| `--font-small` | 12 px | 400 | `RobotoCondensedVF12` | Rôle du trader, métadonnée, hint |
| `--font-tiny` | 10 px | 500 | `RobotoCondensedVF10` | Badge, tag, notification inline |

### 3.3 Règles typographiques

1. **Les libellés de boutons et d'onglets sont en UPPERCASE** avec `letter-spacing` léger (+1 à +2 px visuellement). Raccord stencil militaire.
2. **Les valeurs numériques (prix, stocks, soldes) sont `font-weight: 500`** pour ressortir sans changer de taille.
3. **Jamais d'italique.** Incompatible avec le registre.
4. **Jamais de souligné** sauf indicateur d'onglet actif (qui n'est en réalité pas un underline mais une `border-bottom`).
5. **Alignement numérique** : les stocks et les prix dans les colonnes sont alignés à droite.
6. **Ellipsis** (`…`) plutôt que wrap pour les noms longs. On rogne, on ne retourne pas à la ligne.

---

## 4. Spacing & Grid

### 4.1 Échelle d'espacement

Base de **4 px**. Tout espacement est un multiple de 4. Pas d'exception.

| Token | Valeur | Usage |
|---|---|---|
| `--sp-1` | 4 px | Gap minimal (icône/texte) |
| `--sp-2` | 8 px | Padding interne compact (row de liste) |
| `--sp-3` | 12 px | Padding standard de carte |
| `--sp-4` | 16 px | Séparation entre sections |
| `--sp-5` | 24 px | Marge de bloc majeur |
| `--sp-6` | 32 px | Séparation entre zones principales (sidebar ↔ content) |

### 4.2 Densité

Les rows de liste (inventaire, banking ledger, garage vehicles) doivent tenir **au moins 20 lignes visibles** sur un écran 1920×1080 sans scroll. Hauteur de row = 36 à 44 px max.

### 4.3 Grille globale d'une page

Toute page d'un des trois modules suit ce canevas :

```
┌────────────────────────────────────────────────────────────┐
│ HEADER (12% h)  — trader info • navigation onglets • close │
├────────────────┬───────────────────────────┬───────────────┤
│                │                           │               │
│   SIDEBAR      │      MAIN CONTENT         │   ASIDE       │
│   (15% w)      │      (variable)           │   (22% w)     │
│   catégories   │      liste / détail       │   cart/solde  │
│                │                           │               │
└────────────────┴───────────────────────────┴───────────────┘
```

- **Header** : identique cross-modules. Trader/Banquier/Garagiste + tabs + close.
- **Sidebar** : optionnelle. Catégories pour Trader, comptes pour Banking, NPCs pour Garage.
- **Main** : la liste + détail (variable selon le module).
- **Aside** : contextuel. Shopping cart pour Trader, détail de compte pour Banking, détail véhicule pour Garage.

---

## 5. Iconography & Imagery

### 5.1 Icônes

- **Style** : monochrome, une seule teinte par icône.
- **Couleur** : `--text-ivory` par défaut, `--accent-amber` quand l'icône accompagne une valeur mise en avant (prix, stock), `--text-ash` quand désactivée.
- **Source** : atlas EDDS dans `datasets/icons/` — pas d'import dynamique d'images couleur.
- **Taille** : 16, 20, 24, 32, 42 px. Pas d'entre-deux.

### 5.2 Product previews (ItemPreviewWidget)

- Fond du slot : `--bg-card`, bordure `--border-soft`.
- Au hover : bordure passe à `--accent-amber`.
- Sélectionné : bordure `--accent-amber` 2 px + fond `--bg-elevated`.
- **Pas** d'effet de glow, pas de saturation ajoutée.

### 5.3 Backgrounds

- `bg-preview.jpg` (arrière-plan flouté de Tchernarus) reste le fond de page global.
- Les panneaux posent leurs propres `--bg-*` opaques par-dessus. Pas de transparence partielle qui laisserait voir le paysage à travers les cartes.
- Seule exception : le conteneur racine `MainView` est à ~80% d'opacité pour laisser respirer le fond.

---

## 6. Component Patterns

Chaque pattern a un token TX (`data-style="TX…"`) défini dans `TraderXMainView.html` sous `<style type="dz-styles">`. Ne **jamais** écrire de couleurs inline dans un layout — toujours passer par un token.

### 6.1 Containers

| Token | Rôle |
|---|---|
| `TXContainer` | Conteneur principal d'une page |
| `TXHeader` | Barre de header (bordure inférieure ambre très discrète via `TXDivider`) |
| `TXSidebar` | Colonne latérale (bordure droite) |
| `TXCard` | Carte standard — produit, ligne de ledger, véhicule |
| `TXCardElevated` | Carte sélectionnée ou modal |
| `TXSurface` | Zone grise élevée (cart, detail panel) |
| `TXDivider` | Séparateur horizontal ou vertical |
| `TXPreviewPanel` | Zone d'aperçu produit/véhicule |

### 6.2 Boutons

Quatre variantes. **Zéro** création ad hoc.

| Token | Sémantique | Exemple |
|---|---|---|
| `TXBtnPrimary` | Action positive olive | BUY, DEPOSIT, PARK IN, CONFIRM |
| `TXBtnAccent` | Action valorisée ambre | Ajouter aux favoris, sélectionner preset |
| `TXBtnDanger` | Action destructive rouille | SELL, WITHDRAW, PARK OUT, CLEAR CART |
| `TXBtnDefault` | Action neutre | CANCEL, BACK, RESET |
| `TXBtnLeave` | Fermeture / quitter (sang) | Croix rouge en haut à droite |

**Règles** :
- Hauteur standard : 40 px. Compact : 32 px. Large : 48 px (action finale type "PURCHASE").
- Libellé UPPERCASE, Roboto Condensed 14 px, letter-spacing léger.
- Pas d'icône sans texte sauf close, ±, flèche suivant.

### 6.3 Onglets

- `TXTab` : état inactif.
- `.dz-active` / `:focus` : bordure inférieure `--accent-amber` 3 px.
- Hauteur 50 px, padding horizontal 24 px.
- Libellé UPPERCASE.

### 6.4 Listes et catégories

| Token | Usage |
|---|---|
| `TXCategoryItem` | Item de sidebar (catégorie trader, compte banking, NPC garage) |
| `TXCategoryActive` | Item sélectionné — bordure gauche ambre 3 px |
| `TXListRow` | Row de liste (produit, transaction) |
| `TXListRowAlt` | Row alternée (effet zébré discret) |
| `TXListRowSelected` | Row sélectionnée — bordure gauche ambre 3 px |

Les rows alternent `TXListRow` / `TXListRowAlt` pour faciliter la lecture horizontale sans bordures séparatrices.

### 6.5 Inputs

- `TXInputField` : éditbox. Fond plus clair que la carte parente (`--bg-hover`), bordure `--border-default`, bordure focus `--accent-amber`.
- Hauteur standard : 36 px. Padding horizontal : 12 px.
- Placeholder : `--text-dust`.
- Pas de label flottant style Material. Le label est au-dessus, en `--text-ash` 12 px.

### 6.6 Progress & Status

| Token | Usage |
|---|---|
| `TXProgressBg` / `TXProgressFill` | Barre de progression — stock, solde max, carburant |
| `TXBadge` | Pastille numérique (qté cart, notifications) |
| `TXInfoBox` | Bloc d'information (bordure gauche bleu acier) |
| `TXSuccessBox` | Bloc de succès (bordure gauche olive) |
| `TXWarningBox` | Bloc d'avertissement (bordure gauche ambre) |
| `TXErrorBox` | Bloc d'erreur (bordure gauche `--status-error`) |

### 6.7 Slots & Selection

- `TXSlot` : emplacement de produit dans une grille (favoris, attachements).
- Hover : bordure inférieure ambre 3 px.
- Sélectionné : bordure complète ambre 2 px.

---

## 7. Module-Specific Guidance

**Règle d'architecture fondamentale** : les trois modules sont **indépendants**. Chacun s'ouvre par son propre NPC, possède son propre shell, son propre header et son propre ensemble d'onglets. **Il n'existe pas de navigation croisée** entre Trader, Banking et Garage — fermer l'un ne fait pas passer à l'autre. Une seule et même couleur d'accent (ambre) et un seul design system les unissent visuellement.

### 7.1 TraderX (marchand)

- **Header** : icône `market.edds` + nom du trader en `--text-amber` 28 px + rôle "Trader · [spécialité]" en `--text-ash` 12 px.
- **Onglets de header** (niveau 1 de navigation) : FAVORITES / PURCHASE / SELL / CATALOG. `TXTab`.
- **Hiérarchie de produits à 2 niveaux** sur la page PURCHASE :
  1. **Catégorie** (sidebar gauche) : Weapons, Ammunition, Attachments, Clothing, etc. Utilise `TXCategoryItem` / `TXCategoryActive`.
  2. **Produit** (liste ou grille dans le main) — rendu direct sous la catégorie sélectionnée. Pas de sous-catégorie : les catégories sont plates par design (`TraderXCategory` n'a pas de parent).
- **Filter pills** (horizontal, en haut du main) : raccourcis de filtrage transversal — `All` / `In stock` / `Customizable` / `Favorites` + `Sort`. Ne remplacent pas les catégories, elles filtrent les produits de la catégorie active.
- **Indicateur "customizable"** : toute carte ou ligne de produit qui possède des attachments, variants ou presets **doit** afficher l'icône `⚙` ambre. Position : coin inférieur-droit de la carte (au-dessus de la zone body) ou colonne dédiée dans la liste. Cliquer → page CUSTOMIZE dédiée.
- **Page CUSTOMIZE** (atteignable uniquement depuis un produit customizable) :
  - Breadcrumb de retour : `Catalog › Weapons › Assault rifles › KA-74 · Customize`.
  - Preview live du build en cours.
  - Barre de presets (Stock / Preset 1 / Preset 2 / ...) + bouton "Save as preset".
  - Onglets internes : VARIANTS / ATTACHMENTS / MAGAZINES / AMMO.
  - Grille d'options cliquables (toggle). Les incompatibles ne sont pas affichés.
  - Panneau "Your build" à droite : ligne par élément, prix par ligne, total, "Add to cart" / "Buy now".
- **Prix** : toujours en `--text-amber`, alignés à droite, format `{{ price | number }}`.
- **Stock** : format `X/Y` en `--text-amber` (X) et `--text-ash` (/Y). Si X = 0 → `--text-rust`.
- **Aside droit** (Favorites, Purchase) : cart sous `TXSurface`. Total en `--font-heading` ambre. Bouton final "PURCHASE" en `TXBtnPrimary` 48 px large.
- **Distinction Purchase vs Catalog** — les deux vues partagent les mêmes données mais servent deux intentions différentes, et ne doivent jamais se dupliquer :
  - **Purchase** = vue **ciblée d'achat**. Sidebar catégories à gauche, sous-catégories en pills, panier à droite. Tu entres pour acheter quelque chose de précis.
  - **Catalog** = vue **aperçu/browse**. Pas de sidebar, **pleine largeur**, toutes les catégories empilées en sections. Barre "Jump to" horizontale en haut pour sauter d'une section à l'autre via ancres. Pas de panier — chaque section a un lien "Open in Purchase →" qui redirige vers Purchase filtré. Tu entres pour te balader et voir ce qui est disponible.
  - Corollaire : ne jamais mettre une sidebar de catégories dans le Catalog (redondant avec les sections). Ne jamais empiler toutes les catégories d'un coup dans Purchase (casse le flow ciblé).

### 7.2 TraderXBanking (module indépendant)

- **Header** : NPC banquier, nom en ambre, rôle "Banking · [spécialité]".
- **Onglets de header** : ACCOUNT / TRANSFER / LEDGER. Aucun retour vers Trader ni Garage.
- **Zone ACCOUNT** (page par défaut) :
  - Bloc solde massif centré : `--font-display` 48-56 px en `--text-amber`, libellé "CURRENT BALANCE" au-dessus en `--text-ash` `--font-small`.
  - Barre de progression solde / plafond (`TXProgressFill` ambre).
  - Bloc Account ID (format `76561198·XXXX·XXXX`) en police monospace + bouton Copy.
  - Panels "Deposit" et "Withdraw" empilés : input montant + bouton. Deposit = `TXBtnPrimary`, Withdraw = `TXBtnDanger`.
  - Alerts : `TXWarningBox` si credit card manquante, `TXInfoBox` pour les frais.
- **Zone TRANSFER** :
  - Form : Recipient ID (validation Steam64), Amount, Note optionnelle.
  - Bloc Summary en `TXInfoBox` : Amount / Fee / Recipient receives / Total deducted.
  - Liste latérale "Recent recipients" (contacts) + clan vault card.
- **Zone LEDGER** (historique) :
  - Liste verticale dense, `TXListRow` / `TXListRowAlt`, 20 lignes minimum visibles.
  - Colonnes : date (ash) | type (ivory) | partie adverse (ash) | montant (+ olive / − rust) | solde après (ambre).
  - Séparateurs de jour (`daily-sep`) entre les blocs de dates.
  - Detail panel à droite sur sélection d'une transaction.

### 7.3 TraderXGarage (module indépendant — **vue unifiée**)

- **Header** : NPC garagiste, nom en ambre, rôle "Garage · Keeper".
- **Onglets de header** : MY VEHICLES / INSURANCE / LOG. "My vehicles" est la page principale et unique flow utilisateur.
- **Pas de split park-in/park-out**. Une seule vue unifiée, avec deux zones contextuelles :
  1. **Bandeau park-in contextuel** (en haut du main) :
     - **Si le joueur est à proximité d'un véhicule qu'il peut stocker** : bandeau olive avec preview + nom + health/fuel/cargo/keys + frais + bouton `TXBtnPrimary` "Park in". Fond teinté olive très discret pour signaler l'action disponible.
     - **Sinon** : bandeau gris neutre "No vehicle nearby — approach with a vehicle to park it in". Pas d'action possible.
  2. **Liste des véhicules stockés** (en dessous) : chaque véhicule en `vcard` avec preview + health/fuel gauges + status verrou + bouton `TXBtnPrimary` "Park out · [fee]" directement sur la carte. Si le véhicule est non-restituable (clé manquante, place occupée, santé trop basse), le bouton est désactivé avec tooltip d'explication.
- **Sidebar gauche** : NPCs de garage dans le monde (Svetlana, Fyodor, Oleg...) — permet de voir ses véhicules stockés ailleurs. Celui actuellement ouvert est "is-active".
- **Panneau de droite** (detail) : sur sélection d'un véhicule stocké → preview large + jauges (health, fuel, engine, battery) + cargo grid + actions (Inspect / Park out).
- **Règle critique** : le park-in n'est jamais une "page" — c'est une action contextuelle disponible dans la vue unifiée quand le joueur répond aux conditions (véhicule présent, place dispo, fonds suffisants). Ne jamais séparer en deux écrans.

---

## 8. Do's & Don'ts (résumé actionnable)

### ✅ DO
- Toujours utiliser un token `TX*` via `data-style`.
- Toujours utiliser `dz-font-TraderX/datasets/fonts/RobotoCondensedVFxx`.
- Aligner les valeurs numériques à droite.
- Mettre prix, stocks et sélection en ambre.
- UPPERCASE pour tous les libellés de boutons et d'onglets.
- Densité élevée : 20+ lignes visibles sur un écran de liste.
- Bordures gauche 3 px ambre pour "sélectionné".
- Bordure bas 3 px ambre pour "onglet actif".

### ❌ DON'T
- Jamais de couleur hex inline dans un layout (tout passe par token).
- Jamais de rouge vif générique pour un accent.
- Jamais de vert pomme/flashy pour une action positive.
- Jamais de `border-radius > 3` sauf progress bar (qui est à 3).
- Jamais de blanc pur `#ffffff` en texte.
- Jamais d'italique.
- Jamais d'ombres portées.
- Jamais d'icônes multicolores.
- Jamais de padding "web app" aéré (>32 px) dans une liste.
- Jamais ajouter un nouveau style TX sans l'ajouter à ce document et à `TraderXMainView.html` **ensemble**.

---

## 9. Implementation Reference

### 9.1 Emplacement canonique des styles

Les styles TX sont définis dans un seul endroit : le bloc `<style type="dz-styles">` de
[src/TraderX/datasets/gui/TraderXMainView.html](../src/TraderX/datasets/gui/TraderXMainView.html).

Ce bloc est consommé par la toolchain DZML (`dzml pipeline`) qui génère :
- `dzml-styles.css` (preview web)
- `traderx_styles.styles` (runtime DayZ)
- `traderx_ui.imageset` + `traderx_atlas.edds/png/svg` (atlas compilé)

### 9.2 Ajouter un nouveau composant

1. Vérifier s'il existe déjà un token TX qui couvre le besoin — 90% du temps, oui.
2. Si vraiment nouveau : ajouter le token dans `TraderXMainView.html` + le documenter dans la section 6 de ce fichier.
3. Lancer `dzml pipeline --config dzml.config.yaml`.
4. Lancer `dzml check` sur le layout qui utilise le nouveau style.
5. Mettre à jour les modules Banking et Garage si le nouveau pattern leur est applicable.

### 9.3 Cross-validation

La toolchain `dayz-tools` (`dzml_validator.py --scripts <dir>`) vérifie que les styles référencés par les layouts existent bien dans le `<style type="dz-styles">`. À lancer avant chaque commit qui touche un layout.

---

## 10. Live mockup

Un mockup HTML standalone est disponible pour prévisualiser le design system appliqué à l'ensemble des pages. Ouvrir [src/TraderX/datasets/gui/mockup/index.html](../src/TraderX/datasets/gui/mockup/index.html) dans un navigateur (double-clic ou via `file://`).

**Contenu du mockup :**

| Fichier | Contenu |
|---|---|
| `index.html` | Landing page avec liens vers toutes les pages |
| `components.html` | Showcase complet de tous les patterns (couleurs, typo, boutons, cartes, alerts, notifications, ledger, vehicle card) |
| `trader/favorites.html` | Grille de favoris + panier. Icône ⚙ sur les produits customizables. |
| `trader/purchase.html` | Catégorie sidebar → produits. Filter pills (In stock / Customizable / Favorites) + preview. |
| `trader/customize.html` | Page dédiée à la customisation (presets / variants / attachments / magazines / ammo). |
| `trader/sell.html` | Inventaire joueur par slot + options de vente avancées. |
| `trader/catalog.html` | Vue catalogue en cartes — parcours visuel. |
| `banking/account.html` | Solde + Deposit/Withdraw + transactions récentes + stats 30j. |
| `banking/transfer.html` | Transfert + frais + contacts récents + clan vault. |
| `banking/ledger.html` | Historique complet des transactions + détail sélection. |
| `garage/my-vehicles.html` | Vue unifiée — bandeau park-in contextuel + liste véhicules stockés avec park-out. |

Le CSS du mockup ([field-ledger.css](../src/TraderX/datasets/gui/mockup/field-ledger.css)) est une **transcription directe des tokens** de ce document. Il ne remplace pas la pipeline DZML — c'est un outil de review visuelle pour valider le design avant d'investir dans les layouts DayZ.

---

## 11. Changelog

- **2026-04-18** — Reskin runtime DZML (Banking/Garage layouts, CategoryCard variants, Catalog stacked sections, Purchase filter pills + preview strip, Customize Ammo/Mag chips, Sell slot bar + advanced labels, Garage cargo grid 40-slot). Backend Banking ledger + contacts + memo + cross-module labeled payments. New `EItemCardDisplayMode` enum + `ItemCardRowView.layout` for row-mode product lists. New `CargoSlotViewData` projection with `GetSlotStyleToken` helper.
- Tokens harmonisés sur 3 modules, pipeline DZML stable, transparence shell appliquée par sub-page (`accountPanel`/`transferPanel`/`ledgerPanel`) à 82% pour matcher l'effet mockup.

---

## 12. Roadmap du design system

| Version | Contenu |
|---|---|
| **1.0** (ce document) | Tokens, patterns de base, application Trader/Banking/Garage |
| 1.1 | Motion : transitions d'onglets, apparition de modales (si le moteur le permet) |
| 1.2 | Tokens de son : par défaut coin/error/confirm/quick_event déjà présents — formaliser "quand jouer quoi" |
| 1.3 | Variants : mode "night ops" (contraste plus élevé) pour serveurs PvP hardcore |
| 2.0 | Migration des layouts v2 (Catalog, Purchase avancé) sur ce système |
