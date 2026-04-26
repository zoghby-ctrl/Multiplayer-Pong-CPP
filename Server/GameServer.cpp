#include "GameServer.h"
#include <algorithm>
#include <cmath>

GameServer::GameServer()
    : tick_counter_(0), should_broadcast_(false) {
    // Initialize game state
    ResetGame();
    
    // Initialize input tracking
    current_input_[0] = {};
    current_input_[1] = {};
    
    // Initialize client sequence tracking
    client_seq_[0] = 0;
    client_seq_[1] = 0;
}

void GameServer::ResetGame() {
    current_state_.tick = 0;
    current_state_.status = MatchStatus::InProgress;
    
    // Position players
    current_state_.players[0].x = 20.0f;
    current_state_.players[0].y = ArenaHeight * 0.5f;
    current_state_.players[1].x = ArenaWidth - 20.0f;
    current_state_.players[1].y = ArenaHeight * 0.5f;
    
    // Reset ball to center
    ResetBallToCenter();
    
    // Reset scores
    current_state_.score[0] = 0;
    current_state_.score[1] = 0;
}

void GameServer::Tick() {
    // Process physics simulation at fixed tick rate
    UpdatePlayerInput();
    UpdateBall();
    CheckCollisions();
    
    // Determine if we should broadcast state this tick
    should_broadcast_ = (tick_counter_ % BROADCAST_INTERVAL) == 0;
    
    // Increment tick counter
    tick_counter_++;
    current_state_.tick = tick_counter_;
}

void GameServer::ProcessClientPacket(uint8_t clientId, const Packet& packet) {
    if (clientId >= 2) return;
    
    // Track sequence number for diagnostics/ordering
    client_seq_[clientId] = packet.header.seq;
    
    // Process input packet
    if (packet.header.type == PacketType::Input) {
        current_input_[clientId] = packet.payload.input;
    }
}

void GameServer::UpdatePlayerInput() {
    // Apply input to player 0 (left side)
    if (current_input_[0].up) {
        current_state_.players[0].y -= PaddleSpeed;
    }
    if (current_input_[0].down) {
        current_state_.players[0].y += PaddleSpeed;
    }
    current_state_.players[0].y = std::clamp(
        current_state_.players[0].y,
        PaddleHalfHeight,
        ArenaHeight - PaddleHalfHeight
    );
    
    // Apply input to player 1 (right side)
    if (current_input_[1].up) {
        current_state_.players[1].y -= PaddleSpeed;
    }
    if (current_input_[1].down) {
        current_state_.players[1].y += PaddleSpeed;
    }
    current_state_.players[1].y = std::clamp(
        current_state_.players[1].y,
        PaddleHalfHeight,
        ArenaHeight - PaddleHalfHeight
    );
}

void GameServer::UpdateBall() {
    // Update ball position
    current_state_.ball.x += current_state_.ball.vx;
    current_state_.ball.y += current_state_.ball.vy;
    
    // Wall collision (top/bottom)
    if (current_state_.ball.y <= 0.0f || current_state_.ball.y >= ArenaHeight) {
        current_state_.ball.vy *= -1.0f;
        current_state_.ball.y = std::clamp(current_state_.ball.y, 0.0f, ArenaHeight);
    }
}

void GameServer::CheckCollisions() {
    // Paddle collision detection
    float new_vy = current_state_.ball.vy;
    
    // Check left paddle collision (player 0)
    if (CheckPaddleCollision(current_state_.players[0], current_state_.ball, new_vy)) {
        current_state_.ball.vx = std::abs(current_state_.ball.vx);
        current_state_.ball.vy = new_vy;
    }
    
    // Check right paddle collision (player 1)
    if (CheckPaddleCollision(current_state_.players[1], current_state_.ball, new_vy)) {
        current_state_.ball.vx = -std::abs(current_state_.ball.vx);
        current_state_.ball.vy = new_vy;
    }
    
    // Score on out of bounds
    if (current_state_.ball.x < 0.0f) {
        current_state_.score[1]++;
        ResetBallToCenter();
    } else if (current_state_.ball.x > ArenaWidth) {
        current_state_.score[0]++;
        ResetBallToCenter();
    }
    
    // Check win condition
    if (current_state_.score[0] >= WinningScore || 
        current_state_.score[1] >= WinningScore) {
        current_state_.status = MatchStatus::GameOver;
    }
}

bool GameServer::CheckPaddleCollision(const PlayerData& paddle, const BallData& ball, float& new_vy) {
    const float PADDLE_WIDTH = 10.0f;
    const float PADDLE_HEIGHT = PaddleHalfHeight * 2.0f;
    
    // Check if ball is within paddle bounds horizontally
    float dx_left = std::abs(ball.x - paddle.x);
    if (dx_left > PADDLE_WIDTH + 5.0f) return false; // Ball too far from paddle
    
    // Check if ball is within paddle bounds vertically
    float dy = std::abs(ball.y - paddle.y);
    if (dy > PADDLE_HEIGHT * 0.5f) return false; // Ball above/below paddle
    
    // Only collide if ball is moving towards paddle
    bool moving_towards_left = paddle.x < ArenaWidth * 0.5f && ball.vx < 0.0f;
    bool moving_towards_right = paddle.x > ArenaWidth * 0.5f && ball.vx > 0.0f;
    
    if (!moving_towards_left && !moving_towards_right) return false;
    
    // Calculate new velocity based on where ball hits paddle (top/bottom)
    float paddle_surface_ratio = (ball.y - paddle.y) / (PADDLE_HEIGHT * 0.5f);
    paddle_surface_ratio = std::clamp(paddle_surface_ratio, -1.0f, 1.0f);
    new_vy = paddle_surface_ratio * 3.0f;
    
    return true;
}

void GameServer::ResetBallToCenter() {
    current_state_.ball.x = ArenaWidth * 0.5f;
    current_state_.ball.y = ArenaHeight * 0.5f;
    current_state_.ball.vx = (current_state_.score[0] > current_state_.score[1]) ? BallInitialSpeedX : -BallInitialSpeedX;
    current_state_.ball.vy = BallInitialSpeedY;
}
