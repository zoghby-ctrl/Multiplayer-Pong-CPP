#pragma once

#include "../Common/include/protocol.h"

#include <cstdint>
#include <memory>
#include <string>

class IClientTransport {
public:
    virtual ~IClientTransport() = default;
    virtual bool Connect(const std::string& host, uint16_t port) = 0;
    virtual bool SendPacket(const Protocol::Packet& packet) = 0;
    virtual bool TryReceivePacket(Protocol::Packet& packet) = 0;
    virtual void Close() = 0;
    virtual bool IsConnected() const = 0;
    virtual const std::string& LastError() const = 0;
};

class GameClient final {
public:
    explicit GameClient(std::unique_ptr<IClientTransport> transport = nullptr);
    ~GameClient();

    bool Connect(const std::string& host = "127.0.0.1", uint16_t port = Protocol::DefaultPort);
    void Disconnect();
    void Update(const Protocol::PlayerInput& input);
    void RequestReset();
    void RequestDifficulty(Protocol::Difficulty difficulty);

    const Protocol::GameState& GetDisplayState() const { return displayState_; }
    uint8_t GetLocalPlayerId() const { return localPlayerId_; }
    uint32_t GetClientSequence() const { return clientSequence_; }
    uint32_t GetServerSequence() const { return serverSequence_; }
    bool IsConnected() const;
    const std::string& LastError() const;

private:
    void ApplyPacket(const Protocol::Packet& packet);
    void SendCommand(const Protocol::ClientCommand& command);
    static Protocol::GameState CreateInitialState();

    std::unique_ptr<IClientTransport> transport_;
    Protocol::GameState displayState_{};
    uint8_t localPlayerId_ = 0;
    uint32_t clientSequence_ = 0;
    uint32_t serverSequence_ = 0;
};
