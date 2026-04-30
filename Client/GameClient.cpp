#include "GameClient.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>

#include <array>
#include <cstring>
#include <utility>

#pragma comment(lib, "ws2_32.lib")

namespace {

class TcpClientTransport final : public IClientTransport {
public:
    ~TcpClientTransport() override {
        Close();
        if (winsockStarted_) {
            WSACleanup();
        }
    }

    bool Connect(const std::string& host, uint16_t port) override {
        if (!StartWinsock()) {
            return false;
        }

        socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (socket_ == INVALID_SOCKET) {
            SetLastSocketError("socket");
            return false;
        }

        sockaddr_in serverAddress{};
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(port);

        if (inet_pton(AF_INET, host.c_str(), &serverAddress.sin_addr) != 1) {
            lastError_ = "Only IPv4 addresses are supported by this simple client.";
            Close();
            return false;
        }

        if (connect(socket_, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
            SetLastSocketError("connect");
            Close();
            return false;
        }

        u_long nonBlocking = 1;
        if (ioctlsocket(socket_, FIONBIO, &nonBlocking) != 0) {
            SetLastSocketError("ioctlsocket");
            Close();
            return false;
        }

        connected_ = true;
        return true;
    }

    bool SendPacket(const Protocol::Packet& packet) override {
        if (!connected_) {
            return false;
        }

        const char* bytes = reinterpret_cast<const char*>(&packet);
        int remaining = static_cast<int>(sizeof(packet));

        while (remaining > 0) {
            const int sent = send(socket_, bytes, remaining, 0);
            if (sent == SOCKET_ERROR) {
                const int error = WSAGetLastError();
                if (error == WSAEWOULDBLOCK) {
                    return true;
                }

                SetLastSocketError("send");
                Close();
                return false;
            }

            bytes += sent;
            remaining -= sent;
        }

        return true;
    }

    bool TryReceivePacket(Protocol::Packet& packet) override {
        if (!connected_) {
            return false;
        }

        while (bufferedBytes_ < static_cast<int>(sizeof(Protocol::Packet))) {
            const int received = recv(
                socket_,
                packetBuffer_.data() + bufferedBytes_,
                static_cast<int>(packetBuffer_.size()) - bufferedBytes_,
                0
            );

            if (received > 0) {
                bufferedBytes_ += received;
                continue;
            }

            if (received == 0) {
                lastError_ = "Server disconnected.";
                Close();
                return false;
            }

            const int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK) {
                return false;
            }

            SetLastSocketError("recv");
            Close();
            return false;
        }

        std::memcpy(&packet, packetBuffer_.data(), sizeof(packet));
        bufferedBytes_ = 0;
        return true;
    }

    void Close() override {
        if (socket_ != INVALID_SOCKET) {
            closesocket(socket_);
            socket_ = INVALID_SOCKET;
        }
        connected_ = false;
        bufferedBytes_ = 0;
    }

    bool IsConnected() const override {
        return connected_;
    }

    const std::string& LastError() const override {
        return lastError_;
    }

private:
    bool StartWinsock() {
        if (winsockStarted_) {
            return true;
        }

        WSADATA wsaData{};
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            lastError_ = "WSAStartup failed.";
            return false;
        }

        winsockStarted_ = true;
        return true;
    }

    void SetLastSocketError(const char* operation) {
        lastError_ = std::string(operation) + " failed. Winsock error: " + std::to_string(WSAGetLastError());
    }

    SOCKET socket_ = INVALID_SOCKET;
    bool connected_ = false;
    bool winsockStarted_ = false;
    std::array<char, sizeof(Protocol::Packet)> packetBuffer_{};
    int bufferedBytes_ = 0;
    std::string lastError_;
};

}

GameClient::GameClient(std::unique_ptr<IClientTransport> transport)
    : transport_(std::move(transport)) {
    if (!transport_) {
        transport_ = std::make_unique<TcpClientTransport>();
    }

    displayState_ = CreateInitialState();
}

GameClient::~GameClient() {
    Disconnect();
}

bool GameClient::Connect(const std::string& host, uint16_t port) {
    return transport_->Connect(host, port);
}

void GameClient::Disconnect() {
    if (!transport_ || !transport_->IsConnected()) {
        return;
    }

    Protocol::Packet packet{};
    packet.header.type = Protocol::PacketType::Disconnect;
    packet.header.seq = clientSequence_++;
    transport_->SendPacket(packet);
    transport_->Close();
}

void GameClient::Update(const Protocol::PlayerInput& input) {
    if (!transport_ || !transport_->IsConnected()) {
        return;
    }

    Protocol::Packet inputPacket{};
    inputPacket.header.type = Protocol::PacketType::Input;
    inputPacket.header.seq = clientSequence_++;
    inputPacket.payload.input = input;
    transport_->SendPacket(inputPacket);

    Protocol::Packet incoming{};
    while (transport_->TryReceivePacket(incoming)) {
        ApplyPacket(incoming);
    }
}

void GameClient::RequestReset() {
    Protocol::ClientCommand command{};
    command.type = Protocol::ClientCommandType::ResetMatch;
    command.difficulty = displayState_.difficulty;
    SendCommand(command);
}

void GameClient::RequestDifficulty(Protocol::Difficulty difficulty) {
    Protocol::ClientCommand command{};
    command.type = Protocol::ClientCommandType::SetDifficulty;
    command.difficulty = difficulty;
    SendCommand(command);
}

bool GameClient::IsConnected() const {
    return transport_ && transport_->IsConnected();
}

const std::string& GameClient::LastError() const {
    static const std::string kNoTransport = "No client transport is available.";
    return transport_ ? transport_->LastError() : kNoTransport;
}

void GameClient::SendCommand(const Protocol::ClientCommand& command) {
    if (!transport_ || !transport_->IsConnected()) {
        return;
    }

    Protocol::Packet packet{};
    packet.header.type = Protocol::PacketType::Command;
    packet.header.seq = clientSequence_++;
    packet.payload.command = command;
    transport_->SendPacket(packet);
}

void GameClient::ApplyPacket(const Protocol::Packet& packet) {
    serverSequence_ = packet.header.seq;

    if (packet.header.type == Protocol::PacketType::Welcome) {
        localPlayerId_ = packet.payload.welcome.playerId;
        return;
    }

    if (packet.header.type == Protocol::PacketType::State) {
        displayState_ = packet.payload.state;
    }
}

Protocol::GameState GameClient::CreateInitialState() {
    Protocol::GameState state{};
    state.players[0] = {24.0f, Protocol::ArenaHeight * 0.5f};
    state.players[1] = {Protocol::ArenaWidth - 24.0f, Protocol::ArenaHeight * 0.5f};
    state.ball = {
        Protocol::ArenaWidth * 0.5f,
        Protocol::ArenaHeight * 0.5f,
        0.0f,
        0.0f
    };
    state.status = Protocol::MatchStatus::WaitingForPlayers;
    state.difficulty = Protocol::Difficulty::Normal;
    return state;
}
