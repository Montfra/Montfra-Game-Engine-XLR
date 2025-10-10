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

    GuiText hud2;
    hud2.set_text("v0.3");
    hud2.set_text_size(2);
    hud2.set_text_color(1.0f, 0.6f, 0.3f, 1.0f);

    {
        std::string font_path = choose_default_font();
        if (font_path.empty()) {
            std::fprintf(stderr, "[INFO] Aucune police système trouvée automatiquement. \n"
                                 "       Utilisez set_text_font() avec un chemin .ttf/.otf.\n");
        } else {
            hud.set_text_font(font_path);
            hud2.set_text_font("resources/Jersey25-Regular.ttf");
        }
    }
    hud.set_text_font("resources/Jersey25-Regular.ttf");
    hud2.set_text_font("resources/Jersey25-Regular.ttf");

    // Crée un panneau pour contenir les textes (HUD container)
    GuiPanel panel;
    panel.set_position(20.0f, 20.0f, false); // bottom-left corner
    panel.set_size(200.0f, 120.0f, false);
    panel.setBackgroundColor(0.05f, 0.05f, 0.06f, 0.75f);
    panel.setBorderColor(0.9f, 0.9f, 0.95f, 0.25f);
    panel.setBorderRadius(8.0f);
    panel.setBorderThickness(1.5f);
    panel.setLayout(GuiPanel::LayoutType::VERTICAL);
    panel.setPadding(0.0f);
    panel.setSpacing(4.0f);
    panel.addChild(&hud);
    panel.addChild(&hud2);

    // Button in the same panel
    GuiButton button;
    button.set_text("Clique moi :)");
    button.set_text_font("resources/Jersey25-Regular.ttf");
    button.set_text_size(3);
    button.set_bg_color(0.20f, 0.25f, 0.35f, 0.90f);
    button.set_hover_color(0.30f, 0.40f, 0.55f, 0.95f);
    button.set_padding(10.0f, 6.0f);
    button.set_corner_radius(6.0f);
    button.set_on_click([](){ std::puts("Click"); });
    panel.addChild(&button);
    
    
    
    GuiText hud3;
    hud3.set_text("Panel1");
    hud3.set_text_size(2);
    hud3.set_text_color(1.0f, 0.6f, 0.3f, 1.0f);
    hud3.set_text_font("resources/Jersey25-Regular.ttf");
    
    GuiText hud4;
    hud4.set_text("Panel2");
    hud4.set_text_size(2);
    hud4.set_text_color(1.0f, 0.6f, 0.3f, 1.0f);
    hud4.set_text_font("resources/Jersey25-Regular.ttf");
    
    GuiPanel panel1;
    panel1.set_position(10.0f, 70.0f, true); // bottom-left corner
    panel1.set_size(20.0f, 20.0f, true);
    panel1.setBackgroundColor(0.15f, 0.5f, 0.06f, 0.75f);
    panel1.setBorderColor(0.2f, 0.9f, 0.95f, 0.25f);
    panel1.setBorderRadius(3.0f);
    panel1.setBorderThickness(1.0f);
    panel1.setLayout(GuiPanel::LayoutType::HORIZONTAL);
    panel1.setPadding(5.0f);
    panel1.setSpacing(5.0f);
    panel1.addChild(&hud3);
    
    
    GuiPanel panel2;
    panel2.set_position(10.0f, 80.0f, true); // bottom-left corner
    panel2.set_size(10.0f, 10.0f, true);
    panel2.setBackgroundColor(0.15f, 0.8f, 0.6f, 0.75f);
    panel2.setBorderColor(0.1f, 0.02f, 0.55f, 0.25f);
    panel2.setBorderRadius(9.0f);
    panel2.setBorderThickness(5.0f);
    panel2.setLayout(GuiPanel::LayoutType::VERTICAL);
    panel2.setPadding(0.0f);
    panel2.setSpacing(0.0f);
    panel2.addChild(&hud4);
    
    panel1.addChild(&panel2);
    

    // 7) Boucle principale
    while (!glfwWindowShouldClose(window)) {
        // Reset input one-shot flags, then poll events to fill them
        GuiButton::begin_frame();
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

        // Dessin du panneau (dessine aussi les enfants)
        panel.draw();
        panel1.draw();

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
    GuiButton::glfw_cursor_pos_callback(window, xpos, ypos);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    (void)window; (void)mods;
    GuiButton::glfw_mouse_button_callback(window, button, action, mods);
}

void key_callback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
{
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
