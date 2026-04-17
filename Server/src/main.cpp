#include <iostream>
#include <algorithm>
#include <chrono>
#include <thread>
#include "../../Common/include/protocol.h"

using namespace Protocol;

bool ReceivePacket(Packet& p);
void SendPacketToAll(const Packet& p);

int main() {
    using Clock = std::chrono::steady_clock;
    constexpr auto TickInterval = std::chrono::milliseconds(16);
    constexpr float ArenaWidth = 800.0f;
    constexpr float ArenaHeight = 600.0f;
    constexpr float BallRadius = 8.0f;
    constexpr float PaddleHalfHeight = 40.0f;
    constexpr float PaddleSpeed = 5.0f;
    constexpr float PaddleMinY = PaddleHalfHeight;
    constexpr float PaddleMaxY = ArenaHeight - PaddleHalfHeight;

    GameState global_state{};
    global_state.tick = 0;
    global_state.players[0].x = 20.0f;
    global_state.players[0].y = ArenaHeight / 2.0f;
    global_state.players[1].x = ArenaWidth - 20.0f;
    global_state.players[1].y = ArenaHeight / 2.0f;

    global_state.ball.x = ArenaWidth / 2.0f;
    global_state.ball.y = ArenaHeight / 2.0f;
    global_state.ball.vx = 1.5f;
    global_state.ball.vy = 1.5f;

    std::cout << "Server started..." << std::endl;
    auto next_tick = Clock::now();

    while (true) {
        next_tick += TickInterval;

        Packet clientPacket{};

        if (ReceivePacket(clientPacket)) {
            if (clientPacket.header.type == PacketType::Input) {
                const bool move_up = clientPacket.payload.input.up;
                const bool move_down = clientPacket.payload.input.down;
                if (move_up && !move_down) {
                    global_state.players[0].y -= PaddleSpeed;
                } else if (move_down && !move_up) {
                    global_state.players[0].y += PaddleSpeed;
                }
            }
        }
        global_state.players[0].y = std::clamp(global_state.players[0].y, PaddleMinY, PaddleMaxY);
        global_state.players[1].y = std::clamp(global_state.players[1].y, PaddleMinY, PaddleMaxY);

        global_state.ball.x += global_state.ball.vx;
        global_state.ball.y += global_state.ball.vy;
        if (global_state.ball.x < BallRadius) {
            global_state.ball.x = BallRadius;
            global_state.ball.vx = -global_state.ball.vx;
        } else if (global_state.ball.x > ArenaWidth - BallRadius) {
            global_state.ball.x = ArenaWidth - BallRadius;
            global_state.ball.vx = -global_state.ball.vx;
        }
        if (global_state.ball.y < BallRadius) {
            global_state.ball.y = BallRadius;
            global_state.ball.vy = -global_state.ball.vy;
        } else if (global_state.ball.y > ArenaHeight - BallRadius) {
            global_state.ball.y = ArenaHeight - BallRadius;
            global_state.ball.vy = -global_state.ball.vy;
        }

        global_state.tick++;

        Packet statePacket{};
        statePacket.header.type = PacketType::State;
        statePacket.header.seq = global_state.tick;
        statePacket.payload.state = global_state;

        SendPacketToAll(statePacket);
        std::this_thread::sleep_until(next_tick);
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
