// GuiElement.h - Base class for GUI elements (HUD, menus)
#pragma once

#include <utility>
#include <glad/glad.h>

// Coordinates and sizes are in screen pixels by default (origin at bottom-left),
// or as percentage of framebuffer size when *_is_percent is true.
class GuiElement {
public:
    virtual ~GuiElement() = default;

    // Alignment anchor for automatic positioning relative to a parent rectangle
    enum class GuiAlignment {
        TopLeft,
        TopCenter,
        TopRight,
        CenterLeft,
        Center,
        CenterRight,
        BottomLeft,
        BottomCenter,
        BottomRight
    };

    enum class PositionMode { Manual, Aligned };

    // Position and sizing
    void set_position(float x, float y, bool in_percentage = false) {
        m_pos_x = x; m_pos_y = y; m_pos_is_percent = in_percentage;
    }
    void set_size(float w, float h, bool in_percentage = false) {
        m_size_w = w; m_size_h = h; m_size_is_percent = in_percentage;
    }

    // Alignment configuration. When set, the element's position is computed
    // relative to its parent rectangle (or the framebuffer if none) according
    // to the selected anchor and the optional anchor offset (margin).
    void set_alignment(GuiAlignment a) { m_alignment = a; m_pos_mode = PositionMode::Aligned; }
    GuiAlignment alignment() const { return m_alignment; }
    void clear_alignment() { m_pos_mode = PositionMode::Manual; }
    PositionMode position_mode() const { return m_pos_mode; }

    // Offset (margin) applied from the chosen anchor in pixels or percent of parent size.
    // For left/bottom anchors: +x moves right, +y moves up.
    // For right/top anchors: positive offsets move inward from the edge.
    void set_anchor_offset(float dx, float dy, bool in_percentage = false) {
        m_anchor_dx = dx; m_anchor_dy = dy; m_anchor_is_percent = in_percentage;
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

    // Called by containers (e.g., GuiPanel) prior to drawing children so they can
    // position themselves relative to this rectangle. If never called, the
    // framebuffer size is used as the parent rectangle for alignment.
    void notify_parent_rect(float x, float y, float w, float h) {
        m_parent_x = x; m_parent_y = y; m_parent_w = w; m_parent_h = h; m_has_parent = true;
    }

    // Compute anchor-based position for an element of size (elem_w, elem_h) inside
    // the given parent rectangle. Uses the element's alignment and anchor offset.
    // Returns the bottom-left position for drawing. This ignores PositionMode to
    // allow containers to place children inside sub-rects.
    std::pair<float,float> aligned_position_in(float parent_x, float parent_y, float parent_w, float parent_h,
                                               float elem_w, float elem_h) const
    {
        // Convert margins to pixels
        float dx = m_anchor_is_percent ? (m_anchor_dx * 0.01f * parent_w) : m_anchor_dx;
        float dy = m_anchor_is_percent ? (m_anchor_dy * 0.01f * parent_h) : m_anchor_dy;

        float x = parent_x;
        float y = parent_y;

        switch (m_alignment) {
            case GuiAlignment::BottomLeft:
                x = parent_x + dx;
                y = parent_y + dy;
                break;
            case GuiAlignment::BottomCenter:
                x = parent_x + (parent_w - elem_w) * 0.5f + dx;
                y = parent_y + dy;
                break;
            case GuiAlignment::BottomRight:
                x = parent_x + (parent_w - elem_w) - dx;
                y = parent_y + dy;
                break;
            case GuiAlignment::CenterLeft:
                x = parent_x + dx;
                y = parent_y + (parent_h - elem_h) * 0.5f + dy;
                break;
            case GuiAlignment::Center:
                x = parent_x + (parent_w - elem_w) * 0.5f + dx;
                y = parent_y + (parent_h - elem_h) * 0.5f + dy;
                break;
            case GuiAlignment::CenterRight:
                x = parent_x + (parent_w - elem_w) - dx;
                y = parent_y + (parent_h - elem_h) * 0.5f + dy;
                break;
            case GuiAlignment::TopLeft:
                x = parent_x + dx;
                y = parent_y + parent_h - elem_h - dy;
                break;
            case GuiAlignment::TopCenter:
                x = parent_x + (parent_w - elem_w) * 0.5f + dx;
                y = parent_y + parent_h - elem_h - dy;
                break;
            case GuiAlignment::TopRight:
                x = parent_x + parent_w - elem_w - dx;
                y = parent_y + parent_h - elem_h - dy;
                break;
        }
        return {x, y};
    }

protected:
    // Compute final aligned position for this element given its size.
    // If PositionMode is Manual, returns pixel_x/pixel_y.
    void compute_aligned_xy(float elem_w, float elem_h, float& out_x, float& out_y) const {
        if (m_pos_mode == PositionMode::Manual) {
            out_x = pixel_x();
            out_y = pixel_y();
            return;
        }
        // Resolve parent rect (framebuffer if not provided)
        float px = 0.f, py = 0.f, pw = 0.f, ph = 0.f;
        if (m_has_parent) {
            px = m_parent_x; py = m_parent_y; pw = m_parent_w; ph = m_parent_h;
        } else {
            int fw=0, fh=0; get_framebuffer_size(fw, fh);
            px = 0.f; py = 0.f; pw = static_cast<float>(fw); ph = static_cast<float>(fh);
        }
        auto pos = aligned_position_in(px, py, pw, ph, elem_w, elem_h);
        out_x = pos.first;
        out_y = pos.second;
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

    // Alignment state
    PositionMode m_pos_mode = PositionMode::Manual;
    GuiAlignment m_alignment = GuiAlignment::BottomLeft; // default matches previous behavior
    float m_anchor_dx = 0.0f;
    float m_anchor_dy = 0.0f;
    bool  m_anchor_is_percent = false;

    // Parent rect used for alignment (if provided by a container)
    bool  m_has_parent = false;
    float m_parent_x = 0.0f;
    float m_parent_y = 0.0f;
    float m_parent_w = 0.0f;
    float m_parent_h = 0.0f;
};
