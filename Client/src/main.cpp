#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "GameClient.h"

using namespace Protocol;

// Forward declarations for network I/O
void SendPacketToServer(const Packet& p);
bool ReceiveFromServer(Packet& p);
void RenderHUD(const GameState& state, MatchStatus previousStatus, uint32_t client_seq, uint32_t server_seq);

// Color codes for terminal output
static const std::string CLEAR_SCREEN   = "\033[2J";
static const std::string CURSOR_HOME    = "\033[H";
static const std::string COLOR_RESET    = "\033[0m";
static const std::string COLOR_YELLOW   = "\033[1;33m";
static const std::string COLOR_GREEN    = "\033[1;32m";
static const std::string COLOR_RED      = "\033[1;31m";
static const std::string COLOR_CYAN     = "\033[1;36m";
static const std::string COLOR_WHITE    = "\033[1;37m";
static const std::string COLOR_MAGENTA  = "\033[1;35m";

// ============================================================================
// Main Client Loop with Sequence Tracking
// ============================================================================
int main() {
    std::cout << "===== Multiplayer Pong Client =====" << std::endl;
    std::cout << "Sync Mode: Server Authoritative" << std::endl;
    std::cout << "Sequence Tracking: Enabled" << std::endl;
    std::cout << "====================================\n" << std::endl;

    GameClient game_client;
    MatchStatus previous_status = MatchStatus::WaitingForPlayers;
    
    std::cout << "Client initialized. Waiting for server state...\n" << std::endl;

    // Client main loop
    while (true) {
        auto frame_start = std::chrono::high_resolution_clock::now();

        // =====================================================================
        // PHASE 1: Send Input to Server (with sequence number)
        // =====================================================================
        Packet input_packet;
        input_packet.header.type = PacketType::Input;
        input_packet.header.seq = game_client.GetClientSequence();
        
        // TODO: Get input from input manager
        // For now, send down input to test ball-paddle collision
        input_packet.payload.input.up = false;
        input_packet.payload.input.down = false;
        input_packet.payload.input.left = 0;
        input_packet.payload.input.right = 0;
        
        SendPacketToServer(input_packet);

        // =====================================================================
        // PHASE 2: Receive State Updates from Server
        // =====================================================================
        Packet incoming_packet;
        if (ReceiveFromServer(incoming_packet)) {
            if (incoming_packet.header.type == PacketType::State) {
                game_client.ReceiveStateUpdate(incoming_packet);
                
                // Log state reception periodically
                static uint32_t state_update_count = 0;
                state_update_count++;
                if (state_update_count % 60 == 0) {
                    std::cout << "[CLIENT] Received " << state_update_count 
                              << " state updates (Server Seq: " 
                              << game_client.GetServerSequence() << ")" << std::endl;
                }
            }
        }

        // =====================================================================
        // PHASE 3: Render Display (HUD)
        // =====================================================================
        const GameState& display_state = game_client.GetDisplayState();
        if (display_state.status != previous_status) {
            previous_status = display_state.status;
        }
        
        RenderHUD(display_state, previous_status, 
                  game_client.GetClientSequence(),
                  game_client.GetServerSequence());

        // =====================================================================
        // PHASE 4: Frame Rate Pacing
        // =====================================================================
        auto frame_end = std::chrono::high_resolution_clock::now();
        auto frame_duration = frame_end - frame_start;
        auto sleep_duration = std::chrono::milliseconds(FrameTimeMs) - frame_duration;
        
        if (sleep_duration.count() > 0) {
            std::this_thread::sleep_for(sleep_duration);
        }
    }

    return 0;
}

void RenderHUD(const GameState& state, MatchStatus previousStatus, uint32_t client_seq, uint32_t server_seq) {
   
    std::cout << CLEAR_SCREEN << CURSOR_HOME;

    std::cout << COLOR_WHITE
              << "==============================\n"
              << "       SCORE\n"
              << "  Player 1 : " << COLOR_YELLOW << state.score[0] << COLOR_WHITE
              << "   |   Player 2 : " << COLOR_YELLOW << state.score[1] << COLOR_WHITE << "\n"
              << "==============================\n"
              << COLOR_RESET;
 
    switch (state.status) {

        case MatchStatus::WaitingForPlayers:
            
            std::cout << "\n"
                      << COLOR_CYAN
                      << "  Waiting for opponent...\n"
                      << COLOR_RESET;
            break;

        case MatchStatus::InProgress:
            
            if (previousStatus == MatchStatus::WaitingForPlayers) {
                std::cout << "\n"
                          << COLOR_GREEN
                          << "  Connected! Match started.\n"
                          << COLOR_RESET;
            } else {
                std::cout << "\n"
                          << COLOR_GREEN
                          << "  Connected  |  Tick: " << state.tick << "\n"
                          << COLOR_RESET;
            }
            break;

        case MatchStatus::GameOver:
           
            std::cout << "\n"
                      << COLOR_RED
                      << "  *** GAME OVER ***\n"
                      << COLOR_RESET;

            if (state.score[0] > state.score[1]) {
                std::cout << COLOR_YELLOW << "  Player 1 wins!\n" << COLOR_RESET;
            } else if (state.score[1] > state.score[0]) {
                std::cout << COLOR_YELLOW << "  Player 2 wins!\n" << COLOR_RESET;
            } else {
                std::cout << COLOR_YELLOW << "  It's a draw!\n" << COLOR_RESET;
            }
            break;
    }

    // Display sequence information
    std::cout << "\n" << COLOR_MAGENTA
              << "Client Seq: " << client_seq << "  |  "
              << "Server Seq: " << server_seq << "\n"
              << COLOR_RESET;

    // Display ball and paddle positions (for diagnostics)
    std::cout << COLOR_CYAN
              << "Ball: (" << state.ball.x << ", " << state.ball.y << ")  |  "
              << "P1: " << state.players[0].y << "  P2: " << state.players[1].y << "\n"
              << COLOR_RESET;

    std::cout << "\n";
    std::cout.flush();
}

/**
 * Send packet to server.
 * In production, this would send over network to server.
 */
void SendPacketToServer(const Packet& p) {
    // TODO: Implement actual network sending to server
    (void)p;
}

/**
 * Receive state update from server.
 * In production, this would receive from network socket.
 */
bool ReceiveFromServer(Packet& p) {
    // TODO: Implement actual network receiving from server
    // For now, simulate no packets (clients will wait)
    (void)p;
    return false;
}
