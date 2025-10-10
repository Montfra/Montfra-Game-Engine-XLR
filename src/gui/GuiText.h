// GuiText.h - 2D Text rendering with FreeType + OpenGL 3.3
#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include "GuiElement.h"

// Public API: GuiText class for HUD/menus overlay rendering.
// Coordinates are in screen pixels by default (origin at bottom-left),
// or as percentage of framebuffer size when in_percentage = true.
class GuiText : public GuiElement {
public:
    GuiText();
    ~GuiText();

    // Core setters
    void set_position(float x, float y, bool in_percentage = false); // override position handling
    void set_text(const std::string& str);
    bool set_text_font(const std::string& font_path); // returns true on success
    void set_text_size(int size_1_to_10);             // relative scale 1..10
    void set_text_color(float r, float g, float b, float a);

    // Optional helpers
    void show();
    void hide();
    bool visible() const { return m_visible; }

    // Draw the text. Requires a current OpenGL context.
    void draw() override;

    // Preferred size (in pixels) based on current text and font/size
    std::pair<float,float> preferred_size() const override;

    // Compute vertical extents of the current text in pixels relative to the baseline.
    // - ascent: max distance above baseline
    // - descent: max distance below baseline
    // Returns false if font/text not ready; outputs are zeroed.
    bool vertical_extents(float& ascent, float& descent) const;

    // Allow dynamic update on framebuffer resize if needed in future.
    static void on_framebuffer_resized(int fb_width, int fb_height);

private:
    // Internals
    bool ensure_font_loaded() const; // lazy-load selected font
    static bool init_renderer();     // lazy GL/FT init for shared resources
    static void shutdown_renderer();

    float pixel_x_from_pos() const; // computes pixel x from pos/percent
    float pixel_y_from_pos() const; // computes pixel y from pos/percent
    int   pixel_size_for_level() const; // maps 1..10 to pixel size
    float text_width_pixels() const;    // measured width based on glyph advances

    // Data
    std::string m_text;
    std::string m_font_path;
    mutable bool m_font_ready = false;
    int  m_size_level = 5; // 1..10
    float m_color[4] = {1.f, 1.f, 1.f, 1.f};

    // Rendering backend (shared between all GuiText instances)
    struct Glyph {
        unsigned int texture_id = 0; // GL texture
        int width = 0;
        int height = 0;
        int bearing_x = 0; // left bearing
        int bearing_y = 0; // top bearing
        unsigned int advance = 0; // advance.x in 1/64 pixels (FreeType)
    };

    struct FontKey {
        std::string path;
        int pixel_size;
        bool operator==(const FontKey& o) const {
            return pixel_size == o.pixel_size && path == o.path;
        }
    };

    struct FontKeyHash {
        std::size_t operator()(const FontKey& k) const noexcept {
            std::hash<std::string> h;
            return (h(k.path) ^ (static_cast<std::size_t>(k.pixel_size) * 1315423911u));
        }
    };

    // Cache glyphs per (font_path, pixel_size)
    using GlyphMap = std::unordered_map<unsigned long, Glyph>; // codepoint -> glyph
    static std::unordered_map<FontKey, GlyphMap, FontKeyHash> s_glyph_cache;

    // Shared GL objects
    static unsigned int s_vao;
    static unsigned int s_vbo;
    static unsigned int s_shader;
    static int s_uProjLoc;
    static int s_uTextColorLoc;

    // FreeType
    static void* s_ft_library; // FT_Library (void* to avoid including ft headers in header file)

    // Framebuffer size for ortho projection
    static int s_fb_width;
    static int s_fb_height;
};
