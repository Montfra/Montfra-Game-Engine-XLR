// GuiButton.h - Simple GUI button element with text label
#pragma once

#include <functional>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "GuiElement.h"
#include "GuiText.h"

// GuiButton draws a filled rectangular button and hosts a GuiText label.
// - Layout integrates with GuiPanel via preferred_size()
// - Mouse hover/click handled via GLFW callbacks registered by the app
//   (use glfwSetCursorPosCallback and glfwSetMouseButtonCallback with the
//    provided static functions).
class GuiButton : public GuiElement {
public:
    GuiButton();
    ~GuiButton();

    // Label API (for convenience, forwards to internal GuiText)
    void set_text(const std::string& str) { m_label.set_text(str); }
    bool set_text_font(const std::string& font_path) { return m_label.set_text_font(font_path); }
    void set_text_size(int size_1_to_10) { m_label.set_text_size(size_1_to_10); }
    void set_text_color(float r, float g, float b, float a) { m_label.set_text_color(r,g,b,a); }

    // Button visuals
    void set_bg_color(float r, float g, float b, float a);
    void set_hover_color(float r, float g, float b, float a);
    void set_padding(float px, float py) { m_pad_x = (px < 0 ? 0.f : px); m_pad_y = (py < 0 ? 0.f : py); }
    void set_corner_radius(float r) { m_radius = (r < 0.f ? 0.f : r); }

    // Optional overload (base already has set_size(w,h,bool))
    void set_size(float w, float h) { GuiElement::set_size(w, h, false); }

    // Events: override or assign callbacks.
    virtual void onHover();
    virtual void onClick();
    void set_on_hover(std::function<void()> cb) { m_on_hover = std::move(cb); }
    void set_on_click(std::function<void()> cb) { m_on_click = std::move(cb); }

    // GuiElement interface
    void draw() override;
    std::pair<float,float> preferred_size() const override;

    // Wire these into GLFW in your application (one set per GLFWwindow)
    static void glfw_cursor_pos_callback(GLFWwindow* window, double xpos, double ypos);
    static void glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    // Call once per frame before drawing any buttons (resets one-shot flags)
    static void begin_frame();

private:
    // Internal helpers
    static bool ensure_gl_resources();
    static void make_ortho(float left, float right, float bottom, float top, float znear, float zfar, float out[16]);

    // Hit test in pixel space (origin bottom-left)
    bool hit_test(float px, float py, float x, float y, float w, float h) const;

private:
    GuiText m_label;
    float m_bg[4] = {0.20f, 0.20f, 0.24f, 1.0f};
    float m_hover_bg[4] = {0.30f, 0.30f, 0.36f, 1.0f};
    float m_pad_x = 12.0f;
    float m_pad_y = 8.0f;
    float m_radius = 4.0f;

    // Callbacks
    std::function<void()> m_on_hover;
    std::function<void()> m_on_click;
    bool m_hovered_prev = false; // track hover enter

    // Shared GL resources (simple rounded-rect shader)
    static unsigned int s_vao;
    static unsigned int s_vbo;
    static unsigned int s_shader;
    static int s_uProjLoc;
    static int s_uRectMinLoc;
    static int s_uRectMaxLoc;
    static int s_uRadiusLoc;
    static int s_uBgColorLoc;

    // Input state (in framebuffer pixels, origin bottom-left)
    static double s_mouse_x_px;
    static double s_mouse_y_px;
    static bool   s_left_down;
    static bool   s_left_clicked; // one-shot per begin_frame
};
