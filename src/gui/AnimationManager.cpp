// AnimationManager.cpp - Implementation

#include "AnimationManager.h"
#include "GuiElement.h"

AnimationManager& AnimationManager::instance() {
    static AnimationManager inst;
    return inst;
}

void AnimationManager::update(float dt) {
    // Copy to avoid iterator invalidation if elements modify tracking
    auto copy = m_tracked;
    for (auto* e : copy) {
        if (e) e->update_animations(dt);
    }
}

void AnimationManager::track(GuiElement* e) {
    if (!e) return;
    m_tracked.insert(e);
}

void AnimationManager::untrack(GuiElement* e) {
    if (!e) return;
    m_tracked.erase(e);
}

