#include <iostream>
#include "../../Common/include/protocol.h"

using namespace Protocol;

void SendPacketToAll(const Packet& p);
bool ReceivePacket(Packet& p);

void update_game_logic(GameState& current_state, const PlayerInput& input, int player_index) {
    if (input.up)    current_state.players[player_index].y -= 0.1f;
    if (input.down)  current_state.players[player_index].y += 0.1f;
    if (input.left)  current_state.players[player_index].x -= 0.1f;
    if (input.right) current_state.players[player_index].x += 0.1f;
}

int main() {
    GameState global_state{};
    uint32_t server_seq = 0;

    std::cout << "Server started..." << std::endl;

    while (true) {
        Packet received_packet{};

        if (ReceivePacket(received_packet)) {
            if (received_packet.header.type == PacketType::Input) {
                update_game_logic(global_state, received_packet.payload.input, 0);

                Packet response{};
                response.header.type = PacketType::State;
                response.header.seq = server_seq++;
                response.payload.state = global_state;

                SendPacketToAll(response);
            }
            else if (received_packet.header.type == PacketType::Disconnect) {
                global_state = GameState{};
                std::cout << "Game Reset" << std::endl;
            }
        }

    }
    return 0;
}

void SendPacketToAll(const Packet& p) {
}

bool ReceivePacket(Packet& p) {
    return false;
}