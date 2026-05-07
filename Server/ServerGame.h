#pragma once

#include "../Common/Protocol.h"

class GameObject
{
public:
    virtual ~GameObject() = default;
    virtual void Update(float dt) = 0;

    float X() const { return m_x; }
    float Y() const { return m_y; }

protected:
    void SetPosition(float x, float y);

private:
    float m_x = 0.0f;
    float m_y = 0.0f;
};

class ServerPaddle : public GameObject
{
public:
    void Reset(float x);
    void SetMoveDirection(int direction);
    void Update(float dt) override;

    float HalfWidth() const { return m_halfWidth; }
    float HalfHeight() const { return m_halfHeight; }

private:
    int m_moveDirection = 0;
    float m_halfWidth = 0.025f;
    float m_halfHeight = 0.18f;
    float m_speed = 1.35f;
};

class ServerBall : public GameObject
{
public:
    void Reset(float direction);
    void Update(float dt) override;
    void BounceX();
    void BounceY();
    void SetVelocityY(float velocityY);
    void SetX(float x);
    void SetY(float y);

    float HalfSize() const { return m_halfSize; }
    float VelocityX() const { return m_vx; }

private:
    float m_halfSize = 0.03f;
    float m_vx = 0.75f;
    float m_vy = 0.42f;
};

class ServerGame
{
public:
    void Reset();
    void BeginMatch();
    void Update(float dt, bool p1Up, bool p1Down, bool p2Up, bool p2Down, bool p1Connected, bool p2Connected);
    Net::StateSnapshotPacket MakeSnapshot(int connectedPlayers) const;

private:
    void ResetRound(float direction);
    void HandleWallBounces();
    void HandlePaddleHits();
    void HandleScoring();

private:
    ServerPaddle m_player1;
    ServerPaddle m_player2;
    ServerBall m_ball;

    int m_player1Score = 0;
    int m_player2Score = 0;
    int m_winningScore = 5;
    Net::GamePhase m_phase = Net::GamePhase::Waiting;
    int m_winner = 0;
    float m_gameOverTimer = 0.0f;
};

void ServerGame_Reset(ServerGame& game);
void ServerGame_BeginMatch(ServerGame& game);
void ServerGame_Update(ServerGame& game, float dt, bool p1Up, bool p1Down, bool p2Up, bool p2Down, bool p1Connected, bool p2Connected);
Net::StateSnapshotPacket ServerGame_MakeSnapshot(const ServerGame& game, int connectedPlayers);
