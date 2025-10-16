// GuiButton.cpp - Implementation of GuiButton

#include "GuiButton.h"

#include <cstdio>
#include <cmath>

// Static GL resources
unsigned int GuiButton::s_vao = 0;
unsigned int GuiButton::s_vbo = 0;
unsigned int GuiButton::s_shader = 0;
int GuiButton::s_uProjLoc = -1;
int GuiButton::s_uRectMinLoc = -1;
int GuiButton::s_uRectMaxLoc = -1;
int GuiButton::s_uRadiusLoc = -1;
int GuiButton::s_uBgColorLoc = -1;

// Static input state
double GuiButton::s_mouse_x_px = 0.0;
double GuiButton::s_mouse_y_px = 0.0;
bool   GuiButton::s_left_down = false;
bool   GuiButton::s_left_clicked = false;

namespace {

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
        std::fprintf(stderr, "[GuiButton] Shader compile error (%s):\n%s\n", type==GL_VERTEX_SHADER?"VERTEX":"FRAGMENT", log.c_str());
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
        std::fprintf(stderr, "[GuiButton] Program link error:\n%s\n", log.c_str());
        glDeleteProgram(p);
        return 0;
    }
    return p;
}

} // namespace

GuiButton::GuiButton() {
    // Reasonable defaults for label
    m_label.set_text("Button");
    m_label.set_text_size(3);
}

GuiButton::~GuiButton() {}

void GuiButton::set_bg_color(float r, float g, float b, float a) {
    m_bg[0]=r; m_bg[1]=g; m_bg[2]=b; m_bg[3]=a;
}

void GuiButton::set_hover_color(float r, float g, float b, float a) {
    m_hover_bg[0]=r; m_hover_bg[1]=g; m_hover_bg[2]=b; m_hover_bg[3]=a;
}

void GuiButton::onHover() {
    if (m_on_hover) m_on_hover();
}

void GuiButton::onClick() {
    if (m_on_click) m_on_click();
}

// static
void GuiButton::begin_frame() {
    // Reset per-frame one-shot flags before polling events
    s_left_clicked = false;
}

// static
void GuiButton::glfw_cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
    // Convert from window screen coords (origin top-left, possibly HiDPI) to framebuffer pixel coords (origin bottom-left)
    int ww=1, wh=1, fbw=1, fbh=1;
    glfwGetWindowSize(window, &ww, &wh);
    glfwGetFramebufferSize(window, &fbw, &fbh);
    double sx = ww > 0 ? (static_cast<double>(fbw) / static_cast<double>(ww)) : 1.0;
    double sy = wh > 0 ? (static_cast<double>(fbh) / static_cast<double>(wh)) : 1.0;
    s_mouse_x_px = xpos * sx;
    double ypos_px_top = ypos * sy;
    s_mouse_y_px = static_cast<double>(fbh) - ypos_px_top;
}

// static
void GuiButton::glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int /*mods*/)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            // Ensure we have up-to-date mouse position when click occurs
            double xpos=0.0, ypos=0.0;
            glfwGetCursorPos(window, &xpos, &ypos);
            int ww=1, wh=1, fbw=1, fbh=1;
            glfwGetWindowSize(window, &ww, &wh);
            glfwGetFramebufferSize(window, &fbw, &fbh);
            double sx = ww > 0 ? (static_cast<double>(fbw) / static_cast<double>(ww)) : 1.0;
            double sy = wh > 0 ? (static_cast<double>(fbh) / static_cast<double>(wh)) : 1.0;
            s_mouse_x_px = xpos * sx;
            double ypos_px_top = ypos * sy;
            s_mouse_y_px = static_cast<double>(fbh) - ypos_px_top;

            s_left_down = true;
            s_left_clicked = true; // will be consumed by first button hit in this frame
        } else if (action == GLFW_RELEASE) {
            s_left_down = false;
        }
    }
}

