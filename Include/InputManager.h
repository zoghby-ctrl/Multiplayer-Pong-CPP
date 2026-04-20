#pragma once
#include "Key.h"

class InputManager {
public:
    void handleKeyPress(int platformKey);
    bool isToggleOverlayPressed() const;

private:
    bool toggleOverlayPressed = false;
};