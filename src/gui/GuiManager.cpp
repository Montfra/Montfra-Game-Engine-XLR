// GuiManager.cpp - Implementation of simple GUI page manager

#include "GuiManager.h"

#include <cstdio>

void GuiManager::addPage(GuiPanel&& page, const std::string& name)
{
    // Insert or replace by name; move into container
    auto it = m_pages.find(name);
    if (it == m_pages.end()) {
        m_pages.emplace(name, std::move(page));
    } else {
        it->second = std::move(page);
    }
    // If no active page yet, set the first added as active
    if (!m_active.has_value()) {
        m_active = name;
    }
}

void GuiManager::setActivePage(const std::string& name)
{
    if (m_pages.find(name) != m_pages.end()) {
        m_active = name;
    } else {
        // Non-fatal: keep current active page
        std::fprintf(stderr, "[GuiManager] setActivePage: unknown page '%s'\n", name.c_str());
    }
}

bool GuiManager::hasPage(const std::string& name) const
{
    return m_pages.find(name) != m_pages.end();
}

void GuiManager::draw()
{
    if (!m_active.has_value()) return;
    auto it = m_pages.find(*m_active);
    if (it == m_pages.end()) return;
    // Draw only the active page; elements of other pages won't process input
    it->second.draw();
}
