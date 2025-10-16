// GuiPanel.cpp - Implementation of GuiPanel container

#include "GuiPanel.h"
#include "GuiText.h" // for preferred_size implementation use; not strictly required

#include <glad/glad.h>

#include <cstdio>
#include <cmath>

// Static GL resources
unsigned int GuiPanel::s_vao = 0;
unsigned int GuiPanel::s_vbo = 0;
unsigned int GuiPanel::s_shader = 0;
int GuiPanel::s_uProjLoc = -1;
int GuiPanel::s_uRectMinLoc = -1;
int GuiPanel::s_uRectMaxLoc = -1;
int GuiPanel::s_uRadiusLoc = -1;
int GuiPanel::s_uBgColorLoc = -1;
int GuiPanel::s_uBorderColorLoc = -1;
int GuiPanel::s_uBorderThicknessLoc = -1;

namespace {

static void make_ortho(float left, float right, float bottom, float top, float znear, float zfar, float out[16])
{
    for (int i=0;i<16;++i) out[i] = 0.0f;
    out[0] = 2.0f / (right - left);
    out[5] = 2.0f / (top - bottom);
    out[10] = -2.0f / (zfar - znear);
    out[12] = - (right + left) / (right - left);
    out[13] = - (top + bottom) / (top - bottom);
    out[14] = - (zfar + znear) / (zfar - znear);
    out[15] = 1.0f;
}

static unsigned int compile_shader(GLenum type, const char* src)
{
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    GLint ok = GL_FALSE;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0; glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &len);
        std::string log(len > 0 ? len : 1, '\0');
        glGetShaderInfoLog(sh, len, nullptr, log.data());
        std::fprintf(stderr, "[GuiPanel] Shader compile error (%s):\n%s\n", type==GL_VERTEX_SHADER?"VERTEX":"FRAGMENT", log.c_str());
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

static unsigned int link_program(GLuint vs, GLuint fs)
{
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = GL_FALSE;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0; glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
        std::string log(len > 0 ? len : 1, '\0');
        glGetProgramInfoLog(p, len, nullptr, log.data());
        std::fprintf(stderr, "[GuiPanel] Program link error:\n%s\n", log.c_str());
        glDeleteProgram(p);
        return 0;
    }
    return p;
}

} // namespace

bool GuiPanel::ensure_gl_resources()
{
    if (s_shader && s_vao && s_vbo) return true;

    if (s_vao == 0) {
        glGenVertexArrays(1, &s_vao);
        glBindVertexArray(s_vao);
        glGenBuffers(1, &s_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12, nullptr, GL_DYNAMIC_DRAW); // 2D quad (6 verts * 2 floats)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glBindVertexArray(0);
    }

    if (s_shader == 0) {
        static const char* VERT = R"GLSL(
            #version 330 core
            layout(location = 0) in vec2 aPos; // in pixel space
            uniform mat4 uProjection;
            void main() {
                gl_Position = uProjection * vec4(aPos.xy, 0.0, 1.0);
            }
        )GLSL";
        static const char* FRAG = R"GLSL(
            #version 330 core
            out vec4 FragColor;
            uniform vec2 uRectMin;
            uniform vec2 uRectMax;
            uniform float uRadius;
            uniform vec4 uBgColor;
            uniform vec4 uBorderColor;
            uniform float uBorderThickness;

            // Signed distance field for rounded box in window coords
            float sdRoundBox(vec2 p, vec2 b, float r)
            {
                vec2 q = abs(p) - (b - vec2(r));
                return length(max(q, 0.0)) - r;
            }

            void main() {
                vec2 rectSize = uRectMax - uRectMin;
                vec2 center = uRectMin + rectSize * 0.5;
                vec2 p = gl_FragCoord.xy - center; // window coords, origin bottom-left
                vec2 halfSize = rectSize * 0.5;
                float d = sdRoundBox(p, halfSize, uRadius);
                if (d > 0.0) discard; // outside rounded rect

                // Border if within thickness band near edge (inside)
                float inner = d + uBorderThickness;
                vec4 color = (inner > 0.0 && uBorderColor.a > 0.0) ? uBorderColor : uBgColor;
                FragColor = color;
            }
        )GLSL";

        GLuint vs = compile_shader(GL_VERTEX_SHADER, VERT);
        GLuint fs = compile_shader(GL_FRAGMENT_SHADER, FRAG);
        if (!vs || !fs) return false;
        s_shader = link_program(vs, fs);
        glDeleteShader(vs);
        glDeleteShader(fs);
        if (!s_shader) return false;
        s_uProjLoc = glGetUniformLocation(s_shader, "uProjection");
        s_uRectMinLoc = glGetUniformLocation(s_shader, "uRectMin");
        s_uRectMaxLoc = glGetUniformLocation(s_shader, "uRectMax");
        s_uRadiusLoc = glGetUniformLocation(s_shader, "uRadius");
        s_uBgColorLoc = glGetUniformLocation(s_shader, "uBgColor");
        s_uBorderColorLoc = glGetUniformLocation(s_shader, "uBorderColor");
        s_uBorderThicknessLoc = glGetUniformLocation(s_shader, "uBorderThickness");
    }
    return true;
}

