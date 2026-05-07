#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <cstdio>
#include <thread>
#include <atomic>
#include <mutex>

#include "../Common/Protocol.h"
#include "../Common/SocketHelpers.h"
#include "ServerGame.h"

#pragma comment(lib, "ws2_32.lib")

struct ClientSlot
{
    SOCKET socketHandle = INVALID_SOCKET;
    std::thread recvThread;
    std::atomic_bool connected = false;
    std::atomic_bool moveUp = false;
    std::atomic_bool moveDown = false;
    int playerId = 0;
};

static std::atomic_bool g_running = true;
static ClientSlot g_clients[Net::kMaxPlayers];
static ServerGame g_game;
static constexpr DWORD kServerTickSleepMs = 8;

static void CloseClientSlot(ClientSlot& slot)
{
    slot.connected = false;
    slot.moveUp = false;
    slot.moveDown = false;
    if (slot.socketHandle != INVALID_SOCKET)
    {
        shutdown(slot.socketHandle, SD_BOTH);
        closesocket(slot.socketHandle);
        slot.socketHandle = INVALID_SOCKET;
    }
}

static int CountConnectedPlayers()
{
    int count = 0;
    for (int i = 0; i < Net::kMaxPlayers; ++i)
        if (g_clients[i].connected)
            ++count;
    return count;
}

static void ClientReceiveLoop(int slotIndex)
{
    ClientSlot& slot = g_clients[slotIndex];

    while (g_running && slot.connected)
    {
        Net::InputPacket input = {};
        if (!Net::RecvPacket(slot.socketHandle, input))
            break;

        if (input.type != static_cast<std::uint32_t>(Net::PacketType::Input) || static_cast<int>(input.playerId) != slot.playerId)
            continue;

        slot.moveUp = (input.moveUp != 0);
        slot.moveDown = (input.moveDown != 0);
    }

    CloseClientSlot(slot);
    std::printf("Player %d disconnected.\n", slot.playerId);
}

static void AcceptLoop(SOCKET listenSocket)
{
    while (g_running)
    {
        sockaddr_in clientAddr = {};
        int addrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(listenSocket, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
        if (clientSocket == INVALID_SOCKET)
        {
            if (!g_running)
                return;
            continue;
        }

        int freeSlot = -1;
        for (int i = 0; i < Net::kMaxPlayers; ++i)
        {
            if (!g_clients[i].connected)
            {
                freeSlot = i;
                break;
            }
        }

        if (freeSlot == -1)
        {
            closesocket(clientSocket);
            continue;
        }

        Net::JoinRequestPacket joinRequest = {};
        if (!Net::RecvPacket(clientSocket, joinRequest) ||
            joinRequest.type != static_cast<std::uint32_t>(Net::PacketType::JoinRequest) ||
            joinRequest.protocolVersion != Net::kProtocolVersion)
        {
            closesocket(clientSocket);
            continue;
        }

        ClientSlot& slot = g_clients[freeSlot];
        slot.socketHandle = clientSocket;
        slot.playerId = freeSlot + 1;
        slot.connected = true;
        slot.moveUp = false;
        slot.moveDown = false;

        Net::JoinAcceptPacket acceptPacket = {};
        acceptPacket.playerId = static_cast<std::uint32_t>(slot.playerId);
        if (!Net::SendPacket(clientSocket, acceptPacket))
        {
            CloseClientSlot(slot);
            continue;
        }

        if (slot.recvThread.joinable())
            slot.recvThread.join();
        slot.recvThread = std::thread(ClientReceiveLoop, freeSlot);

        std::printf("Player %d connected.\n", slot.playerId);
    }
}

int main()
{
    WSADATA wsaData = {};
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::printf("WSAStartup failed.\n");
        return -1;
    }

    ServerGame_Reset(g_game);

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET)
    {
        std::printf("Failed to create listen socket.\n");
        WSACleanup();
        return -1;
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(Net::kDefaultPort);

    if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::printf("Bind failed.\n");
        closesocket(listenSocket);
        WSACleanup();
        return -1;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        std::printf("Listen failed.\n");
        closesocket(listenSocket);
        WSACleanup();
        return -1;
    }

    std::printf("LAN Multiplayer Pong Server running on port %u\n", Net::kDefaultPort);
    std::printf("Waiting for up to 2 clients...\n");

    std::thread acceptThread(AcceptLoop, listenSocket);

    LARGE_INTEGER freq = {};
    LARGE_INTEGER prev = {};
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&prev);

    while (g_running)
    {
        LARGE_INTEGER now = {};
        QueryPerformanceCounter(&now);
        float dt = static_cast<float>(double(now.QuadPart - prev.QuadPart) / double(freq.QuadPart));
        prev = now;
        if (dt > 0.05f)
            dt = 0.05f;

        int connectedPlayers = CountConnectedPlayers();
        bool p1Up = g_clients[0].moveUp;
        bool p1Down = g_clients[0].moveDown;
        bool p2Up = g_clients[1].moveUp;
        bool p2Down = g_clients[1].moveDown;
        bool p1Connected = g_clients[0].connected;
        bool p2Connected = g_clients[1].connected;

        ServerGame_Update(g_game, dt, p1Up, p1Down, p2Up, p2Down, p1Connected, p2Connected);
        Net::StateSnapshotPacket snapshot = ServerGame_MakeSnapshot(g_game, connectedPlayers);

        for (int i = 0; i < Net::kMaxPlayers; ++i)
        {
            if (g_clients[i].connected)
            {
                if (!Net::SendPacket(g_clients[i].socketHandle, snapshot))
                    CloseClientSlot(g_clients[i]);
            }
        }

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
            g_running = false;

        Sleep(kServerTickSleepMs);
    }

    closesocket(listenSocket);
    if (acceptThread.joinable())
        acceptThread.join();

    for (int i = 0; i < Net::kMaxPlayers; ++i)
    {
        CloseClientSlot(g_clients[i]);
        if (g_clients[i].recvThread.joinable())
            g_clients[i].recvThread.join();
    }

    WSACleanup();
    return 0;
}
