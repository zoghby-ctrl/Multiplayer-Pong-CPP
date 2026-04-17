#pragma once
#include <cstdint>
#include <type_traits>

#pragma pack(push, 1)
namespace Protocol {

    enum class PacketType : uint8_t {
        Input,
        State,
        Disconnect
    };

    struct PlayerData {
        float x, y;
    };

    struct BallData {
        float x, y;
        float vx, vy;
    };

    struct PlayerInput {
        uint8_t up;
        uint8_t down;
        uint8_t left;
        uint8_t right;
    };

    struct GameState {
        uint32_t tick;
        PlayerData players[2];
        BallData ball;
        uint16_t score[2];
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
            Payload() : input{} {}
        } payload;
    };

    static_assert(std::is_trivially_copyable_v<Packet>, "Packet must stay trivially copyable for network serialization.");
}
#pragma pack(pop)
