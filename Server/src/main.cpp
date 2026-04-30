#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "../GameServer.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <array>
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

namespace {

struct ClientPeer {
    SOCKET socket = INVALID_SOCKET;
    std::size_t playerId = 0;
    bool connected = false;
    std::array<char, sizeof(Protocol::Packet)> packetBuffer{};
    int bufferedBytes = 0;
};

bool SetNonBlocking(SOCKET socket) {
    u_long mode = 1;
    return ioctlsocket(socket, FIONBIO, &mode) == 0;
}

bool SendPacket(SOCKET socket, const Protocol::Packet& packet) {
    const char* bytes = reinterpret_cast<const char*>(&packet);
    int remaining = static_cast<int>(sizeof(packet));

    while (remaining > 0) {
        const int sent = send(socket, bytes, remaining, 0);
        if (sent == SOCKET_ERROR) {
            const int error = WSAGetLastError();
            return error == WSAEWOULDBLOCK;
        }

        bytes += sent;
        remaining -= sent;
    }

    return true;
}

void ClosePeer(ClientPeer& peer, GameServer& server) {
    if (!peer.connected) {
        return;
    }

    closesocket(peer.socket);
    peer.socket = INVALID_SOCKET;
    peer.connected = false;
    peer.bufferedBytes = 0;
    server.SetPlayerConnected(peer.playerId, false);
    std::cout << "Player " << (peer.playerId + 1) << " disconnected.\n";
}

bool TryReceivePacket(ClientPeer& peer, Protocol::Packet& packet, GameServer& server) {
    while (peer.connected && peer.bufferedBytes < static_cast<int>(sizeof(Protocol::Packet))) {
        const int received = recv(
            peer.socket,
            peer.packetBuffer.data() + peer.bufferedBytes,
            static_cast<int>(peer.packetBuffer.size()) - peer.bufferedBytes,
            0
        );

        if (received > 0) {
            peer.bufferedBytes += received;
            continue;
        }

        if (received == 0) {
            ClosePeer(peer, server);
            return false;
        }

        const int error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK) {
            return false;
        }

        std::cout << "recv failed for player " << (peer.playerId + 1) << ". Error: " << error << "\n";
        ClosePeer(peer, server);
        return false;
    }

    if (peer.bufferedBytes == static_cast<int>(sizeof(Protocol::Packet))) {
        std::memcpy(&packet, peer.packetBuffer.data(), sizeof(packet));
        peer.bufferedBytes = 0;
        return true;
    }

    return false;
}

SOCKET CreateListenSocket() {
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::cout << "Socket creation failed. Error: " << WSAGetLastError() << "\n";
        return INVALID_SOCKET;
    }

    BOOL reuseAddress = TRUE;
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuseAddress), sizeof(reuseAddress));

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(Protocol::DefaultPort);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cout << "Bind failed. Error: " << WSAGetLastError() << "\n";
        closesocket(listenSocket);
        return INVALID_SOCKET;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cout << "Listen failed. Error: " << WSAGetLastError() << "\n";
        closesocket(listenSocket);
        return INVALID_SOCKET;
    }

    if (!SetNonBlocking(listenSocket)) {
        std::cout << "Failed to set listen socket non-blocking. Error: " << WSAGetLastError() << "\n";
        closesocket(listenSocket);
        return INVALID_SOCKET;
    }

    return listenSocket;
}

void AcceptNewClients(SOCKET listenSocket, std::array<ClientPeer, Protocol::MaxPlayers>& peers, GameServer& server, uint32_t& sequence) {
    for (;;) {
        SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            const int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) {
                std::cout << "Accept failed. Error: " << error << "\n";
            }
            return;
        }

        auto slot = std::find_if(peers.begin(), peers.end(), [](const ClientPeer& peer) {
            return !peer.connected;
        });

        if (slot == peers.end()) {
            closesocket(clientSocket);
            continue;
        }

        SetNonBlocking(clientSocket);
        slot->socket = clientSocket;
        slot->playerId = static_cast<std::size_t>(slot - peers.begin());
        slot->connected = true;
        slot->bufferedBytes = 0;

        server.SetPlayerConnected(slot->playerId, true);
        SendPacket(slot->socket, server.BuildWelcomePacket(slot->playerId, sequence++));

        std::cout << "Player " << (slot->playerId + 1) << " connected.\n";
    }
}

void BroadcastState(
    std::array<ClientPeer, Protocol::MaxPlayers>& peers,
    const Protocol::Packet& packet,
    GameServer& server
) {
    for (ClientPeer& peer : peers) {
        if (peer.connected && !SendPacket(peer.socket, packet)) {
            ClosePeer(peer, server);
        }
    }
}

}

int main() {
    WSADATA wsaData{};
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cout << "WSAStartup failed.\n";
        return 1;
    }

    SOCKET listenSocket = CreateListenSocket();
    if (listenSocket == INVALID_SOCKET) {
        WSACleanup();
        return 1;
    }

    GameServer server;
    std::array<ClientPeer, Protocol::MaxPlayers> peers{};
    uint32_t sequence = 0;

    std::cout << "Multiplayer Pong server listening on port " << Protocol::DefaultPort << ".\n";
    std::cout << "Start one or two clients. Player 2 uses AI until a second client joins.\n";

    while (true) {
        AcceptNewClients(listenSocket, peers, server, sequence);

        for (ClientPeer& peer : peers) {
            Protocol::Packet packet{};
            while (TryReceivePacket(peer, packet, server)) {
                server.ProcessClientPacket(peer.playerId, packet);
            }
        }

        server.Tick();
        BroadcastState(peers, server.BuildStatePacket(sequence++), server);
        std::this_thread::sleep_for(std::chrono::milliseconds(Protocol::FrameTimeMs));
    }

    closesocket(listenSocket);
    WSACleanup();
    return 0;
}
