#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>
#include "../../Common/include/protocol.h"

using namespace Protocol;

bool ReceivePacket(Packet& p);
void SendPacketToAll(const Packet& p);

int main() {
    constexpr float kArenaWidth = 800.0f;
    constexpr float kArenaHeight = 600.0f;
    constexpr float kPaddleSpeed = 5.0f;
    constexpr float kPaddleHalfHeight = 40.0f;

    GameState global_state{};
    global_state.tick = 0;
    global_state.status = MatchStatus::InProgress;
    global_state.players[0].x = 20.0f;
    global_state.players[1].x = kArenaWidth - 20.0f;
    global_state.players[0].y = kArenaHeight * 0.5f;
    global_state.players[1].y = kArenaHeight * 0.5f;

    global_state.ball.x = 400.0f;
    global_state.ball.y = 300.0f;
    global_state.ball.vx = 1.5f;
    global_state.ball.vy = 1.5f;

    std::cout << "Server started..." << std::endl;

    while (true) {
        Packet clientPacket{};

        if (ReceivePacket(clientPacket)) {
            if (clientPacket.header.type == PacketType::Input) {
                global_state.players[0].y += clientPacket.payload.input.up ? -kPaddleSpeed : 0.0f;
                global_state.players[0].y += clientPacket.payload.input.down ? kPaddleSpeed : 0.0f;
                global_state.players[0].y = std::clamp(global_state.players[0].y, kPaddleHalfHeight, kArenaHeight - kPaddleHalfHeight);
            }
        }

        global_state.ball.x += global_state.ball.vx;
        global_state.ball.y += global_state.ball.vy;
        if (global_state.ball.y <= 0.0f || global_state.ball.y >= kArenaHeight) {
            global_state.ball.vy *= -1.0f;
            global_state.ball.y = std::clamp(global_state.ball.y, 0.0f, kArenaHeight);
        }

        if (global_state.ball.x < 0.0f) {
            ++global_state.score[1];
            global_state.ball = {kArenaWidth * 0.5f, kArenaHeight * 0.5f, 1.5f, 1.5f};
        } else if (global_state.ball.x > kArenaWidth) {
            ++global_state.score[0];
            global_state.ball = {kArenaWidth * 0.5f, kArenaHeight * 0.5f, -1.5f, 1.5f};
        }

        if (global_state.score[0] >= 5 || global_state.score[1] >= 5) {
            global_state.status = MatchStatus::GameOver;
        }

        global_state.tick++;

        Packet statePacket{};
        statePacket.header.type = PacketType::State;
        statePacket.payload.state = global_state;

        SendPacketToAll(statePacket);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
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
