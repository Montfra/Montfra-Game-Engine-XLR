// GuiDraw.cpp - Implementation of simple 2D drawing helpers

#include "GuiDraw.h"

#include <glad/glad.h>
#include <string>

namespace {

static unsigned int s_rect_vao = 0;
static unsigned int s_rect_vbo = 0;
static unsigned int s_rect_shader = 0;
static int s_rect_uProjLoc = -1;
static int s_rect_uRectMinLoc = -1;
static int s_rect_uRectMaxLoc = -1;
static int s_rect_uRadiusLoc = -1;
static int s_rect_uColorLoc = -1;

static unsigned int s_tex_vao = 0;
static unsigned int s_tex_vbo = 0;
static unsigned int s_tex_shader = 0;
static int s_tex_uProjLoc = -1;
static int s_tex_uSamplerLoc = -1;

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
        // Silent in release helpers
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
        glDeleteProgram(p);
        return 0;
    }
    return p;
}

} // namespace

namespace GuiDraw {

bool ensure_rect_renderer()
{
    if (s_rect_shader && s_rect_vao && s_rect_vbo) return true;
    if (s_rect_vao == 0) {
        glGenVertexArrays(1, &s_rect_vao);
        glBindVertexArray(s_rect_vao);
        glGenBuffers(1, &s_rect_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, s_rect_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 12, nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glBindVertexArray(0);
    }
    if (s_rect_shader == 0) {
        static const char* VERT = R"GLSL(
            #version 330 core
            layout(location = 0) in vec2 aPos;
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
            uniform vec4 uColor;
            float sdRoundBox(vec2 p, vec2 b, float r){ vec2 q=abs(p)-(b-vec2(r)); return length(max(q,0.0))-r; }
            void main(){
                vec2 size = uRectMax - uRectMin;
                vec2 center = uRectMin + size * 0.5;
                vec2 p = gl_FragCoord.xy - center;
                vec2 halfSize = size * 0.5;
                float d = sdRoundBox(p, halfSize, uRadius);
                if (d > 0.0) discard;
                FragColor = uColor;
            }
        )GLSL";
        GLuint vs = compile_shader(GL_VERTEX_SHADER, VERT);
        GLuint fs = compile_shader(GL_FRAGMENT_SHADER, FRAG);
        if (!vs || !fs) return false;
        s_rect_shader = link_program(vs, fs);
        glDeleteShader(vs);
        glDeleteShader(fs);
        if (!s_rect_shader) return false;
        s_rect_uProjLoc = glGetUniformLocation(s_rect_shader, "uProjection");
        s_rect_uRectMinLoc = glGetUniformLocation(s_rect_shader, "uRectMin");
        s_rect_uRectMaxLoc = glGetUniformLocation(s_rect_shader, "uRectMax");
        s_rect_uRadiusLoc = glGetUniformLocation(s_rect_shader, "uRadius");
        s_rect_uColorLoc = glGetUniformLocation(s_rect_shader, "uColor");
    }
    return true;
}

void draw_rounded_rect(float x, float y, float w, float h, float radius, const float color[4])
{
    if (!ensure_rect_renderer()) return;
    // Quad verts
    float verts[12] = {
        x,     y,
        x,     y + h,
        x + w, y + h,
        x,     y,
        x + w, y + h,
        x + w, y
    };

    // Ortho from current framebuffer size
    GLint vp[4] = {0,0,0,0};
    glGetIntegerv(GL_VIEWPORT, vp);
    float proj[16];
    make_ortho(0.0f, static_cast<float>(vp[2]), 0.0f, static_cast<float>(vp[3]), -1.0f, 1.0f, proj);

    glUseProgram(s_rect_shader);
    glUniformMatrix4fv(s_rect_uProjLoc, 1, GL_FALSE, proj);
    glUniform2f(s_rect_uRectMinLoc, x, y);
    glUniform2f(s_rect_uRectMaxLoc, x + w, y + h);
    glUniform1f(s_rect_uRadiusLoc, radius);
    glUniform4fv(s_rect_uColorLoc, 1, color);

    glBindVertexArray(s_rect_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_rect_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glUseProgram(0);
}

bool ensure_tex_renderer()
{
    if (s_tex_shader && s_tex_vao && s_tex_vbo) return true;
    if (s_tex_vao == 0) {
        glGenVertexArrays(1, &s_tex_vao);
        glBindVertexArray(s_tex_vao);
        glGenBuffers(1, &s_tex_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, s_tex_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2*sizeof(float)));
        glBindVertexArray(0);
    }
    if (s_tex_shader == 0) {
        static const char* VERT = R"GLSL(
            #version 330 core
            layout(location = 0) in vec2 aPos;
            layout(location = 1) in vec2 aUV;
            out vec2 vUV;
            uniform mat4 uProjection;
            void main(){ vUV = aUV; gl_Position = uProjection * vec4(aPos.xy, 0.0, 1.0); }
        )GLSL";
        static const char* FRAG = R"GLSL(
            #version 330 core
            in vec2 vUV;
            out vec4 FragColor;
            uniform sampler2D uTex;
            void main(){ FragColor = texture(uTex, vUV); }
        )GLSL";
        GLuint vs = compile_shader(GL_VERTEX_SHADER, VERT);
        GLuint fs = compile_shader(GL_FRAGMENT_SHADER, FRAG);
        if (!vs || !fs) return false;
        s_tex_shader = link_program(vs, fs);
        glDeleteShader(vs);
        glDeleteShader(fs);
        if (!s_tex_shader) return false;
        s_tex_uProjLoc = glGetUniformLocation(s_tex_shader, "uProjection");
        s_tex_uSamplerLoc = glGetUniformLocation(s_tex_shader, "uTex");
    }
    return true;
}

void draw_textured_quad(float x, float y, float w, float h, GLuint texture)
{
    if (!ensure_tex_renderer()) return;
    float verts[24] = {
        // x, y, u, v
        x,     y,     0.0f, 0.0f,
        x,     y + h, 0.0f, 1.0f,
        x + w, y + h, 1.0f, 1.0f,
        x,     y,     0.0f, 0.0f,
        x + w, y + h, 1.0f, 1.0f,
        x + w, y,     1.0f, 0.0f,
    };

    GLint vp[4] = {0,0,0,0};
    glGetIntegerv(GL_VIEWPORT, vp);
    float proj[16];
    make_ortho(0.0f, static_cast<float>(vp[2]), 0.0f, static_cast<float>(vp[3]), -1.0f, 1.0f, proj);

    glUseProgram(s_tex_shader);
    glUniformMatrix4fv(s_tex_uProjLoc, 1, GL_FALSE, proj);
    glUniform1i(s_tex_uSamplerLoc, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glBindVertexArray(s_tex_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_tex_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glUseProgram(0);
}

} // namespace GuiDraw