bool GuiButton::ensure_gl_resources()
{
    if (s_shader && s_vao && s_vbo) return true;

    if (s_vao == 0) {
        glGenVertexArrays(1, &s_vao);
        glBindVertexArray(s_vao);
        glGenBuffers(1, &s_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12, nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glBindVertexArray(0);
    }

    if (s_shader == 0) {
        static const char* VERT = R"GLSL(
            #version 330 core
            layout(location = 0) in vec2 aPos; // pixel space
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

            float sdRoundBox(vec2 p, vec2 b, float r)
            {
                vec2 q = abs(p) - (b - vec2(r));
                return length(max(q, 0.0)) - r;
            }

            void main() {
                vec2 rectSize = uRectMax - uRectMin;
                vec2 center = uRectMin + rectSize * 0.5;
                vec2 p = gl_FragCoord.xy - center; // bottom-left origin
                vec2 halfSize = rectSize * 0.5;
                float d = sdRoundBox(p, halfSize, uRadius);
                if (d > 0.0) discard; // outside
                FragColor = uBgColor;
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
    }
    return true;
}

void GuiButton::make_ortho(float left, float right, float bottom, float top, float znear, float zfar, float out[16])
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

std::pair<float,float> GuiButton::preferred_size() const
{
    // If explicit size set on base, honor it
    if (m_size_w > 0.0f && m_size_h > 0.0f) return {pixel_w(), pixel_h()};
    auto label_pref = m_label.preferred_size();
    float w = label_pref.first + 2.0f * m_pad_x;
    float h = label_pref.second + 2.0f * m_pad_y;
    return {w, h};
}

bool GuiButton::hit_test(float px, float py, float x, float y, float w, float h) const
{
    return (px >= x && px <= x + w && py >= y && py <= y + h);
}

void GuiButton::draw()
{
    if (!m_visible) return;
    if (!ensure_gl_resources()) return;

    // Compute pixel rect
    float x = pixel_x();
    float y = pixel_y();
    float w = pixel_w();
    float h = pixel_h();
    if (w <= 0.0f || h <= 0.0f) {
        auto pref = preferred_size();
        w = (w > 0.0f) ? w : pref.first;
        h = (h > 0.0f) ? h : pref.second;
    }
    if (w <= 0.0f || h <= 0.0f) return;

    // Apply alignment only when not placed by a container.
    if (position_mode() == PositionMode::Aligned && !m_has_parent) {
        compute_aligned_xy(w, h, x, y);
    }

    // Apply animations (position offset + scaling)
    apply_animation_to_rect(x, y, w, h);

    // Hover/Click detection (onHover on edge: enter only)
    const bool hovered = hit_test(static_cast<float>(s_mouse_x_px), static_cast<float>(s_mouse_y_px), x, y, w, h);
    if (hovered && !m_hovered_prev) onHover();
    bool clicked_now = false;
    if (hovered && s_left_clicked) {
        clicked_now = true;
        s_left_clicked = false; // consume click for this frame
    }
    if (clicked_now) onClick();
    m_hovered_prev = hovered;

    // Prepare GL state
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    if (depthWasEnabled) glDisable(GL_DEPTH_TEST);

    int fw=0, fh=0; get_framebuffer_size(fw, fh);
    float proj[16];
    make_ortho(0.0f, static_cast<float>(fw), 0.0f, static_cast<float>(fh), -1.0f, 1.0f, proj);

    // Choose color
    const float* color = hovered ? m_hover_bg : m_bg;
    float final_col[4];
    apply_animation_to_color(color, final_col);

    // Quad verts
    float verts[12] = {
        x,     y,
        x,     y + h,
        x + w, y + h,
        x,     y,
        x + w, y + h,
        x + w, y
    };

    glUseProgram(s_shader);
    glUniformMatrix4fv(s_uProjLoc, 1, GL_FALSE, proj);
    glUniform2f(s_uRectMinLoc, x, y);
    glUniform2f(s_uRectMaxLoc, x + w, y + h);
    glUniform1f(s_uRadiusLoc, m_radius);
    glUniform4fv(s_uBgColorLoc, 1, final_col);

    glBindVertexArray(s_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glUseProgram(0);

    // Position label (baseline). Simple placement: left padding + baseline slightly below vertical center.
    // Compute precise baseline to center the text bounding box vertically
    float asc = 0.0f, desc = 0.0f;
    bool have_extents = m_label.vertical_extents(asc, desc);
    float label_x = x + m_pad_x;
    float center_y = y + h * 0.5f;
    float baseline_y;
    if (have_extents) {
        // Center the glyph box: baseline at center minus half (ascent - descent)
        baseline_y = center_y - 0.5f * (asc - desc);
    } else {
        // Fallback using preferred height
        float label_h = m_label.preferred_size().second;
        baseline_y = y + (h - label_h) * 0.5f + label_h * 0.6f;
    }
    m_label.set_position(label_x, baseline_y, false);
    m_label.draw();

    if (depthWasEnabled) glEnable(GL_DEPTH_TEST);
}
