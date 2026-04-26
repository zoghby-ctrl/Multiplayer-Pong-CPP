#include "GameClient.h"
#include <iostream>

GameClient::GameClient()
    : client_sequence_(0), last_server_sequence_(0) {
    // Initialize display state
    display_state_.tick = 0;
    display_state_.status = MatchStatus::WaitingForPlayers;
    display_state_.score[0] = 0;
    display_state_.score[1] = 0;
    
    // Initialize player positions
    display_state_.players[0].x = 20.0f;
    display_state_.players[0].y = ArenaHeight * 0.5f;
    display_state_.players[1].x = ArenaWidth - 20.0f;
    display_state_.players[1].y = ArenaHeight * 0.5f;
    
    // Initialize ball
    display_state_.ball.x = ArenaWidth * 0.5f;
    display_state_.ball.y = ArenaHeight * 0.5f;
    display_state_.ball.vx = 0.0f;
    display_state_.ball.vy = 0.0f;
    
    // Initialize input
    current_input_ = {};
}

void GameClient::Update() {
    // This is called each frame to send input to server
    // The actual input gathering happens externally via input manager
}

void GameClient::SendInput(bool up, bool down) {
    // Update input state (this would normally come from keyboard/controller)
    current_input_.up = up ? 1 : 0;
    current_input_.down = down ? 1 : 0;
    current_input_.left = 0;
    current_input_.right = 0;
    
    // In production, this would queue a packet to send to server:
    // Packet input_packet;
    // input_packet.header.type = PacketType::Input;
    // input_packet.header.seq = client_sequence_;
    // input_packet.payload.input = current_input_;
    // SendPacketToServer(input_packet);
    // client_sequence_++;
}

void GameClient::ReceiveStateUpdate(const Packet& state_packet) {
    if (state_packet.header.type != PacketType::State) {
        return;
    }
    
    // Track server sequence number for diagnostics
    last_server_sequence_ = state_packet.header.seq;
    
    // Apply server state directly (authoritative)
    ApplyServerState(state_packet.payload.state);
}

void GameClient::ApplyServerState(const GameState& server_state) {
    // Update display state with authoritative server state
    display_state_ = server_state;
}

void GameClient::InterpolateState() {
    // Placeholder for future interpolation logic
    // Could be used for smooth movement between server updates
}
