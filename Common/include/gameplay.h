#pragma once

#include "protocol.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace Pong {

struct PaddleIntent {
    bool up = false;
    bool down = false;
};

class IInputSource {
public:
    virtual ~IInputSource() = default;
    virtual PaddleIntent GetIntent(const Protocol::GameState& state, std::size_t playerIndex) const = 0;
};

class HumanInputSource final : public IInputSource {
public:
    explicit HumanInputSource(const Protocol::PlayerInput& input) : input_(input) {}

    PaddleIntent GetIntent(const Protocol::GameState&, std::size_t) const override {
        return {
            input_.up != 0 || input_.left != 0,
            input_.down != 0 || input_.right != 0
        };
    }

private:
    const Protocol::PlayerInput& input_;
};

class AiInputSource final : public IInputSource {
public:
    explicit AiInputSource(Protocol::Difficulty difficulty) : difficulty_(difficulty) {}

    PaddleIntent GetIntent(const Protocol::GameState& state, std::size_t playerIndex) const override {
        const float paddleY = state.players[playerIndex].y;
        const float deadzone = DifficultyDeadzone();
        return {
            state.ball.y < paddleY - deadzone,
            state.ball.y > paddleY + deadzone
        };
    }

private:
    float DifficultyDeadzone() const {
        switch (difficulty_) {
            case Protocol::Difficulty::Easy:
                return Protocol::AiDeadzone * 2.0f;
            case Protocol::Difficulty::Hard:
                return Protocol::AiDeadzone * 0.45f;
            case Protocol::Difficulty::Normal:
            default:
                return Protocol::AiDeadzone;
        }
    }

    Protocol::Difficulty difficulty_;
};

class IdleInputSource final : public IInputSource {
public:
    PaddleIntent GetIntent(const Protocol::GameState&, std::size_t) const override {
        return {};
    }
};

class IGameObject {
public:
    virtual ~IGameObject() = default;
    virtual void Update() = 0;
};

class PaddleEntity final : public IGameObject {
public:
    PaddleEntity(Protocol::PlayerData& data, PaddleIntent intent)
        : data_(data), intent_(intent) {}

    void Update() override {
        if (intent_.up != intent_.down) {
            data_.y += intent_.up ? -Protocol::PaddleSpeed : Protocol::PaddleSpeed;
        }

        data_.y = std::clamp(
            data_.y,
            Protocol::PaddleHalfHeight,
            Protocol::ArenaHeight - Protocol::PaddleHalfHeight
        );
    }

private:
    Protocol::PlayerData& data_;
    PaddleIntent intent_;
};

class BallEntity final : public IGameObject {
public:
    explicit BallEntity(Protocol::BallData& data) : data_(data) {}

    void Update() override {
        data_.x += data_.vx;
        data_.y += data_.vy;
    }

private:
    Protocol::BallData& data_;
};

class IGameMode {
public:
    virtual ~IGameMode() = default;
    virtual void Reset(Protocol::GameState& state) = 0;
    virtual void Step(
        Protocol::GameState& state,
        const std::array<Protocol::PlayerInput, Protocol::MaxPlayers>& inputs,
        const std::array<bool, Protocol::MaxPlayers>& humanPlayers
    ) = 0;
};

class ClassicPongMode final : public IGameMode {
public:
    void Reset(Protocol::GameState& state) override {
        const Protocol::Difficulty difficulty = state.difficulty;
        state = {};
        state.players[0] = {24.0f, Protocol::ArenaHeight * 0.5f};
        state.players[1] = {Protocol::ArenaWidth - 24.0f, Protocol::ArenaHeight * 0.5f};
        state.status = Protocol::MatchStatus::WaitingForPlayers;
        state.difficulty = difficulty;
        ResetBall(state, 1.0f, false);
    }

    void Step(
        Protocol::GameState& state,
        const std::array<Protocol::PlayerInput, Protocol::MaxPlayers>& inputs,
        const std::array<bool, Protocol::MaxPlayers>& humanPlayers
    ) override {
        if (state.status == Protocol::MatchStatus::GameOver) {
            return;
        }

        const bool hasControllingPlayer = humanPlayers[0];
        state.status = hasControllingPlayer
            ? Protocol::MatchStatus::InProgress
            : Protocol::MatchStatus::WaitingForPlayers;

        if (!hasControllingPlayer) {
            ResetBall(state, 1.0f, false);
            return;
        }

        if (state.ball.vx == 0.0f && state.ball.vy == 0.0f) {
            ResetBall(state, 1.0f, true);
        }

        HumanInputSource human0(inputs[0]);
        HumanInputSource human1(inputs[1]);
        AiInputSource ai(state.difficulty);
        IdleInputSource idle;

        const IInputSource* sources[Protocol::MaxPlayers] = {
            humanPlayers[0] ? static_cast<const IInputSource*>(&human0) : static_cast<const IInputSource*>(&idle),
            humanPlayers[1] ? static_cast<const IInputSource*>(&human1) : static_cast<const IInputSource*>(&ai)
        };

        for (std::size_t i = 0; i < Protocol::MaxPlayers; ++i) {
            PaddleEntity paddle(state.players[i], sources[i]->GetIntent(state, i));
            paddle.Update();
        }

        BallEntity ball(state.ball);
        ball.Update();

        ResolveWallCollision(state);
        ResolvePaddleCollision(state, 0, 1.0f);
        ResolvePaddleCollision(state, 1, -1.0f);
        ResolveScoring(state);
    }

private:
    static void ResetBall(Protocol::GameState& state, float directionX, bool moving) {
        state.ball.x = Protocol::ArenaWidth * 0.5f;
        state.ball.y = Protocol::ArenaHeight * 0.5f;
        state.ball.vx = moving ? directionX * DifficultySpeedX(state.difficulty) : 0.0f;
        state.ball.vy = moving ? DifficultySpeedY(state.difficulty) : 0.0f;
    }

