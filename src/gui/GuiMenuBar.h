// GuiMenuBar.h - Horizontal menu bar with drop-down menus
#pragma once

#include <string>
#include <vector>
#include <functional>

#include "GuiElement.h"
#include "GuiText.h"

class GuiMenuBar : public GuiElement {
public:
    GuiMenuBar();

    void add_menu(const std::string& label);
    void add_menu_item(const std::string& menu, const std::string& item_label, std::function<void()> callback);
    virtual void onMenuSelect(const std::string& menu, const std::string& item);

    bool set_text_font(const std::string& path);
    void set_text_size(int sz_1_to_10);
    void set_colors(float bg_r, float bg_g, float bg_b, float bg_a,
                    float hi_r, float hi_g, float hi_b, float hi_a);
    void set_spacing(float s) { m_spacing = (s < 0.f ? 0.f : s); }
    void set_padding(float px, float py) { m_pad_x = (px < 0 ? 0.f : px); m_pad_y = (py < 0 ? 0.f : py); }

    void draw() override;
    std::pair<float,float> preferred_size() const override;

private:
    struct Item { std::string label; std::function<void()> cb; };
    struct Menu { std::string label; std::vector<Item> items; };
    std::vector<Menu> m_menus;
    mutable GuiText m_label_helper; // for measuring and rendering labels
    float m_bg[4] = {0.10f, 0.10f, 0.12f, 1.0f};
    float m_hi[4] = {0.20f, 0.30f, 0.55f, 1.0f};
    float m_spacing = 16.0f;
    float m_pad_x = 10.0f;
    float m_pad_y = 6.0f;
    int m_open_menu = -1; // index of open top-level menu, -1 none
};
