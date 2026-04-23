#include "Include/InputManager.h"

/*
 * Experimental prototype helper kept for reference.
 * This file is not part of MultiplayerPong.sln.
 */
bool PrototypeProcessOverlayToggle(InputManager& inputManager, int platformKey) {
    inputManager.handleKeyPress(platformKey);
    return inputManager.consumeToggleOverlayPressed();
}
