// GuiProgressBar.cpp - Implementation of GuiProgressBar

#include "GuiProgressBar.h"
#include "GuiDraw.h"

#include <algorithm>
#include <cstdio>
#include <cmath>

GuiProgressBar::GuiProgressBar()
{
    m_text.set_text_size(3);
}

void GuiProgressBar::set_progress(float percent)
{
    float np = std::clamp(percent, 0.0f, 100.0f);
    if (np != m_progress) {
        m_progress = np;
    }
}

void GuiProgressBar::set_bar_color(float r, float g, float b, float a)
{
    m_bar[0]=r; m_bar[1]=g; m_bar[2]=b; m_bar[3]=a;
}

void GuiProgressBar::set_background_color(float r, float g, float b, float a)
{
    m_bg[0]=r; m_bg[1]=g; m_bg[2]=b; m_bg[3]=a;
}

std::pair<float,float> GuiProgressBar::preferred_size() const
{
    if (m_size_w > 0.0f && m_size_h > 0.0f) return {pixel_w(), pixel_h()};
    return {220.0f, 24.0f};
}

void GuiProgressBar::draw()
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

    GuiDraw::draw_rounded_rect(x, y, w, h, 4.0f, m_bg);
    float t = std::clamp(m_progress / 100.0f, 0.0f, 1.0f);
    GuiDraw::draw_rounded_rect(x, y, w * t, h, 4.0f, m_bar);

    if (m_show_text) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%d%%", (int)std::round(m_progress));
        m_text.set_text(buf);
        float asc=0.0f, desc=0.0f;
        bool have = m_text.vertical_extents(asc, desc);
        float base_y = have ? (y + (h - (asc - desc)) * 0.5f + (asc - desc) * 0.6f)
                            : (y + (h - m_text.preferred_size().second) * 0.5f + m_text.preferred_size().second * 0.6f);
        float base_x = x + (w - m_text.preferred_size().first) * 0.5f;
        m_text.set_position(base_x, base_y, false);
        m_text.draw();
    }
}
