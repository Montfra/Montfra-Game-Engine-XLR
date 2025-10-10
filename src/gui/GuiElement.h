// GuiElement.h - Base class for GUI elements (HUD, menus)
#pragma once

#include <utility>
#include <glad/glad.h>

// Coordinates and sizes are in screen pixels by default (origin at bottom-left),
// or as percentage of framebuffer size when *_is_percent is true.
class GuiElement {
public:
    virtual ~GuiElement() = default;

    // Position and sizing
    void set_position(float x, float y, bool in_percentage = false) {
        m_pos_x = x; m_pos_y = y; m_pos_is_percent = in_percentage;
    }
    void set_size(float w, float h, bool in_percentage = false) {
        m_size_w = w; m_size_h = h; m_size_is_percent = in_percentage;
    }

    // Visibility
    void show() { m_visible = true; }
    void hide() { m_visible = false; }
    bool visible() const { return m_visible; }

    // Rendering contract
    virtual void draw() = 0;

    // Preferred content size in pixels. Default uses set_size if provided, otherwise {0,0}.
    virtual std::pair<float,float> preferred_size() const {
        if (m_size_w > 0.0f && m_size_h > 0.0f) return {pixel_w(), pixel_h()};
        return {0.0f, 0.0f};
    }

protected:
    // Convert stored position/size to pixels using current GL viewport
    static void get_framebuffer_size(int& out_w, int& out_h) {
        // Query GL viewport to get pixel size when not explicitly notified
        int vp[4] = {0,0,0,0};
        glGetIntegerv(GL_VIEWPORT, vp);
        out_w = vp[2]; out_h = vp[3];
    }

    float pixel_x() const {
        if (!m_pos_is_percent) return m_pos_x;
        int fw=0, fh=0; get_framebuffer_size(fw, fh);
        return m_pos_x * 0.01f * static_cast<float>(fw);
    }
    float pixel_y() const {
        if (!m_pos_is_percent) return m_pos_y;
        int fw=0, fh=0; get_framebuffer_size(fw, fh);
        return m_pos_y * 0.01f * static_cast<float>(fh);
    }
    float pixel_w() const {
        if (!m_size_is_percent) return m_size_w;
        int fw=0, fh=0; get_framebuffer_size(fw, fh);
        (void)fh; // unused
        return m_size_w * 0.01f * static_cast<float>(fw);
    }
    float pixel_h() const {
        if (!m_size_is_percent) return m_size_h;
        int fw=0, fh=0; get_framebuffer_size(fw, fh);
        return m_size_h * 0.01f * static_cast<float>(fh);
    }

protected:
    float m_pos_x = 0.0f;
    float m_pos_y = 0.0f;
    bool  m_pos_is_percent = false;

    float m_size_w = 0.0f; // 0 => auto/preferred
    float m_size_h = 0.0f;
    bool  m_size_is_percent = false;

    bool  m_visible = true;
};