GuiPanel::GuiPanel() {}
GuiPanel::~GuiPanel() {}

void GuiPanel::addChild(GuiElement* element)
{
    if (!element) return;
    m_children.push_back(element);
}

void GuiPanel::removeChild(GuiElement* element)
{
    auto it = std::remove(m_children.begin(), m_children.end(), element);
    m_children.erase(it, m_children.end());
}

void GuiPanel::setBackgroundColor(float r, float g, float b, float a)
{
    m_bg[0]=r; m_bg[1]=g; m_bg[2]=b; m_bg[3]=a;
}

void GuiPanel::setBorderColor(float r, float g, float b, float a)
{
    m_border[0]=r; m_border[1]=g; m_border[2]=b; m_border[3]=a;
}

void GuiPanel::setBorderRadius(float r)
{
    if (r < 0.0f) r = 0.0f;
    m_radius = r;
}

void GuiPanel::setBorderThickness(float t)
{
    if (t < 0.0f) t = 0.0f;
    m_border_thickness = t;
}

void GuiPanel::setLayout(LayoutType type) { m_layout = type; }
void GuiPanel::setPadding(float p) { m_padding = (p < 0.0f) ? 0.0f : p; }
void GuiPanel::setSpacing(float s) { m_spacing = (s < 0.0f) ? 0.0f : s; }

void GuiPanel::draw()
{
    if (!m_visible) return;
    if (!ensure_gl_resources()) return;

    // Determine pixel rect for this panel
    float w = pixel_w();
    float h = pixel_h();
    if (w <= 0.0f || h <= 0.0f) return;

    // Support alignment relative to parent (framebuffer if none)
    float x = 0.0f, y = 0.0f;
    compute_aligned_xy(w, h, x, y);
    // Apply animations (offset + scaling) to this panel rect
    apply_animation_to_rect(x, y, w, h);

    // Render state
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    if (depthWasEnabled) glDisable(GL_DEPTH_TEST); // HUD overlay

    // Projection
    int fw=0, fh=0; get_framebuffer_size(fw, fh);
    float proj[16];
    make_ortho(0.0f, static_cast<float>(fw), 0.0f, static_cast<float>(fh), -1.0f, 1.0f, proj);

    // Draw panel quad (background + border via shader)
    draw_panel_quad(x, y, w, h);

    // Layout and draw children within inner rect
    layout_children(x, y, w, h);

    if (depthWasEnabled) glEnable(GL_DEPTH_TEST);
}

