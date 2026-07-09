# Gribouillot — pistes d'amélioration

Audit réalisé le 2026-07-09 (v2.2.1) à partir du code, du manuel utilisateur et du
workflow chouetteur. Ce document sert de backlog : chaque amélioration est
indépendante, à cocher quand elle est faite, avec les fichiers concernés et
comment la tester.

## Points forts à préserver

- Construction géométrique guidée pas-à-pas dans la barre de statut (`drawingTips`).
- Échelle km/px précise, mesures passives riches à la sélection (longueurs,
  angle entre 2 droites, distances point-droite/cercle) — `sceneSelectionChanged`,
  `gribouillot_toolbar.cpp`.
- Calques = protection : on peut se référencer aux tracés d'un autre calque sans
  pouvoir les modifier.
- Spirale à 4 centres (spécifique chasse à la Chouette d'Or).
- Import d'images avec rotation (flèches, ±1°/±10°), échelle (+/-, Maj+molette,
  presets 0/1/2) et transparence par calque.

## Constats

### Le plus grave : aucun undo
- Pas de `QUndoStack` dans le code. `Suppr` détruit définitivement
  (`deleteSelectedItems`, `gribouillotlayer.cpp:369`).
- Supprimer un calque efface immédiatement son fichier XML du disque
  (`gribouillotlayer.cpp:459-476`).

### Ergonomie
- Figures non déplaçables après création (`ItemIsMovable` seulement sur les
  images, `item_pixmap.cpp:27`). Un cercle mal placé = supprimer + redessiner.
- Échec **silencieux** des outils à référence (parallèle, perpendiculaire,
  tangente, cercle-depuis-rayon...) quand le clic ne sélectionne pas le bon
  item : le garde `isOnlySelected` échoue sans message
  (`gribouillot_toolbar.cpp:60-96`).
- Réarmement incohérent : segment/cercle/arc/ellipse/spirale se désarment après
  une figure ; parallèle/perpendiculaire/tangente restent armés « pour dessiner
  en série » ; point/horizontale/verticale restent armés implicitement.
- Zoom : pas de borne min/max (`zoomablegraphicsview.cpp` `wheelEvent`), pas
  d'affichage du niveau, pas de raccourci « ajuster »/« 100 % » (le
  `zoomToFit()` existe mais n'est appelé qu'au chargement), zoom centré sur la
  vue et non sur le curseur.
- Aucun raccourci clavier pour les outils de dessin. Tooltip de la flèche de
  sélection annonce `{s}` mais le raccourci réel est `J` (et `S` = options
  spirale).
- Calques non réordonnables : `setMovable(true)` commenté
  (`gribouillottabwidget.cpp:19`) à cause d'un bug Qt5 d'époque — l'ordre est
  pourtant persisté dans le `.grib` (`layersOrder`).

### Fonctionnalités absentes
- **Texte/annotation** : rien. Impossible d'écrire une hypothèse sur la carte ou
  de nommer un point (seul contournement : la pondération des points).
- **Mesures volatiles** : distance/angle uniquement en barre de statut, rien
  d'épinglable sur la carte.
- **Export limité** : `actionExportAs` fait un `grab()` du viewport à la
  résolution écran (`gribouillot.cpp:963-980`). Pas de rendu pleine carte.
- **GPS : code mort.** Bouton caché (`//TODO: implement GPS`,
  `gribouillot.cpp:146`), `getFix()` renvoie "no data"
  (`dlg_setupgps.cpp:33-37`), pas de conversion px↔lat/long.
- Zoom/position non sauvegardés dans le projet.
- Pas de copier/coller ni de déplacement d'items entre calques.
- Pas de rectangle/polygone/main levée.

### Dette technique (au fil de l'eau)
- Fuites mémoire dans la conversion noir & blanc : `mapImage`/`bwMapImage`
  jamais libérés, tableau `std::thread[]` jamais `delete[]`
  (`gribouillot.cpp:1126-1164`).
- `break` manquant après le case `PROTRACTOR`
  (`gribouillot_toolbar.cpp:410-412`) — bénin (tombe dans NONE).
- Typos et chaînes hors `tr()` (ex. "Maj" en dur, `item_arc.cpp:96`).
- Valeurs codées en dur : monde ±50000px pour les droites « infinies »,
  cycle d'épaisseurs 1/3/5/7, limite image Qt 512 Mo.

## Backlog priorisé

Chaque item est autonome et testable séparément. Cocher quand fait.

### Priorité 1 — sécurité du travail et confort immédiat

