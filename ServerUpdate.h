#pragma once
#include "GameState.h"

class ServerUpdate {
public:
    ServerUpdate();

    void spawnPlayer(const std::string& name, int side);
    void setInput(int side, const std::string& input);

    GameState update(float dt);

private:
    struct Ball {
        float x, y;
        float vx, vy;
    };

    struct Paddle {
        std::string name;
        float y;
        int score;
        int dir; // -1 up, 0 stop, 1 down
    };

    Ball ball;
    Paddle paddles[2];

    float worldHeight = 24.0f;
    float paddleSpeed = 20.0f;
    float ballSpeed = 15.0f;

private:
    void movePaddles(float dt);
    void moveBall(float dt);
    void checkCollision();
};