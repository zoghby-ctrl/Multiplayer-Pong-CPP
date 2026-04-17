#include <iostream>
#include <chrono>
#include <thread>
#include "../../Common/include/protocol.h"

using namespace Protocol;

void SendPacketToServer(const Packet& p);
bool ReceiveFromServer(Packet& p);

int main() {
    using Clock = std::chrono::steady_clock;
    constexpr auto TickInterval = std::chrono::milliseconds(16);

    uint32_t client_seq = 0;
    uint32_t last_server_seq = 0;
    std::cout << "Client started..." << std::endl;
    auto next_tick = Clock::now();

    while (true) {
        next_tick += TickInterval;

        Packet inputPacket{};
        inputPacket.header.type = PacketType::Input;
        inputPacket.header.seq = client_seq++;
        inputPacket.payload.input.up = true;

        SendPacketToServer(inputPacket);

        Packet incomingState{};
        if (ReceiveFromServer(incomingState)) {
            if (incomingState.header.type == PacketType::State && incomingState.header.seq > last_server_seq) {
                last_server_seq = incomingState.header.seq;
                
            }
        }

        std::this_thread::sleep_until(next_tick);
    }
    return 0;
}

void SendPacketToServer(const Packet& p) {
    (void)p;
}
bool ReceiveFromServer(Packet& p) {
    (void)p;
    return false; 
}
