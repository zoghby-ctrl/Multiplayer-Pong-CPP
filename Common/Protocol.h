#pragma once

#include <cstdint>

namespace Net
{
    static constexpr std::uint32_t kProtocolVersion = 1;
    static constexpr unsigned short kDefaultPort = 27015;
    static constexpr int kMaxPlayers = 2;

    enum class PacketType : std::uint32_t
    {
        JoinRequest = 1,
        JoinAccept = 2,
        Input = 3,
        State = 4
    };

    enum class GamePhase : std::uint32_t
    {
        Waiting = 0,
        Playing = 1,
        GameOver = 2
    };

#pragma pack(push, 1)
    struct JoinRequestPacket
    {
        std::uint32_t type = static_cast<std::uint32_t>(PacketType::JoinRequest);
        std::uint32_t protocolVersion = kProtocolVersion;
    };

    struct JoinAcceptPacket
    {
        std::uint32_t type = static_cast<std::uint32_t>(PacketType::JoinAccept);
        std::uint32_t playerId = 0;
    };

    struct InputPacket
    {
        std::uint32_t type = static_cast<std::uint32_t>(PacketType::Input);
        std::uint32_t playerId = 0;
        std::uint8_t moveUp = 0;
        std::uint8_t moveDown = 0;
        std::uint16_t reserved = 0;
    };

    struct StateSnapshotPacket
    {
        std::uint32_t type = static_cast<std::uint32_t>(PacketType::State);
        std::uint32_t phase = static_cast<std::uint32_t>(GamePhase::Waiting);

        std::uint32_t connectedPlayers = 0;
        std::uint32_t player1Score = 0;
        std::uint32_t player2Score = 0;
        std::uint32_t winner = 0;

        float player1Y = 0.0f;
        float player2Y = 0.0f;
        float ballX = 0.0f;
        float ballY = 0.0f;
    };
#pragma pack(pop)
}
