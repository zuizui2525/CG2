#ifdef _USEIMGUI
#pragma once

class GameViewWindow {
public:
    GameViewWindow() = default;
    ~GameViewWindow() = default;

    void Draw(bool* show, bool* isVisible);
};
#endif
