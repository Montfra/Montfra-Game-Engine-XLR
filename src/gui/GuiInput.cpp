// GuiInput.cpp - Implementation of GuiInput

#include "GuiInput.h"
#include "GuiButton.h" // chain callbacks for backward compatibility

#include <GLFW/glfw3.h>

double GuiInput::s_mouse_x_px = 0.0;
double GuiInput::s_mouse_y_px = 0.0;
bool   GuiInput::s_left_down = false;
bool   GuiInput::s_left_clicked = false;
std::array<bool, 512> GuiInput::s_key_down{};
std::array<bool, 512> GuiInput::s_key_pressed{};
std::vector<unsigned int> GuiInput::s_chars;

void GuiInput::begin_frame()
{
    s_left_clicked = false;
    s_chars.clear();
    s_key_pressed.fill(false);
}

void GuiInput::glfw_cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
    // Convert from window coords (origin top-left) to framebuffer pixel coords (origin bottom-left)
    int ww=1, wh=1, fbw=1, fbh=1;
    glfwGetWindowSize(window, &ww, &wh);
    glfwGetFramebufferSize(window, &fbw, &fbh);
    const double sx = ww > 0 ? (static_cast<double>(fbw) / static_cast<double>(ww)) : 1.0;
    const double sy = wh > 0 ? (static_cast<double>(fbh) / static_cast<double>(wh)) : 1.0;
    s_mouse_x_px = xpos * sx;
    const double ypos_px_top = ypos * sy;
    s_mouse_y_px = static_cast<double>(fbh) - ypos_px_top;

    // Keep GuiButton in sync for existing code
    GuiButton::glfw_cursor_pos_callback(window, xpos, ypos);
}

void GuiInput::glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    (void)mods;
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            // Ensure we have current mouse position
            double xpos=0.0, ypos=0.0;
            glfwGetCursorPos(window, &xpos, &ypos);
            int ww=1, wh=1, fbw=1, fbh=1;
            glfwGetWindowSize(window, &ww, &wh);
            glfwGetFramebufferSize(window, &fbw, &fbh);
            const double sx = ww > 0 ? (static_cast<double>(fbw) / static_cast<double>(ww)) : 1.0;
            const double sy = wh > 0 ? (static_cast<double>(fbh) / static_cast<double>(wh)) : 1.0;
            s_mouse_x_px = xpos * sx;
            const double ypos_px_top = ypos * sy;
            s_mouse_y_px = static_cast<double>(fbh) - ypos_px_top;

            s_left_down = true;
            s_left_clicked = true;
        } else if (action == GLFW_RELEASE) {
            s_left_down = false;
        }
    }

    GuiButton::glfw_mouse_button_callback(window, button, action, mods);
}

void GuiInput::glfw_key_callback(GLFWwindow* /*window*/, int key, int /*scancode*/, int action, int /*mods*/)
{
    if (key >= 0 && key < static_cast<int>(s_key_down.size())) {
        if (action == GLFW_PRESS) {
            s_key_down[static_cast<size_t>(key)] = true;
            s_key_pressed[static_cast<size_t>(key)] = true;
        } else if (action == GLFW_RELEASE) {
            s_key_down[static_cast<size_t>(key)] = false;
        }
    }
}

void GuiInput::glfw_char_callback(GLFWwindow* /*window*/, unsigned int codepoint)
{
    s_chars.push_back(codepoint);
}

std::pair<double,double> GuiInput::mouse_pos_px()
{
    return {s_mouse_x_px, s_mouse_y_px};
}

bool GuiInput::left_down() { return s_left_down; }
bool GuiInput::left_clicked() { return s_left_clicked; }

bool GuiInput::key_down(int key)
{
    if (key >= 0 && key < static_cast<int>(s_key_down.size()))
        return s_key_down[static_cast<size_t>(key)];
    return false;
}

bool GuiInput::key_pressed(int key)
{
    if (key >= 0 && key < static_cast<int>(s_key_pressed.size()))
        return s_key_pressed[static_cast<size_t>(key)];
    return false;
}

std::vector<unsigned int> GuiInput::consume_chars()
{
    std::vector<unsigned int> out;
    out.swap(s_chars);
    return out;
}

