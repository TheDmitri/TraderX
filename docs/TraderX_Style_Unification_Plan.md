# TraderX — Plan d'unification du style (parité CourseBarterTrader)

But : que TraderX et CourseBarterTrader **partagent le même langage visuel**, que TraderX soit **plus simple à maintenir**, et que **tout le style + les polices passent par DayZular/l'atlas** pour qu'un autre owner/modder puisse re-skinner l'UI entière en éditant **une seule source** (tokens + polices), sans toucher au code.

Principe directeur : **les couleurs viennent des STYLES (atlas) + des ÉTATS, jamais d'un `SetColor()` codé en dur pour un état statique.** Le code ne garde que le vraiment dynamique (dégradé de santé). Le texte se colore en `dz-c` (cf. CLAUDE.md). Source unique des styles : `_fixtures/field-ledger.html` → `build-styles.sh` → atlas + `dzml-styles.css`.

---

## 1. Inventaire (core TraderX)

~44 composants UI. Deux problèmes transverses :

### 1a. Double pile de binders (dette)
Certains composants existent en **double** : ancien Dabs `*ViewController` (sous `GUI/Prefabs/**`) **et** nouveau DZlr `*Component` (sous `GUI/Cards/**`, `GUI/Pages/**`). Ex. : `ItemCardViewController` (Dabs) **vs** `ItemCardComponent` (DZlr) ; `TraderXMainViewController` (Dabs) vs `TraderXAppComponent` (DZlr). Logique dupliquée (ex. les mêmes `SetColor` apparaissent dans les deux). → cible : **une seule pile DZlr**, supprimer les Dabs morts. (Vérifier lesquels sont réellement actifs avant suppression.)

### 1b. Styles legacy encore utilisés (27 distincts, ~15 composants)
| Préfixe | Styles | Où |
|---|---|---|
| `DA_*` (9) | DA_Round1, DA_Smooth, DA_Smooth5px_Colored, DA_Smooth10px_Colored, DA_Smooth_Small_5px/_10px/_10px_Stroke, DA_Squad/1 | Upgrade page, combobox, preset cards, vieux item cards |
| `Corny_*` (5) | Corny_Rect_Side_Red/Green/Orange/Blue, Corny_Square_Shadow_5px | **ancienne pile notif** (cartes couleur), preview bg |
| `TP_*` (3) | TP_CardBtn, TP_PurchaseBtn, TP_BackgroundStroke | modals, preset (pré-TX) |
| autre | DayZNormal (2), rover_sim_colorable (7), RoundedPanel_Colorable (1) | textes, recolor runtime |

Composants à reskinner (priorité) : **Category cards (3 variantes), ancienne pile Notification (4 cartes couleur), Modals (2), Preset/Variant cards (3), Upgrade page, ComboBox, PaymentResume, PanelShadow**. Le reste est déjà en `TX*`.

---

## 2. Suppression de `TraderXViewStyles` + des SetColor codés

Audit : ~22 refs `TraderXViewStyles.*` (6 tokens) + ~16 `SetColor`/`SetTextColor` littéraux. Classés :

### (A) Statique / état → DÉPLACER dans des styles (≈25 sites)
| Couleur codée | Sites | Devient (style/état) |
|---|---|---|
| Nav actif `#afbc38` / inactif `#a5a5a5` | AppComponent, MainViewController | états du style `TXTab` (déjà actif=or via underline) → enlever le SetColor, laisser le style gérer |
| Player slot actif `#aa4444` / inactif `#8b8f9a` | PlayerSlotNav ×2 | styles `TXBtnSlot` (normal/active) |
| Item card IDLE/WARNING | ItemCardComponent/ViewController | styles `TXCard` (normal) / **`TXCardWarn`** (variante chaude) → swap `data-style`, plus de SetColor |
| Tooltip dispo `#6b8e5a` / indispo `#754d57` | BuyTooltip ×2 | styles texte `TXTextOk` / `TXTextWarn` (ou `dz-c` fixe) |
| Notif succès/erreur | NotificationDetails (ancienne pile, dormante) | mort si on supprime l'ancienne pile notif |
| Preset default `#ff0000` / non `#fff` | PresetCard ×2 | style `TXBadgeDefault` ou icône dédiée |

