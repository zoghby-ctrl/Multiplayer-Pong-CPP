#include "ServerGame.h"

#include <array>

static float ClampFloat(float value, float minValue, float maxValue)
{
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

static float AbsFloat(float value)
{
    return (value < 0.0f) ? -value : value;
}

static bool AABBOverlap(float ax, float ay, float ahw, float ahh,
    float bx, float by, float bhw, float bhh)
{
    return AbsFloat(ax - bx) <= (ahw + bhw) &&
        AbsFloat(ay - by) <= (ahh + bhh);
}

class PaddleController
{
public:
    virtual ~PaddleController() = default;
    virtual int GetMoveDirection(const ServerPaddle& paddle, const ServerBall& ball) const = 0;
};

class HumanController final : public PaddleController
{
public:
    HumanController(bool moveUp, bool moveDown)
        : m_moveUp(moveUp), m_moveDown(moveDown)
    {
    }

    int GetMoveDirection(const ServerPaddle&, const ServerBall&) const override
    {
        if (m_moveUp == m_moveDown)
            return 0;
        return m_moveUp ? 1 : -1;
    }

private:
    bool m_moveUp = false;
    bool m_moveDown = false;
};

class AiController final : public PaddleController
{
public:
    int GetMoveDirection(const ServerPaddle& paddle, const ServerBall& ball) const override
    {
        const bool leftPaddle = paddle.X() < 0.0f;
        const bool ballComing = (leftPaddle && ball.VelocityX() < 0.0f) || (!leftPaddle && ball.VelocityX() > 0.0f);
        const float targetY = ballComing ? ball.Y() : 0.0f;
        const float deadZone = 0.035f;

        if (targetY > paddle.Y() + deadZone)
            return 1;
        if (targetY < paddle.Y() - deadZone)
            return -1;
        return 0;
    }
};

void GameObject::SetPosition(float x, float y)
{
    m_x = x;
    m_y = y;
}

void ServerPaddle::Reset(float x)
{
    SetPosition(x, 0.0f);
    m_moveDirection = 0;
}

void ServerPaddle::SetMoveDirection(int direction)
{
    const float value = ClampFloat(static_cast<float>(direction), -1.0f, 1.0f);
    m_moveDirection = value > 0.0f ? 1 : (value < 0.0f ? -1 : 0);
}

void ServerPaddle::Update(float dt)
{
    SetPosition(X(), ClampFloat(Y() + static_cast<float>(m_moveDirection) * m_speed * dt, -0.80f, 0.80f));
}

void ServerBall::Reset(float direction)
{
    SetPosition(0.0f, 0.0f);
    m_halfSize = 0.03f;
    m_vx = 0.75f * direction;
    m_vy = 0.42f;
}

void ServerBall::Update(float dt)
{
    SetPosition(X() + m_vx * dt, Y() + m_vy * dt);
}

void ServerBall::BounceX()
{
    m_vx *= -1.04f;
    m_vx = ClampFloat(m_vx, -1.25f, 1.25f);
}

void ServerBall::BounceY()
{
    m_vy *= -1.0f;
}

void ServerBall::SetVelocityY(float velocityY)
{
    m_vy = ClampFloat(velocityY, -0.95f, 0.95f);
}

void ServerBall::SetX(float x)
{
    SetPosition(x, Y());
}

void ServerBall::SetY(float y)
{
    SetPosition(X(), y);
}

void ServerGame::Reset()
{
    m_player1.Reset(-0.92f);
    m_player2.Reset(0.92f);

    m_player1Score = 0;
    m_player2Score = 0;
    m_winner = 0;
    m_phase = Net::GamePhase::Waiting;
    m_gameOverTimer = 0.0f;

    ResetRound(1.0f);
}

void ServerGame::BeginMatch()
{
    m_player1.Reset(-0.92f);
    m_player2.Reset(0.92f);
    m_player1Score = 0;
    m_player2Score = 0;
    m_winner = 0;
    m_phase = Net::GamePhase::Playing;
    m_gameOverTimer = 0.0f;
    ResetRound(1.0f);
}

void ServerGame::ResetRound(float direction)
{
    m_ball.Reset(direction);
}

void ServerGame::Update(float dt, bool p1Up, bool p1Down, bool p2Up, bool p2Down, bool p1Connected, bool p2Connected)
{
    const int connectedPlayers = (p1Connected ? 1 : 0) + (p2Connected ? 1 : 0);
    if (connectedPlayers == 0)
    {
        Reset();
        return;
    }

    if (m_phase == Net::GamePhase::Waiting)
        BeginMatch();

    if (m_phase == Net::GamePhase::GameOver)
    {
        m_gameOverTimer += dt;
        if (m_gameOverTimer >= 2.0f)
            BeginMatch();
        return;
    }

    HumanController p1Human(p1Up, p1Down);
    HumanController p2Human(p2Up, p2Down);
    AiController ai;

    const PaddleController& p1Controller = p1Connected ? static_cast<const PaddleController&>(p1Human) : static_cast<const PaddleController&>(ai);
    const PaddleController& p2Controller = p2Connected ? static_cast<const PaddleController&>(p2Human) : static_cast<const PaddleController&>(ai);

    m_player1.SetMoveDirection(p1Controller.GetMoveDirection(m_player1, m_ball));
    m_player2.SetMoveDirection(p2Controller.GetMoveDirection(m_player2, m_ball));

    std::array<GameObject*, 3> objects = { &m_player1, &m_player2, &m_ball };
    for (GameObject* object : objects)
        object->Update(dt);

    HandleWallBounces();
    HandlePaddleHits();
    HandleScoring();
}

void ServerGame::HandleWallBounces()
{
    if (m_ball.Y() + m_ball.HalfSize() >= 1.0f)
    {
        m_ball.SetY(1.0f - m_ball.HalfSize());
        m_ball.BounceY();
    }

    if (m_ball.Y() - m_ball.HalfSize() <= -1.0f)
    {
        m_ball.SetY(-1.0f + m_ball.HalfSize());
        m_ball.BounceY();
    }
}

void ServerGame::HandlePaddleHits()
{
    if (AABBOverlap(m_ball.X(), m_ball.Y(), m_ball.HalfSize(), m_ball.HalfSize(),
        m_player1.X(), m_player1.Y(), m_player1.HalfWidth(), m_player1.HalfHeight()) && m_ball.VelocityX() < 0.0f)
    {
        m_ball.SetX(m_player1.X() + m_player1.HalfWidth() + m_ball.HalfSize());
        m_ball.BounceX();
        const float offset = (m_ball.Y() - m_player1.Y()) / m_player1.HalfHeight();
        m_ball.SetVelocityY(0.90f * offset);
    }

    if (AABBOverlap(m_ball.X(), m_ball.Y(), m_ball.HalfSize(), m_ball.HalfSize(),
        m_player2.X(), m_player2.Y(), m_player2.HalfWidth(), m_player2.HalfHeight()) && m_ball.VelocityX() > 0.0f)
    {
        m_ball.SetX(m_player2.X() - m_player2.HalfWidth() - m_ball.HalfSize());
        m_ball.BounceX();
        const float offset = (m_ball.Y() - m_player2.Y()) / m_player2.HalfHeight();
        m_ball.SetVelocityY(0.90f * offset);
    }
}

void ServerGame::HandleScoring()
{
    if (m_ball.X() < -1.10f)
    {
        m_player2Score += 1;
        if (m_player2Score >= m_winningScore)
        {
            m_phase = Net::GamePhase::GameOver;
            m_winner = 2;
            m_gameOverTimer = 0.0f;
        }
        else
        {
            ResetRound(1.0f);
        }
    }

    if (m_ball.X() > 1.10f)
    {
        m_player1Score += 1;
        if (m_player1Score >= m_winningScore)
        {
            m_phase = Net::GamePhase::GameOver;
            m_winner = 1;
            m_gameOverTimer = 0.0f;
        }
        else
        {
            ResetRound(-1.0f);
        }
    }
}

Net::StateSnapshotPacket ServerGame::MakeSnapshot(int connectedPlayers) const
{
    Net::StateSnapshotPacket snapshot;
    snapshot.phase = static_cast<std::uint32_t>(m_phase);
    snapshot.connectedPlayers = static_cast<std::uint32_t>(connectedPlayers);
    snapshot.player1Score = static_cast<std::uint32_t>(m_player1Score);
    snapshot.player2Score = static_cast<std::uint32_t>(m_player2Score);
    snapshot.winner = static_cast<std::uint32_t>(m_winner);
    snapshot.player1Y = m_player1.Y();
    snapshot.player2Y = m_player2.Y();
    snapshot.ballX = m_ball.X();
    snapshot.ballY = m_ball.Y();
    return snapshot;
}

void ServerGame_Reset(ServerGame& game)
{
    game.Reset();
}

void ServerGame_BeginMatch(ServerGame& game)
{
    game.BeginMatch();
}

void ServerGame_Update(ServerGame& game, float dt, bool p1Up, bool p1Down, bool p2Up, bool p2Down, bool p1Connected, bool p2Connected)
{
    game.Update(dt, p1Up, p1Down, p2Up, p2Down, p1Connected, p2Connected);
}

Net::StateSnapshotPacket ServerGame_MakeSnapshot(const ServerGame& game, int connectedPlayers)
{
    return game.MakeSnapshot(connectedPlayers);
}
