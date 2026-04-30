//#pragma once
//#include <cstdint>
//namespace Protocol
//{
//    constexpr int DefaultPort = 7777;
//
//    enum class PacketType : uint8_t
//    {
//        Unknown = 0,
//        Input,
//        State
//    };
//
//    struct PlayerInput
//    {
//        bool up = false;//0 = the  original state  so it doesnt move when we start game
//        bool down = false;//0 = the  original state so it doesnt move when we start game
//    };
//
//    struct GameState
//    {
//        float ballX, ballY;
//        float leftPaddleY, rightPaddleY;
//        int leftScore, rightScore;
//    };
//}

#pragma once
#include <cstdint>
#include <cstddef>
#include <type_traits>

#pragma pack(push, 1)
namespace Protocol {

    // -------------------------------------------------------------------------
    // Shared protocol metadata
    // -------------------------------------------------------------------------
    constexpr uint16_t ProtocolVersion = 1;

    // -------------------------------------------------------------------------
    // Shared gameplay/network constants (single source of truth)
    // -------------------------------------------------------------------------
    constexpr int DefaultPort = 7777;
    constexpr float ArenaWidth = 800.0f;
    constexpr float ArenaHeight = 600.0f;
    constexpr float PaddleSpeed = 5.0f;
    constexpr float PaddleHalfHeight = 40.0f;
    constexpr float BallInitialSpeedX = 1.5f;
    constexpr float BallInitialSpeedY = 1.5f;
    constexpr uint16_t WinningScore = 5;
    constexpr uint32_t FrameTimeMs = 16;

    // -------------------------------------------------------------------------
    // Enums
    // -------------------------------------------------------------------------
    enum class MatchStatus : uint8_t {
        WaitingForPlayers,
        InProgress,
        GameOver
    };

    enum class PacketType : uint8_t {
        Input,
        State,
        Disconnect
    };

    // -------------------------------------------------------------------------
    // Packet structs
    // -------------------------------------------------------------------------
    // Position of a single player paddle in world space.
    struct PlayerData {
        float x = 0.0f, y = 0.0f;
    };

    // Ball position and velocity in world space.
    struct BallData {
        float x = 0.0f, y = 0.0f;
        float vx = 0.0f, vy = 0.0f;
    };

    // Client input snapshot for one simulation tick (1 byte per direction).
    struct PlayerInput {
        uint8_t up = 0;
        uint8_t down = 0;
        uint8_t left = 0;
        uint8_t right = 0;
    };

    // Authoritative state broadcast from server to clients.
    struct GameState {
        // Monotonic server simulation tick.
        uint32_t tick = 0;
        // Player paddles indexed by player id [0..1].
        PlayerData players[2];
        // Current ball state.
        BallData ball;
        // Scores indexed by player id [0..1].
        uint16_t score[2] = {0, 0};
        // Match lifecycle state.
        MatchStatus status = MatchStatus::WaitingForPlayers;
    };

    // Common packet header included in all protocol packets.
    struct PacketHeader {
        // Identifies which payload variant is active.
        PacketType type;
        // Sender sequence number for ordering/diagnostics.
        uint32_t seq;
    };

    // Network packet envelope used by both client and server.
    struct Packet {
        PacketHeader header;
        union Payload {
            PlayerInput input;
            GameState state;
            Payload() : input{} {}
        } payload;
    };

    // -------------------------------------------------------------------------
    // Layout/triviality safeguards for binary compatibility
    // -------------------------------------------------------------------------
    static_assert(std::is_trivially_copyable_v<PlayerData>, "PlayerData must be trivially copyable");
    static_assert(std::is_trivially_copyable_v<BallData>, "BallData must be trivially copyable");
    static_assert(std::is_trivially_copyable_v<PlayerInput>, "PlayerInput must be trivially copyable");
    static_assert(std::is_trivially_copyable_v<GameState>, "GameState must be trivially copyable");
    static_assert(std::is_trivially_copyable_v<PacketHeader>, "PacketHeader must be trivially copyable");
    static_assert(std::is_trivially_copyable_v<Packet>, "Packet must be trivially copyable");

    static_assert(sizeof(PlayerData) == 8, "Unexpected PlayerData size");
    static_assert(sizeof(BallData) == 16, "Unexpected BallData size");
    static_assert(sizeof(PlayerInput) == 4, "Unexpected PlayerInput size");
    static_assert(sizeof(GameState) == 41, "Unexpected GameState size");
    static_assert(sizeof(PacketHeader) == 5, "Unexpected PacketHeader size");
    static_assert(sizeof(Packet) == 46, "Unexpected Packet size");

    static_assert(offsetof(Packet, header) == 0, "PacketHeader must start at offset 0");
    static_assert(offsetof(Packet, payload) == sizeof(PacketHeader), "Packet payload offset changed");
}
#pragma pack(pop)
