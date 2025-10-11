// GuiMenuBar.cpp - Implementation of GuiMenuBar

#include "GuiMenuBar.h"
#include "GuiDraw.h"
#include "GuiInput.h"

#include <algorithm>

GuiMenuBar::GuiMenuBar()
{
    m_label_helper.set_text_size(3);
}

void GuiMenuBar::add_menu(const std::string& label)
{
    // Avoid duplicates
    auto it = std::find_if(m_menus.begin(), m_menus.end(), [&](const Menu& m){ return m.label == label; });
    if (it == m_menus.end()) m_menus.push_back(Menu{label, {}});
}

void GuiMenuBar::add_menu_item(const std::string& menu, const std::string& item_label, std::function<void()> callback)
{
    auto it = std::find_if(m_menus.begin(), m_menus.end(), [&](const Menu& m){ return m.label == menu; });
    if (it == m_menus.end()) {
        m_menus.push_back(Menu{menu, {}});
        it = std::prev(m_menus.end());
    }
    it->items.push_back(Item{item_label, std::move(callback)});
}

void GuiMenuBar::onMenuSelect(const std::string& /*menu*/, const std::string& /*item*/)
{
    // Default: no-op. Users can override if subclassing.
}

bool GuiMenuBar::set_text_font(const std::string& path)
{
    return m_label_helper.set_text_font(path);
}

void GuiMenuBar::set_text_size(int sz_1_to_10)
{
    m_label_helper.set_text_size(sz_1_to_10);
}

void GuiMenuBar::set_colors(float bg_r, float bg_g, float bg_b, float bg_a,
                            float hi_r, float hi_g, float hi_b, float hi_a)
{
    m_bg[0]=bg_r; m_bg[1]=bg_g; m_bg[2]=bg_b; m_bg[3]=bg_a;
    m_hi[0]=hi_r; m_hi[1]=hi_g; m_hi[2]=hi_b; m_hi[3]=hi_a;
}

std::pair<float,float> GuiMenuBar::preferred_size() const
{
    if (m_size_w > 0.0f && m_size_h > 0.0f) return {pixel_w(), pixel_h()};
    // Height from text size + padding; width sum of labels + spacing
    GuiText tmp = m_label_helper;
    float width = m_pad_x * 2.0f;
    float height = std::max(22.0f, tmp.preferred_size().second + 2.0f * m_pad_y);
    for (const auto& m : m_menus) {
        tmp.set_text(m.label);
        width += tmp.preferred_size().first + m_spacing;
    }
    return {std::max(200.0f, width), height};
}

void GuiMenuBar::draw()
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

    GuiDraw::draw_rect(x, y, w, h, m_bg);

    const auto [mx, my] = GuiInput::mouse_pos_px();

    // Layout top-level menus
    float pen_x = x + m_pad_x;
    int hovered_menu = -1;
    for (int i = 0; i < static_cast<int>(m_menus.size()); ++i) {
        const auto& m = m_menus[i];
        m_label_helper.set_text(m.label);
        auto sz = m_label_helper.preferred_size();
        float bx = pen_x - 4.0f;
        float by = y;
        float bw = sz.first + 8.0f;
        float bh = h;
        bool hovered = (mx >= bx && mx <= bx + bw && my >= by && my <= by + bh);
        if (hovered) hovered_menu = i;
        if (i == m_open_menu || hovered) {
            GuiDraw::draw_rect(bx, by, bw, bh, m_hi);
        }
        float asc=0.0f, desc=0.0f; bool have = m_label_helper.vertical_extents(asc, desc);
        float base_y = have ? (y + (h - (asc - desc)) * 0.5f + (asc - desc) * 0.6f)
                            : (y + (h - sz.second) * 0.5f + sz.second * 0.6f);
        m_label_helper.set_position(pen_x, base_y, false);
        m_label_helper.draw();
        pen_x += sz.first + m_spacing;
    }

    // Draw dropdown for open menu
    int hovered_item = -1;
    if (m_open_menu >= 0 && m_open_menu < static_cast<int>(m_menus.size())) {
        // Find rect of the open menu again (to place dropdown)
        pen_x = x + m_pad_x;
        for (int i = 0; i < m_open_menu; ++i) {
            m_label_helper.set_text(m_menus[i].label);
            pen_x += m_label_helper.preferred_size().first + m_spacing;
        }
        m_label_helper.set_text(m_menus[m_open_menu].label);
        float menu_w = m_label_helper.preferred_size().first + 8.0f;
        float drop_x = pen_x - 4.0f;
        float drop_y = y - 2.0f; // just under bar
        // Compute dropdown width based on items
        float item_h = m_label_helper.preferred_size().second + 8.0f;
        float max_w = menu_w;
        for (const auto& it : m_menus[m_open_menu].items) {
            m_label_helper.set_text(it.label);
            max_w = std::max(max_w, m_label_helper.preferred_size().first + 12.0f);
        }
        float drop_h = static_cast<float>(m_menus[m_open_menu].items.size()) * item_h + 4.0f;
        GuiDraw::draw_rect(drop_x, drop_y - drop_h, max_w, drop_h, m_bg);

        // Items
        for (int idx = 0; idx < static_cast<int>(m_menus[m_open_menu].items.size()); ++idx) {
            float iy = drop_y - (idx+1) * item_h;
            bool item_hovered = (mx >= drop_x && mx <= drop_x + max_w && my >= iy && my <= iy + item_h);
            if (item_hovered) { hovered_item = idx; GuiDraw::draw_rect(drop_x, iy, max_w, item_h, m_hi); }
            m_label_helper.set_text(m_menus[m_open_menu].items[idx].label);
            auto isz = m_label_helper.preferred_size();
            float asc=0.0f, desc=0.0f; bool have = m_label_helper.vertical_extents(asc, desc);
            float base_y = have ? (iy + (item_h - (asc - desc)) * 0.5f + (asc - desc) * 0.6f)
                                : (iy + (item_h - isz.second) * 0.5f + isz.second * 0.6f);
            m_label_helper.set_position(drop_x + 6.0f, base_y, false);
            m_label_helper.draw();
        }
    }

    // Click handling: prioritize dropdown items, then top-level, else close
    if (GuiInput::left_clicked()) {
        if (m_open_menu >= 0 && hovered_item >= 0) {
            auto cb = m_menus[m_open_menu].items[hovered_item].cb;
            auto menu_label = m_menus[m_open_menu].label;
            auto item_label = m_menus[m_open_menu].items[hovered_item].label;
            if (cb) cb();
            onMenuSelect(menu_label, item_label);
            m_open_menu = -1;
        } else if (hovered_menu >= 0) {
            if (m_open_menu == hovered_menu) m_open_menu = -1; else m_open_menu = hovered_menu;
        } else {
            m_open_menu = -1;
        }
    }
}
