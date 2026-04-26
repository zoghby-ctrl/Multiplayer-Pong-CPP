#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <queue>
#include "GameServer.h"

using namespace Protocol;

// Forward declarations for network simulation
struct IncomingPacket {
    uint8_t client_id;
    Packet packet;
};

std::queue<IncomingPacket> incoming_packets;
std::vector<Packet> outgoing_packets;

// Network I/O stubs
void SendPacketToAll(const Packet& p);
bool TryReceivePacket(uint8_t& client_id, Packet& p);

// ============================================================================
// Main Server Loop with Fixed Timestep and Tick Pacing
// ============================================================================
int main() {
    std::cout << "===== Multiplayer Pong Server =====" << std::endl;
    std::cout << "Tick Rate: 60 Hz (16.67 ms per tick)" << std::endl;
    std::cout << "Fixed Timestep: Enabled" << std::endl;
    std::cout << "====================================\n" << std::endl;

    GameServer game_server;
    
    // Fixed timestep constants
    const double TICK_RATE = 1000.0 / static_cast<double>(FrameTimeMs); // 60 Hz
    const std::chrono::nanoseconds TICK_DURATION(
        static_cast<long long>(1e9 / TICK_RATE)  // Nanoseconds per tick
    );

    // Time tracking
    auto last_tick_time = std::chrono::high_resolution_clock::now();
    uint32_t total_ticks = 0;
    uint32_t broadcast_count = 0;

    std::cout << "Server initialized. Waiting for clients...\n" << std::endl;

    // Main server loop
    while (true) {
        auto frame_start = std::chrono::high_resolution_clock::now();

        // =====================================================================
        // PHASE 1: Process Incoming Packets (Sequence Handling)
        // =====================================================================
        uint8_t client_id;
        Packet incoming_packet;
        
        while (TryReceivePacket(client_id, incoming_packet)) {
            // Log incoming packet for diagnostics
            if (incoming_packet.header.type == PacketType::Input) {
                std::cout << "[RECV] Client " << static_cast<int>(client_id) 
                          << " - Input Seq:" << incoming_packet.header.seq << std::endl;
            }
            
            // Process packet in game server (updates input state)
            game_server.ProcessClientPacket(client_id, incoming_packet);
        }

        // =====================================================================
        // PHASE 2: Server Tick (Physics Simulation)
        // =====================================================================
        game_server.Tick();
        total_ticks++;

        // =====================================================================
        // PHASE 3: Broadcast State (Clean Cadence)
        // =====================================================================
        if (game_server.ShouldBroadcastState()) {
            Packet state_packet;
            state_packet.header.type = PacketType::State;
            state_packet.header.seq = total_ticks;  // Server tick as sequence
            state_packet.payload.state = game_server.GetGameState();
            
            SendPacketToAll(state_packet);
            broadcast_count++;

            // Log state broadcast periodically
            if (broadcast_count % 60 == 0) {  // Every 60 broadcasts (~1 second)
                std::cout << "[BCAST] Tick " << game_server.GetServerTick() 
                          << " - Score: " << game_server.GetGameState().score[0]
                          << " vs " << game_server.GetGameState().score[1]
                          << " - Ball: (" << game_server.GetGameState().ball.x << ", "
                          << game_server.GetGameState().ball.y << ")" << std::endl;
            }
        }

        // =====================================================================
        // PHASE 4: Frame Rate Pacing (Sleep to maintain tick rate)
        // =====================================================================
        auto frame_end = std::chrono::high_resolution_clock::now();
        auto frame_duration = frame_end - frame_start;
        
        if (frame_duration < TICK_DURATION) {
            auto sleep_duration = TICK_DURATION - frame_duration;
            std::this_thread::sleep_for(sleep_duration);
        } else {
            // Frame took longer than expected (log warning)
            std::cerr << "[WARN] Frame took " 
                      << std::chrono::duration_cast<std::chrono::milliseconds>(frame_duration).count()
                      << " ms (target: " << FrameTimeMs << " ms)" << std::endl;
        }
    }

    return 0;
}

// ============================================================================
// Network I/O Implementations (Stubs for integration)
// ============================================================================

/**
 * Send packet to all connected clients.
 * In production, this would send over network to all client connections.
 */
void SendPacketToAll(const Packet& p) {
    // TODO: Implement actual network sending to all connected clients
    // For now, this is a placeholder
    (void)p;  // Suppress unused warning
}

/**
 * Try to receive a packet from any connected client.
 * Returns true if a packet was available and placed in `p`.
 * Returns false if no packet was available.
 * 
 * In production, this would receive from network socket(s).
 */
bool TryReceivePacket(uint8_t& client_id, Packet& p) {
    // TODO: Implement actual network receiving from all client connections
    // This would use non-blocking socket I/O or message queue
    
    // For now, simulate no packets received
    (void)client_id;
    (void)p;
    return false;
}
