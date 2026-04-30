#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

#pragma pack(push, 1)
namespace Protocol {

constexpr uint16_t ProtocolVersion = 3;
constexpr int DefaultPort = 7777;
constexpr uint8_t MaxPlayers = 2;

constexpr float ArenaWidth = 800.0f;
constexpr float ArenaHeight = 600.0f;
constexpr float PaddleHalfWidth = 10.0f;
constexpr float PaddleHalfHeight = 48.0f;
constexpr float PaddleSpeed = 7.0f;
constexpr float BallRadius = 8.0f;
constexpr float BallInitialSpeedX = 4.8f;
constexpr float BallInitialSpeedY = 2.6f;
constexpr float BallMaxSpeedX = 9.5f;
constexpr float BallMaxSpeedY = 7.8f;
constexpr float BallSpeedupPerHit = 1.07f;
constexpr float AiDeadzone = 8.0f;
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
    Disconnect,
    Welcome,
    Command
};

enum class Difficulty : uint8_t {
    Easy,
    Normal,
    Hard
};

enum class ClientCommandType : uint8_t {
    ResetMatch,
    SetDifficulty
};

struct PlayerData {
    float x = 0.0f;
    float y = 0.0f;
};

struct BallData {
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
};

struct PlayerInput {
    uint8_t up = 0;
    uint8_t down = 0;
    uint8_t left = 0;
    uint8_t right = 0;
};

struct ServerWelcome {
    uint8_t playerId = 0;
    uint8_t playerCount = MaxPlayers;
};

struct ClientCommand {
    ClientCommandType type = ClientCommandType::ResetMatch;
    Difficulty difficulty = Difficulty::Normal;
};

struct GameState {
    uint32_t tick = 0;
    PlayerData players[MaxPlayers];
    BallData ball;
    uint16_t score[MaxPlayers] = {0, 0};
    MatchStatus status = MatchStatus::WaitingForPlayers;
    Difficulty difficulty = Difficulty::Normal;
};

struct PacketHeader {
    PacketType type = PacketType::Input;
    uint32_t seq = 0;
};

struct Packet {
    PacketHeader header;
    union Payload {
        PlayerInput input;
        GameState state;
        ServerWelcome welcome;
        ClientCommand command;

        Payload() : input{} {}
    } payload;
};

static_assert(std::is_trivially_copyable_v<PlayerData>, "PlayerData must be trivially copyable");
static_assert(std::is_trivially_copyable_v<BallData>, "BallData must be trivially copyable");
static_assert(std::is_trivially_copyable_v<PlayerInput>, "PlayerInput must be trivially copyable");
static_assert(std::is_trivially_copyable_v<ServerWelcome>, "ServerWelcome must be trivially copyable");
static_assert(std::is_trivially_copyable_v<ClientCommand>, "ClientCommand must be trivially copyable");
static_assert(std::is_trivially_copyable_v<GameState>, "GameState must be trivially copyable");
static_assert(std::is_trivially_copyable_v<PacketHeader>, "PacketHeader must be trivially copyable");
static_assert(std::is_trivially_copyable_v<Packet>, "Packet must be trivially copyable");

static_assert(sizeof(PlayerData) == 8, "Unexpected PlayerData size");
static_assert(sizeof(BallData) == 16, "Unexpected BallData size");
static_assert(sizeof(PlayerInput) == 4, "Unexpected PlayerInput size");
static_assert(sizeof(ServerWelcome) == 2, "Unexpected ServerWelcome size");
static_assert(sizeof(ClientCommand) == 2, "Unexpected ClientCommand size");
static_assert(sizeof(GameState) == 42, "Unexpected GameState size");
static_assert(sizeof(PacketHeader) == 5, "Unexpected PacketHeader size");
static_assert(sizeof(Packet) == 47, "Unexpected Packet size");

static_assert(offsetof(Packet, header) == 0, "PacketHeader must start at offset 0");
static_assert(offsetof(Packet, payload) == sizeof(PacketHeader), "Packet payload offset changed");

}
#pragma pack(pop)
