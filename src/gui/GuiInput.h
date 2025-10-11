// GuiInput.h - Global input aggregator for GUI widgets (mouse/keyboard/char)
#pragma once

#include <vector>
#include <array>
#include <cstdint>
#include <utility>

struct GLFWwindow;

class GuiInput {
public:
    // Call once per frame BEFORE glfwPollEvents
    static void begin_frame();

    // GLFW wiring (set these as the callbacks on the window)
    static void glfw_cursor_pos_callback(GLFWwindow* window, double xpos, double ypos);
    static void glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void glfw_char_callback(GLFWwindow* window, unsigned int codepoint);

    // Mouse state (in framebuffer pixels, origin bottom-left)
    static std::pair<double,double> mouse_pos_px();
    static bool left_down();
    static bool left_clicked(); // one-shot for the current frame

    // Keyboard
    static bool key_down(int key);      // held state
    static bool key_pressed(int key);   // one-shot for current frame

    // Text input (UTF-32 codepoints) accumulated this frame; consuming clears the buffer
    static std::vector<unsigned int> consume_chars();

private:
    static double s_mouse_x_px;
    static double s_mouse_y_px;
    static bool   s_left_down;
    static bool   s_left_clicked;

    static std::array<bool, 512> s_key_down;
    static std::array<bool, 512> s_key_pressed;
    static std::vector<unsigned int> s_chars; // codepoints from glfwCharCallback
};

