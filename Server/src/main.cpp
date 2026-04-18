#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>
#include "../../Common/include/protocol.h"

using namespace Protocol;

bool ReceivePacket(Packet& p);
void SendPacketToAll(const Packet& p);

int main() {
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

    std::cout << "Server started..." << std::endl;

    while (true) {
        Packet clientPacket{};

        if (ReceivePacket(clientPacket)) {
            if (clientPacket.header.type == PacketType::Input) {
                global_state.players[0].y += clientPacket.payload.input.up ? -PaddleSpeed : 0.0f;
                global_state.players[0].y += clientPacket.payload.input.down ? PaddleSpeed : 0.0f;
                global_state.players[0].y = std::clamp(global_state.players[0].y, PaddleHalfHeight, ArenaHeight - PaddleHalfHeight);
            }
        }

        global_state.ball.x += global_state.ball.vx;
        global_state.ball.y += global_state.ball.vy;
        if (global_state.ball.y <= 0.0f || global_state.ball.y >= ArenaHeight) {
            global_state.ball.vy *= -1.0f;
            global_state.ball.y = std::clamp(global_state.ball.y, 0.0f, ArenaHeight);
        }

        if (global_state.ball.x < 0.0f) {
            ++global_state.score[1];
            resetBall(ArenaWidth * 0.5f, ArenaHeight * 0.5f, BallInitialSpeedX, BallInitialSpeedY, 1.0f);
        } else if (global_state.ball.x > ArenaWidth) {
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
