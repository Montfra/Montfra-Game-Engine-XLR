# MGE_XLR — C++ Minimal OpenGL (GLFW + GLAD)

Projet C++ minimal multiplateforme (Windows/macOS/Linux) utilisant OpenGL 3.3 Core, GLFW pour la fenêtre/saisie, et GLAD pour le chargement d'OpenGL. Il ouvre une fenêtre, rend un triangle, gère Échap (quitter), F (plein écran ⇄ fenêtré), et adapte le viewport/la projection au redimensionnement.

## Caractéristiques
- OpenGL 3.3 Core Profile (GLAD)
- GLFW: création de fenêtre, clavier, bascule plein écran, fenêtre sans bordure
- Contenu responsive: viewport et matrice de projection mis à jour sur resize
- C++17, CMake pur (pas de dépendance IDE)

## Contenu du repo
- `CMakeLists.txt` — configuration CMake (FetchContent pour GLAD/GLFW)
- `src/main.cpp` — application principale (triangle + input + resize)
- `shaders/vertex.glsl`, `shaders/fragment.glsl` — shaders GLSL

## Dépendances
- Outils: `cmake`, un compilateur C++17 (GCC/Clang/MSVC), `git`
- OpenGL runtime selon la plateforme
- Linux: bibliothèques de développement X11 (ou Wayland) si compilation de GLFW à partir des sources

GLFW et GLAD sont récupérés automatiquement via `FetchContent` (option par défaut). Sur Linux, la configuration force X11 et désactive Wayland pour éviter la dépendance à `wayland-scanner` si Wayland n'est pas installé.

## Compilation

### Linux (Debian/Ubuntu)
- Installer les dépendances de build et X11:
  - `sudo apt update`
  - `sudo apt install -y build-essential cmake git libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev mesa-common-dev libgl1-mesa-dev`
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

