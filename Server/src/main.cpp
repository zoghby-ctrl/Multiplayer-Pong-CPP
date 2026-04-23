#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>

#include "../../Common/include/protocol.h"

namespace {
using namespace Protocol;

constexpr float kPaddleHalfWidth = 10.0f;
constexpr float kBallRadius = 8.0f;
constexpr float kMaxBallSpeedY = 5.0f;
constexpr float kAiDeadzone = 6.0f;
constexpr float kLeftPaddleX = 20.0f;
constexpr float kRightPaddleX = ArenaWidth - 20.0f;

bool ReceivePacket(Packet& packet) {
    // Networking is not wired up yet in the active solution, so the server
    // keeps running a local simulation and falls back to AI for paddle two.
    packet = {};
    return false;
}

void SendPacketToAll(const Packet& packet) {
    (void)packet;
}

GameState CreateInitialGameState() {
    GameState state{};
    state.status = MatchStatus::InProgress;
    state.players[0].x = kLeftPaddleX;
    state.players[1].x = kRightPaddleX;
    state.players[0].y = ArenaHeight * 0.5f;
    state.players[1].y = ArenaHeight * 0.5f;
    state.ball.x = ArenaWidth * 0.5f;
    state.ball.y = ArenaHeight * 0.5f;
    state.ball.vx = BallInitialSpeedX;
    state.ball.vy = BallInitialSpeedY;
    return state;
}

bool IsGameOver(const GameState& state) {
    return state.status == MatchStatus::GameOver;
}

void ResetBall(GameState& state, float directionX) {
    state.ball.x = ArenaWidth * 0.5f;
    state.ball.y = ArenaHeight * 0.5f;
    state.ball.vx = directionX * BallInitialSpeedX;
    state.ball.vy = BallInitialSpeedY;
}

void ApplyPaddleInput(GameState& state, std::size_t playerIndex, bool up, bool down) {
    float deltaY = 0.0f;
    if (up != down) {
        deltaY = up ? -PaddleSpeed : PaddleSpeed;
    }

    state.players[playerIndex].y += deltaY;
    state.players[playerIndex].y = std::clamp(
        state.players[playerIndex].y,
        PaddleHalfHeight,
        ArenaHeight - PaddleHalfHeight
    );
}

bool ClientControlsSecondPaddle(const Packet& packet) {
    return packet.payload.input.left != 0 || packet.payload.input.right != 0;
}

void HandleClientPacket(GameState& state, const Packet& packet, bool& hasSecondPlayer) {
    if (packet.header.type == PacketType::Disconnect) {
        hasSecondPlayer = false;
        return;
    }

    if (packet.header.type != PacketType::Input) {
        return;
    }

    ApplyPaddleInput(state, 0, packet.payload.input.up != 0, packet.payload.input.down != 0);

    if (ClientControlsSecondPaddle(packet)) {
        hasSecondPlayer = true;
        ApplyPaddleInput(state, 1, packet.payload.input.left != 0, packet.payload.input.right != 0);
    }
}

void ApplyFallbackAi(GameState& state, bool hasSecondPlayer) {
    if (hasSecondPlayer) {
        return;
    }

    const bool aiUp = state.ball.y < (state.players[1].y - kAiDeadzone);
    const bool aiDown = state.ball.y > (state.players[1].y + kAiDeadzone);
    ApplyPaddleInput(state, 1, aiUp, aiDown);
}

void AdvanceBall(GameState& state) {
    state.ball.x += state.ball.vx;
    state.ball.y += state.ball.vy;

    if (state.ball.y <= kBallRadius || state.ball.y >= ArenaHeight - kBallRadius) {
        state.ball.vy *= -1.0f;
        state.ball.y = std::clamp(state.ball.y, kBallRadius, ArenaHeight - kBallRadius);
    }
}

bool CollidesWithPaddle(const GameState& state, std::size_t playerIndex) {
    const float paddleLeft = state.players[playerIndex].x - kPaddleHalfWidth;
    const float paddleRight = state.players[playerIndex].x + kPaddleHalfWidth;
    const float paddleTop = state.players[playerIndex].y - PaddleHalfHeight;
    const float paddleBottom = state.players[playerIndex].y + PaddleHalfHeight;

    return state.ball.x + kBallRadius >= paddleLeft &&
           state.ball.x - kBallRadius <= paddleRight &&
           state.ball.y + kBallRadius >= paddleTop &&
           state.ball.y - kBallRadius <= paddleBottom;
}

void BounceOffPaddle(GameState& state, std::size_t playerIndex) {
    const float paddleDirection = playerIndex == 0 ? 1.0f : -1.0f;
    const float paddleSurface = state.players[playerIndex].x + (paddleDirection * kPaddleHalfWidth);
    const float normalizedOffset =
        (state.ball.y - state.players[playerIndex].y) / (PaddleHalfHeight + kBallRadius);

    state.ball.x = paddleSurface + (paddleDirection * kBallRadius);
    state.ball.vx = paddleDirection * std::abs(state.ball.vx);
    state.ball.vy = std::clamp(state.ball.vy + normalizedOffset, -kMaxBallSpeedY, kMaxBallSpeedY);
}

void HandlePaddleCollisions(GameState& state) {
    if (state.ball.vx < 0.0f && CollidesWithPaddle(state, 0)) {
        BounceOffPaddle(state, 0);
    } else if (state.ball.vx > 0.0f && CollidesWithPaddle(state, 1)) {
        BounceOffPaddle(state, 1);
    }
}

void HandleScoring(GameState& state) {
    if (state.ball.x + kBallRadius < 0.0f) {
        ++state.score[1];
        ResetBall(state, 1.0f);
    } else if (state.ball.x - kBallRadius > ArenaWidth) {
        ++state.score[0];
        ResetBall(state, -1.0f);
    }

    if (state.score[0] >= WinningScore || state.score[1] >= WinningScore) {
        state.status = MatchStatus::GameOver;
    }
}

Packet BuildStatePacket(const GameState& state, uint32_t sequence) {
    Packet packet{};
    packet.header.type = PacketType::State;
    packet.header.seq = sequence;
    packet.payload.state = state;
    return packet;
}
}

int main() {
    GameState globalState = CreateInitialGameState();
    uint32_t nextBroadcastSequence = 0;
    bool hasSecondPlayer = false;

    std::cout << "Server demo started (transport stub; player 2 falls back to AI)." << std::endl;

    while (true) {
        if (!IsGameOver(globalState)) {
            Packet clientPacket{};
            if (ReceivePacket(clientPacket)) {
                HandleClientPacket(globalState, clientPacket, hasSecondPlayer);
            }

            ApplyFallbackAi(globalState, hasSecondPlayer);
            AdvanceBall(globalState);
            HandlePaddleCollisions(globalState);
            HandleScoring(globalState);

            if (!IsGameOver(globalState)) {
                ++globalState.tick;
            }
        }

        const Packet statePacket = BuildStatePacket(globalState, nextBroadcastSequence++);
        SendPacketToAll(statePacket);
        std::this_thread::sleep_for(std::chrono::milliseconds(FrameTimeMs));
    }

    return 0;
}
