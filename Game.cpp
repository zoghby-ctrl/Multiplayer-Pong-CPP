#include "InputManager.h"

void Game::processEvents() {
    sf::Event event;
    while (mWindow.pollEvent(event)) {
        if (event.type == sf::Event::Closed)
            mWindow.close();

        // توحيد المعالجة هنا
        if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::F3) {
                mShowDebugOverlay = !mShowDebugOverlay; // تبديل حالة الظهور
            }
        }
    }
}
