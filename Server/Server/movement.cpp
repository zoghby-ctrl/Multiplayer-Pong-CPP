#include "ServerUpdate.h"

void ServerUpdate::movePaddles(float dt) {
    for (int i = 0; i < 2; i++) {
        paddles[i].y += paddles[i].dir * paddleSpeed * dt;

        if (paddles[i].y < 0) paddles[i].y = 0;
        if (paddles[i].y > worldHeight) paddles[i].y = worldHeight;
    }
}

void ServerUpdate::moveBall(float dt) {
    ball.x += ball.vx * dt;
    ball.y += ball.vy * dt;

    if (ball.y <= 0 || ball.y >= worldHeight) {
        ball.vy *= -1;
    }
}