#ifdef _USEIMGUI
#pragma once
#include "Engine/Debug/PopAnimation.h"

class GameViewWindow {
public:
    GameViewWindow();
    ~GameViewWindow() = default;

    void Draw(bool* show, bool* isVisible);

private:
    PopAnimation popAnim_;
    bool wasPaused_ = false;
};
#endif