    static float DifficultySpeedX(Protocol::Difficulty difficulty) {
        switch (difficulty) {
            case Protocol::Difficulty::Easy:
                return Protocol::BallInitialSpeedX * 0.88f;
            case Protocol::Difficulty::Hard:
                return Protocol::BallInitialSpeedX * 1.22f;
            case Protocol::Difficulty::Normal:
            default:
                return Protocol::BallInitialSpeedX;
        }
    }

    static float DifficultySpeedY(Protocol::Difficulty difficulty) {
        switch (difficulty) {
            case Protocol::Difficulty::Easy:
                return Protocol::BallInitialSpeedY * 0.85f;
            case Protocol::Difficulty::Hard:
                return Protocol::BallInitialSpeedY * 1.18f;
            case Protocol::Difficulty::Normal:
            default:
                return Protocol::BallInitialSpeedY;
        }
    }

    static void ResolveWallCollision(Protocol::GameState& state) {
        if (state.ball.y <= Protocol::BallRadius ||
            state.ball.y >= Protocol::ArenaHeight - Protocol::BallRadius) {
            state.ball.vy *= -1.0f;
            state.ball.y = std::clamp(
                state.ball.y,
                Protocol::BallRadius,
                Protocol::ArenaHeight - Protocol::BallRadius
            );
        }
    }

    static bool IsTouchingPaddle(const Protocol::GameState& state, std::size_t playerIndex) {
        const Protocol::PlayerData& paddle = state.players[playerIndex];
        const float paddleLeft = paddle.x - Protocol::PaddleHalfWidth;
        const float paddleRight = paddle.x + Protocol::PaddleHalfWidth;
        const float paddleTop = paddle.y - Protocol::PaddleHalfHeight;
        const float paddleBottom = paddle.y + Protocol::PaddleHalfHeight;

        return state.ball.x + Protocol::BallRadius >= paddleLeft &&
               state.ball.x - Protocol::BallRadius <= paddleRight &&
               state.ball.y + Protocol::BallRadius >= paddleTop &&
               state.ball.y - Protocol::BallRadius <= paddleBottom;
    }

    static void ResolvePaddleCollision(Protocol::GameState& state, std::size_t playerIndex, float directionX) {
        const bool movingTowardPaddle =
            (directionX > 0.0f && state.ball.vx < 0.0f) ||
            (directionX < 0.0f && state.ball.vx > 0.0f);

        if (!movingTowardPaddle || !IsTouchingPaddle(state, playerIndex)) {
            return;
        }

        const Protocol::PlayerData& paddle = state.players[playerIndex];
        const float normalizedOffset =
            std::clamp((state.ball.y - paddle.y) / Protocol::PaddleHalfHeight, -1.0f, 1.0f);
        const float nextSpeedX =
            std::min(std::abs(state.ball.vx) * Protocol::BallSpeedupPerHit, Protocol::BallMaxSpeedX);

        state.ball.x = paddle.x + (directionX * (Protocol::PaddleHalfWidth + Protocol::BallRadius));
        state.ball.vx = directionX * std::max(nextSpeedX, Protocol::BallInitialSpeedX);
        state.ball.vy = std::clamp(
            state.ball.vy + (normalizedOffset * 2.4f),
            -Protocol::BallMaxSpeedY,
            Protocol::BallMaxSpeedY
        );
    }

    static void ResolveScoring(Protocol::GameState& state) {
        if (state.ball.x + Protocol::BallRadius < 0.0f) {
            ++state.score[1];
            ResetBall(state, 1.0f, true);
        } else if (state.ball.x - Protocol::BallRadius > Protocol::ArenaWidth) {
            ++state.score[0];
            ResetBall(state, -1.0f, true);
        }

        if (state.score[0] >= Protocol::WinningScore ||
            state.score[1] >= Protocol::WinningScore) {
            state.status = Protocol::MatchStatus::GameOver;
            state.ball.vx = 0.0f;
            state.ball.vy = 0.0f;
        }
    }
};

}
