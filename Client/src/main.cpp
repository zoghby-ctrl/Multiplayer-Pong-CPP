#include <iostream>
#include "protocol.h"

using namespace Protocol;

void SendPacketToServer(const Packet& p);
bool ReceiveFromServer(Packet& p);

int main() {
    uint32_t client_seq = 0;
    std::cout << "Client started..." << std::endl;

    while (true) {
        Packet inputPacket{};
        inputPacket.header.type = PacketType::Input;
        inputPacket.header.seq = client_seq++;
        inputPacket.payload.input.up = 1;

        SendPacketToServer(inputPacket);

        Packet incomingState{};
        if (ReceiveFromServer(incomingState)) {
            if (incomingState.header.type == PacketType::State) {
                std::cout << "Received state tick: "
                    << incomingState.payload.state.tick
                    << std::endl;
            }
        }

        // TODO: add timing/sleep later to avoid max CPU usage
    }

    return 0;
}

void SendPacketToServer(const Packet& p) {
    // TODO: actual socket send will be implemented by the networking layer
}

bool ReceiveFromServer(Packet& p) {
    // TODO: actual socket receive will be implemented by the networking layer
    return false;
}