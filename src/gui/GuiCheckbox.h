// GuiCheckbox.h - Boolean checkbox with optional text label
#pragma once

#include <functional>
#include "GuiElement.h"
#include "GuiText.h"

class GuiCheckbox : public GuiElement {
public:
    GuiCheckbox();

    void set_checked(bool checked);
    bool is_checked() const { return m_checked; }
    virtual void onToggle();
    void set_on_toggle(std::function<void(bool)> cb) { m_on_toggle = std::move(cb); }

    void set_label(const std::string& text) { m_label.set_text(text); }
    bool set_text_font(const std::string& path) { return m_label.set_text_font(path); }
    void set_text_size(int sz_1_to_10) { m_label.set_text_size(sz_1_to_10); }
    void set_text_color(float r, float g, float b, float a) { m_label.set_text_color(r,g,b,a); }

    void set_colors(float box_r, float box_g, float box_b, float box_a,
                    float check_r, float check_g, float check_b, float check_a);
    void set_spacing(float s) { m_spacing = (s < 0.f ? 0.f : s); }

    void draw() override;
    std::pair<float,float> preferred_size() const override;

private:
    bool hit_test(float px, float py, float x, float y, float w, float h) const;

private:
    bool m_checked = false;
    float m_box_color[4] = {0.18f, 0.18f, 0.20f, 1.0f};
    float m_check_color[4] = {0.30f, 0.90f, 0.50f, 1.0f};
    float m_spacing = 8.0f; // between box and label
    GuiText m_label;
    std::function<void(bool)> m_on_toggle;
};

