// AnimationManager.h - Global per-frame updater for GUI animations
#pragma once

#include <unordered_set>

class GuiElement;

class AnimationManager {
public:
    static AnimationManager& instance();

    void update(float dt);
    void track(GuiElement* e);
    void untrack(GuiElement* e);

private:
    std::unordered_set<GuiElement*> m_tracked;
};

