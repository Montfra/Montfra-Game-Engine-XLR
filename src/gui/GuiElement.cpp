// GuiElement.cpp - Animation plumbing for GuiElement

#include "GuiElement.h"
#include "GuiAnimation.h"
#include "AnimationManager.h"

#include <algorithm>

GuiElement::~GuiElement() = default;

// Define deleter after Animation is complete
void AnimDeleter::operator()(Animation* p) const { delete p; }

// Update animations: reset accumulation, then update and cull finished ones
void GuiElement::update_animations(float dt)
{
    // Reset accumulators to identity
    m_anim.offset_x = 0.0f; m_anim.offset_y = 0.0f;
    m_anim.scale_x = 1.0f; m_anim.scale_y = 1.0f;
    m_anim.alpha_mul = 1.0f;
    // color override is set by animations if needed per-frame
    m_anim.has_color_override = false;

    // Update all, removing finished
    if (m_animations.empty()) {
        AnimationManager::instance().untrack(this);
        return;
    }

    for (size_t i = 0; i < m_animations.size(); ) {
        auto& a = m_animations[i];
        bool done = a->update(dt, *this);
        if (done) {
            // erase by swap-pop
            m_animations[i] = std::move(m_animations.back());
            m_animations.pop_back();
        } else {
            ++i;
        }
    }

    if (m_animations.empty()) {
        AnimationManager::instance().untrack(this);
    }
}

// ------------------- High-level helpers -------------------

void GuiElement::fadeIn(float duration_sec)
{
    m_animations.emplace_back(std::unique_ptr<Animation, AnimDeleter>(new FadeAnimation(0.0f, 1.0f, duration_sec)));
    AnimationManager::instance().track(this);
}

void GuiElement::fadeOut(float duration_sec)
{
    m_animations.emplace_back(std::unique_ptr<Animation, AnimDeleter>(new FadeAnimation(1.0f, 0.0f, duration_sec)));
    AnimationManager::instance().track(this);
}

void GuiElement::moveTo(float offset_x_px, float offset_y_px, float duration_sec)
{
    m_animations.emplace_back(std::unique_ptr<Animation, AnimDeleter>(new MoveAnimation(0.0f, 0.0f, offset_x_px, offset_y_px, duration_sec)));
    AnimationManager::instance().track(this);
}

void GuiElement::moveBy(float dx_px, float dy_px, float duration_sec)
{
    m_animations.emplace_back(std::unique_ptr<Animation, AnimDeleter>(new MoveAnimation(0.0f, 0.0f, dx_px, dy_px, duration_sec)));
    AnimationManager::instance().track(this);
}

void GuiElement::scaleTo(float s, float duration_sec)
{
    m_animations.emplace_back(std::unique_ptr<Animation, AnimDeleter>(new ScaleAnimation(1.0f, s, duration_sec)));
    AnimationManager::instance().track(this);
}

void GuiElement::pulse(float max_scale, float duration_sec)
{
    m_animations.emplace_back(std::unique_ptr<Animation, AnimDeleter>(new PulseAnimation(max_scale, duration_sec)));
    AnimationManager::instance().track(this);
}

void GuiElement::colorTo(float r, float g, float b, float a, float duration_sec)
{
    m_animations.emplace_back(std::unique_ptr<Animation, AnimDeleter>(new ColorAnimation(r, g, b, a, duration_sec)));
    AnimationManager::instance().track(this);
}

void GuiElement::shake(float amplitude_px, float duration_sec, float freq_hz)
{
    m_animations.emplace_back(std::unique_ptr<Animation, AnimDeleter>(new ShakeAnimation(amplitude_px, duration_sec, freq_hz)));
    AnimationManager::instance().track(this);
}

void GuiElement::slideIn(SlideDir dir, float duration_sec)
{
    using Dir = SlideAnimation::Dir;
    Dir d = Dir::Left;
    switch (dir) {
        case SlideDir::Left:  d = Dir::Left; break;
        case SlideDir::Right: d = Dir::Right; break;
        case SlideDir::Up:    d = Dir::Up; break;
        case SlideDir::Down:  d = Dir::Down; break;
    }
    m_animations.emplace_back(std::unique_ptr<Animation, AnimDeleter>(new SlideAnimation(d, SlideAnimation::Type::In, duration_sec)));
    AnimationManager::instance().track(this);
}

void GuiElement::slideOut(SlideDir dir, float duration_sec)
{
    using Dir = SlideAnimation::Dir;
    Dir d = Dir::Left;
    switch (dir) {
        case SlideDir::Left:  d = Dir::Left; break;
        case SlideDir::Right: d = Dir::Right; break;
        case SlideDir::Up:    d = Dir::Up; break;
        case SlideDir::Down:  d = Dir::Down; break;
    }
    m_animations.emplace_back(std::unique_ptr<Animation, AnimDeleter>(new SlideAnimation(d, SlideAnimation::Type::Out, duration_sec)));
    AnimationManager::instance().track(this);
}

void GuiElement::stopAnimations()
{
    m_animations.clear();
    AnimationManager::instance().untrack(this);
}
