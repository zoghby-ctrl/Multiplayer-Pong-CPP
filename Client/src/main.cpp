#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <conio.h>
#include "../../Common/include/protocol.h"

#pragma comment(lib, "ws2_32.lib")
using namespace Protocol;

SOCKET clientSocket;
sockaddr_in serverAddr;

void SendPacketToServer(const Packet& p);
bool ReceiveFromServer(Packet& p);
void RenderHUD(const GameState& state, MatchStatus previousStatus);

static const std::string CLEAR_SCREEN = "\033[2J";
static const std::string CURSOR_HOME = "\033[H";
static const std::string COLOR_RESET = "\033[0m";
static const std::string COLOR_YELLOW = "\033[1;33m";
static const std::string COLOR_GREEN = "\033[1;32m";
static const std::string COLOR_RED = "\033[1;31m";
static const std::string COLOR_CYAN = "\033[1;36m";
static const std::string COLOR_WHITE = "\033[1;37m";

int main() {
    uint32_t client_seq = 0;

    
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
        return 1;
    }

    clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "socket() failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    GameState lastState{};
    lastState.status = MatchStatus::WaitingForPlayers;
    MatchStatus previousStatus = MatchStatus::WaitingForPlayers;

    RenderHUD(lastState, previousStatus);
    std::cout << "Client started..." << std::endl;

    while (true) {
        Packet inputPacket{};
        inputPacket.header.type = PacketType::Input;
        inputPacket.header.seq = client_seq++;
        inputPacket.payload.input.up = false;
        inputPacket.payload.input.down = false;

        if (_kbhit()) {
            int key = _getch();
            if (key == 72) inputPacket.payload.input.up = true; // Arrow Up
            if (key == 80) inputPacket.payload.input.down = true; // Arrow Down
            if (key == 'w' || key == 'W') inputPacket.payload.input.up = true;
            if (key == 's' || key == 'S') inputPacket.payload.input.down = true;
        }

        SendPacketToServer(inputPacket);

        Packet incomingState{};
        if (ReceiveFromServer(incomingState)) {
            if (incomingState.header.type == PacketType::State) {
                previousStatus = lastState.status;
                lastState = incomingState.payload.state;
                RenderHUD(lastState, previousStatus);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(FrameTimeMs));
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}

void RenderHUD(const GameState& state, MatchStatus previousStatus) {
    std::cout << CLEAR_SCREEN << CURSOR_HOME;

    std::cout << COLOR_WHITE
        << "==============================\n"
        << "       SCORE\n"
        << "  Player 1 : " << COLOR_YELLOW << state.score[0] << COLOR_WHITE
        << "   |   Player 2 : " << COLOR_YELLOW << state.score[1] << COLOR_WHITE << "\n"
        << "==============================\n"
        << COLOR_RESET;

    switch (state.status) {
    case MatchStatus::WaitingForPlayers:
        std::cout << "\n" << COLOR_CYAN
            << "  Waiting for opponent...\n"
            << COLOR_RESET;
        break;

    case MatchStatus::InProgress:
        if (previousStatus == MatchStatus::WaitingForPlayers) {
            std::cout << "\n" << COLOR_GREEN
                << "  Connected! Match started.\n"
                << COLOR_RESET;
        }
        else {
            std::cout << "\n" << COLOR_GREEN
                << "  Connected  |  Tick: " << state.tick << "\n"
                << COLOR_RESET;
        }
        break;

    case MatchStatus::GameOver:
        std::cout << "\n" << COLOR_RED
            << "  *** GAME OVER ***\n"
            << COLOR_RESET;

        if (state.score[0] > state.score[1]) {
            std::cout << COLOR_YELLOW << "  Player 1 wins!\n" << COLOR_RESET;
        }
        else if (state.score[1] > state.score[0]) {
            std::cout << COLOR_YELLOW << "  Player 2 wins!\n" << COLOR_RESET;
        }
        else {
            std::cout << COLOR_YELLOW << "  It's a draw!\n" << COLOR_RESET;
        }
        break;
    }

    std::cout << "\n";
    std::cout.flush();
}

void SendPacketToServer(const Packet& p) {
    (void)sendto(
        clientSocket,
        reinterpret_cast<const char*>(&p),
        sizeof(Packet), 0,
        reinterpret_cast<sockaddr*>(&serverAddr),
        sizeof(serverAddr)
    );
}

bool ReceiveFromServer(Packet& p) {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(clientSocket, &readSet);

    timeval timeout{};
    timeout.tv_sec = 0;
    timeout.tv_usec = 16000; // ~16ms timeout

    int ready = select(0, &readSet, nullptr, nullptr, &timeout);
    if (ready <= 0) return false; // timeout أو error

    sockaddr_in fromAddr{};
    int fromLen = sizeof(fromAddr);

    int received = recvfrom(
        clientSocket,
        reinterpret_cast<char*>(&p),
        sizeof(Packet), 0,
        reinterpret_cast<sockaddr*>(&fromAddr),
        &fromLen
    );

    return received == sizeof(Packet);
}