// GuiDraw.h - Lightweight 2D drawing helpers (rectangles, rounded rects, textured quads)
#pragma once

#include <glad/glad.h>

namespace GuiDraw {

// Ensure GL resources for solid rectangle/rounded-rect rendering
bool ensure_rect_renderer();
// Draw a filled rectangle with optional rounded corners
void draw_rounded_rect(float x, float y, float w, float h, float radius, const float color[4]);
inline void draw_rect(float x, float y, float w, float h, const float color[4]) {
    draw_rounded_rect(x, y, w, h, 0.0f, color);
}

// Ensure GL resources for textured quad
bool ensure_tex_renderer();
// Draw a textured quad (no tint), texture must be GL_TEXTURE_2D
void draw_textured_quad(float x, float y, float w, float h, GLuint texture);

} // namespace GuiDraw

