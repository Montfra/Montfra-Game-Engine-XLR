// GuiSlider.h - Horizontal/vertical slider widget
#pragma once

#include <functional>
#include "GuiElement.h"

class GuiSlider : public GuiElement {
public:
    enum class Orientation { Horizontal, Vertical };

    GuiSlider();

    void set_range(float mn, float mx);
    void set_value(float v);
    float get_value() const { return m_value; }
    void set_orientation(Orientation o) { m_orientation = o; }

    virtual void onValueChanged();
    void set_on_value_changed(std::function<void(float)> cb) { m_on_changed = std::move(cb); }

    // Visuals
    void set_colors(float bg_r, float bg_g, float bg_b, float bg_a,
                    float fill_r, float fill_g, float fill_b, float fill_a,
                    float knob_r, float knob_g, float knob_b, float knob_a);
    void set_corner_radius(float r) { m_radius = (r < 0.f ? 0.f : r); }

    void draw() override;
    std::pair<float,float> preferred_size() const override;

private:
    bool hit_test(float px, float py, float x, float y, float w, float h) const;

private:
    float m_min = 0.0f;
    float m_max = 100.0f;
    float m_value = 0.0f;
    Orientation m_orientation = Orientation::Horizontal;
    bool m_dragging = false;
    float m_radius = 5.0f;
    float m_colors_bg[4]   = {0.18f, 0.18f, 0.20f, 1.0f};
    float m_colors_fill[4] = {0.30f, 0.55f, 0.90f, 1.0f};
    float m_colors_knob[4] = {0.95f, 0.95f, 1.00f, 1.0f};
    std::function<void(float)> m_on_changed;
};

