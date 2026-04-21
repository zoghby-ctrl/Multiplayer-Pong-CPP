#pragma once

#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <cstring>

#pragma pack(push, 1)
namespace Protocol {

    constexpr uint16_t ProtocolVersion = 1;

    constexpr float    ArenaWidth = 800.0f;
    constexpr float    ArenaHeight = 600.0f;
    constexpr float    PaddleSpeed = 5.0f;
    constexpr float    PaddleHalfHeight = 40.0f;
    constexpr float    BallInitialSpeedX = 1.5f;
    constexpr float    BallInitialSpeedY = 1.5f;
    constexpr uint16_t WinningScore = 5;
    constexpr uint32_t FrameTimeMs = 16;

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

    struct PlayerData {
        float x = 0.0f, y = 0.0f;
    };

    struct BallData {
        float x = 0.0f, y = 0.0f;
        float vx = 0.0f, vy = 0.0f;
    };

    struct PlayerInput {
        uint8_t up = 0;
        uint8_t down = 0;
        uint8_t left = 0;
        uint8_t right = 0;
    };

    struct GameState {
        uint32_t   tick = 0;
        PlayerData players[2];
        BallData   ball;
        uint16_t   score[2] = { 0, 0 };
        MatchStatus status = MatchStatus::WaitingForPlayers;
    };

    struct PacketHeader {
        PacketType type;
        uint32_t   seq;
    };

    struct Packet {
        PacketHeader header;
        union Payload {
            PlayerInput input;
            GameState   state;
            Payload() { memset(this, 0, sizeof(*this)); }
        } payload;
    };

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
