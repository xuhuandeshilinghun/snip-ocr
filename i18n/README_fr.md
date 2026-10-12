<div align="center">

**Readme Language / Langue：**
[English](../README.md)｜[日本語](README_ja.md)｜[한국어](README_ko.md)｜[Русский](README_ru.md)｜[Français](README_fr.md)｜[Deutsch](README_de.md)｜[简体中文](README_zh.md)｜[繁體中文](README_zh_TW.md)




💖 Soutenez ce projet : [Ko‑fi](https://ko‑fi.com/edenls)

</div>

# SnapOCR
> Utilitaire haute‑performance de capture d’écran, autocollants de bureau et OCR hors‑ligne pour Windows, développé en C++17 / Qt6.

SnapOCR est un utilitaire de bureau open‑source tout‑en‑un pour Windows 10 / 11 x64. Il intègre la capture d’écran haute‑performance, l’aperçu des autocollants flottants sur le bureau, les annotations d’image, la reconnaissance OCR multilingue hors‑ligne, la capture différée en boucle et le système de mise à jour automatique.

**Langues prises en charge par l’interface : chinois simplifié, chinois traditionnel, anglais, japonais, coréen, allemand, français, russe ; la reconnaissance de texte OCR est disponible pour toutes ces langues.**

Tous les traitements d’image et de reconnaissance de texte s’exécutent **localement sur votre appareil. Les captures d’écran ne sont jamais envoyées vers le cloud, vos données personnelles sont protégées.**
Adapté à l’extraction de documents professionnels, l’aide à la lecture et à la traduction, la récupération de texte depuis des pages web, la capture de ressources pour développeurs, les annotations de capture pour des démonstrations pédagogiques et bien d’autres cas d’usage.

Développé avec les API natives Windows et Qt6. Il prend en charge les écrans multiples haute‑DPI, propose des thèmes clair / sombre, une interface entièrement internationalisée multilingue et peut s’exécuter en arrière‑plan depuis la barre‑des‑tâches système.

---
## Fonctionnalités
### 📷 Module de capture d’écran
- **Capture d’écran par raccourci global** : implémentée via le hook clavier bas‑niveau Windows `WH_KEYBOARD_LL`. Déclenchez la capture d’écran avec un seul raccourci indépendamment de la fenêtre active.
- **Sélection de zone libre** : faites glisser la souris pour dessiner un rectangle de sélection avec 8 points de redimensionnement. Réglage fin avec les flèches : 1 pixel par pas, `Maj + flèche` pour un déplacement de 10 pixels pour une sélection précise au pixel près.
- **Accrochage aux bords des fenêtres** : inspiré de Snipaste. Les bords de la sélection s’accrochent automatiquement aux limites des fenêtres (seuil par défaut 8 pixels) pour capturer rapidement une fenêtre complète.
- **Détection précise des fenêtres et contrôles** : utilise `WindowFromPoint` + `RealChildWindowFromPoint` pour détecter les sous‑contrôles tels que boutons, champs de saisie, barres d’outils et sélectionner directement leur zone.
- **Récupération des vraies limites de fenêtre DWM** : appelle `DwmGetWindowAttribute(DWMWA_EXTENDED_FRAME_BOUNDS)` pour exclure les ombres des fenêtres et obtenir les dimensions réelles de rendu.
- **Loupe pixel** : superposition de loupe en mode capture, affiche les valeurs de couleur RGB / HEX et les coordonnées écran. Format de couleur commutable pour la pipette et l’inspection de pixels.
- **Étiquette de dimension de sélection** : étiquette de taille `W × H` sur fond bleu arrondi près de la sélection. La couleur du texte s’adapte automatiquement au thème clair / sombre.
- **Barre d’outils de capture** : barre d’outils inférieure qui apparaît après la capture de zone. Prend en charge l’épinglage d’autocollant, la reconnaissance OCR, la reconnaissance QR‑code, copier l’image, enregistrer au format PNG et annuler.

### 🖼️ Autocollants de bureau et annotations
- **Autocollants flottants de bureau** : épingler directement les captures d’écran à leur position d’origine sur l’écran (style Snipaste). Prise en charge complète du ratio de pixel DPR pour éviter le floutage sur les écrans haute‑résolution.
- **Boîte à outils complète d’annotations** : clic droit pour ouvrir la barre de dessin : sélection, stylo, flèche, rectangle, ellipse, surbrillance, pixellisation mosaïque, note de texte, gomme.
- **Transformation d’image** : les autocollants acceptent une mise à l’échelle de 20 %‑500 % ; rotation 90 ° horaire / anti‑horaire, retournement horizontal et vertical. Toutes les opérations fonctionnent sur la résolution physique originale sans perte de qualité.
- **Cuisson des annotations pour export** : fusionner toutes les annotations dessinées sur l’image source et exporter un PNG complet en conservant la résolution d’origine.
- **Panneau de pipette couleur écran** : implémentation indépendante de pipette pour échantillonner n’importe quel pixel de l’écran, compatible avec l’interface multilingue.
- **Basculer l’ombre des fenêtres d’autocollant** : activer ou désactiver l’effet d’ombre pour les fenêtres d’autocollant, conforme au style visuel des coins arrondis natifs de Windows 11.
- **Gestion des groupes d’autocollants** : plusieurs groupes d’autocollants (groupe par défaut + groupes personnalisés). Changer de groupe masque les autocollants du groupe courant et restaure la disposition d’un autre groupe.
- **Historique persistant des captures** : les captures originales sont sauvegardées au format PNG dans le dossier local `history`. La position, l’échelle et l’état des annotations des autocollants sont stockés dans les fichiers `.sp1` avec une obfuscation légère XOR+Base64 (pas un chiffrement fort, seulement pour éviter les modifications accidentelles).
- **Restauration automatique des autocollants au redémarrage** : au lancement de l’application, tous les autocollants flottants retrouvent leur dernière position, mise à l’échelle et appartenance au groupe.

### 🔍 Reconnaissance OCR hors‑ligne
- **Chargement parallèle de plusieurs modèles** : charger plusieurs modèles de reconnaissance simultanément. Prend en charge le texte mixte chinois‑anglais, coréen, cyrillique russe et écriture latine.
- **Détection automatique de langue** : l’algorithme analyse la proportion des caractères dans l’image et bascule automatiquement vers le modèle OCR correspondant, pas besoin de sélection manuelle de langue.
- **Inférence CPU ONNX Runtime** : fonctionne exclusivement sur CPU. **Carte graphique dédiée non requise**, fonctionne sur des ordinateurs portables classiques.
- **Thread‑arrière‑plan non bloquant** : les tâches OCR s’exécutent dans un thread de travail QThread indépendant afin que l’interface reste réactive pendant la reconnaissance.
- **Boîte‑dialogue de résultat de reconnaissance** : fenêtre de sortie OCR modifiable. Prend en charge la copie presse‑papier en un clic et l’enregistrement du texte dans un fichier local.

### ⏱️ Capture différée
- **Capture différée et en boucle** : définir un délai de 1‑10 secondes, configurer le nombre de captures en boucle pour une sauvegarde automatique par lot.
- **Notification toast système** : popup native Windows quand la séquence de capture est terminée, affiche le chemin de sauvegarde et le nombre de captures. Cliquer sur la notification copie le chemin du fichier.

### 🔄 Système de mise à jour automatique
- **Vérification de version** : vérification automatique au démarrage ; vérification manuelle disponible dans le panneau des paramètres.
- **Mécanisme de mise à jour delta** : génère des paquets complets et des correctifs différentiels à partir d’une version de référence. Seuls les fichiers modifiés sont téléchargés pour réduire la taille du téléchargement.
- **Routage double source** : source de téléchargement Gitee pour utilisateurs de Chine continentale, GitHub Releases pour les utilisateurs étrangers pour résoudre les différences d’accès réseau.
- **Binaire de mise à jour indépendant** : `updater.exe` construit sur API Win32 brute + minizip, sans dépendance Qt.
- **Conserver la configuration et les données utilisateur** : sauvegarde du fichier de configuration utilisateur `.config_settings.json` pendant la mise à jour. Restauration des préférences après mise à jour ; raccourcis, thèmes et paramètres des groupes d’autocollants sont conservés.
- **Flux de redémarrage après mise à jour** : une fois la mise à jour terminée, appelle `ShellExecuteExW` pour lancer le nouveau binaire avec l’argument `--restart` afin de contourner le mutex d’instance unique pour un redémarrage propre.

### ⚙️ Intégration système
- **Service d’arrière‑plan barre‑des‑tâches** : minimiser vers la barre‑des‑tâches. Menu clic‑droit barre‑des‑tâches : capture d’écran, OCR, capture différée, groupes d’autocollants, effacer l’historique, préférences, redémarrer l’application, quitter.
- **Internationalisation de l’application** : interface intégrée chinois simplifié `zh_CN` et anglais `en_US`. Changement de langue à l’exécution. Les boîtes‑dialogue natives Qt sont aussi localisées.
- **Adaptation thème clair‑sombre** : suivre le système ou basculement manuel. Les masques de superposition, séparateurs de menu et ombres de fenêtre s’adaptent en conséquence.
- **Mutex d’instance unique** : Mutex global empêche les lancements multiples de l’application. Un argument spécial saute la vérification du mutex pendant le redémarrage de mise à jour.
- **Compatibilité chemins UTF‑8 chinois** : solution de contournement pour le bogue API native Qt qui peut renvoyer des valeurs vides sur des chemins Windows UTF‑8 contenant des caractères chinois.

---
## Aperçu
| Zone de capture | Annotation autocollant | Résultat OCR |
| --- | --- | --- |
| ![capture](./docs/images/capture.png) | ![sticker](./docs/images/sticker.png) | ![ocr](./docs/images/ocr.png) |

## Téléchargement et installation
1. Obtenez la dernière version sur [Releases](https://github.com/xuhuandeshilinghun/snip‑ocr/releases) : `SnapOCR‑<version>‑win64.zip`
2. Décompressez l’archive dans n’importe quel dossier local. Logiciel portable, pas d’installeur requis.
3. Téléchargez l’archive du modèle OCR séparément et décompressez‑la dans le sous‑dossier `model/` du programme.
4. Lancez `SnapOCR.exe`.

> **Exigence système : Windows 10 / Windows 11 x64**
> Les fichiers de modèle OCR sont volumineux et ne sont pas inclus dans l’archive principale du programme.
> Évitez de décompresser dans des dossiers protégés système comme `C:\Windows` ou `Program Files` pour prévenir les problèmes de permissions lecture‑écriture.

## Raccourcis clavier
| Raccourci | Fonction |
|---|---|
| `F1` | Lancer le mode capture d’écran |
| `Ctrl+Shift+S` | Capture d’écran + exécuter reconnaissance OCR |
| `Esc` | Quitter / annuler la capture |
| `Molette souris` | Réglage fin de la sélection de 1 pixel |
| `C` | Copier la valeur de couleur de la pipette de la loupe |

## Pile technique
- Langage : C++17
- Cadre GUI : Qt 6.5.3+
- Système de build : CMake 3.20+ / MSVC
- Inférence OCR : ONNX Runtime 1.18.1 CPU
- Traitement d’image : OpenCV 4.x
- Analyse JSON : nlohmann/json
- Compression‑décompression Zip : minizip
- Interfaces système Windows : API Win32, DWM, hook clavier bas‑niveau
- Compilateur : MSVC2019 / MSVC2022

## Notes de confidentialité et sécurité
1. Le traitement des captures, le rendu des autocollants et la reconnaissance OCR s’exécutent entièrement localement. Le logiciel **n’envoie jamais automatiquement de captures ou de textes reconnus sur internet.**
2. La fonction de mise à jour automatique se connecte aux serveurs GitHub / Gitee exclusivement pour la validation de version et le téléchargement des paquets de mise à jour.
3. Les captures originales sont stockées en PNG brut dans le dossier `history`. Nettoyez manuellement ce dossier si vous avez capturé des mots‑de‑passe, documents d’identité ou autres contenus sensibles.
4. Les fichiers `.sp1` et `index.dat` utilisent une simple obfuscation XOR+base64 **ce n’est pas un chiffrement sécurisé. Ne stockez pas de données hautement confidentielles dans ces fichiers.**
5. Vous pouvez désactiver l’enregistrement de l’historique des captures dans les paramètres de l’application.

## Limitations connues
- Seul Windows 10 / 11 x64 est pris en charge actuellement. Linux et macOS ne sont pas disponibles.
- L’OCR utilise le CPU ; la vitesse de reconnaissance dépend des performances de votre CPU.
- Les fichiers de modèle OCR sont volumineux et doivent être téléchargés et déployés manuellement.
- Les configurations multi‑écrans DPI mixtes doivent être testées en conditions réelles ; des problèmes de compatibilité peuvent apparaître sur certains matériels rares.

## Contribuer
Les rapports de bogues, demandes de fonctionnalités (Issues) et Pull Requests sont bienvenus.
1. Fork ce dépôt
2. Créez une branche fonctionnalité : `git checkout -b feature/your‑feature‑name`
3. Validez vos modifications
4. Poussez la branche et soumettez un PR

> Consignes de soumission : ne validez pas les artefacts de compilation, le contenu du dossier build ou les gros fichiers de modèle. Préférez git submodule pour les bibliothèques tierces.

## Développeur : Compilation, empaquetage et publication

> Non destiné aux utilisateurs finaux, réservé aux mainteneurs du projet.

### Prétraitement des ressources avant publication

Exécutez **obligatoirement** ces scripts de compilation des ressources avant chaque empaquetage de version :

```
# Compiler les ressources de traduction multilingue
python tools\compile_locales.py
# Empaqueter les ressources d’images
python tools\pack_img_res.py
```

### Logique de sélection du paquet de mise à jour (règles du programme de mise à jour automatique)

| Version installée localement | Type de paquet à télécharger |
| --- | --- |
| V1.0.1 (version officielle précédente) | Paquet delta (le plus léger, quelques Mo seulement) |
| V1.0.0 (version de référence) | Paquet complet (calcul des différences par rapport à la référence 1.0.0) |
| Autres anciennes versions | Paquet complet (calcul des différences par rapport à la référence 1.0.0) |
| Non installé | **La version de référence V1.0.0 doit d’abord être installée** |

> ⚠️ `build_release_1.0.0` est le répertoire de référence global. **Générez‑le une seule fois, conservez‑le définitivement, ne le supprimez ni ne le modifiez**. Toutes les mises à jour delta ultérieures dépendent de cette référence.

### Exemples de script make_release.py

```
# 1.【À exécuter une seule fois】Créer la version de référence 1.0.0
python tools/make_release.py --version 1.0.0 --no-build

# 2.Publier la version 1.0.1 (référence fixée à 1.0.0, comparaison avec l’ancien build_release_1.0.0)
python tools/make_release.py --version 1.0.1 --make-update --old build_release_1.0.0 --base-dir build_release_1.0.0 --no-build

# 3.Publier la version 1.0.2 (référence fixée à 1.0.0, comparaison avec l’ancien build_release_1.0.1)
python tools/make_release.py --version 1.0.2 --make-update --old build_release_1.0.1 --base-dir build_release_1.0.0 --no-build

# 4.Publier la version 1.0.3 (référence fixée à 1.0.0, comparaison avec l’ancien build_release_1.0.2)
python tools/make_release.py --version 1.0.3 --make-update --old build_release_1.0.2 --base-dir build_release_1.0.0 --no-build

# Commandes fréquemment utilisées
# Flux complet : compilation + collecte des artefacts de publication
python tools/make_release.py --version 1.0.1

# Compiler + générer le paquet complet et le paquet delta de mise à jour
python tools/make_release.py --version 1.0.1 --make-update --old build_release_1.0.0

# Binaires déjà compilés, empaquetage uniquement, ignorer l’étape de compilation
python tools/make_release.py --version 1.0.1 --no-build

# Générer les paquets de mise à jour avec notes de version et drapeau de mise à jour forcée activé
python tools/make_release.py --version 1.0.1 --make-update --old build_release_1.0.0 --update-log "1.Optimiser l’interface utilisateur\n2.Corriger les bogues connus" --force-update
```

### Référence succincte des paramètres

- `--version x.x.x` : définir le numéro de version cible de la publication
- `--no-build` : ignorer la compilation C++, réaliser uniquement l’empaquetage (utiliser quand les binaires sont déjà construits)
- `--make-update` : activer la génération du paquet delta ; produit à la fois le paquet complet et le correctif delta
- `--old <dir>` : répertoire de la version officielle précédente, utilisé pour la comparaison de fichiers afin de générer le correctif delta
- `--base-dir <dir>` : répertoire de référence global, toujours renseigné avec `build_release_1.0.0`
- `--update-log` : notes de version de cette version ; utiliser `\n` pour les sauts de ligne
- `--force-update` : activer le drapeau de mise à jour forcée ; le client imposera la mise à niveau

> Les fichiers issus de la compilation sont générés dans `build_release_<version>`. Téléversez les archives ZIP de ce dossier sur GitHub Releases.

## Licence
Licence MIT, voir le fichier `LICENSE` à la racine du dépôt.

## Soutenir le projet
SnapOCR est entièrement open‑source et gratuit pour un usage personnel. Si vous trouvez cet outil utile, vous pouvez soutenir volontairement son développement via Ko‑fi :
👉 [https://ko‑fi.com/edenls](https://ko‑fi.com/edenls)

## Contact et retours
‑ Courriel : senjin@foxmail.com
‑ Il est recommandé de signaler les bogues et les suggestions de fonctionnalités via GitHub Issues.
