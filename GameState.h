#pragma once
#include <string>

struct GameState {
    ball Ball;

    float paddleY[2];
    int score[2];

    std::string name[2];
};
struct ball {
    float ballX;
    float ballY;
    float ballVX;
    float ballVY;
};