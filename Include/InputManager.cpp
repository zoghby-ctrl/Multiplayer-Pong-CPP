#include "InputManager.h"
#include "KeyMapper.h"

void InputManager::handleKeyPress(int platformKey) {
    Key key = mapKey(platformKey);

    if (key == Key::F3) {
        toggleOverlayPressed = true;
    }
}

bool InputManager::isToggleOverlayPressed() const {
    return toggleOverlayPressed;
}