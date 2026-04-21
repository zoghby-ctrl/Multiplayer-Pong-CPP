#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <vector>
#include "../../Common/include/protocol.h"
#pragma comment(lib, "ws2_32.lib")
using namespace Protocol;

static SOCKET serverSocket = INVALID_SOCKET;
static std::vector<sockaddr_in> connectedClients;

bool ReceivePacket(Packet& p, sockaddr_in& fromAddr);
void SendPacketToAll(const Packet& p);
void RegisterClient(const sockaddr_in& addr);

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
        return 1;
    }

    serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "socket() failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "bind() failed: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    GameState global_state{};
    global_state.tick = 0;
    global_state.status = MatchStatus::WaitingForPlayers;
    global_state.players[0].x = 20.0f;
    global_state.players[1].x = ArenaWidth - 20.0f;
    global_state.players[0].y = ArenaHeight * 0.5f;
    global_state.players[1].y = ArenaHeight * 0.5f;
    global_state.ball.x = ArenaWidth * 0.5f;
    global_state.ball.y = ArenaHeight * 0.5f;
    global_state.ball.vx = BallInitialSpeedX;
    global_state.ball.vy = BallInitialSpeedY;

    const auto resetBall = [&global_state](float centerX, float centerY,
        float speedX, float speedY,
        float directionX) {
            global_state.ball = { centerX, centerY, directionX * speedX, speedY };
        };

    std::cout << "Server started on port 12345..." << std::endl;
    std::cout << "Waiting for 2 players..." << std::endl;

    while (true) {
        Packet      clientPacket{};
        sockaddr_in fromAddr{};

        if (ReceivePacket(clientPacket, fromAddr)) {
            RegisterClient(fromAddr);

            if (connectedClients.size() >= 2 &&
                global_state.status == MatchStatus::WaitingForPlayers) {
                global_state.status = MatchStatus::InProgress;
                std::cout << "Both players connected! Match started." << std::endl;
            }

            if (clientPacket.header.type == PacketType::Input &&
                global_state.status == MatchStatus::InProgress) {

                int playerIndex = -1;
                for (int i = 0; i < (int)connectedClients.size(); i++) {
                    if (connectedClients[i].sin_addr.s_addr == fromAddr.sin_addr.s_addr &&
                        connectedClients[i].sin_port == fromAddr.sin_port) {
                        playerIndex = i;
                        break;
                    }
                }

                if (playerIndex == 0 || playerIndex == 1) {
                    global_state.players[playerIndex].y +=
                        clientPacket.payload.input.up ? -PaddleSpeed : 0.0f;
                    global_state.players[playerIndex].y +=
                        clientPacket.payload.input.down ? PaddleSpeed : 0.0f;

                    auto& py = global_state.players[playerIndex].y;
                    if (py < PaddleHalfHeight)               py = PaddleHalfHeight;
                    if (py > ArenaHeight - PaddleHalfHeight) py = ArenaHeight - PaddleHalfHeight;
                }
            }
        }

        if (global_state.status != MatchStatus::InProgress) {
            std::this_thread::sleep_for(std::chrono::milliseconds(FrameTimeMs));
            continue;
        }

        // Physics
        global_state.ball.x += global_state.ball.vx;
        global_state.ball.y += global_state.ball.vy;

        // ✅ الـ } اتحذفت من هنا — كانت بتقفل الـ loop بدري
        if (global_state.ball.y < 0.0f)         global_state.ball.y = 0.0f;
        if (global_state.ball.y > ArenaHeight)   global_state.ball.y = ArenaHeight;

        if (global_state.ball.x < 0.0f) {
            ++global_state.score[1];
            resetBall(ArenaWidth * 0.5f, ArenaHeight * 0.5f,
                BallInitialSpeedX, BallInitialSpeedY, 1.0f);
        }
        else if (global_state.ball.x > ArenaWidth) {
            ++global_state.score[0];
            resetBall(ArenaWidth * 0.5f, ArenaHeight * 0.5f,
                BallInitialSpeedX, BallInitialSpeedY, -1.0f);
        }

        if (global_state.score[0] >= WinningScore ||
            global_state.score[1] >= WinningScore) {
            global_state.status = MatchStatus::GameOver;
        }

        global_state.tick++;

        Packet statePacket{};
        statePacket.header.type = PacketType::State;
        statePacket.payload.state = global_state;
        SendPacketToAll(statePacket);

        std::this_thread::sleep_for(std::chrono::milliseconds(FrameTimeMs));
    } // ✅ دي الـ } الصح اللي بتقفل الـ while

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}

bool ReceivePacket(Packet& p, sockaddr_in& fromAddr) {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(serverSocket, &readSet);

    timeval timeout{};
    timeout.tv_sec = 0;
    timeout.tv_usec = 16000;

    int ready = select(0, &readSet, nullptr, nullptr, &timeout);
    if (ready <= 0) return false;

    int fromLen = sizeof(fromAddr);
    int received = recvfrom(
        serverSocket,
        reinterpret_cast<char*>(&p),
        sizeof(Packet), 0,
        reinterpret_cast<sockaddr*>(&fromAddr),
        &fromLen
    );
    return received == sizeof(Packet);
}

void SendPacketToAll(const Packet& p) {
    for (const auto& client : connectedClients) {
        sendto(
            serverSocket,
            reinterpret_cast<const char*>(&p),
            sizeof(Packet), 0,
            reinterpret_cast<const sockaddr*>(&client),
            sizeof(client)
        );
    }
}

void RegisterClient(const sockaddr_in& addr) {
    if (connectedClients.size() >= 2) return;

    for (const auto& c : connectedClients) {
        if (c.sin_addr.s_addr == addr.sin_addr.s_addr &&
            c.sin_port == addr.sin_port) {
            return;
        }
    }
    connectedClients.push_back(addr);
    std::cout << "Player " << connectedClients.size() << " connected!" << std::endl;
}