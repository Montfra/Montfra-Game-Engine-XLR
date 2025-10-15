// GuiManager.h - Simple page manager for GUI panels
#pragma once

#include <string>
#include <unordered_map>
#include <optional>
#include "GuiPanel.h"

// GuiManager stores named GUI pages (each a GuiPanel) and draws only the active one.
// - addPage: registers/replaces a page by name (copy or move into manager)
// - setActivePage: selects the page to be drawn
// - draw: draws only the active page (no state mutation on panels)
class GuiManager {
public:
    GuiManager() = default;
    ~GuiManager() = default;

    // Adds or replaces a page under the given name. Copies the panel by value.
    void addPage(GuiPanel page, const std::string& name);

    // Sets the active page by name. If not found, does nothing.
    void setActivePage(const std::string& name);

    // Draw only the active page (if any).
    void draw();

    // Optional helpers
    bool hasPage(const std::string& name) const;
    std::optional<std::string> activePageName() const { return m_active; }

private:
    std::unordered_map<std::string, GuiPanel> m_pages;
    std::optional<std::string> m_active; // empty => draw nothing
};
