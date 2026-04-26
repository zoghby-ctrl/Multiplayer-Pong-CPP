#pragma once
#include <cstdint>
#include "../Common/include/protocol.h"

using namespace Protocol;

class GameClient {
public:
    GameClient();
    ~GameClient() = default;

    // Update client (handle input, request network updates)
    void Update();

    // Process state update from server
    void ReceiveStateUpdate(const Packet& state_packet);

    // Send input to server
    void SendInput(bool up, bool down);

    // Get current displayed state
    const GameState& GetDisplayState() const { return display_state_; }

    // Get client sequence for diagnostics
    uint32_t GetClientSequence() const { return client_sequence_; }

    // Get last received server sequence
    uint32_t GetServerSequence() const { return last_server_sequence_; }

private:
    GameState display_state_;
    PlayerInput current_input_;
    
    // Sequence tracking
    uint32_t client_sequence_;
    uint32_t last_server_sequence_;
    
    // State synchronization
    void InterpolateState();
    void ApplyServerState(const GameState& server_state);
};
