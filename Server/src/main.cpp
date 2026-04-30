#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h> // for Sleep
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>

#include "../../Common/include/protocol.h"

#pragma comment(lib, "ws2_32.lib")

namespace {
    using namespace Protocol;

    constexpr float kPaddleHalfWidth = 10.0f;
    constexpr float kBallRadius = 8.0f;
    constexpr float kMaxBallSpeedY = 5.0f;
    constexpr float kAiDeadzone = 6.0f;
    constexpr float kLeftPaddleX = 20.0f;
    constexpr float kRightPaddleX = ArenaWidth - 20.0f;

    // TCP client socket — set in main() after accept()
    SOCKET g_clientSocket = INVALID_SOCKET;
    bool g_clientDisconnected = false;

    // Real TCP receive: returns true when a complete Packet was received.
    bool ReceivePacket(Packet& packet) {
        packet = {};
        int bytesReceived = recv(
            g_clientSocket,
            reinterpret_cast<char*>(&packet),
            sizeof(packet),
            0
        );
        if (bytesReceived == static_cast<int>(sizeof(packet))) {
            return true;
        }
        if (bytesReceived == 0) {
            std::cout << "Client disconnected\n";
            g_clientDisconnected = true;
        } else if (bytesReceived == SOCKET_ERROR) {
            int error = WSAGetLastError();
            // WSAEWOULDBLOCK is normal on a non-blocking socket with no data.
            if (error != WSAEWOULDBLOCK) {
                std::cout << "recv failed. Error: " << error << "\n";
                g_clientDisconnected = true;
            }
        }
        return false;
    }

    // Real TCP send: broadcasts the packet to the connected client.
    void SendPacketToAll(const Packet& packet) {
        int bytesSent = send(
            g_clientSocket,
            reinterpret_cast<const char*>(&packet),
            sizeof(packet),
            0
        );
        if (bytesSent == SOCKET_ERROR) {
            int error = WSAGetLastError();
            // WSAEWOULDBLOCK is normal on a non-blocking socket.
            if (error != WSAEWOULDBLOCK) {
                std::cout << "send failed. Error: " << error << "\n";
                g_clientDisconnected = true;
            }
        }
    }

    GameState CreateInitialGameState() {
        GameState state{};
        state.status = MatchStatus::InProgress;
        state.players[0].x = kLeftPaddleX;
        state.players[1].x = kRightPaddleX;
        state.players[0].y = ArenaHeight * 0.5f;
        state.players[1].y = ArenaHeight * 0.5f;
        state.ball.x = ArenaWidth * 0.5f;
        state.ball.y = ArenaHeight * 0.5f;
        state.ball.vx = BallInitialSpeedX;
        state.ball.vy = BallInitialSpeedY;
        return state;
    }

    bool IsGameOver(const GameState& state) {
        return state.status == MatchStatus::GameOver;
    }

    void ResetBall(GameState& state, float directionX) {
        state.ball.x = ArenaWidth * 0.5f;
        state.ball.y = ArenaHeight * 0.5f;
        state.ball.vx = directionX * BallInitialSpeedX;
        state.ball.vy = BallInitialSpeedY;
    }

    void ApplyPaddleInput(GameState& state, std::size_t playerIndex, bool up, bool down) {
        float deltaY = 0.0f;
        if (up != down) {
            deltaY = up ? -PaddleSpeed : PaddleSpeed;
        }

        state.players[playerIndex].y += deltaY;
        state.players[playerIndex].y = std::clamp(
            state.players[playerIndex].y,
            PaddleHalfHeight,
            ArenaHeight - PaddleHalfHeight
        );
    }

    bool ClientControlsSecondPaddle(const Packet& packet) {
        return packet.payload.input.left != 0 || packet.payload.input.right != 0;
    }

    void HandleClientPacket(GameState& state, const Packet& packet, bool& hasSecondPlayer) {
        if (packet.header.type == PacketType::Disconnect) {
            hasSecondPlayer = false;
            return;
        }

        if (packet.header.type != PacketType::Input) {
            return;
        }

        ApplyPaddleInput(state, 0, packet.payload.input.up != 0, packet.payload.input.down != 0);

        if (ClientControlsSecondPaddle(packet)) {
            hasSecondPlayer = true;
            ApplyPaddleInput(state, 1, packet.payload.input.left != 0, packet.payload.input.right != 0);
        }
    }

    void ApplyFallbackAi(GameState& state, bool hasSecondPlayer) {
        if (hasSecondPlayer) {
            return;
        }

        const bool aiUp = state.ball.y < (state.players[1].y - kAiDeadzone);
        const bool aiDown = state.ball.y > (state.players[1].y + kAiDeadzone);
        ApplyPaddleInput(state, 1, aiUp, aiDown);
    }

    void AdvanceBall(GameState& state) {
        state.ball.x += state.ball.vx;
        state.ball.y += state.ball.vy;

        if (state.ball.y <= kBallRadius || state.ball.y >= ArenaHeight - kBallRadius) {
            state.ball.vy *= -1.0f;
            state.ball.y = std::clamp(state.ball.y, kBallRadius, ArenaHeight - kBallRadius);
        }
    }

