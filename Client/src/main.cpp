#include <iostream>
#include "../../Common/include/protocol.h"
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
        inputPacket.payload.input.up = true;

        SendPacketToServer(inputPacket);

        Packet incomingState{};
        if (ReceiveFromServer(incomingState)) {
            if (incomingState.header.type == PacketType::State) {
                std::cout << "Received Tick: " << incomingState.payload.state.tick << std::endl;
            }
        }
    }
    return 0;
}
void SendPacketToServer(const Packet& p) {

}

bool ReceiveFromServer(Packet& p) {

    return false;
}