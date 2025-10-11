// GuiImage.h - Simple image display element
#pragma once

#include <string>
#include <glad/glad.h>

#include "GuiElement.h"

class GuiImage : public GuiElement {
public:
    GuiImage();
    ~GuiImage();

    bool set_texture(const std::string& texture_path); // loads image from path (PPM P6 supported), returns success
    void set_size(float width, float height) { GuiElement::set_size(width, height, false); }
    void set_position(float x, float y) { GuiElement::set_position(x, y, false); }

    void draw() override;
    std::pair<float,float> preferred_size() const override;

private:
    bool load_ppm(const std::string& path);
    void create_placeholder();

private:
    GLuint m_tex = 0;
    int m_tex_w = 0;
    int m_tex_h = 0;
};

