#pragma once

namespace Protocol
{
    constexpr int DefaultPort = 7777;

    enum class PacketType : uint8_t
    {
        Unknown = 0,
        Input,
        State
    };

    struct PlayerInput
    {
        bool up = false;
        bool down = false;
    };

    struct GameState
    {
        float ballX, ballY;
        float leftPaddleY, rightPaddleY;
        int leftScore, rightScore;
    };
}