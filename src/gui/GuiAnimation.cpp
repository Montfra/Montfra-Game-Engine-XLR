// GuiAnimation.cpp - Implementation of animation primitives

#include "GuiAnimation.h"
#include "GuiElement.h"
#include <glad/glad.h>

static inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

void FadeAnimation::on_apply(GuiElement& e, float et) {
    float v = lerp(m_from, m_to, et);
    if (v < 0.f) v = 0.f; if (v > 1.f) v = 1.f;
    e.anim_mul_alpha(v);
}

void MoveAnimation::on_apply(GuiElement& e, float et) {
    float ox = lerp(m_sx, m_tx, et);
    float oy = lerp(m_sy, m_ty, et);
    e.anim_add_offset(ox, oy);
}

void ScaleAnimation::on_apply(GuiElement& e, float et) {
    float s = lerp(m_from, m_to, et);
    if (s < 0.001f) s = 0.001f;
    e.anim_mul_scale(s, s);
}

void PulseAnimation::on_apply(GuiElement& e, float et) {
    // et in [0..1]; map to sin(0..pi): produces one pulse
    float v = std::sin(et * 3.1415926535f);
    float s = 1.0f + (m_max - 1.0f) * v; // goes from 1 -> max -> 1
    e.anim_mul_scale(s, s);
}

void ColorAnimation::on_start(GuiElement& e) {
    (void)e;
    // Start from current override if any; since we cannot query element base color here,
    // use white if no override currently set.
    // Concrete start color is latched in first on_apply where et=0.
}

void ColorAnimation::on_apply(GuiElement& e, float et) {
    // Interpolate override color
    // On the first frame (et ~ 0), latch current override or white
    static const float WHITE[4] = {1.f,1.f,1.f,1.f};
    // We don't have a getter; we just linearly approach target from m_from defaulting to WHITE.
    float r = lerp(m_from[0], m_tgt[0], et);
    float g = lerp(m_from[1], m_tgt[1], et);
    float b = lerp(m_from[2], m_tgt[2], et);
    float a = lerp(m_from[3], m_tgt[3], et);
    // To allow correct start from WHITE, keep m_from as white unless on_apply first set is captured
    if (!m_started) {
        (void)WHITE;
    }
    e.anim_set_color_override(r, g, b, a);
}

void ShakeAnimation::on_apply(GuiElement& e, float et) {
    // Exponential decay of amplitude, sinusoidal oscillation
    float amp = m_amp * (1.0f - et);
    float t_total = m_elapsed; // seconds
    float phase = 2.0f * 3.1415926535f * m_freq * t_total;
    float dx = std::sin(phase) * amp;
    e.anim_add_offset(dx, 0.0f);
}

void SlideAnimation::on_apply(GuiElement& e, float et) {
    // Compute framebuffer size
    GLint vp[4] = {0,0,0,0};
    glGetIntegerv(GL_VIEWPORT, vp);
    float fw = static_cast<float>(vp[2]);
    float fh = static_cast<float>(vp[3]);

    float start_x = 0.f, start_y = 0.f;
    float end_x = 0.f, end_y = 0.f;
    switch (m_dir) {
        case Dir::Left:  start_x = (m_type == Type::In ? -fw : 0.f); end_x = (m_type == Type::In ? 0.f : -fw); break;
        case Dir::Right: start_x = (m_type == Type::In ? +fw : 0.f); end_x = (m_type == Type::In ? 0.f : +fw); break;
        case Dir::Up:    start_y = (m_type == Type::In ? +fh : 0.f); end_y = (m_type == Type::In ? 0.f : +fh); break;
        case Dir::Down:  start_y = (m_type == Type::In ? -fh : 0.f); end_y = (m_type == Type::In ? 0.f : -fh); break;
    }
    float ox = lerp(start_x, end_x, et);
    float oy = lerp(start_y, end_y, et);
    e.anim_add_offset(ox, oy);
}

