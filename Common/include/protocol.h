#pragma once
#include <cstdint>
namespace Protocol {
    constexpr int DefaultPort = 7777;
    constexpr int MAX_PLAYERS = 4;

    enum class PacketType : uint8_t {
        Unknown = 0,
        Input = 1,
        State = 2,
        Disconnect = 3
    };

#pragma pack(push, 1)

    struct PlayerInput {
        bool up = false;
        bool down = false;
        bool left = false;
        bool right = false;
    };

    struct PlayerState {
        int32_t id = 0;
        float x = 0, y = 0;
    };

    struct GameState {
        uint32_t tick = 0;
        PlayerState players[MAX_PLAYERS];
    };

    struct PacketHeader {
        PacketType type;
        uint32_t seq;
    };

    struct Packet {
        PacketHeader header;
        union {
            PlayerInput input;
            GameState state;
        } payload;
    };

#pragma pack(pop)
}