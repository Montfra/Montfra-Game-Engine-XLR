// GuiSlider.cpp - Implementation of GuiSlider

#include "GuiSlider.h"
#include "GuiInput.h"
#include "GuiDraw.h"

#include <algorithm>

GuiSlider::GuiSlider() {}

void GuiSlider::set_range(float mn, float mx)
{
    if (mx < mn) std::swap(mn, mx);
    m_min = mn; m_max = mx;
    set_value(m_value);
}

void GuiSlider::set_value(float v)
{
    float nv = std::clamp(v, m_min, m_max);
    if (nv != m_value) {
        m_value = nv;
        onValueChanged();
    }
}

void GuiSlider::onValueChanged()
{
    if (m_on_changed) m_on_changed(m_value);
}

void GuiSlider::set_colors(float bg_r, float bg_g, float bg_b, float bg_a,
                           float fill_r, float fill_g, float fill_b, float fill_a,
                           float knob_r, float knob_g, float knob_b, float knob_a)
{
    m_colors_bg[0]=bg_r; m_colors_bg[1]=bg_g; m_colors_bg[2]=bg_b; m_colors_bg[3]=bg_a;
    m_colors_fill[0]=fill_r; m_colors_fill[1]=fill_g; m_colors_fill[2]=fill_b; m_colors_fill[3]=fill_a;
    m_colors_knob[0]=knob_r; m_colors_knob[1]=knob_g; m_colors_knob[2]=knob_b; m_colors_knob[3]=knob_a;
}

bool GuiSlider::hit_test(float px, float py, float x, float y, float w, float h) const
{
    return (px >= x && px <= x + w && py >= y && py <= y + h);
}

std::pair<float,float> GuiSlider::preferred_size() const
{
    if (m_size_w > 0.0f && m_size_h > 0.0f) return {pixel_w(), pixel_h()};
    if (m_orientation == Orientation::Horizontal)
        return {200.0f, 24.0f};
    else
        return {24.0f, 120.0f};
}

void GuiSlider::draw()
{
    if (!m_visible) return;

    float x = pixel_x();
    float y = pixel_y();
    float w = pixel_w();
    float h = pixel_h();
    if (w <= 0.0f || h <= 0.0f) {
        auto ps = preferred_size();
        if (w <= 0.0f) w = ps.first;
        if (h <= 0.0f) h = ps.second;
    }
    if (w <= 0.0f || h <= 0.0f) return;

    const auto [mx, my] = GuiInput::mouse_pos_px();
    const bool hovered = hit_test(static_cast<float>(mx), static_cast<float>(my), x, y, w, h);

    // Dragging logic
    if (GuiInput::left_clicked() && hovered) {
        m_dragging = true;
    }
    if (!GuiInput::left_down()) {
        m_dragging = false;
    }
    if (m_dragging) {
        if (m_orientation == Orientation::Horizontal) {
            float t = (static_cast<float>(mx) - x) / std::max(1.0f, w);
            t = std::clamp(t, 0.0f, 1.0f);
            set_value(m_min + t * (m_max - m_min));
        } else {
            float t = (static_cast<float>(my) - y) / std::max(1.0f, h);
            t = std::clamp(t, 0.0f, 1.0f);
            set_value(m_min + t * (m_max - m_min));
        }
    }

    // Draw track
    GuiDraw::draw_rounded_rect(x, y, w, h, m_radius, m_colors_bg);

    // Draw fill
    float t = (m_value - m_min) / std::max(0.0001f, (m_max - m_min));
    t = std::clamp(t, 0.0f, 1.0f);
    if (m_orientation == Orientation::Horizontal) {
        GuiDraw::draw_rounded_rect(x, y, w * t, h, m_radius, m_colors_fill);
        // Knob
        float kw = std::max(10.0f, h);
        float kx = x + w * t - kw * 0.5f;
        GuiDraw::draw_rounded_rect(kx, y, kw, h, m_radius, m_colors_knob);
    } else {
        GuiDraw::draw_rounded_rect(x, y, w, h * t, m_radius, m_colors_fill);
        float kh = std::max(10.0f, w);
        float ky = y + h * t - kh * 0.5f;
        GuiDraw::draw_rounded_rect(x, ky, w, kh, m_radius, m_colors_knob);
    }
}

