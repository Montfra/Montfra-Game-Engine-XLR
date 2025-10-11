// GuiCheckbox.cpp - Implementation of GuiCheckbox

#include "GuiCheckbox.h"
#include "GuiInput.h"
#include "GuiDraw.h"

GuiCheckbox::GuiCheckbox()
{
    m_label.set_text_size(3);
}

void GuiCheckbox::set_checked(bool checked)
{
    if (m_checked != checked) {
        m_checked = checked;
        onToggle();
    }
}

void GuiCheckbox::onToggle()
{
    if (m_on_toggle) m_on_toggle(m_checked);
}

void GuiCheckbox::set_colors(float box_r, float box_g, float box_b, float box_a,
                             float check_r, float check_g, float check_b, float check_a)
{
    m_box_color[0]=box_r; m_box_color[1]=box_g; m_box_color[2]=box_b; m_box_color[3]=box_a;
    m_check_color[0]=check_r; m_check_color[1]=check_g; m_check_color[2]=check_b; m_check_color[3]=check_a;
}

bool GuiCheckbox::hit_test(float px, float py, float x, float y, float w, float h) const
{
    return (px >= x && px <= x + w && py >= y && py <= y + h);
}

std::pair<float,float> GuiCheckbox::preferred_size() const
{
    if (m_size_w > 0.0f && m_size_h > 0.0f) return {pixel_w(), pixel_h()};
    auto ts = m_label.preferred_size();
    float box = std::max(18.0f, ts.second); // square box at least text height
    float w = box + (ts.first > 0.0f ? (m_spacing + ts.first) : 0.0f);
    float h = std::max(box, ts.second);
    return {w, h};
}

void GuiCheckbox::draw()
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

    // Size of square box
    float box = std::min(h, std::max(18.0f, m_label.preferred_size().second));
    float box_x = x;
    float box_y = y + (h - box) * 0.5f;

    const auto [mx, my] = GuiInput::mouse_pos_px();
    const bool hovered = hit_test(static_cast<float>(mx), static_cast<float>(my), box_x, box_y, box, box);
    if (GuiInput::left_clicked() && hovered) {
        set_checked(!m_checked);
    }

    // Draw box
    GuiDraw::draw_rounded_rect(box_x, box_y, box, box, 3.0f, m_box_color);
    if (m_checked) {
        float pad = box * 0.2f;
        GuiDraw::draw_rounded_rect(box_x + pad, box_y + pad, box - 2*pad, box - 2*pad, 2.0f, m_check_color);
    }

    // Label to the right
    auto ts = m_label.preferred_size();
    if (ts.first > 0.0f) {
        float asc=0.0f, desc=0.0f;
        bool have = m_label.vertical_extents(asc, desc);
        float base_y = have ? (y + (h - (asc - desc)) * 0.5f + (asc - desc) * 0.6f) : (y + (h - ts.second) * 0.5f + ts.second * 0.6f);
        m_label.set_position(x + box + m_spacing, base_y, false);
        m_label.draw();
    }
}

