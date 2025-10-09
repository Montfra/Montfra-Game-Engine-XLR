# MGE_XLR — C++ Minimal OpenGL (GLFW + GLAD)

Projet C++ minimal multiplateforme (Windows/macOS/Linux) utilisant OpenGL 3.3 Core, GLFW pour la fenêtre/saisie, et GLAD pour le chargement d'OpenGL. Il ouvre une fenêtre, rend un triangle, gère Échap (quitter), F (plein écran ⇄ fenêtré), et adapte le viewport/la projection au redimensionnement.

Ajout: rendu de texte 2D (HUD) basé sur FreeType via la classe `Text` (voir `src/text/Text.*`). La démo affiche « Hello Text! » en haut à gauche.

## Caractéristiques
- OpenGL 3.3 Core Profile (GLAD)
- GLFW: création de fenêtre, clavier, bascule plein écran, fenêtre sans bordure
- Contenu responsive: viewport et matrice de projection mis à jour sur resize
- C++17, CMake pur (pas de dépendance IDE)

## Contenu du repo
- `CMakeLists.txt` — configuration CMake (FetchContent pour GLAD/GLFW) + FreeType
- `src/main.cpp` — application principale (triangle + input + resize) + démo texte
- `src/text/Text.h`, `src/text/Text.cpp` — classe de rendu de texte (FreeType + OpenGL)
- `shaders/vertex.glsl`, `shaders/fragment.glsl` — shaders GLSL (triangle)

## Dépendances
- Outils: `cmake`, un compilateur C++17 (GCC/Clang/MSVC), `git`
- OpenGL runtime selon la plateforme
- Linux: bibliothèques de développement X11 (ou Wayland) si compilation de GLFW à partir des sources

GLFW et GLAD sont récupérés automatiquement via `FetchContent` (option par défaut). Sur Linux, la configuration force X11 et désactive Wayland pour éviter la dépendance à `wayland-scanner` si Wayland n'est pas installé.

FreeType est requis pour le rendu de texte. La démo tente de sélectionner automatiquement une police système courante (DejaVuSans, FreeSans, LiberationSans, Arial/Helvetica) selon l'OS. Vous pouvez forcer la police via `Text::set_text_font("/chemin/vers/police.ttf")`.

## Compilation

### Linux (Debian/Ubuntu)
- Installer les dépendances de build et X11:
  - `sudo apt update`
  - `sudo apt install -y build-essential cmake git libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev mesa-common-dev libgl1-mesa-dev`
- Installer FreeType (rendu texte):
  - `sudo apt install -y libfreetype6-dev`
- Générer et compiler:
  - `cmake -S . -B build -DUSE_FETCHCONTENT=ON`
  - `cmake --build build -j`
- Lancer:
  - `./build/MGE_XLR`

Notes:
- Si vous préférez vos paquets systèmes GLFW/GLAD, désactivez le fetching: `-DUSE_FETCHCONTENT=OFF` et assurez-vous que `glfw3` (avec un package CMake config) et un target `glad` existent dans votre environnement.
- En environnement Linux sans serveur d’affichage (ex: CI, conteneur), l’exécutable détecte l’absence de `DISPLAY`/connexion et quitte proprement en indiquant que l’exécution graphique est sautée.

### macOS
- Installer outils:
  - `brew install cmake git`
  - (Optionnel) `brew install glfw` si vous souhaitez utiliser une version système
- Installer FreeType:
  - `brew install freetype`
- Générer et compiler:
  - `cmake -S . -B build -DUSE_FETCHCONTENT=ON`
  - `cmake --build build -j`
- Lancer:
  - `./build/MGE_XLR`

### Windows

#### Visual Studio (recommandé)
- Installer Visual Studio 2022 (Workload: Desktop development with C++)
- Installer CMake et Git (si besoin):
  - `winget install Kitware.CMake Git.Git`
- Générer et compiler:
  - `cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DUSE_FETCHCONTENT=ON`
- `cmake --build build --config Release`
- Installer FreeType avec vcpkg (optionnel) ou via un package manager:
  - vcpkg: `vcpkg install freetype` puis `-DCMAKE_TOOLCHAIN_FILE=.../vcpkg.cmake`
- Lancer:
  - `build\Release\MGE_XLR.exe`

#### MSYS2/MinGW
- Installer MSYS2, puis:
  - `pacman -S --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake git`
- Générer et compiler (Invite MinGW64):
  - `cmake -S . -B build -G "MinGW Makefiles" -DUSE_FETCHCONTENT=ON`
  - `cmake --build build -j`
- Lancer:
  - `build\MGE_XLR.exe`

## Utilisation
- Échap: quitter
- `F`: bascule plein écran ⇄ fenêtré
- Fenêtre sans bordure: modifier `set_window_frameless(true)` dans `src/main.cpp` avant la création de la fenêtre

### API Texte (HUD)

Exemple rapide (voir `main.cpp`):
- `Text hud;`
- `hud.set_text_font("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");`
- `hud.set_text("Hello Text!");`
- `hud.set_text_size(6); // 1..10`
- `hud.set_text_color(1,1,0,1);`
- `hud.set_position(2, 95, true); // en %`
- Pendant le rendu: `hud.draw();`

Caractéristiques:
- Position en pixels ou % de la taille framebuffer (conserve la position relative au redimensionnement).
- Projection orthographique recalculée sur resize (`Text::on_framebuffer_resized(width,height)` est appelé par le callback).
- Viewport OpenGL mis à jour automatiquement (callback GLFW déjà en place).
- Le texte est rendu en overlay (depth test désactivé) pour rester net et non obstrué par la 3D.

## Détails techniques
- `CMakeLists.txt`
  - Utilise `FetchContent` pour récupérer GLAD (génère automatiquement les bindings GL 3.3 core) et GLFW.
  - Sur Linux, force `GLFW_BUILD_WAYLAND=OFF` et `GLFW_BUILD_X11=ON` pour éviter la dépendance `wayland-scanner` si Wayland est absent.
  - Définit la macro `SHADER_DIR` pour référencer les shaders au runtime.
- `src/main.cpp`
  - Initialisation GLFW/GLAD, création fenêtre, callbacks (`framebuffer_size_callback`, `key_callback`).
  - Triangle via VAO/VBO + shaders; `uProjection` mis à jour en fonction du ratio.
  - Toggle plein écran via `glfwSetWindowMonitor` (sauvegarde/restauration taille/position).
  - Détection d’environnement sans affichage (Linux) pour éviter les erreurs d’exécution en CI.

## Problèmes fréquents
- Linux: si la configuration échoue en cherchant Wayland, installez `wayland-scanner`/`libwayland-dev` ou laissez `GLFW_BUILD_WAYLAND=OFF` (défaut ici).
- En headless, l’exécution graphique n’est pas possible; l’app se termine proprement avec un message d’information.
