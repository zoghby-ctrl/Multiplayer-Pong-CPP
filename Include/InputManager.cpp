#include "InputManager.h"
#include "KeyMapper.h"

void InputManager::handleKeyPress(int platformKey) {
    Key key = mapKey(platformKey);

    if (key == Key::F3) {
        toggleOverlayRequested = true;
    }
}

bool InputManager::consumeToggleOverlayPressed() {
    const bool wasRequested = toggleOverlayRequested;
    toggleOverlayRequested = false;
    return wasRequested;
}
