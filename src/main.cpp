// MGE_XLR - Minimal GLFW + GLAD + OpenGL 3.3 example
// - Fenêtre OpenGL (bordée ou sans bordure via set_window_frameless)
// - Triangle simple rendu avec shaders
// - Échap pour quitter, F pour bascule plein écran/fenêtré
// - Redimensionnement : viewport + matrice de projection mis à jour dynamiquement

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "gui/GuiText.h"
#include "gui/GuiPanel.h"
#include "gui/GuiButton.h"
#include "gui/GuiInput.h"
#include "gui/GuiImage.h"
#include "gui/GuiInputText.h"
#include "gui/GuiSlider.h"
#include "gui/GuiCheckbox.h"
#include "gui/GuiProgressBar.h"
#include "gui/GuiMenuBar.h"
#include "gui/GuiManager.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <fstream>
#include <sstream>
#include <array>
#include <cmath>

#ifndef SHADER_DIR
#  define SHADER_DIR "./shaders"
#endif

// État global minimal de la fenêtre
static bool g_frameless = false;
static bool g_fullscreen = false;
static int  g_windowed_x = 100, g_windowed_y = 100;
static int  g_windowed_w = 1280, g_windowed_h = 720;

// Prototypes
void set_window_frameless(bool frameless);
GLFWwindow* create_window(int w, int h, const char* title);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void toggle_fullscreen(GLFWwindow* window);
// Forward input to GuiButton
void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void char_callback(GLFWwindow* window, unsigned int codepoint);

std::string load_text_file(const std::string& path);
GLuint compile_shader(GLenum type, const std::string& src);
GLuint link_program(GLuint vs, GLuint fs);

// Matrice de projection perspective (colonne-major pour OpenGL)
std::array<float, 16> make_perspective(float fov_deg, float aspect, float znear, float zfar)
{
    const float pi = 3.14159265359f;
    float f = 1.0f / std::tan((fov_deg * pi / 180.0f) * 0.5f);
    std::array<float,16> m{};
    m[0]  = f / aspect; m[4]  = 0.0f; m[8]  = 0.0f;                                 m[12] = 0.0f;
    m[1]  = 0.0f;       m[5]  = f;    m[9]  = 0.0f;                                 m[13] = 0.0f;
    m[2]  = 0.0f;       m[6]  = 0.0f; m[10] = (zfar + znear) / (znear - zfar);     m[14] = (2.0f * zfar * znear) / (znear - zfar);
    m[3]  = 0.0f;       m[7]  = 0.0f; m[11] = -1.0f;                                m[15] = 0.0f;
    return m;
}

