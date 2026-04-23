#include "ServerUpdate.h"

void ServerUpdate::checkCollision() {

    if (ball.x <= 2) {
        if (ball.y >= paddles[0].y - 2 && ball.y <= paddles[0].y + 2) {
            ball.vx *= -1;
        } else {
            paddles[1].score++;
            ball.x = 40;
            ball.y = 12;
        }
    }

    if (ball.x >= 78) {
        if (ball.y >= paddles[1].y - 2 && ball.y <= paddles[1].y + 2) {
            ball.vx *= -1;
        } else {
            paddles[0].score++;
            ball.x = 40;
            ball.y = 12;
        }
    }
}