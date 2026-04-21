#include <iostream>
#include <algorithm>
#include <cmath>
#include <thread>
#include <chrono>
#include "../../Common/include/protocol.h"

using namespace Protocol;

bool ReceivePacket(Packet& p);
void SendPacketToAll(const Packet& p);

int main() {
    constexpr float PaddleHalfWidth = 10.0f;
    constexpr float BallRadius = 8.0f;
    constexpr float MaxBallSpeedY = 5.0f;
    constexpr float AiDeadzone = 6.0f;

    GameState global_state{};
    global_state.tick = 0;
    global_state.status = MatchStatus::InProgress;
    global_state.players[0].x = 20.0f;
    global_state.players[1].x = ArenaWidth - 20.0f;
    global_state.players[0].y = ArenaHeight * 0.5f;
    global_state.players[1].y = ArenaHeight * 0.5f;

    global_state.ball.x = ArenaWidth * 0.5f;
    global_state.ball.y = ArenaHeight * 0.5f;
    global_state.ball.vx = BallInitialSpeedX;
    global_state.ball.vy = BallInitialSpeedY;

    const auto resetBall = [&global_state](float centerX, float centerY, float speedX, float speedY, float directionX) {
        global_state.ball = {centerX, centerY, directionX * speedX, speedY};
    };
    const auto applyPaddleInput = [&global_state](std::size_t playerIndex, bool up, bool down) {
        global_state.players[playerIndex].y += up ? -PaddleSpeed : 0.0f;
        global_state.players[playerIndex].y += down ? PaddleSpeed : 0.0f;
        global_state.players[playerIndex].y = std::clamp(global_state.players[playerIndex].y, PaddleHalfHeight, ArenaHeight - PaddleHalfHeight);
    };

    std::cout << "Server started..." << std::endl;

    while (true) {
        Packet clientPacket{};
        bool hasSecondPlayerInput = false;

        if (ReceivePacket(clientPacket)) {
            if (clientPacket.header.type == PacketType::Input) {
                applyPaddleInput(0, clientPacket.payload.input.up != 0, clientPacket.payload.input.down != 0);
                hasSecondPlayerInput = (clientPacket.payload.input.left != 0 || clientPacket.payload.input.right != 0);
                if (hasSecondPlayerInput) {
                    applyPaddleInput(1, clientPacket.payload.input.left != 0, clientPacket.payload.input.right != 0);
                }
            }
        }
        if (!hasSecondPlayerInput) {
            const bool aiUp = global_state.ball.y < (global_state.players[1].y - AiDeadzone);
            const bool aiDown = global_state.ball.y > (global_state.players[1].y + AiDeadzone);
            applyPaddleInput(1, aiUp, aiDown);
        }

        global_state.ball.x += global_state.ball.vx;
        global_state.ball.y += global_state.ball.vy;
        if (global_state.ball.y <= BallRadius || global_state.ball.y >= ArenaHeight - BallRadius) {
            global_state.ball.vy *= -1.0f;
            global_state.ball.y = std::clamp(global_state.ball.y, BallRadius, ArenaHeight - BallRadius);
        }

        const auto collidesWithPaddle = [&](std::size_t playerIndex) {
            const float paddleLeft = global_state.players[playerIndex].x - PaddleHalfWidth;
            const float paddleRight = global_state.players[playerIndex].x + PaddleHalfWidth;
            const float paddleTop = global_state.players[playerIndex].y - PaddleHalfHeight;
            const float paddleBottom = global_state.players[playerIndex].y + PaddleHalfHeight;

            return global_state.ball.x + BallRadius >= paddleLeft &&
                   global_state.ball.x - BallRadius <= paddleRight &&
                   global_state.ball.y + BallRadius >= paddleTop &&
                   global_state.ball.y - BallRadius <= paddleBottom;
        };

        if (global_state.ball.vx < 0.0f && collidesWithPaddle(0)) {
            global_state.ball.x = global_state.players[0].x + PaddleHalfWidth + BallRadius;
            global_state.ball.vx = std::abs(global_state.ball.vx);
            const float normalizedOffset = (global_state.ball.y - global_state.players[0].y) / PaddleHalfHeight;
            global_state.ball.vy = std::clamp(global_state.ball.vy + normalizedOffset, -MaxBallSpeedY, MaxBallSpeedY);
        } else if (global_state.ball.vx > 0.0f && collidesWithPaddle(1)) {
            global_state.ball.x = global_state.players[1].x - PaddleHalfWidth - BallRadius;
            global_state.ball.vx = -std::abs(global_state.ball.vx);
            const float normalizedOffset = (global_state.ball.y - global_state.players[1].y) / PaddleHalfHeight;
            global_state.ball.vy = std::clamp(global_state.ball.vy + normalizedOffset, -MaxBallSpeedY, MaxBallSpeedY);
        }

        if (global_state.ball.x + BallRadius < 0.0f) {
            ++global_state.score[1];
            resetBall(ArenaWidth * 0.5f, ArenaHeight * 0.5f, BallInitialSpeedX, BallInitialSpeedY, 1.0f);
        } else if (global_state.ball.x - BallRadius > ArenaWidth) {
            ++global_state.score[0];
            resetBall(ArenaWidth * 0.5f, ArenaHeight * 0.5f, BallInitialSpeedX, BallInitialSpeedY, -1.0f);
        }

        if (global_state.score[0] >= WinningScore || global_state.score[1] >= WinningScore) {
            global_state.status = MatchStatus::GameOver;
        }

        global_state.tick++;

        Packet statePacket{};
        statePacket.header.type = PacketType::State;
        statePacket.payload.state = global_state;

        SendPacketToAll(statePacket);
        std::this_thread::sleep_for(std::chrono::milliseconds(FrameTimeMs));
    }
    return 0;
}

bool ReceivePacket(Packet& p) {
    (void)p;
    return false;
}

void SendPacketToAll(const Packet& p) {
    (void)p;
}