int main()
{
    // Environnement headless (ex: CI, conteneur sans serveur graphique)
#if defined(__linux__)
    const char* disp = std::getenv("DISPLAY");
    const char* wayl = std::getenv("WAYLAND_DISPLAY");
    if (!disp && !wayl) {
        std::fprintf(stdout, "Environnement sans affichage détecté (pas de DISPLAY/WAYLAND_DISPLAY). Exécution sautée.\n");
        return EXIT_SUCCESS; // Quitte proprement pour éviter une erreur dans les environnements headless
    }
#endif

    // 1) Init GLFW
    if (!glfwInit()) {
        std::fprintf(stdout, "GLFW n'a pas pu s'initialiser (probable absence d'affichage). Exécution sautée.\n");
        return EXIT_SUCCESS;
    }

    // Sélection du contexte OpenGL 3.3 core
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE); // nécessaire sur macOS
#endif
    glfwWindowHint(GLFW_DECORATED, g_frameless ? GLFW_FALSE : GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    // Facultatif : autoriser framebuffer Retina sur macOS
#ifdef __APPLE__
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
#endif

    // Configure le style de fenêtre via API
    set_window_frameless(false); // changer en true pour fenêtre sans bordure

    // 2) Création de la fenêtre
    GLFWwindow* window = create_window(g_windowed_w, g_windowed_h, "MGE_XLR - OpenGL 3.3");
    if (!window) {
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // vsync

    // 3) Init GLAD après avoir un contexte courant
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::fprintf(stderr, "[ERREUR] Échec du chargement GLAD.\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    // 4) Callbacks
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCharCallback(window, char_callback);

    // 5) Géométrie simple (triangle)
    const float vertices[] = {
        // positions (x,y,z) en espace monde (z négatif dans le frustum)
        -0.5f, -0.5f, -2.0f,
         0.5f, -0.5f, -2.0f,
         0.0f,  0.5f, -2.0f,
    };

    GLuint vao = 0, vbo = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // 6) Shaders (chargés depuis le dossier shaders)
    const std::string vert_src = load_text_file(std::string(SHADER_DIR) + "/vertex.glsl");
    const std::string frag_src = load_text_file(std::string(SHADER_DIR) + "/fragment.glsl");
    if (vert_src.empty() || frag_src.empty()) {
        std::fprintf(stderr, "[ERREUR] Impossible de charger les shaders depuis '%s'.\n", SHADER_DIR);
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    GLuint vs = compile_shader(GL_VERTEX_SHADER,   vert_src);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, frag_src);
    GLuint prog = link_program(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!prog) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    // Récupération de l'uniforme de projection
    GLint uProjLoc = glGetUniformLocation(prog, "uProjection");

    glEnable(GL_DEPTH_TEST);

    // Viewport initiale
    int fbw = 0, fbh = 0;
    glfwGetFramebufferSize(window, &fbw, &fbh);
    glViewport(0, 0, fbw, fbh);

    // 6bis) Préparer un texte HUD (utilise FreeType)
    auto file_exists = [](const std::string& p) {
        std::ifstream f(p, std::ios::binary);
        return f.good();
    };
    auto choose_default_font = [&]() -> std::string {
#ifdef __APPLE__
        const char* candidates[] = {
            "/System/Library/Fonts/Supplemental/Arial.ttf",
            "/System/Library/Fonts/Supplemental/Helvetica.ttc",
            "/Library/Fonts/Arial.ttf",
        };
#elif defined(_WIN32)
        const char* candidates[] = {
            "C:/Windows/Fonts/arial.ttf",
            "C:/Windows/Fonts/segoeui.ttf",
        };
#else
        const char* candidates[] = {
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        };
#endif
        for (const char* c : candidates) if (file_exists(c)) return std::string(c);
        return std::string();
    };

    GuiText hud;
    hud.set_text("MGE-XLR");
    hud.set_text_size(4);
    hud.set_text_color(0.9f, 0.9f, 0.95f, 0.95f);

    {
        std::string font_path = choose_default_font();
        if (font_path.empty()) {
            std::fprintf(stderr, "[INFO] Aucune police système trouvée automatiquement. \n"
                                 "       Utilisez set_text_font() avec un chemin .ttf/.otf.\n");
        } else {
            hud.set_text_font(font_path);
        }
    }
    hud.set_text_font("resources/Jersey25-Regular.ttf");

    // Crée un panneau pour contenir les textes (HUD container)
    // Demo GUI Panel containing all new widgets
    GuiPanel panel;
    panel.set_position(20.0f, 20.0f, false);
    panel.set_size(540.0f, 380.0f, false);
    panel.setBackgroundColor(0.05f, 0.05f, 0.06f, 0.75f);
    panel.setBorderColor(0.9f, 0.9f, 0.95f, 0.25f);
    panel.setBorderRadius(8.0f);
    panel.setBorderThickness(1.5f);
    panel.setLayout(GuiPanel::LayoutType::VERTICAL);
    panel.setPadding(6.0f);
    panel.setSpacing(8.0f);

    // MenuBar inside panel
    GuiMenuBar menubar;
    menubar.set_text_font("resources/Jersey25-Regular.ttf");
    menubar.set_text_size(3);
    menubar.add_menu("Fichier");
    menubar.add_menu_item("Fichier", "Nouveau", [](){ std::puts("Menu: Fichier > Nouveau"); });
    menubar.add_menu_item("Fichier", "Ouvrir", [](){ std::puts("Menu: Fichier > Ouvrir"); });
    menubar.add_menu_item("Fichier", "Quitter", [](){ std::puts("Menu: Fichier > Quitter"); });
    panel.addChild(&menubar);

    // Title text
    panel.addChild(&hud);

    // InputText with placeholder
    GuiInputText input;
    input.set_text_font("resources/Jersey25-Regular.ttf");
    input.set_text_size(3);
    input.set_placeholder("Tapez votre nom...");
    input.set_on_text_change([](const std::string& s){ std::printf("[Input] text=""%s""\n", s.c_str()); });
    panel.addChild(&input);

    // Image
    GuiImage image;
    image.set_texture("resources/icon.ppm");
    image.set_size(64, 64);
    panel.addChild(&image);

    // Slider controlling value shown in text
    GuiText value_text; value_text.set_text_font("resources/Jersey25-Regular.ttf"); value_text.set_text_size(3); value_text.set_text_color(0.9f,0.9f,0.95f,1.0f);
    value_text.set_text("Slider: 0.0");
    GuiSlider slider; slider.set_range(0.0f, 100.0f); slider.set_value(0.0f);
    slider.set_on_value_changed([&](float v){ char buf[64]; std::snprintf(buf, sizeof(buf), "Slider: %.1f", v); value_text.set_text(buf); std::printf("[Slider] value=%.3f\n", v); });
    panel.addChild(&value_text);
    panel.addChild(&slider);

    // Checkbox toggling console action
    GuiCheckbox checkbox; checkbox.set_text_font("resources/Jersey25-Regular.ttf"); checkbox.set_text_size(3); checkbox.set_label("Activer logs");
    bool logs_enabled = true;
    checkbox.set_checked(logs_enabled);
    checkbox.set_on_toggle([&](bool checked){ logs_enabled = checked; std::printf("[Checkbox] logs %s\n", checked?"ON":"OFF"); });
    panel.addChild(&checkbox);

    // Progress bar updated dynamically
    GuiProgressBar progress; progress.set_text_font("resources/Jersey25-Regular.ttf"); progress.set_text_size(3);
    panel.addChild(&progress);

    // --- Désactive l'ancien panneau de démo pour cette démonstration de pages ---
    panel.hide();

    // -----------------------------
    // Démo : Gestion de pages GUI
    // -----------------------------
    GuiManager guiManager;

    // Page 1: Main Menu
    GuiPanel mainMenu;
    mainMenu.set_position(40.0f, 40.0f, false);
    mainMenu.set_size(460.0f, 300.0f, false);
    mainMenu.setBackgroundColor(0.05f, 0.05f, 0.06f, 0.80f);
    mainMenu.setBorderColor(0.9f, 0.9f, 0.95f, 0.25f);
    mainMenu.setBorderRadius(8.0f);
    mainMenu.setBorderThickness(1.5f);
    mainMenu.setLayout(GuiPanel::LayoutType::VERTICAL);
    mainMenu.setPadding(10.0f);
    mainMenu.setSpacing(12.0f);

    GuiText titleMain; titleMain.set_text_font("resources/Jersey25-Regular.ttf"); titleMain.set_text_size(6);
    titleMain.set_text("Main Menu"); titleMain.set_text_color(0.95f,0.95f,1.0f,1.0f);
    mainMenu.addChild(&titleMain);

    GuiButton btnPlay; btnPlay.set_text_font("resources/Jersey25-Regular.ttf"); btnPlay.set_text_size(4);
    btnPlay.set_text("Play"); btnPlay.set_corner_radius(6.0f);
    btnPlay.set_on_click([](){ std::puts("[Main Menu] Play clicked"); });
    mainMenu.addChild(&btnPlay);

    GuiButton btnOptions; btnOptions.set_text_font("resources/Jersey25-Regular.ttf"); btnOptions.set_text_size(4);
    btnOptions.set_text("Options"); btnOptions.set_corner_radius(6.0f);
    btnOptions.set_on_click([&guiManager](){ guiManager.setActivePage("Options Menu"); });
    mainMenu.addChild(&btnOptions);

    GuiButton btnQuit; btnQuit.set_text_font("resources/Jersey25-Regular.ttf"); btnQuit.set_text_size(4);
    btnQuit.set_text("Quit"); btnQuit.set_corner_radius(6.0f);
    btnQuit.set_on_click([&](void){ glfwSetWindowShouldClose(window, GLFW_TRUE); });
    mainMenu.addChild(&btnQuit);

    // Page 2: Options Menu
    GuiPanel optionsMenu;
    optionsMenu.set_position(40.0f, 40.0f, false);
    optionsMenu.set_size(460.0f, 260.0f, false);
    optionsMenu.setBackgroundColor(0.06f, 0.06f, 0.08f, 0.80f);
    optionsMenu.setBorderColor(0.9f, 0.9f, 0.95f, 0.25f);
    optionsMenu.setBorderRadius(8.0f);
    optionsMenu.setBorderThickness(1.5f);
    optionsMenu.setLayout(GuiPanel::LayoutType::VERTICAL);
    optionsMenu.setPadding(10.0f);
    optionsMenu.setSpacing(12.0f);

    GuiText titleOptions; titleOptions.set_text_font("resources/Jersey25-Regular.ttf"); titleOptions.set_text_size(6);
    titleOptions.set_text("Options Menu"); titleOptions.set_text_color(0.95f,0.95f,1.0f,1.0f);
    optionsMenu.addChild(&titleOptions);

    GuiText optLabel; optLabel.set_text_font("resources/Jersey25-Regular.ttf"); optLabel.set_text_size(4);
    optLabel.set_text("(Exemple) Réglages à venir...");
    optionsMenu.addChild(&optLabel);

    GuiButton btnBack; btnBack.set_text_font("resources/Jersey25-Regular.ttf"); btnBack.set_text_size(4);
    btnBack.set_text("Back"); btnBack.set_corner_radius(6.0f);
    btnBack.set_on_click([&guiManager](){ guiManager.setActivePage("Main Menu"); });
    optionsMenu.addChild(&btnBack);

    // Add pages and set initial active
    guiManager.addPage(mainMenu, "Main Menu");
    guiManager.addPage(optionsMenu, "Options Menu");
    guiManager.setActivePage("Main Menu");
    

    // 7) Boucle principale
    while (!glfwWindowShouldClose(window)) {
        // Reset input one-shot flags, then poll events to fill them
        GuiButton::begin_frame(); // keep old button system in sync if used
        GuiInput::begin_frame();
        glfwPollEvents();

        // Calcul projection responsive à chaque frame (aspect peut changer)
        glfwGetFramebufferSize(window, &fbw, &fbh);
        float aspect = (fbh > 0) ? (static_cast<float>(fbw) / static_cast<float>(fbh)) : 1.0f;
        auto proj = make_perspective(60.0f, aspect, 0.1f, 100.0f);

        glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(prog);
        if (uProjLoc >= 0) {
            glUniformMatrix4fv(uProjLoc, 1, GL_FALSE, proj.data());
        }

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // Update progress for demo
        {
            double t = glfwGetTime();
            float p = static_cast<float>( (std::sin(t)*0.5 + 0.5) * 100.0 );
            progress.set_progress(p);
        }

        // Dessin du panneau (désactivé via panel.hide()) puis de la page active
        panel.draw();
        guiManager.draw();

        glfwSwapBuffers(window);
    }

    // Nettoyage
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(prog);

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}

void set_window_frameless(bool frameless)
{
    // Enregistre l'intention. L'attribut décoré est appliqué à la création
    // et synchronisé après création si possible.
    g_frameless = frameless;
}

GLFWwindow* create_window(int w, int h, const char* title)
{
    GLFWwindow* window = glfwCreateWindow(w, h, title, nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "[ERREUR] glfwCreateWindow a échoué.\n");
        return nullptr;
    }

    // Position par défaut (uniquement pour mode fenêtré)
    glfwSetWindowPos(window, g_windowed_x, g_windowed_y);

    // Essaye d'appliquer/définir la décoration au runtime si supporté
    glfwSetWindowAttrib(window, GLFW_DECORATED, g_frameless ? GLFW_FALSE : GLFW_TRUE);

    return window;
}

void framebuffer_size_callback(GLFWwindow* /*window*/, int width, int height)
{
    // Important : utiliser la taille du framebuffer (pixels), pas celle de la fenêtre (points)
    glViewport(0, 0, width, height);
    // Notifier le module texte pour recalculer la projection orthographique
    GuiText::on_framebuffer_resized(width, height);
}

void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
    GuiInput::glfw_cursor_pos_callback(window, xpos, ypos);
    // Also forward to button system for hover/click detection
    GuiButton::glfw_cursor_pos_callback(window, xpos, ypos);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    GuiInput::glfw_mouse_button_callback(window, button, action, mods);
    // Also forward to button system for hover/click detection
    GuiButton::glfw_mouse_button_callback(window, button, action, mods);
}

