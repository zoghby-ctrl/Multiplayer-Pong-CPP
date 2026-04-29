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
        bool up = false;   // 0 = the original state so it doesnt move when we start game
        bool down = false; // 0 = the original state so it doesnt move when we start game
    };

    struct GameState
    {
        float ballX = 0;
        float ballY = 0;

        float leftPaddleY = 0;
        float rightPaddleY = 0;

        int leftScore = 0;
        int rightScore = 0;
    };

    // CLIENT sends this to SERVER
    struct InputPacket
    {
        PacketType type = PacketType::Input;
        PlayerInput input;
    };

    // SERVER sends this to CLIENT
    struct StatePacket
    {
        PacketType type = PacketType::State;
        GameState state;
    };
}