// GuiPanel.h - Container for GUI elements with basic layout and background/border rendering
#pragma once

#include <vector>
#include <algorithm>
#include "GuiElement.h"

class GuiPanel : public GuiElement {
public:
    enum class LayoutType { HORIZONTAL, VERTICAL, GRID, ABSOLUTE };

    GuiPanel();
    ~GuiPanel();

    void addChild(GuiElement* element);
    void removeChild(GuiElement* element);

    void setBackgroundColor(float r, float g, float b, float a);
    void setBorderColor(float r, float g, float b, float a);
    void setBorderRadius(float r);
    void setBorderThickness(float t); // optional helper

    void setLayout(LayoutType type);
    void setPadding(float p);
    void setSpacing(float s);

    void draw() override;

private:
    // Helpers
    void draw_panel_quad(float x, float y, float w, float h);
    void layout_children(float x, float y, float w, float h);
    static bool ensure_gl_resources();

private:
    std::vector<GuiElement*> m_children;
    float m_bg[4] = {0.f, 0.f, 0.f, 0.0f};
    float m_border[4] = {0.f, 0.f, 0.f, 0.0f};
    float m_radius = 0.0f;
    float m_border_thickness = 1.0f;
    float m_padding = 8.0f;
    float m_spacing = 6.0f;
    LayoutType m_layout = LayoutType::HORIZONTAL;

    // Shared GL resources
    static unsigned int s_vao;
    static unsigned int s_vbo;
    static unsigned int s_shader;
    static int s_uProjLoc;
    static int s_uRectMinLoc;
    static int s_uRectMaxLoc;
    static int s_uRadiusLoc;
    static int s_uBgColorLoc;
    static int s_uBorderColorLoc;
    static int s_uBorderThicknessLoc;
};