void char_callback(GLFWwindow* window, unsigned int codepoint)
{
    (void)window;
    GuiInput::glfw_char_callback(window, codepoint);
}

void key_callback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
{
    // Forward to GUI input system (records both PRESS and RELEASE)
    GuiInput::glfw_key_callback(window, key, 0, action, 0);

    if (action != GLFW_PRESS)
        return; // on déclenche uniquement au press pour éviter les répétitions

    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    } else if (key == GLFW_KEY_F) {
        toggle_fullscreen(window);
    }
}

void toggle_fullscreen(GLFWwindow* window)
{
    if (!g_fullscreen) {
        // Sauvegarde l'état fenêtré
        glfwGetWindowPos(window, &g_windowed_x, &g_windowed_y);
        glfwGetWindowSize(window, &g_windowed_w, &g_windowed_h);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        if (monitor && mode) {
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            g_fullscreen = true;
        }
    } else {
        // Restaure mode fenêtré
        glfwSetWindowMonitor(window, nullptr, g_windowed_x, g_windowed_y, g_windowed_w, g_windowed_h, 0);
        g_fullscreen = false;
    }
}

std::string load_text_file(const std::string& path)
{
    std::ifstream file(path, std::ios::in);
    if (!file.is_open()) {
        std::fprintf(stderr, "[ERREUR] Ouverture du fichier : %s\n", path.c_str());
        return {};
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

GLuint compile_shader(GLenum type, const std::string& src)
{
    GLuint sh = glCreateShader(type);
    const char* csrc = src.c_str();
    glShaderSource(sh, 1, &csrc, nullptr);
    glCompileShader(sh);
    GLint ok = GL_FALSE;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0; glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &len);
        std::string log; log.resize(std::max(1, len));
        glGetShaderInfoLog(sh, len, nullptr, log.data());
        std::fprintf(stderr, "[ERREUR] Compilation shader (%s) :\n%s\n",
                     (type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT"), log.c_str());
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

GLuint link_program(GLuint vs, GLuint fs)
{
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = GL_FALSE;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0; glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
        std::string log; log.resize(std::max(1, len));
        glGetProgramInfoLog(p, len, nullptr, log.data());
        std::fprintf(stderr, "[ERREUR] Édition de liens du programme :\n%s\n", log.c_str());
        glDeleteProgram(p);
        return 0;
    }
    return p;
}
