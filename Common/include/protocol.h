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
        bool up = false;//0 = the  original state  so it doesnt move when we start game
        bool down = false;//0 = the  original state so it doesnt move when we start game
    };

    struct GameState
    {
        float ballX, ballY;
        float leftPaddleY, rightPaddleY;
        int leftScore, rightScore;
    };
}