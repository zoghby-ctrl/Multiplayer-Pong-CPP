#include <chrono>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <sstream>
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
constexpr uint32_t kConnectionFrameMs = 100;
constexpr uint32_t kHostJoinTicks = 20;
constexpr uint32_t kJoinConnectTicks = 12;
constexpr uint32_t kConnectionTimeoutTicks = 80;

enum class MenuSelection {
    Host,
    Join,
    Quit
};

enum class SessionRole {
    Host,
    Join
};

struct SessionConfig {
    SessionRole role = SessionRole::Host;
    std::string address;
};

struct ConnectionResult {
    bool connected = false;
    std::string error;
};

struct DemoStateFeed {
    uint32_t tick = 0;
    uint16_t leftScore = 0;
    uint16_t rightScore = 0;
};

DemoStateFeed g_demoFeed{};

std::string TrimWhitespace(const std::string& value) {
    const std::string whitespace = " \t\n\r";
    const std::size_t start = value.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return {};
    }
    const std::size_t end = value.find_last_not_of(whitespace);
    return value.substr(start, end - start + 1);
}

std::string ToLower(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

bool ReadLine(std::string& line) {
    std::getline(std::cin, line);
    return static_cast<bool>(std::cin);
}

bool IsValidIpv4(const std::string& address) {
    std::istringstream stream(address);
    int octet = 0;
    char dot = '\0';

    for (int index = 0; index < 4; ++index) {
        if (!(stream >> octet)) {
            return false;
        }
        if (octet < 0 || octet > 255) {
            return false;
        }
        if (index < 3) {
            if (!(stream >> dot) || dot != '.') {
                return false;
            }
        }
    }

    return stream.rdbuf()->in_avail() == 0;
}

bool IsReservedJoinAddress(const std::string& address) {
    return address == "0.0.0.0" || address == "255.255.255.255";
}

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
    packet = {};
    packet.header.type = PacketType::State;
    packet.header.seq = g_demoFeed.tick;
    packet.payload.state = BuildDemoState(g_demoFeed);

    AdvanceDemoFeed(g_demoFeed);
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

void RenderMenuScreen() {
    std::cout << kClearScreen << kCursorHome;
    std::cout << kColorWhite
              << "==============================\n"
              << "     MULTIPLAYER PONG SETUP\n"
              << "==============================\n"
              << kColorReset;
    std::cout << kColorGreen << "1) Host a match\n" << kColorReset;
    std::cout << kColorCyan << "2) Join by IP\n" << kColorReset;
    std::cout << kColorYellow << "Q) Quit\n" << kColorReset;
    std::cout << "\nSelect an option: ";
    std::cout.flush();
}

MenuSelection PromptMenuSelection() {
    while (true) {
        RenderMenuScreen();
        std::string input;
        if (!ReadLine(input)) {
            return MenuSelection::Quit;
        }
        input = ToLower(TrimWhitespace(input));
        if (input == "1" || input == "host" || input == "h") {
            return MenuSelection::Host;
        }
        if (input == "2" || input == "join" || input == "j") {
            return MenuSelection::Join;
        }
        if (input == "q" || input == "quit" || input == "exit") {
            return MenuSelection::Quit;
        }
    }
}

bool PromptJoinAddress(std::string& address) {
    std::cout << kClearScreen << kCursorHome;
    std::cout << kColorWhite
              << "==============================\n"
              << "         JOIN BY IP\n"
              << "==============================\n"
              << kColorReset;
    std::cout << "Enter server IP (blank to return): ";
    std::cout.flush();

    std::string input;
    if (!ReadLine(input)) {
        return false;
    }
    input = TrimWhitespace(input);
    if (input.empty()) {
        return false;
    }
    address = input;
    return true;
}

void RenderWaitingScreen(const SessionConfig& config, uint32_t elapsedTicks) {
    std::cout << kClearScreen << kCursorHome;
    std::cout << kColorWhite
              << "==============================\n"
              << "       CONNECTION STATUS\n"
              << "==============================\n"
              << kColorReset;

    if (config.role == SessionRole::Host) {
        std::cout << kColorGreen << "Hosting on: " << config.address << "\n" << kColorReset;
        std::cout << "Waiting for opponent to join...\n";
    } else {
        std::cout << kColorCyan << "Joining: " << config.address << "\n" << kColorReset;
        std::cout << "Connecting to host...\n";
    }

    const double elapsedSeconds = (elapsedTicks * kConnectionFrameMs) / 1000.0;
    std::cout << "\nElapsed: " << elapsedSeconds << "s\n";
    std::cout.flush();
}

void RenderConnectionFailure(const std::string& error) {
    std::cout << kClearScreen << kCursorHome;
    std::cout << kColorRed
              << "==============================\n"
              << "       CONNECTION FAILED\n"
              << "==============================\n"
              << kColorReset;
    std::cout << error << "\n";
    std::cout << "\nPress Enter to return to the menu.";
    std::cout.flush();
}

void WaitForEnter() {
    std::string ignored;
    ReadLine(ignored);
}

ConnectionResult WaitForConnection(const SessionConfig& config) {
    if (config.role == SessionRole::Join && !IsValidIpv4(config.address)) {
        return {false, "Invalid IP address. Use format 0-255.0-255.0-255.0-255 (e.g. 192.168.1.10)."};
    }

    const bool autoConnect =
        !(config.role == SessionRole::Join && IsReservedJoinAddress(config.address));
    const uint32_t unreachableConnectTicks = kConnectionTimeoutTicks + 1;

    const uint32_t connectTicks = autoConnect
        ? (config.role == SessionRole::Host ? kHostJoinTicks : kJoinConnectTicks)
        : unreachableConnectTicks;

    for (uint32_t tick = 0; tick <= kConnectionTimeoutTicks; ++tick) {
        RenderWaitingScreen(config, tick);
        if (tick >= connectTicks) {
            return {true, {}};
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(kConnectionFrameMs));
    }

    if (config.role == SessionRole::Host) {
        return {false, "No opponent joined before the timeout."};
    }

    return {false, "Unable to reach the host before the timeout."};
}

void ResetDemoFeed(bool skipWaiting) {
    g_demoFeed = {};
    if (skipWaiting) {
        g_demoFeed.tick = kWaitingTicks;
    }
}

void RunGameplayLoop() {
    uint32_t nextClientSequence = 0;
    GameState lastState{};
    lastState.status = MatchStatus::InProgress;

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
}
}

int main() {
    while (true) {
        const MenuSelection selection = PromptMenuSelection();
        if (selection == MenuSelection::Quit) {
            break;
        }

        SessionConfig config{};
        if (selection == MenuSelection::Host) {
            config.role = SessionRole::Host;
            config.address = "0.0.0.0";
        } else {
            config.role = SessionRole::Join;
            if (!PromptJoinAddress(config.address)) {
                continue;
            }
        }

        const ConnectionResult result = WaitForConnection(config);
        if (!result.connected) {
            RenderConnectionFailure(result.error);
            WaitForEnter();
            continue;
        }

        ResetDemoFeed(true);
        RunGameplayLoop();
    }

    return 0;
}
