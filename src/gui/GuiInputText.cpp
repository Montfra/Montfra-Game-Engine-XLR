// GuiInputText.cpp - Implementation of GuiInputText

#include "GuiInputText.h"
#include "GuiInput.h"
#include "GuiDraw.h"

#include <cstdio>
#include <algorithm>
#include <cmath>

GuiInputText::GuiInputText()
{
    m_label.set_text_size(3);
}

GuiInputText::~GuiInputText() {}

void GuiInputText::set_placeholder(const std::string& text)
{
    m_placeholder = text;
}

void GuiInputText::set_text(const std::string& text)
{
    if (m_text != text) {
        m_text = text;
        onTextChange();
    }
}

void GuiInputText::onTextChange()
{
    if (m_on_changed) m_on_changed(m_text);
}

void GuiInputText::set_colors(float bg_r, float bg_g, float bg_b, float bg_a,
                              float border_r, float border_g, float border_b, float border_a,
                              float text_r, float text_g, float text_b, float text_a,
                              float placeholder_r, float placeholder_g, float placeholder_b, float placeholder_a)
{
    m_bg[0]=bg_r; m_bg[1]=bg_g; m_bg[2]=bg_b; m_bg[3]=bg_a;
    m_border[0]=border_r; m_border[1]=border_g; m_border[2]=border_b; m_border[3]=border_a;
    m_text_color[0]=text_r; m_text_color[1]=text_g; m_text_color[2]=text_b; m_text_color[3]=text_a;
    m_placeholder_color[0]=placeholder_r; m_placeholder_color[1]=placeholder_g; m_placeholder_color[2]=placeholder_b; m_placeholder_color[3]=placeholder_a;
}

bool GuiInputText::hit_test(float px, float py, float x, float y, float w, float h) const
{
    return (px >= x && px <= x + w && py >= y && py <= y + h);
}

std::pair<float,float> GuiInputText::preferred_size() const
{
    if (m_size_w > 0.0f && m_size_h > 0.0f) return {pixel_w(), pixel_h()};
    // Default width: based on placeholder/typical text width + padding; min height from text size
    // Use the internal label for measurement (avoid copying GuiText since it is non-copyable)
    std::string measure = !m_text.empty() ? m_text : (m_placeholder.empty() ? std::string(10,' ') : m_placeholder);
    m_label.set_text(measure);
    auto label_size = m_label.preferred_size();
    float w = std::max(160.0f, label_size.first + 2.0f * m_pad_x);
    float h = std::max(24.0f, label_size.second + 2.0f * m_pad_y);
    return {w, h};
}

void GuiInputText::draw()
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

    // Input handling (focus + characters + backspace)
    const auto [mx, my] = GuiInput::mouse_pos_px();
    const bool hovered = hit_test(static_cast<float>(mx), static_cast<float>(my), x, y, w, h);
    if (GuiInput::left_clicked()) {
        m_focused = hovered;
    }
    if (m_focused) {
        // Consume this frame's typed characters
        auto chars = GuiInput::consume_chars();
        bool changed = false;
        for (unsigned int c : chars) {
            if (c >= 32 && c != 127) { // printable
                m_text.push_back(static_cast<char>(c));
                changed = true;
            }
        }
        // Backspace
        if (GuiInput::key_pressed(GLFW_KEY_BACKSPACE)) {
            if (!m_text.empty()) { m_text.pop_back(); changed = true; }
        }
        if (changed) onTextChange();
    }

    // Visuals
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    if (depthWasEnabled) glDisable(GL_DEPTH_TEST);

    // Border changes if focused
    float bg[4] = { m_bg[0], m_bg[1], m_bg[2], m_bg[3] };
    float border[4] = { m_border[0], m_border[1], m_border[2], m_focused ? std::max(0.9f, m_border[3]) : m_border[3] };
    GuiDraw::draw_rounded_rect(x, y, w, h, m_radius, bg);
    // Inner border by slightly shrinking rect and drawing with border color and small thickness simulation
    GuiDraw::draw_rounded_rect(x-1.0f, y-1.0f, w+2.0f, h+2.0f, m_radius+1.0f, border);

    // Text rendering
    std::string display = m_text.empty() ? m_placeholder : m_text;
    float color[4];
    if (m_text.empty()) { for (int i=0;i<4;++i) color[i] = m_placeholder_color[i]; }
    else { for (int i=0;i<4;++i) color[i] = m_text_color[i]; }
    m_label.set_text(display);
    m_label.set_text_color(color[0], color[1], color[2], color[3]);

    // Baseline centered vertically
    float asc=0.0f, desc=0.0f;
    bool have_extents = m_label.vertical_extents(asc, desc);
    float baseline_y;
    if (have_extents) {
        float center_y = y + h * 0.5f;
        baseline_y = center_y - 0.5f * (asc - desc);
    } else {
        auto pref = m_label.preferred_size();
        baseline_y = y + (h - pref.second) * 0.5f + pref.second * 0.6f;
    }
    float text_x = x + m_pad_x;
    m_label.set_position(text_x, baseline_y, false);
    m_label.draw();

    // Caret
    if (m_focused) {
        double t = glfwGetTime();
        if (std::fmod(t, 1.0) < 0.5) { // blink 0.5s on/off
            // Measure text width to place caret at end
            // Reuse m_label for measurement safely (we already drew the label above)
            m_label.set_text(m_text);
            float caret_x = text_x + m_label.preferred_size().first + 1.0f;
            float caret_h = have_extents ? (asc + desc) : m_label.preferred_size().second;
            float caret_y = baseline_y - (have_extents ? desc : caret_h * 0.6f);
            const float caret_col[4] = { m_text_color[0], m_text_color[1], m_text_color[2], 0.95f };
            GuiDraw::draw_rect(caret_x, caret_y, 1.0f, caret_h, caret_col);
            // Restore label text for next frame semantics
            m_label.set_text(display);
        }
    }

    if (depthWasEnabled) glEnable(GL_DEPTH_TEST);
}