void GuiPanel::draw_panel_quad(float x, float y, float w, float h)
{
    // Vertex positions for the quad in pixel space (two triangles)
    float verts[12] = {
        x,     y,
        x,     y + h,
        x + w, y + h,

        x,     y,
        x + w, y + h,
        x + w, y
    };

    int fw=0, fh=0; get_framebuffer_size(fw, fh);
    float proj[16];
    make_ortho(0.0f, static_cast<float>(fw), 0.0f, static_cast<float>(fh), -1.0f, 1.0f, proj);

    glUseProgram(s_shader);
    glUniformMatrix4fv(s_uProjLoc, 1, GL_FALSE, proj);
    glUniform2f(s_uRectMinLoc, x, y);
    glUniform2f(s_uRectMaxLoc, x + w, y + h);
    glUniform1f(s_uRadiusLoc, m_radius);
    float bg_col[4];
    apply_animation_to_color(m_bg, bg_col);
    float border_col[4];
    apply_animation_to_color(m_border, border_col);
    glUniform4fv(s_uBgColorLoc, 1, bg_col);
    glUniform4fv(s_uBorderColorLoc, 1, border_col);
    glUniform1f(s_uBorderThicknessLoc, m_border_thickness);

    glBindVertexArray(s_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glUseProgram(0);
}

void GuiPanel::layout_children(float x, float y, float w, float h)
{
    // Inner content rect
    const float cx = x + m_padding;
    const float cy = y + m_padding;
    const float cw = std::max(0.0f, w - 2.0f * m_padding);
    const float ch = std::max(0.0f, h - 2.0f * m_padding);

    if (m_children.empty()) return;

    switch (m_layout) {
        case LayoutType::HORIZONTAL: {
            float pen_x = cx;
            for (auto* child : m_children) {
                if (!child || !child->visible()) continue;
                auto [pw, ph] = child->preferred_size();
                if (pw <= 0.0f) pw = 0.0f;
                if (ph <= 0.0f) ph = 0.0f;
                // Vertical alignment according to child's alignment inside panel's inner rect
                child->notify_parent_rect(cx, cy, cw, ch);
                auto pos = child->aligned_position_in(cx, cy, cw, ch, pw, ph);
                float px = pen_x;
                float py = pos.second; // honor vertical anchor
                child->set_position(px, py, false);
                child->draw();
                pen_x += pw + m_spacing;
            }
        } break;

        case LayoutType::VERTICAL: {
            float pen_y = cy + ch; // start at top
            for (auto* child : m_children) {
                if (!child || !child->visible()) continue;
                auto [pw, ph] = child->preferred_size();
                if (pw <= 0.0f) pw = 0.0f;
                if (ph <= 0.0f) ph = 0.0f;
                pen_y -= ph; // place this element
                // Horizontal alignment according to child's alignment inside panel's inner rect
                child->notify_parent_rect(cx, cy, cw, ch);
                auto pos = child->aligned_position_in(cx, cy, cw, ch, pw, ph);
                float px = pos.first; // honor horizontal anchor
                child->set_position(px, pen_y, false);
                child->draw();
                pen_y -= m_spacing;
            }
        } break;

        case LayoutType::GRID: {
            const int n = static_cast<int>(m_children.size());
            int cols = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(n))));
            if (cols <= 0) cols = 1;
            int rows = static_cast<int>(std::ceil(n / static_cast<float>(cols)));
            float cell_w = (cw - m_spacing * (cols - 1)) / static_cast<float>(cols);
            float cell_h = (ch - m_spacing * (rows - 1)) / static_cast<float>(rows);
            int idx = 0;
            for (auto* child : m_children) {
                if (!child || !child->visible()) { ++idx; continue; }
                int r = idx / cols;
                int c = idx % cols;
                float px = cx + c * (cell_w + m_spacing);
                float py = cy + ch - (r + 1) * cell_h - r * m_spacing; // top to bottom
                auto [pw, ph] = child->preferred_size();
                // center inside cell
                // Use child's alignment inside its cell rect
                child->notify_parent_rect(px, py, cell_w, cell_h);
                auto pos = child->aligned_position_in(px, py, cell_w, cell_h, pw, ph);
                child->set_position(pos.first, pos.second, false);
                child->draw();
                ++idx;
            }
        } break;

        case LayoutType::ABSOLUTE: {
            // Children are positioned solely according to their alignment/offset
            for (auto* child : m_children) {
                if (!child || !child->visible()) continue;
                child->notify_parent_rect(cx, cy, cw, ch);
                auto [pw, ph] = child->preferred_size();
                // If element has an explicit size, prefer it
                if (pw <= 0.0f || ph <= 0.0f) {
                    // keep preferred values; drawers may handle unknown sizes
                }
                auto pos = child->aligned_position_in(cx, cy, cw, ch, pw, ph);
                child->set_position(pos.first, pos.second, false);
                child->draw();
            }
        } break;
    }
}
