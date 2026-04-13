#pragma once
#include <cstdint>

#pragma pack(push, 1)
namespace Protocol {

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

    // Compact input (one byte per direction)
    struct PlayerInput {
        uint8_t up = 0;
        uint8_t down = 0;
        uint8_t left = 0;
        uint8_t right = 0;
    };

    struct GameState {
        uint32_t tick = 0;
        PlayerData players[2];
        BallData ball;
        uint16_t score[2] = {0, 0};
    };

    struct PacketHeader {
        PacketType type;
        uint32_t seq;
    };

    struct Packet {
        PacketHeader header;
        union Payload {
            PlayerInput input;
            GameState state;
            Payload() {}
        } payload;
    };
}
#pragma pack(pop)