// GuiImage.cpp - Implementation of GuiImage

#include "GuiImage.h"
#include "GuiDraw.h"

#include <cstdio>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <cctype>

GuiImage::GuiImage() {}
GuiImage::~GuiImage() {
    if (m_tex) glDeleteTextures(1, &m_tex);
}

bool GuiImage::set_texture(const std::string& texture_path)
{
    // Try simple PPM (P6) first
    if (load_ppm(texture_path)) return true;
    std::fprintf(stderr, "[GuiImage] Failed to load '%s'. Using placeholder.\n", texture_path.c_str());
    create_placeholder();
    return false;
}

std::pair<float,float> GuiImage::preferred_size() const
{
    if (m_size_w > 0.0f && m_size_h > 0.0f) return {pixel_w(), pixel_h()};
    return {static_cast<float>(m_tex_w), static_cast<float>(m_tex_h)};
}

void GuiImage::draw()
{
    if (!m_visible) return;
    if (m_tex == 0) create_placeholder();
    if (m_tex == 0) return;

    float x = pixel_x();
    float y = pixel_y();
    float w = pixel_w();
    float h = pixel_h();
    if (w <= 0.0f || h <= 0.0f) {
        auto ps = preferred_size();
        if (w <= 0.0f) w = ps.first;
        if (h <= 0.0f) h = ps.second;
    }
    if (w <= 0.0f || h <= 0.0f) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    if (depthWasEnabled) glDisable(GL_DEPTH_TEST);

    GuiDraw::draw_textured_quad(x, y, w, h, m_tex);

    if (depthWasEnabled) glEnable(GL_DEPTH_TEST);
}

bool GuiImage::load_ppm(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return false;
    std::string magic;
    f >> magic;
    if (magic != "P6" && magic != "P3") return false;
    // skip comments and read width/height
    int w=0, h=0, maxv=0;
    auto skip_comments = [&]() {
        for (;;) {
            int c = f.peek();
            if (c == '#') { std::string dummy; std::getline(f, dummy); }
            else if (isspace(c)) { f.get(); }
            else break;
        }
    };
    skip_comments();
    f >> w >> h;
    skip_comments();
    f >> maxv;
    if (w <= 0 || h <= 0 || maxv <= 0 || maxv > 255) return false;
    if (magic == "P6") {
        f.get(); // single whitespace after maxv
        const size_t size = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
        std::vector<unsigned char> data(size);
        f.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(size));
        if (static_cast<size_t>(f.gcount()) != size) return false;

        if (m_tex) { glDeleteTextures(1, &m_tex); m_tex = 0; }
        glGenTextures(1, &m_tex);
        glBindTexture(GL_TEXTURE_2D, m_tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        m_tex_w = w; m_tex_h = h;
        return true;
    } else {
        // P3 ASCII
        std::vector<unsigned char> data;
        data.reserve(static_cast<size_t>(w)*static_cast<size_t>(h)*3u);
        for (int i = 0; i < w*h; ++i) {
            int r=0,g=0,b=0; f >> r >> g >> b;
            data.push_back(static_cast<unsigned char>(r));
            data.push_back(static_cast<unsigned char>(g));
            data.push_back(static_cast<unsigned char>(b));
        }
        if (static_cast<int>(data.size()) != w*h*3) return false;

    if (m_tex) { glDeleteTextures(1, &m_tex); m_tex = 0; }
    glGenTextures(1, &m_tex);
    glBindTexture(GL_TEXTURE_2D, m_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_tex_w = w; m_tex_h = h;
    return true;
    }
}

void GuiImage::create_placeholder()
{
    // 2x2 checkerboard RGBA
    const unsigned char pixels[2*2*4] = {
        200,200,210,255,  80,80,100,255,
         80,80,100,255, 200,200,210,255,
    };
    if (m_tex) glDeleteTextures(1, &m_tex);
    glGenTextures(1, &m_tex);
    glBindTexture(GL_TEXTURE_2D, m_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    m_tex_w = 64; m_tex_h = 64; // nominal preferred size
}