    bool CollidesWithPaddle(const GameState& state, std::size_t playerIndex) {
        const float paddleLeft = state.players[playerIndex].x - kPaddleHalfWidth;
        const float paddleRight = state.players[playerIndex].x + kPaddleHalfWidth;
        const float paddleTop = state.players[playerIndex].y - PaddleHalfHeight;
        const float paddleBottom = state.players[playerIndex].y + PaddleHalfHeight;

        return state.ball.x + kBallRadius >= paddleLeft &&
            state.ball.x - kBallRadius <= paddleRight &&
            state.ball.y + kBallRadius >= paddleTop &&
            state.ball.y - kBallRadius <= paddleBottom;
    }

    void BounceOffPaddle(GameState& state, std::size_t playerIndex) {
        const float paddleDirection = playerIndex == 0 ? 1.0f : -1.0f;
        const float paddleSurface = state.players[playerIndex].x + (paddleDirection * kPaddleHalfWidth);
        const float normalizedOffset =
            (state.ball.y - state.players[playerIndex].y) / (PaddleHalfHeight + kBallRadius);

        state.ball.x = paddleSurface + (paddleDirection * kBallRadius);
        state.ball.vx = paddleDirection * std::abs(state.ball.vx);
        state.ball.vy = std::clamp(state.ball.vy + normalizedOffset, -kMaxBallSpeedY, kMaxBallSpeedY);
    }

    void HandlePaddleCollisions(GameState& state) {
        if (state.ball.vx < 0.0f && CollidesWithPaddle(state, 0)) {
            BounceOffPaddle(state, 0);
        }
        else if (state.ball.vx > 0.0f && CollidesWithPaddle(state, 1)) {
            BounceOffPaddle(state, 1);
        }
    }

    void HandleScoring(GameState& state) {
        if (state.ball.x + kBallRadius < 0.0f) {
            ++state.score[1];
            ResetBall(state, 1.0f);
        }
        else if (state.ball.x - kBallRadius > ArenaWidth) {
            ++state.score[0];
            ResetBall(state, -1.0f);
        }

        if (state.score[0] >= WinningScore || state.score[1] >= WinningScore) {
            state.status = MatchStatus::GameOver;
        }
    }

    Packet BuildStatePacket(const GameState& state, uint32_t sequence) {
        Packet packet{};
        packet.header.type = PacketType::State;
        packet.header.seq = sequence;
        packet.payload.state = state;
        return packet;
    }

    bool ServerUpdate(GameState& state, bool& hasSecondPlayer) {
        if (g_clientDisconnected || IsGameOver(state)) {
            return false;
        }

        Packet clientPacket{};
        if (ReceivePacket(clientPacket)) {
            HandleClientPacket(state, clientPacket, hasSecondPlayer);
        }

        ApplyFallbackAi(state, hasSecondPlayer);
        AdvanceBall(state);
        HandlePaddleCollisions(state);
        HandleScoring(state);

        if (!IsGameOver(state)) {
            ++state.tick;
        }

        return true;
    }
} // namespace

int main() {
    ///////////////////////////////////////////////////////////////////////////
    // TCP NETWORKING SETUP
    // WSAStartup = starts Winsock library so sockets can work on Windows

    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cout << "WSAStartup failed\n";
        return 1;
    }

    // Create TCP listening socket
    // AF_INET = IPv4, SOCK_STREAM = TCP

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (listenSocket == INVALID_SOCKET) {
        std::cout << "Socket creation failed. Error: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    // INADDR_ANY means accept connections from any IP on this PC

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(Protocol::DefaultPort);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cout << "Bind failed. Error: " << WSAGetLastError() << "\n";
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cout << "Listen failed. Error: " << WSAGetLastError() << "\n";
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server listening on port " << Protocol::DefaultPort << "...\n";
    std::cout << "Waiting for client...\n";

    g_clientSocket = accept(listenSocket, nullptr, nullptr);

    if (g_clientSocket == INVALID_SOCKET) {
        std::cout << "Accept failed. Error: " << WSAGetLastError() << "\n";
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Client connected!\n";

    // We only accept one client; listening socket no longer needed.
    closesocket(listenSocket);

    // Make client socket non-blocking so recv/send don't stall the game loop.
    u_long mode = 1;
    ioctlsocket(g_clientSocket, FIONBIO, &mode);

    ///////////////////////////////////////////////////////////////////////////
    // GAME LOOP

    using namespace Protocol;

    GameState globalState = CreateInitialGameState();
    uint32_t nextBroadcastSequence = 0;
    bool hasSecondPlayer = false;

    std::cout << "Server started (player 2 falls back to AI if no second input).\n";

    while (ServerUpdate(globalState, hasSecondPlayer)) {
        const Packet statePacket = BuildStatePacket(globalState, nextBroadcastSequence++);
        SendPacketToAll(statePacket);
        std::this_thread::sleep_for(std::chrono::milliseconds(FrameTimeMs));
    }

    std::cout << "Game over. Final score: "
              << globalState.score[0] << " - " << globalState.score[1] << "\n";

    closesocket(g_clientSocket);
    WSACleanup();

    return 0;
}

/*
    GetTickCount() / std::this_thread::sleep_for()
    Used for frame timing.

    DWORD ≈ unsigned int
*/
