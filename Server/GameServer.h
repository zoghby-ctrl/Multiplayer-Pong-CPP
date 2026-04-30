#pragma once
#include <cstdint>
#include <chrono>
#include <queue>
#include <array>
#include "../Common/include/protocol.h"

using namespace Protocol;

class GameServer {
public:
    GameServer();
    ~GameServer() = default;

    // Main server update loop tick
    void Tick();

    // Process incoming client packet
    void ProcessClientPacket(uint8_t clientId, const Packet& packet);

    // Get current game state
    const GameState& GetGameState() const { return current_state_; }

    // Get whether we should broadcast state this frame
    bool ShouldBroadcastState() const { return should_broadcast_; }

    // Get current server tick
    uint32_t GetServerTick() const { return current_state_.tick; }

    // Reset game state for new match
    void ResetGame();

private:
    // Internal state
    GameState current_state_;
    std::array<PlayerInput, 2> current_input_;
    std::array<uint32_t, 2> client_seq_;
    
    // Tick pacing
    uint32_t tick_counter_;
    bool should_broadcast_;
    static constexpr uint32_t BROADCAST_INTERVAL = 1; // Broadcast every tick

    // Physics simulation
    void UpdateBall();
    void UpdatePlayerInput();
    void CheckCollisions();
    void ResetBallToCenter();

    // Paddle collision helpers
    bool CheckPaddleCollision(const PlayerData& paddle, const BallData& ball, float& new_vy);
};
