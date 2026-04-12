#include <iostream>
#include "../../Common/include/protocol.h"

using namespace Protocol;

void SendPacketToAll(const Packet& p);
void ReceivePacket(Packet& p);

int main() {
    GameState global_state{};
    global_state.tick = 0;

    global_state.ball.x = 400.0f;
    global_state.ball.y = 300.0f;
    global_state.ball.vx = 1.5f;
    global_state.ball.vy = 1.5f;

    std::cout << "Server started..." << std::endl;

    while (true) {
        Packet clientPacket{};
        ReceivePacket(clientPacket);

        if (clientPacket.header.type == PacketType::Input) {
            global_state.players[0].y += clientPacket.payload.input.up ? -5.0f : 0.0f;
            global_state.players[0].y += clientPacket.payload.input.down ? 5.0f : 0.0f;
        }

        global_state.ball.x += global_state.ball.vx;
        global_state.ball.y += global_state.ball.vy;

        global_state.tick++;

        Packet statePacket{};
        statePacket.header.type = PacketType::State;
        statePacket.payload.state = global_state;

        SendPacketToAll(statePacket);
    }
    return 0;
}

void SendPacketToAll(const Packet& p) {}
void ReceivePacket(Packet& p) {}