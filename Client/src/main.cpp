#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>

#include "../../Common/include/protocol.h"

namespace {
using namespace Protocol;

constexpr const char* kClearScreen = "\033[2J";
constexpr const char* kCursorHome = "\033[H";
constexpr const char* kColorReset = "\033[0m";
constexpr const char* kColorYellow = "\033[1;33m";
constexpr const char* kColorGreen = "\033[1;32m";
constexpr const char* kColorRed = "\033[1;31m";
constexpr const char* kColorCyan = "\033[1;36m";
constexpr const char* kColorWhite = "\033[1;37m";

constexpr uint32_t kWaitingTicks = 90;
constexpr uint32_t kLeftScoreInterval = 120;
constexpr uint32_t kRightScoreInterval = 180;

struct DemoStateFeed {
    uint32_t tick = 0;
    uint16_t leftScore = 0;
    uint16_t rightScore = 0;
};

Packet BuildNeutralInputPacket(uint32_t sequence) {
    Packet packet{};
    packet.header.type = PacketType::Input;
    packet.header.seq = sequence;
    return packet;
}

void SendPacketToServer(const Packet& packet) {
    // Transport is stubbed for now; keep the call site explicit so a real
    // networking layer can replace this without reshaping the client loop.
    (void)packet;
}

PlayerData MakePlayer(float x, float y) {
    PlayerData player{};
    player.x = x;
    player.y = y;
    return player;
}

BallData MakeBall(float x, float y, float vx, float vy) {
    BallData ball{};
    ball.x = x;
    ball.y = y;
    ball.vx = vx;
    ball.vy = vy;
    return ball;
}

MatchStatus DetermineMatchStatus(const DemoStateFeed& feed) {
    if (feed.leftScore >= WinningScore || feed.rightScore >= WinningScore) {
        return MatchStatus::GameOver;
    }

    if (feed.tick < kWaitingTicks) {
        return MatchStatus::WaitingForPlayers;
    }

    return MatchStatus::InProgress;
}

GameState BuildDemoState(const DemoStateFeed& feed) {
    GameState state{};
    state.tick = feed.tick;
    state.players[0] = MakePlayer(20.0f, ArenaHeight * 0.5f);
    state.players[1] = MakePlayer(ArenaWidth - 20.0f, ArenaHeight * 0.5f);
    state.ball = MakeBall(ArenaWidth * 0.5f, ArenaHeight * 0.5f, 0.0f, 0.0f);
    state.score[0] = feed.leftScore;
    state.score[1] = feed.rightScore;
    state.status = DetermineMatchStatus(feed);
    return state;
}

void AdvanceDemoFeed(DemoStateFeed& feed) {
    if (DetermineMatchStatus(feed) == MatchStatus::GameOver) {
        return;
    }

    ++feed.tick;

    if (feed.tick < kWaitingTicks) {
        return;
    }

    if (feed.tick % kLeftScoreInterval == 0 && feed.leftScore < WinningScore) {
        ++feed.leftScore;
    }

    if (feed.tick % kRightScoreInterval == 0 && feed.rightScore < WinningScore) {
        ++feed.rightScore;
    }
}

bool ReceiveFromServer(Packet& packet) {
    static DemoStateFeed demoFeed{};

    packet = {};
    packet.header.type = PacketType::State;
    packet.header.seq = demoFeed.tick;
    packet.payload.state = BuildDemoState(demoFeed);

    AdvanceDemoFeed(demoFeed);
    return true;
}

void PrintScoreboard(const GameState& state) {
    std::cout << kColorWhite
              << "==============================\n"
              << "        SCOREBOARD\n"
              << "  Player 1 : " << kColorYellow << state.score[0] << kColorWhite
              << "   |   Player 2 : " << kColorYellow << state.score[1] << kColorWhite << "\n"
              << "==============================\n"
              << kColorReset;
}

void PrintStatusLine(const GameState& state, MatchStatus previousStatus) {
    switch (state.status) {
        case MatchStatus::WaitingForPlayers:
            std::cout << "\n"
                      << kColorCyan
                      << "  Demo transport active. Waiting for a remote player.\n"
                      << kColorReset;
            break;

        case MatchStatus::InProgress:
            std::cout << "\n"
                      << kColorGreen;

            if (previousStatus == MatchStatus::WaitingForPlayers) {
                std::cout << "  Demo match started. Rendering scripted server state.\n";
            } else {
                std::cout << "  Demo match in progress  |  Tick: " << state.tick << "\n";
            }

            std::cout << kColorReset;
            break;

        case MatchStatus::GameOver:
            std::cout << "\n"
                      << kColorRed
                      << "  *** DEMO ROUND COMPLETE ***\n"
                      << kColorReset;

            if (state.score[0] > state.score[1]) {
                std::cout << kColorYellow << "  Player 1 wins the scripted round.\n" << kColorReset;
            } else if (state.score[1] > state.score[0]) {
                std::cout << kColorYellow << "  Player 2 wins the scripted round.\n" << kColorReset;
            } else {
                std::cout << kColorYellow << "  The scripted round ended in a draw.\n" << kColorReset;
            }
            break;
    }
}

void RenderHud(const GameState& state, MatchStatus previousStatus) {
    std::cout << kClearScreen << kCursorHome;
    PrintScoreboard(state);
    PrintStatusLine(state, previousStatus);
    std::cout << "\n";
    std::cout.flush();
}
}

int main() {
    uint32_t nextClientSequence = 0;
    GameState lastState{};
    lastState.status = MatchStatus::WaitingForPlayers;

    RenderHud(lastState, lastState.status);
    std::cout << "Client demo started (transport stub; no live network)." << std::endl;

    while (true) {
        SendPacketToServer(BuildNeutralInputPacket(nextClientSequence++));

        Packet incomingState{};
        if (ReceiveFromServer(incomingState) && incomingState.header.type == PacketType::State) {
            const MatchStatus previousStatus = lastState.status;
            lastState = incomingState.payload.state;
            RenderHud(lastState, previousStatus);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(FrameTimeMs));
    }

    return 0;
}
