// GuiProgressBar.h - Simple progress bar 0..100%
#pragma once

#include "GuiElement.h"
#include "GuiText.h"

class GuiProgressBar : public GuiElement {
public:
    GuiProgressBar();

    void set_progress(float percent); // 0..100
    float get_progress() const { return m_progress; }
    void set_bar_color(float r, float g, float b, float a);
    void set_background_color(float r, float g, float b, float a);
    void show_text(bool enabled) { m_show_text = enabled; }
    bool set_text_font(const std::string& path) { return m_text.set_text_font(path); }
    void set_text_size(int sz_1_to_10) { m_text.set_text_size(sz_1_to_10); }

    void draw() override;
    std::pair<float,float> preferred_size() const override;

private:
    float m_progress = 0.0f; // percent
    float m_bg[4] = {0.16f, 0.16f, 0.18f, 1.0f};
    float m_bar[4] = {0.20f, 0.70f, 0.40f, 1.0f};
    GuiText m_text;
    bool m_show_text = true;
};

