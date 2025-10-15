// GuiText.cpp - Implementation of GuiText using FreeType and OpenGL

#include "GuiText.h"

#include <glad/glad.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <vector>
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <algorithm>

// Static storage
std::unordered_map<GuiText::FontKey, GuiText::GlyphMap, GuiText::FontKeyHash> GuiText::s_glyph_cache;
unsigned int GuiText::s_vao = 0;
unsigned int GuiText::s_vbo = 0;
unsigned int GuiText::s_shader = 0;
int GuiText::s_uProjLoc = -1;
int GuiText::s_uTextColorLoc = -1;
void* GuiText::s_ft_library = nullptr; // FT_Library
int GuiText::s_fb_width = 0;
int GuiText::s_fb_height = 0;

namespace {

// Simple orthographic projection matrix, origin at bottom-left, z range [-1,1]
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
        std::fprintf(stderr, "[GuiText] Shader compile error (%s):\n%s\n", type==GL_VERTEX_SHADER?"VERTEX":"FRAGMENT", log.c_str());
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
        std::fprintf(stderr, "[GuiText] Program link error:\n%s\n", log.c_str());
        glDeleteProgram(p);
        return 0;
    }
    return p;
}

} // namespace

GuiText::GuiText() { /* lazy init in draw */ }
GuiText::~GuiText() { /* glyph cache persists for process lifetime */ }

void GuiText::set_position(float x, float y, bool in_percentage) {
    GuiElement::set_position(x, y, in_percentage);
}

void GuiText::set_text(const std::string& str) {
    m_text = str;
}

bool GuiText::set_text_font(const std::string& font_path) {
    m_font_path = font_path;
    m_font_ready = false; // will load lazily on next draw or preferred_size
    return true;
}

void GuiText::set_text_size(int size_1_to_10) {
    if (size_1_to_10 < 1) size_1_to_10 = 1;
    if (size_1_to_10 > 10) size_1_to_10 = 10;
    if (m_size_level != size_1_to_10) {
        m_size_level = size_1_to_10;
        m_font_ready = false; // pixel size changed: refresh glyphs
    }
}

void GuiText::set_text_color(float r, float g, float b, float a) {
    m_color[0]=r; m_color[1]=g; m_color[2]=b; m_color[3]=a;
}

void GuiText::show() { m_visible = true; }
void GuiText::hide() { m_visible = false; }

void GuiText::on_framebuffer_resized(int fb_width, int fb_height) {
    s_fb_width = fb_width;
    s_fb_height = fb_height;
}

