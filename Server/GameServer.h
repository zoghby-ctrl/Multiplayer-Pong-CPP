#pragma once

#include "../Common/include/gameplay.h"
#include "../Common/include/protocol.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

class GameServer final {
public:
    GameServer();

    void Tick();
    void ResetGame();
    void SetDifficulty(Protocol::Difficulty difficulty);
    void SetPlayerConnected(std::size_t playerId, bool connected);
    void ProcessClientPacket(std::size_t playerId, const Protocol::Packet& packet);

    Protocol::Packet BuildStatePacket(uint32_t sequence) const;
    Protocol::Packet BuildWelcomePacket(std::size_t playerId, uint32_t sequence) const;

    const Protocol::GameState& GetGameState() const { return state_; }
    bool IsPlayerConnected(std::size_t playerId) const;
    std::size_t ConnectedPlayerCount() const;

private:
    void ProcessCommand(const Protocol::ClientCommand& command);

    Protocol::GameState state_{};
    std::array<Protocol::PlayerInput, Protocol::MaxPlayers> inputs_{};
    std::array<bool, Protocol::MaxPlayers> connectedPlayers_{};
    std::unique_ptr<Pong::IGameMode> gameMode_;
};
