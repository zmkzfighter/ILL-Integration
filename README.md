# Impossible Levels List — mod Geode

[![Build Geode Mod](https://github.com/zmkzfighter/ILL-Integration/actions/workflows/build.yml/badge.svg)](https://github.com/zmkzfighter/ILL-Integration/actions/workflows/build.yml)

Mod pour Geometry Dash (via [Geode](https://geode-sdk.org)) qui ajoute un
écran permettant de parcourir et charger directement les niveaux de
**[impossiblelevels.com](https://impossiblelevels.com/)**.

## Ce qui est inclus

- **Bouton dans l'écran "Rechercher un niveau"** (`LevelSearchLayer`), placé
  automatiquement juste au-dessus du bouton retour en bas à gauche (repéré
  via l'ID `back-button` que Geode assigne automatiquement).
- **Écran complet** avec :
  - Onglets **Tous / Cette semaine / Ce mois-ci**.
  - Barre de **recherche** (nom du niveau ou du créateur).
  - **Filtre par plage de rang** (bouton "filtres").
  - **Mise en avant** des nouveautés de la semaine et du mois en haut de
    l'onglet "Tous", dans des cellules plus grandes et marquées "NOUVEAU".
  - Chaque cellule a un bouton **Jouer** qui télécharge le niveau depuis les
    serveurs GD par son ID et ouvre directement `LevelInfoLayer` (l'écran
    d'info du niveau, comme si tu l'avais cherché toi-même), ainsi qu'un
    bouton pour copier l'ID et (si l'API le fournit) un lien vers les
    records.
  - Bouton **Site web** pour ouvrir impossiblelevels.com dans le navigateur.
  - Bouton **rafraîchir** pour forcer un nouveau fetch réseau (sinon un cache
    configurable est utilisé, réglable dans les paramètres du mod).

## ⚠️ Point important sur l'API (à lire avant de compiler)

Le site `impossiblelevels.com` charge ses données via JavaScript côté
client : je n'ai pas pu observer le JSON exact renvoyé par
`api.impossiblelevels.com` (site rendu dynamiquement, endpoint racine
`/api` seul renvoie une 404 — ce n'est pas un endpoint utilisable tel quel).

Pour ne pas bloquer sur cette inconnue, le mod est construit ainsi :

1. Un seul appel réseau vers `{api-base-url}/levels` (configurable dans les
   paramètres du mod si l'URL réelle diffère) est fait pour récupérer
   **toute** la liste.
2. Les catégories "Semaine" / "Mois" sont calculées **côté mod**, en filtrant
   par date d'ajout (7 et 30 jours par défaut, réglables). Ça évite de
   deviner des endpoints séparés qui n'existent peut-être pas.
3. Le parsing JSON (`ImpossibleLevelsAPI::fromJson` dans
   `ImpossibleLevelsAPI.cpp`) essaie **plusieurs noms de champs plausibles**
   pour chaque donnée (ex: `levelID`, `levelId`, `level_id`...) et ne plante
   jamais sur un champ manquant.

### Si la liste reste vide ou mal remplie après compilation

1. Lance le jeu avec le mod installé, ouvre la console Geode
   (`Geode → Console` dans le menu du loader).
2. Ouvre l'écran Impossible Levels en jeu : la première réponse brute de
   l'API est loguée (`log::debug("[ImpossibleLevels] Raw JSON...`).
3. Compare les vraies clés du JSON avec celles testées dans
   `ImpossibleLevel::fromJson` (fichier `src/ImpossibleLevelsAPI.cpp`) et
   ajoute/corrige les noms de clés en conséquence — c'est centralisé dans
   cette seule fonction.
4. Si l'API a un chemin différent de `/levels` (par ex. `/list`,
   `/api/v1/levels`...), change simplement la valeur "URL de base de l'API"
   dans les paramètres du mod, ou modifie `baseUrl() + "/levels"` dans
   `ImpossibleLevelsAPI::fetchLevels`.

Autrement dit : l'architecture réseau est isolée dans un seul fichier pour
que cet ajustement soit rapide, sans toucher à l'UI.

## Compilation

Prérequis : [CLI Geode](https://docs.geode-sdk.org/getting-started/) installé
et configuré (`geode sdk install`).

```bash
geode build
```

ou, si tu utilises directement CMake :

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

Le fichier `.geode` généré peut ensuite être installé via
`geode install` ou glissé dans le dossier `mods` de Geode.

### Compilation automatique (GitHub Actions)

Le workflow [`.github/workflows/build.yml`](.github/workflows/build.yml)
compile le mod à chaque push et chaque pull request sur `main` (et à la
demande via **Actions → Build Geode Mod → Run workflow**), pour quatre
cibles : **Win64**, **MacOS**, **Android32** et **Android64**.

Le job `package` fusionne ensuite les quatre binaires en **un seul fichier
`.geode` multi-plateforme**, publié comme artefact du run
(`ImpossibleLevelsList.geode`) — téléchargeable en bas de la page du run
dans l'onglet Actions, puis installable directement.

La CI compile contre la version de Geode déclarée dans `mod.json`
(`"geode": "5.10.1"`) grâce à `sdk: given` : bumper cette valeur suffit
pour changer de SDK, la CI suit automatiquement.

Pense à :
- Adapter `mod.json` → `id`, `developer`, `repository` avec tes propres
  infos.
- Vérifier que la version `gd` dans `mod.json` correspond à ta version de
  Geometry Dash installée.
- Ajouter tes propres sprites si tu veux remplacer les icônes
  (`GJ_challengeBtn_001.png` pour le bouton d'ouverture) par une icône
  dédiée (place-la dans `resources/` et référence-la dans `mod.json` →
  `resources`, puis charge-la via son nom de fichier au lieu du sprite
  stock).

## Points à vérifier / limites connues

- Le hook `LevelManagerDelegate` (chargement d'un niveau par ID depuis les
  serveurs GD) a une signature qui a légèrement varié selon les versions de
  GD/Geode. Le fichier `ImpossibleLevelsLayer.cpp` fournit les deux formes
  (avec et sans paramètre entier final) **sans `override`** exprès, pour que
  ça compile quelle que soit la version exacte de ton SDK — vérifie dans la
  console que le chargement fonctionne bien après compilation.
- Aucune image de niveau (thumbnail) n'est affichée pour l'instant : l'API
  ne semble pas exposer d'images statiques facilement téléchargeables dans
  le jeu (nécessiterait un chargement de texture réseau supplémentaire). Le
  champ `thumbnailUrl` est récupéré et prêt à être utilisé si tu veux
  ajouter ça.
- La compilation est vérifiée automatiquement par la CI (Win64, MacOS,
  Android32/64), mais le mod n'a pas été testé **en jeu** : récupère
  l'artefact `.geode` du dernier run, installe-le, et ajuste le parsing de
  l'API en suivant la section ci-dessus si la liste reste vide.