bool GuiText::init_renderer()
{
    if (s_shader != 0 && s_vao != 0 && s_vbo != 0 && s_ft_library != nullptr) return true;

    // Init FreeType
    if (!s_ft_library) {
        FT_Library lib = nullptr;
        if (FT_Init_FreeType(&lib) != 0) {
            std::fprintf(stderr, "[GuiText] FreeType init failed.\n");
            return false;
        }
        s_ft_library = lib;
    }

    // GL objects
    if (s_vao == 0) {
        glGenVertexArrays(1, &s_vao);
        glBindVertexArray(s_vao);
        glGenBuffers(1, &s_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
        // dynamic buffer for quads: each vertex = 4 floats (x,y,u,v)
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glBindVertexArray(0);
    }

    if (s_shader == 0) {
        static const char* VERT = R"GLSL(
            #version 330 core
            layout(location = 0) in vec4 aPosUV; // x,y,u,v
            uniform mat4 uProjection;
            out vec2 vUV;
            void main() {
                vUV = aPosUV.zw;
                gl_Position = uProjection * vec4(aPosUV.xy, 0.0, 1.0);
            }
        )GLSL";
        static const char* FRAG = R"GLSL(
            #version 330 core
            in vec2 vUV;
            out vec4 FragColor;
            uniform sampler2D uTex;
            uniform vec4 uTextColor;
            void main() {
                float a = texture(uTex, vUV).r; // glyph stored in RED
                FragColor = vec4(uTextColor.rgb, uTextColor.a * a);
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
        s_uTextColorLoc = glGetUniformLocation(s_shader, "uTextColor");
        int uTexLoc = glGetUniformLocation(s_shader, "uTex");
        glUseProgram(s_shader);
        if (uTexLoc >= 0) glUniform1i(uTexLoc, 0);
        glUseProgram(0);
    }
    return true;
}

void GuiText::shutdown_renderer()
{
    // Not used; kept for completeness if later needed.
}

int GuiText::pixel_size_for_level() const
{
    const int base = 18;
    const int step = 6;
    return base + (m_size_level - 1) * step; // 18,24,30,...,72
}

float GuiText::pixel_x_from_pos() const {
    if (m_pos_is_percent) return (s_fb_width > 0 ? (m_pos_x * 0.01f * s_fb_width) : m_pos_x);
    return m_pos_x;
}

float GuiText::pixel_y_from_pos() const {
    if (m_pos_is_percent) return (s_fb_height > 0 ? (m_pos_y * 0.01f * s_fb_height) : m_pos_y);
    return m_pos_y;
}

bool GuiText::ensure_font_loaded() const
{
    if (m_font_ready) return true;
    if (m_font_path.empty()) {
        std::fprintf(stderr, "[GuiText] No font path set. Call set_text_font().\n");
        return false;
    }
    if (!init_renderer()) return false;

    const int px = pixel_size_for_level();
    FontKey key{m_font_path, px};
    auto it = s_glyph_cache.find(key);
    if (it != s_glyph_cache.end()) {
        m_font_ready = true;
        return true;
    }

    // Load face
    FT_Library lib = reinterpret_cast<FT_Library>(s_ft_library);
    FT_Face face = nullptr;
    if (FT_New_Face(lib, m_font_path.c_str(), 0, &face) != 0) {
        std::fprintf(stderr, "[GuiText] Failed to load font face: %s\n", m_font_path.c_str());
        return false;
    }
    FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(px));

    // Setup GL unpack alignment for single channel glyph bitmaps
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    GlyphMap glyphs;
    glyphs.reserve(96);

    // ASCII range 32..126 (printables)
    for (unsigned long c = 32; c <= 126; ++c) {
        if (FT_Load_Char(face, static_cast<FT_ULong>(c), FT_LOAD_RENDER) != 0) {
            std::fprintf(stderr, "[GuiText] FT_Load_Char failed for '%c' (U+%lu)\n", (char)c, c);
            continue;
        }
        FT_GlyphSlot g = face->glyph;

        unsigned int tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        // Store grayscale in RED channel
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_R8,
            g->bitmap.width,
            g->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            g->bitmap.buffer
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        GLint swizzleMask[] = {GL_RED, GL_RED, GL_RED, GL_RED};
        glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);

        Glyph ch;
        ch.texture_id = tex;
        ch.width = g->bitmap.width;
        ch.height = g->bitmap.rows;
        ch.bearing_x = g->bitmap_left;
        ch.bearing_y = g->bitmap_top;
        ch.advance = static_cast<unsigned int>(g->advance.x);

        glyphs.emplace(c, ch);
    }

    FT_Done_Face(face);

    s_glyph_cache.emplace(std::move(key), std::move(glyphs));
    m_font_ready = true;
    return true;
}

float GuiText::text_width_pixels() const
{
    if (m_text.empty()) return 0.0f;
    if (!ensure_font_loaded()) return 0.0f;
    const int px = pixel_size_for_level();
    FontKey key{m_font_path, px};
    auto it = s_glyph_cache.find(key);
    if (it == s_glyph_cache.end()) return 0.0f;
    const GlyphMap& glyphs = it->second;
    float width = 0.0f;
    for (unsigned char ch : m_text) {
        auto ig = glyphs.find(ch);
        if (ig == glyphs.end()) continue;
        const Glyph& g = ig->second;
        width += static_cast<float>(g.advance >> 6);
    }
    return width;
}

std::pair<float,float> GuiText::preferred_size() const
{
    // width = sum of advances, height = pixel size
    float w = text_width_pixels();
    float h = static_cast<float>(pixel_size_for_level());
    return {w, h};
}

