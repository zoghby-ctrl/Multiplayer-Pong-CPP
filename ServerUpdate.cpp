#include "ServerUpdate.h"

ServerUpdate::ServerUpdate() {
    ball.x = 40;
    ball.y = 12;
    ball.vx = ballSpeed;
    ball.vy = ballSpeed;

    for (int i = 0; i < 2; i++) {
        paddles[i].y = worldHeight / 2;
        paddles[i].score = 0;
        paddles[i].dir = 0;
    }
}

void ServerUpdate::spawnPlayer(const std::string& name, int side) {
    paddles[side].name = name;
}

void ServerUpdate::setInput(int side, const std::string& input) {
    if (input == "UP") paddles[side].dir = -1;
    else if (input == "DOWN") paddles[side].dir = 1;
    else paddles[side].dir = 0;
}

GameState ServerUpdate::update(float dt) {
    movePaddles(dt);
    moveBall(dt);
    checkCollision();

    GameState state;

    state.ballX = ball.x;
    state.ballY = ball.y;
    state.ballVX = ball.vx;
    state.ballVY = ball.vy;

    for (int i = 0; i < 2; i++) {
        state.paddleY[i] = paddles[i].y;
        state.score[i] = paddles[i].score;
        state.name[i] = paddles[i].name;
    }

    return state;
}