### (B) Dynamique → reste en code, mais CENTRALISÉ (≈5 sites)
- **Dégradé de santé** (`GetHealthFromLevel` → ARGB par état pristine/damaged/…) : calcul runtime, garder.
- **Stock bas en rouge** (`ARGB(255,0,0,0)`) : garder, mais via un token central (pas `0xff0000` en dur).

→ Remplacer `TraderXViewStyles` par un **mini `TraderXTheme`** (ou des constantes dans un seul fichier) ne contenant **que** le dynamique restant (santé, stock-bas), idéalement dérivé des mêmes hex que les tokens. Tout le reste disparaît au profit des styles. Net : de ~38 sites de couleur → ~5.

---

## 3. Polices via DayZular (re-skin facile)

Aujourd'hui : 6 refs **path-based** `dz-font-TraderX/datasets/fonts/RobotoCondensedVF{12..28}` (~375 usages). Pas d'alias.

Cible : un **`dzml.config.yaml`** (comme CBT) avec des **alias sémantiques** :
```yaml
fonts:
  default:  TraderX/datasets/fonts/RobotoCondensedVF
  heading:  TraderX/datasets/fonts/RobotoCondensedVF   # → Oswald plus tard
  body:     TraderX/datasets/fonts/RobotoCondensedVF
  mono:     TraderX/datasets/fonts/RobotoCondensedVF   # → JetBrains Mono plus tard
```
Puis remplacer les refs par `dz-font-heading-28`, `dz-font-body-14`, `dz-font-mono-16`. Un modder change 1 ligne d'alias → toute l'UI change de police. (Permet d'introduire Oswald/JetBrains plus tard sans re-toucher 375 sites — juste l'alias.)

---

## 4. Styles à AJOUTER à l'atlas (tokens field-ledger)

Pour absorber les états codés : `TXCardWarn` (carte warning), `TXBtnSlot` (normal/active), `TXTextOk`/`TXTextWarn` (ou couleurs `dz-c` fixes), `TXBadgeDefault`, `TXModal`, `TXBtnPreset`, `TXStatusRed/Green/Orange/Blue` (remplacent `Corny_Rect_Side_*`). Tous définis dans `field-ledger.html`, un seul rebuild.

---

## 5. Phasage proposé (stories)

| Story | Contenu | Risque |
|---|---|---|
| **US-024** | Reskin legacy → TX (category cards, modals, preset cards, combobox, upgrade, paymentresume) + ajout styles manquants à l'atlas | moyen (visuel) |
| **US-025** | Tuer `TraderXViewStyles` : statiques → styles/états, dynamiques → mini `TraderXTheme` ; retirer les SetColor d'état | moyen (touche code) |
| **US-026** | Système d'alias polices `dzml.config.yaml` + refs sémantiques (sans changer la police encore) | bas |
| **US-027** | Consolidation binders : supprimer la pile Dabs `*ViewController` morte, garder DZlr | élevé (à valider) |
| **US-028** (option) | Bascule polices Oswald/JetBrains via les alias | bas |

Chaque story = test in-game vert avant la suivante (TSL). US-019→023 déjà en attente de validation in-game — à builder d'abord.

---

## 6. Résultat visé

- 1 source de tokens (`field-ledger.html`) + 1 config polices (`dzml.config.yaml`) = **re-skin complet sans toucher au code**.
- `data-style` partout, `dz-c` pour le texte, ~0 `SetColor` statique.
- Pile binder unique (DZlr).
- TraderX et CBT visuellement cousins.