bool GuiText::vertical_extents(float& ascent, float& descent) const
{
    ascent = 0.0f; descent = 0.0f;
    if (m_text.empty()) return false;
    if (!ensure_font_loaded()) return false;

    const int px = pixel_size_for_level();
    FontKey key{m_font_path, px};
    auto it = s_glyph_cache.find(key);
    if (it == s_glyph_cache.end()) return false;
    const GlyphMap& glyphs = it->second;

    float a = 0.0f, d = 0.0f;
    for (unsigned char ch : m_text) {
        auto ig = glyphs.find(ch);
        if (ig == glyphs.end()) continue;
        const Glyph& g = ig->second;
        a = std::max(a, static_cast<float>(g.bearing_y));
        d = std::max(d, static_cast<float>(g.height - g.bearing_y));
    }
    if (a == 0.0f && d == 0.0f) {
        // Fallback heuristic: typical ascent/descent split
        a = px * 0.8f;
        d = px * 0.2f;
    }
    ascent = a; descent = d;
    return true;
}

void GuiText::draw()
{
    if (!m_visible) return;
    if (m_text.empty()) return;
    if (!ensure_font_loaded()) return;
    if (!init_renderer()) return;

    // Ensure we have framebuffer size (if user didn't notify via on_framebuffer_resized)
    if (s_fb_width <= 0 || s_fb_height <= 0) {
        GLint vp[4] = {0,0,0,0};
        glGetIntegerv(GL_VIEWPORT, vp);
        s_fb_width = vp[2];
        s_fb_height = vp[3];
    }

    float proj[16];
    make_ortho(0.0f, static_cast<float>(s_fb_width), 0.0f, static_cast<float>(s_fb_height), -1.0f, 1.0f, proj);

    const int px = pixel_size_for_level();
    FontKey key{m_font_path, px};
    auto it = s_glyph_cache.find(key);
    if (it == s_glyph_cache.end()) return; // should not happen
    const GlyphMap& glyphs = it->second;

    // Determine bounding box from alignment or manual position
    float x = pixel_x_from_pos();
    float y = pixel_y_from_pos();
    // Prefer bounding box width/height for anchor computation
    float box_w = text_width_pixels();
    float box_h = static_cast<float>(pixel_size_for_level());
    if (position_mode() == PositionMode::Aligned && !m_has_parent) {
        // Use framebuffer as parent if none was provided
        // compute_aligned_xy expects bottom-left of the bounding box
        compute_aligned_xy(box_w, box_h, x, y);
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    if (depthWasEnabled) glDisable(GL_DEPTH_TEST); // HUD overlay

    glActiveTexture(GL_TEXTURE0);
    glUseProgram(s_shader);
    glUniformMatrix4fv(s_uProjLoc, 1, GL_FALSE, proj);
    glUniform4fv(s_uTextColorLoc, 1, m_color);

    glBindVertexArray(s_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);

    float pen_x = x;
    float baseline_y = y; // y is currently bottom of bounding box
    float asc = 0.0f, desc = 0.0f;
    bool have_extents = vertical_extents(asc, desc);
    if (have_extents) {
        baseline_y = y + desc; // move from bottom of box to baseline
    }

    for (unsigned char ch : m_text) {
        auto ig = glyphs.find(ch);
        if (ig == glyphs.end()) continue;
        const Glyph& g = ig->second;

        float xpos = pen_x + static_cast<float>(g.bearing_x);
        float ypos = baseline_y - static_cast<float>(g.height - g.bearing_y);
        float w = static_cast<float>(g.width);
        float h = static_cast<float>(g.height);

        float verts[6][4] = {
            { xpos,     ypos + h, 0.0f, 0.0f },
            { xpos,     ypos,     0.0f, 1.0f },
            { xpos + w, ypos,     1.0f, 1.0f },

            { xpos,     ypos + h, 0.0f, 0.0f },
            { xpos + w, ypos,     1.0f, 1.0f },
            { xpos + w, ypos + h, 1.0f, 0.0f },
        };

        glBindTexture(GL_TEXTURE_2D, g.texture_id);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        pen_x += static_cast<float>(g.advance >> 6);
    }

    glBindVertexArray(0);
    glUseProgram(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (depthWasEnabled) glEnable(GL_DEPTH_TEST);
}