- [ ] **1. Undo/redo (Ctrl+Z / Ctrl+Y)** — effort : moyen/gros
  - Voir le [plan d'implémentation détaillé](#plan-détaillé--1-undoredo) en fin
    de document. Phase 1 = création/suppression d'items + corbeille de calque ;
    phases 2-3 = couleur/épaisseur puis transformations d'images.

- [ ] **2. Zoom ergonomique** — effort : faible
  - `setTransformationAnchor(AnchorUnderMouse)` ; bornes min/max de zoom ;
    raccourci `F` = ajuster la carte (réutiliser `zoomToFit()`), `1` = 100 % ;
    afficher le niveau de zoom (il n'apparaît que sur la barre d'échelle).
  - Fichiers : `zoomablegraphicsview.cpp/h`, `gribouillot.ui` (actions).
  - Test : zoomer molette sur un coin → le point sous le curseur ne bouge pas ;
    F/1 fonctionnent ; impossible de zoomer à l'infini.

- [ ] **3. Feedback des échecs silencieux** — effort : faible
  - Dans chaque garde `isOnlySelected(...)` qui échoue, afficher en barre de
    statut pourquoi (« Sélectionnez d'abord une droite ou un segment »).
  - Fichiers : `gribouillot_toolbar.cpp` (cases PARALLEL, PERPENDICULAR,
    TANGENT_TOCIRCLE, CIRCLE_FROMSELECTEDRADIUS/DIAMETER, ARC_FROMCIRCLE...).
  - Test : armer « perpendiculaire », cliquer dans le vide → message clair.

### Priorité 2 — forte valeur chouetteur

- [ ] **4. Outil texte/étiquette** — effort : moyen
  - Nouvel `Item_text` (QGraphicsTextItem dérivé), persisté dans le XML de
    calque, éditable (double-clic), couleur du calque.
  - Fichiers : nouveau `src/items/item_text.*`, `gribouillotitem.h`,
    `gribouillotlayer.cpp` (write/loadXML), toolbar.
  - Test : poser un texte, sauvegarder, rouvrir → le texte est là.

- [ ] **5. Mesures épinglables** — effort : moyen
  - Après une mesure (distance/angle), proposer d'épingler le résultat comme
    étiquette persistante sur la carte (dépend de l'item texte du n°4).
  - Test : mesurer 157 km, épingler, rouvrir le projet → l'étiquette est là.

- [ ] **6. Export pleine résolution** — effort : faible/moyen
  - Rendre la scène complète (`QGraphicsScene::render` dans un QImage à la
    taille de la carte) au lieu du `grab()` du viewport ; proposer viewport ou
    carte entière dans le dialogue.
  - Fichiers : `gribouillot.cpp` (`on_actionExportAs_triggered`).
  - Test : exporter → l'image fait la taille de la carte, tracés inclus, pas de
    barres de défilement.

- [ ] **7. Réordonnancement des calques** — effort : faible (si le bug Qt a disparu)
  - Réactiver `setMovable(true)` (`gribouillottabwidget.cpp:19`), tester en Qt6,
    synchroniser `layersOrder` à la sauvegarde (déjà persisté).
  - Test : déplacer des onglets, sauvegarder, rouvrir → ordre conservé, pas de
    crash en déplaçant autour de l'onglet carte et du « + ».

### Priorité 3 — chantiers ambitieux

- [ ] **8. Géoréférencement réel** — effort : gros
  - Calage par 2 points connus → conversion px↔lat/long ; saisie de coordonnées
    pour placer un point ; export GPX des points. Finir ou retirer le code GPS
    mort (`dlg_setupgps.cpp`).
  - Test : caler sur 2 villes, placer un point par coordonnées → position
    correcte ; export GPX lisible par un GPS/appli.

- [ ] **9. Figures déplaçables/éditables** — effort : moyen/gros
  - `ItemIsMovable` optionnel (mode « édition »), ou déplacement fin aux flèches
    de l'item sélectionné ; attention à l'esprit « précision » de l'outil
    (positions calculées). À coupler avec l'undo (n°1).

- [ ] **10. Divers**
  - Sauvegarder zoom/pan dans le `.grib` et restaurer à l'ouverture.
  - Copier/dupliquer des items, déplacer un item vers un autre calque.
  - Outil polygone / main levée.
  - Nettoyage dette technique (fuites B&W, `break` PROTRACTOR, typos, `tr()`).

## Méthode de travail

1 amélioration = 1 branche courte ou 1 commit isolé sur `master` + test manuel
(section « Test » de l'item) + release mineure quand un lot cohérent est prêt.
Le CI construit les binaires 3 plateformes à chaque tag `v*`.

---

## Plan détaillé — n°1 undo/redo

### Pourquoi c'est simple à greffer ici

Le code a deux goulots d'étranglement uniques, parfaits pour accrocher l'undo :

- **Toute création d'item** (les ~20 `drawXxx()`) finit dans
  `GribouillotLayer::addItemToLayer(QGraphicsItem*)`
  (`gribouillotlayer.cpp:142-156`) : ajout à `itemsList` + signal
  `addItemToScene` → `Gribouillot::doAddItemToScene` → `scene->addItem`.
- **Toute suppression d'item** passe par
  `GribouillotLayer::deleteSelectedItems()` (`gribouillotlayer.cpp:369-387`),
  appelée uniquement par `Gribouillot::keyDeleteFromScene`
  (`gribouillot_toolbar.cpp`).

On n'a donc PAS besoin de toucher aux 20 fonctions de dessin.

### Architecture

- `QUndoStack* undoStack` membre de `Gribouillot` (créé dans le constructeur).
- Nouveau fichier `src/app/commands.h/.cpp` (à ajouter au `.pro` dans
  HEADERS/SOURCES) avec 2 classes pour la phase 1 :

```cpp
// Créé au moment où l'item vient d'être ajouté par le flux normal.
// Particularité : le premier redo() ne doit RIEN faire (l'ajout a déjà eu
// lieu), d'où le drapeau firstRedo. Pattern classique de retrofit.
class AddItemCommand : public QUndoCommand {
public:
    AddItemCommand(GribouillotLayer* layer, QGraphicsItem* item);
    ~AddItemCommand();     // delete item SI détaché (non présent sur la scène)
    void undo() override;  // layer->detachItem(item)
    void redo() override;  // si firstRedo → no-op ; sinon layer->attachItem(item)
private:
    GribouillotLayer* layer;
    QGraphicsItem* item;
    bool firstRedo = true;
};

class DeleteItemsCommand : public QUndoCommand {
public:
    // items + leurs positions dans itemsList (pour ré-insérer au même endroit,
    // l'ordre compte pour selectNextItem)
    DeleteItemsCommand(GribouillotLayer* layer, const QList<QGraphicsItem*>& items);
    ~DeleteItemsCommand(); // delete des items SI détachés
    void undo() override;  // ré-attache chaque item à son index d'origine
    void redo() override;  // détache tous les items
private:
    GribouillotLayer* layer;
    QList<QGraphicsItem*> items;
    QList<int> indices;
    bool detached = false;
};
```

- Deux méthodes utilitaires dans `GribouillotLayer` (publiques, utilisées
  uniquement par les commandes) :
  - `attachItem(QGraphicsItem* item, int index = -1)` : insère dans
    `itemsList` (à `index` si fourni), `emit addItemToScene(item)`, réapplique
    `setOpacity`/`setVisible` selon l'état courant du calque (mêmes lignes que
    `addItemToLayer`).
  - `detachItem(QGraphicsItem* item)` : `item->scene()->removeItem(item)` +
    `itemsList.removeOne(item)` — SANS `delete`. C'est la commande qui devient
    propriétaire de l'item détaché (règle d'ownership : un item est détruit
    soit par le destructeur de commande s'il est hors scène, soit par la scène
    à la fermeture).

### Branchements dans le code existant

1. **Création** : dans `addItemToLayer`, en fin de fonction, émettre un nouveau
   signal `itemAdded(GribouillotLayer*, QGraphicsItem*)` (ou passer par le
   signal existant en ajoutant le layer). Dans `Gribouillot`, connecter ce
   signal → `undoStack->push(new AddItemCommand(layer, item))`.
   NB : `push()` appelle `redo()` immédiatement, d'où le no-op du premier
   `redo()` décrit plus haut.
2. **Suppression** : remplacer le corps de `keyDeleteFromScene`
   (`gribouillot_toolbar.cpp`) : récupérer
   `currentLayer->selectedItems()` ; si vide → `userCanNotDo("delete")` comme
   aujourd'hui ; sinon `undoStack->push(new DeleteItemsCommand(...))`. La
   méthode `deleteSelectedItems()` de la couche peut être supprimée ou vidée.
3. **Actions UI** : dans le constructeur de `Gribouillot` :
   ```cpp
   QAction* undoAction = undoStack->createUndoAction(this, tr("&Undo"));
   undoAction->setShortcut(QKeySequence::Undo);   // Ctrl+Z
   QAction* redoAction = undoStack->createRedoAction(this, tr("&Redo"));
   redoAction->setShortcut(QKeySequence::Redo);   // Ctrl+Shift+Z / Ctrl+Y
   ```
   Les ajouter au menu **Outils** (ou créer un menu Édition dans le .ui) et,
   si souhaité, à la toolbar. Pas de conflit clavier : la scène ne consomme
   que Del/Espace/C/T/Échap (`gribouillotscene.cpp`), les shortcuts
   d'application passent avant.

### Règles d'invalidation (CRITIQUE pour éviter les pointeurs pendants)

`undoStack->clear()` obligatoire dans :
- `Gribouillot::doDeleteLayer` (`gribouillot.cpp:1334`) — des commandes
  peuvent pointer vers le calque détruit ;
- `Gribouillot::openProject` et `resetUi`/`initProject` — changement de projet ;
- l'import de calque (`on_actionImport_layer_triggered`) par prudence.

Optionnel : `undoStack->setClean()` dans `saveProject()` — permettra plus tard
un indicateur « modifications non sauvegardées ».

Attention : ne PAS pousser de commande pour les items transitoires (helpers
`Item_arcDrawer`, `Item_spiralDrawer`, `Item_scaleRuler`, protractor) — ils
sont ajoutés directement à la scène par `Gribouillot`, pas via
`addItemToLayer`, donc le branchement n°1 ne les capte naturellement pas. Ne
rien faire de spécial, juste le vérifier au test.

### Corbeille pour la suppression de calque

Dans `GribouillotLayer::on_deleteLayerTlBtt_clicked`
(`gribouillotlayer.cpp:459-476`) : remplacer `file.remove()` par :

```cpp
QFile::remove(label+".xml.bak");          // écraser une vieille corbeille
QFile::rename(label+".xml", label+".xml.bak");
```

Adapter le texte de la boîte de confirmation (« le calque sera conservé en
.xml.bak »). La restauration se fait à la main (renommer le fichier puis
Fichier → Importer un calque) — suffisant pour la phase 1, documenter dans le
message de la boîte.

### Phase 2 — couleur et épaisseur (après validation phase 1)

`keyTFromScene` (`gribouillot_toolbar.cpp:566-613`) et
`on_actionChooseColor_triggered` (`gribouillot_toolbar.cpp:986`) modifient les
items sélectionnés par des casts par type (`newPen`/`newBrush`). Pattern :

- `ChangeStyleCommand` qui capture AVANT modification, pour chaque item, la
  paire (couleur, épaisseur) — lisible uniformément via `pen()`/`brush()` selon
  le type, comme le fait déjà le code existant — puis applique la nouvelle
  valeur dans `redo()` et restaure l'ancienne dans `undo()`.
- Factoriser le switch par type existant dans une fonction libre
  `applyStyle(QGraphicsItem*, QColor, int)` dans `commands.cpp`, utilisée par
  les deux sens.

### Phase 3 — optionnelle

- Transformations des images (`item_pixmap.cpp` : drag, rotation flèches,
  échelle +/-) : capturer pos/rotation/échelle au début et à la fin d'un geste,
  pousser une commande snapshot. Demande d'émettre un signal depuis
  `Item_pixmap` (actuellement autonome) — plus invasif.
- Pondération de point (`w`) et angle d'arc (`a`) : même pattern snapshot.

### Tests de la phase 1

1. Dessiner un segment → Ctrl+Z → il disparaît → Ctrl+Y → il revient.
2. Dessiner 3 points, en sélectionner 2, Suppr → Ctrl+Z → les 2 reviennent, à
   leur position, sélectionnables, et Espace les parcourt dans l'ordre initial.
3. Ctrl+Z répété jusqu'au projet vide → pas de crash ; Ctrl+Y tout du long.
4. Supprimer un calque → `<nom>.xml.bak` présent sur disque ; Ctrl+Z ne doit
   PAS tenter de restaurer le calque (pile vidée) ; pas de crash.
5. Ouvrir un autre projet après des undo → pile vidée, pas de crash.
6. Arc/spirale : vérifier que les helpers transitoires ne polluent pas la pile
   (dessiner un arc = 1 seule entrée d'undo, l'arc final).
7. Sauvegarder, Ctrl+Z, sauvegarder de nouveau → le XML reflète bien l'état
   affiché.
8. Changer la couleur d'un item d'un AUTRE calque (interdit) → toujours refusé,
   pas d'entrée d'undo parasite.
