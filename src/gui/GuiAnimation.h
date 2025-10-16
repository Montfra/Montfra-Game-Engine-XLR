// GuiAnimation.h - Per-element animation primitives
#pragma once

#include <memory>
#include <cmath>

class GuiElement;

// Simple easing functions
enum class Easing { Linear, EaseInOut }; // extendable

inline float ease_eval(Easing e, float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    switch (e) {
        case Easing::EaseInOut: {
            // Smoothstep-like (ease in/out quad)
            return t * t * (3.0f - 2.0f * t);
        } break;
        case Easing::Linear:
        default: return t;
    }
}

class Animation {
public:
    virtual ~Animation() = default;
    explicit Animation(float duration, Easing easing = Easing::EaseInOut, float delay = 0.0f)
        : m_duration(duration > 0.0f ? duration : 0.0001f), m_easing(easing), m_delay(delay) {}

    // Returns true when finished; applies contributions into element's accumulators
    bool update(float dt, GuiElement& e) {
        if (!m_started) { on_start(e); m_started = true; }
        if (m_delay > 0.0f) {
            m_delay -= dt;
            // Apply initial state at t=0
            on_apply(e, 0.0f);
            return false;
        }
        m_elapsed += dt;
        float t = m_elapsed / m_duration;
        if (t > 1.0f) t = 1.0f;
        float et = ease_eval(m_easing, t);
        on_apply(e, et);
        if (m_elapsed >= m_duration) {
            on_finish(e);
            return true;
        }
        return false;
    }

protected:
    virtual void on_start(GuiElement&) {}
    virtual void on_apply(GuiElement& e, float et) = 0; // et in [0,1]
    virtual void on_finish(GuiElement&) {}

    float m_duration;
    float m_elapsed = 0.0f;
    Easing m_easing = Easing::EaseInOut;
    float m_delay = 0.0f;
    bool  m_started = false;
};

// Fade: controls alpha multiplier from a to b
class FadeAnimation : public Animation {
public:
    FadeAnimation(float from, float to, float duration, Easing ease = Easing::EaseInOut)
        : Animation(duration, ease), m_from(from), m_to(to) {}
protected:
    void on_apply(GuiElement& e, float et) override;
    float m_from, m_to;
};

// Move offset: animates additional position offset (pixels) from start to target
class MoveAnimation : public Animation {
public:
    MoveAnimation(float sx, float sy, float tx, float ty, float duration, Easing ease = Easing::EaseInOut)
        : Animation(duration, ease), m_sx(sx), m_sy(sy), m_tx(tx), m_ty(ty) {}
protected:
    void on_apply(GuiElement& e, float et) override;
    float m_sx, m_sy, m_tx, m_ty;
};

// Scale: multiplies scale around center (sx, sy) from start to target
class ScaleAnimation : public Animation {
public:
    ScaleAnimation(float s_from, float s_to, float duration, Easing ease = Easing::EaseInOut)
        : Animation(duration, ease), m_from(s_from), m_to(s_to) {}
protected:
    void on_apply(GuiElement& e, float et) override;
    float m_from, m_to;
};

// Pulse: oscillates scale between 1 and max_scale
class PulseAnimation : public Animation {
public:
    PulseAnimation(float max_scale, float duration)
        : Animation(duration, Easing::EaseInOut), m_max(max_scale) {}
protected:
    void on_apply(GuiElement& e, float et) override;
    float m_max;
};

// Color override animation: overrides element color towards target RGBA
class ColorAnimation : public Animation {
public:
    ColorAnimation(float r, float g, float b, float a, float duration, Easing ease = Easing::EaseInOut)
        : Animation(duration, ease) { m_tgt[0]=r; m_tgt[1]=g; m_tgt[2]=b; m_tgt[3]=a; }
protected:
    void on_start(GuiElement& e) override;
    void on_apply(GuiElement& e, float et) override;
    float m_from[4] = {1,1,1,1};
    float m_tgt[4] = {1,1,1,1};
};

// Shake: lateral sinusoidal offset decaying to zero
class ShakeAnimation : public Animation {
public:
    ShakeAnimation(float amplitude_px, float duration, float freq_hz)
        : Animation(duration, Easing::Linear), m_amp(amplitude_px), m_freq(freq_hz) {}
protected:
    void on_apply(GuiElement& e, float et) override;
    float m_amp;
    float m_freq;
};

// Slide: from offscreen to 0 (in) or 0 to offscreen (out)
class SlideAnimation : public Animation {
public:
    enum class Type { In, Out };
    enum class Dir { Left, Right, Up, Down };
    SlideAnimation(Dir d, Type t, float duration)
        : Animation(duration, Easing::EaseInOut), m_dir(d), m_type(t) {}
protected:
    void on_apply(GuiElement& e, float et) override;
    Dir m_dir; Type m_type;
};

