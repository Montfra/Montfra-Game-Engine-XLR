// GuiInputText.h - Text input field widget
#pragma once

#include <string>
#include <functional>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "GuiElement.h"
#include "GuiText.h"

class GuiInputText : public GuiElement {
public:
    GuiInputText();
    ~GuiInputText();

    void set_placeholder(const std::string& text);
    void set_text(const std::string& text);
    const std::string& get_text() const { return m_text; }

    // Events: override to customize, or set callback
    virtual void onTextChange();
    void set_on_text_change(std::function<void(const std::string&)> cb) { m_on_changed = std::move(cb); }

    // Visuals
    void set_colors(float bg_r, float bg_g, float bg_b, float bg_a,
                    float border_r, float border_g, float border_b, float border_a,
                    float text_r, float text_g, float text_b, float text_a,
                    float placeholder_r, float placeholder_g, float placeholder_b, float placeholder_a);
    void set_corner_radius(float r) { m_radius = (r < 0.f ? 0.f : r); }
    void set_padding(float px, float py) { m_pad_x = (px < 0 ? 0.f : px); m_pad_y = (py < 0 ? 0.f : py); }
    void set_text_size(int sz_1_to_10) { m_label.set_text_size(sz_1_to_10); }
    bool set_text_font(const std::string& path) { return m_label.set_text_font(path); }

    void draw() override;
    std::pair<float,float> preferred_size() const override;

private:
    bool hit_test(float px, float py, float x, float y, float w, float h) const;

private:
    mutable GuiText m_label;
    std::string m_text;
    std::string m_placeholder;
    bool m_focused = false;
    float m_pad_x = 8.0f;
    float m_pad_y = 6.0f;
    float m_radius = 4.0f;
    float m_bg[4] = {0.12f, 0.12f, 0.14f, 1.0f};
    float m_border[4] = {0.35f, 0.45f, 0.95f, 0.65f};
    float m_text_color[4] = {0.92f, 0.94f, 0.98f, 1.0f};
    float m_placeholder_color[4] = {0.6f, 0.65f, 0.7f, 0.75f};

    std::function<void(const std::string&)> m_on_changed;
};
