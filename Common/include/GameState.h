#pragma once
#include <string>

struct GameState {
    float ballX, ballY;
    float ballVX, ballVY;

    float paddleY[2];
    int score[2];

    std::string name[2];
};