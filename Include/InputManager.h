#pragma once
#include "Key.h"

class InputManager {
public:
    void handleKeyPress(int platformKey);
    bool consumeToggleOverlayPressed();

private:
    bool toggleOverlayRequested = false;
};
