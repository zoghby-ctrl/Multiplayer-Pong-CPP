#include "Renderer.h"

#include <gl/GL.h>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "GameObject.h"

static GLuint g_fontBase = 0;
static HFONT g_font = nullptr;
static bool g_trailReady = false;
static float g_trailX[8] = {};
static float g_trailY[8] = {};
static float g_lastBallX = 0.0f;
static float g_lastBallY = 0.0f;

static float GetTimeSeconds()
{
    return static_cast<float>(GetTickCount64() % 100000) / 1000.0f;
}

static void DrawRect(float x, float y, float halfW, float halfH, float r, float g, float b, float a = 1.0f)
{
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex2f(x - halfW, y - halfH);
    glVertex2f(x + halfW, y - halfH);
    glVertex2f(x + halfW, y + halfH);
    glVertex2f(x - halfW, y + halfH);
    glEnd();
}

static void DrawDiamond(float x, float y, float size, float r, float g, float b, float a = 1.0f)
{
    glColor4f(r, g, b, a);
    glBegin(GL_QUADS);
    glVertex2f(x, y + size);
    glVertex2f(x + size, y);
    glVertex2f(x, y - size);
    glVertex2f(x - size, y);
    glEnd();
}

static void DrawGradientRect(float x, float y, float halfW, float halfH,
    float topR, float topG, float topB, float bottomR, float bottomG, float bottomB, float a = 1.0f)
{
    glBegin(GL_QUADS);
    glColor4f(bottomR, bottomG, bottomB, a);
    glVertex2f(x - halfW, y - halfH);
    glVertex2f(x + halfW, y - halfH);
    glColor4f(topR, topG, topB, a);
    glVertex2f(x + halfW, y + halfH);
    glVertex2f(x - halfW, y + halfH);
    glEnd();
}

static void DrawCircle(float x, float y, float radius, float r, float g, float b, float a = 1.0f)
{
    constexpr float pi = 3.14159265f;
    glColor4f(r, g, b, a);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int i = 0; i <= 32; ++i)
    {
        const float angle = (2.0f * pi * static_cast<float>(i)) / 32.0f;
        glVertex2f(x + std::cos(angle) * radius, y + std::sin(angle) * radius);
    }
    glEnd();
}

static void DrawLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a, float width)
{
    glLineWidth(width);
    glColor4f(r, g, b, a);
    glBegin(GL_LINES);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glEnd();
}

static void DrawVignette()
{
    DrawRect(-1.03f, 0.0f, 0.08f, 1.1f, 0.0f, 0.0f, 0.0f, 0.32f);
    DrawRect(1.03f, 0.0f, 0.08f, 1.1f, 0.0f, 0.0f, 0.0f, 0.32f);
    DrawRect(0.0f, 1.03f, 1.1f, 0.08f, 0.0f, 0.0f, 0.0f, 0.30f);
    DrawRect(0.0f, -1.03f, 1.1f, 0.08f, 0.0f, 0.0f, 0.0f, 0.34f);
}

static void DrawBackground(float time)
{
    DrawGradientRect(0.0f, 0.0f, 1.0f, 1.0f, 0.09f, 0.10f, 0.13f, 0.015f, 0.018f, 0.025f, 1.0f);

    const float glow = 0.5f + 0.5f * std::sin(time * 0.9f);
    DrawCircle(-0.78f, 0.70f, 0.36f, 0.08f, 0.78f, 0.82f, 0.045f + glow * 0.025f);
    DrawCircle(0.78f, -0.66f, 0.42f, 1.0f, 0.34f, 0.20f, 0.045f + (1.0f - glow) * 0.025f);

    for (float x = -0.80f; x <= 0.81f; x += 0.20f)
        DrawLine(x, -0.96f, x, 0.96f, 0.70f, 0.85f, 0.90f, 0.035f, 1.0f);
    for (float y = -0.72f; y <= 0.73f; y += 0.18f)
        DrawLine(-0.98f, y, 0.98f, y, 0.90f, 0.58f, 0.42f, 0.030f, 1.0f);

    for (int i = 0; i < 10; ++i)
    {
        const float y = -0.90f + i * 0.20f;
        const float drift = std::sin(time * 0.75f + i * 0.9f) * 0.012f;
        const float alpha = 0.08f + 0.04f * std::sin(time * 1.2f + i);
        DrawDiamond(-0.72f + drift, y, 0.010f, 0.20f, 0.85f, 0.95f, alpha);
        DrawDiamond(0.72f - drift, -y, 0.010f, 1.0f, 0.45f, 0.30f, alpha);
    }

    DrawRect(-0.55f, 0.0f, 0.003f, 1.0f, 0.25f, 0.90f, 0.95f, 0.11f);
    DrawRect(0.55f, 0.0f, 0.003f, 1.0f, 1.0f, 0.45f, 0.35f, 0.11f);
}

static void DrawArena(float time)
{
    DrawRect(-0.27f, 0.86f, 0.20f, 0.082f, 0.02f, 0.03f, 0.04f, 0.70f);
    DrawRect(0.27f, 0.86f, 0.20f, 0.082f, 0.02f, 0.03f, 0.04f, 0.70f);
    DrawGradientRect(-0.27f, 0.86f, 0.18f, 0.065f, 0.04f, 0.28f, 0.32f, 0.02f, 0.11f, 0.13f, 0.82f);
    DrawGradientRect(0.27f, 0.86f, 0.18f, 0.065f, 0.33f, 0.14f, 0.09f, 0.12f, 0.04f, 0.03f, 0.82f);

    glLineWidth(2.0f);
    glColor4f(0.64f, 0.76f, 0.85f, 0.42f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-0.98f, -0.96f);
    glVertex2f(0.98f, -0.96f);
    glVertex2f(0.98f, 0.96f);
    glVertex2f(-0.98f, 0.96f);
    glEnd();

    DrawLine(-0.99f, 0.97f, 0.99f, 0.97f, 0.35f, 0.95f, 0.95f, 0.32f, 4.0f);
    DrawLine(-0.99f, -0.97f, 0.99f, -0.97f, 1.0f, 0.52f, 0.32f, 0.26f, 4.0f);

    for (int i = 0; i < 12; ++i)
    {
        const float y = -0.88f + i * 0.16f;
        const float pulse = 0.55f + 0.45f * std::sin(time * 3.0f + i * 0.45f);
        DrawRect(0.0f, y, 0.006f, 0.050f, 0.70f, 0.82f, 0.90f, 0.20f + pulse * 0.24f);
    }

    DrawCircle(0.0f, 0.0f, 0.155f, 0.75f, 0.88f, 1.0f, 0.045f);
    DrawCircle(0.0f, 0.0f, 0.104f, 1.0f, 0.64f, 0.40f, 0.040f);
}

static void ResetTrail(float x, float y)
{
    for (int i = 0; i < 8; ++i)
    {
        g_trailX[i] = x;
        g_trailY[i] = y;
    }
    g_trailReady = true;
    g_lastBallX = x;
    g_lastBallY = y;
}

static void UpdateTrail(float x, float y)
{
    const float dx = x - g_trailX[0];
    const float dy = y - g_trailY[0];
    if (!g_trailReady || dx * dx + dy * dy > 0.35f)
    {
        ResetTrail(x, y);
        return;
    }

    for (int i = 7; i > 0; --i)
    {
        g_trailX[i] = g_trailX[i - 1];
        g_trailY[i] = g_trailY[i - 1];
    }
    g_trailX[0] = x;
    g_trailY[0] = y;
    g_lastBallX = x;
    g_lastBallY = y;
}

static void DrawBallTrail(float ballHalf)
{
    for (int i = 7; i >= 1; --i)
    {
        const float alpha = 0.035f + (7 - i) * 0.035f;
        const float size = ballHalf * (1.35f - i * 0.075f);
        DrawCircle(g_trailX[i], g_trailY[i], size * 1.8f, 0.25f, 0.90f, 1.0f, alpha * 0.34f);
        DrawCircle(g_trailX[i], g_trailY[i], size, 1.0f, 0.78f, 0.42f, alpha);
    }
}

static void DrawPaddle(float x, float y, float halfW, float halfH, float r, float g, float b, bool localPlayer)
{
    const float rimAlpha = localPlayer ? 0.52f : 0.28f;
    DrawRect(x + 0.012f, y - 0.012f, halfW + 0.012f, halfH + 0.020f, 0.0f, 0.0f, 0.0f, 0.34f);
    DrawRect(x, y, halfW + 0.020f, halfH + 0.035f, r, g, b, rimAlpha * 0.55f);
    DrawRect(x, y, halfW + 0.009f, halfH + 0.018f, 1.0f, 1.0f, 1.0f, rimAlpha * 0.10f);
    DrawGradientRect(x, y, halfW, halfH, r + 0.12f, g + 0.12f, b + 0.12f, r * 0.65f, g * 0.65f, b * 0.65f, 0.98f);
    DrawRect(x, y + halfH * 0.55f, halfW * 0.62f, halfH * 0.05f, 1.0f, 1.0f, 1.0f, 0.30f);
    DrawRect(x, y, halfW * 0.22f, halfH * 0.86f, 1.0f, 1.0f, 1.0f, 0.18f);
    DrawDiamond(x, y + halfH * 0.92f, halfW * 0.42f, 1.0f, 1.0f, 1.0f, rimAlpha * 0.28f);
    DrawDiamond(x, y - halfH * 0.92f, halfW * 0.42f, 1.0f, 1.0f, 1.0f, rimAlpha * 0.20f);
}

static void DrawBall(float x, float y, float halfSize, float time)
{
    const float pulse = 1.0f + 0.10f * std::sin(time * 7.0f);
    DrawLine(g_lastBallX, g_lastBallY, x, y, 0.40f, 0.90f, 1.0f, 0.22f, 5.0f);
    DrawCircle(x, y, halfSize * 2.8f * pulse, 0.50f, 0.95f, 1.0f, 0.10f);
    DrawCircle(x, y, halfSize * 1.65f * pulse, 1.0f, 0.72f, 0.38f, 0.22f);
    DrawCircle(x, y, halfSize, 1.0f, 1.0f, 1.0f, 1.0f);
    DrawCircle(x - halfSize * 0.25f, y + halfSize * 0.28f, halfSize * 0.32f, 0.35f, 0.95f, 1.0f, 0.85f);
}

static GameObject MakeCenteredObject(float x, float y, float halfW, float halfH)
{
    return GameObject(x, y, halfW * 2.0f, halfH * 2.0f);
}

static void BeginTextPass(int width, int height)
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, static_cast<double>(width), static_cast<double>(height), 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
}

static void EndTextPass()
{
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}

static void DrawTextLine(int x, int y, float r, float g, float b, const char* text)
{
    if (!text || !g_fontBase)
        return;
    glColor3f(r, g, b);
    glRasterPos2i(x, y);
    glListBase(g_fontBase - 32);
    glCallLists(static_cast<GLsizei>(std::strlen(text)), GL_UNSIGNED_BYTE, text);
}

static int ApproxTextWidth(const char* text)
{
    if (!text) return 0;
    return static_cast<int>(std::strlen(text)) * 9;
}

static void DrawCenteredText(int width, int y, float r, float g, float b, const char* text)
{
    int x = (width - ApproxTextWidth(text)) / 2;
    DrawTextLine(x, y, r, g, b, text);
}

bool Renderer_Initialize(HDC hdc)
{
    g_fontBase = glGenLists(96);
    if (g_fontBase == 0)
        return false;

    g_font = CreateFontA(-20, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
        FF_DONTCARE | DEFAULT_PITCH, "Segoe UI");
    if (!g_font)
        return false;

    SelectObject(hdc, g_font);
    if (!wglUseFontBitmapsA(hdc, 32, 96, g_fontBase))
        return false;

    return true;
}

void Renderer_Shutdown()
{
    if (g_fontBase)
    {
        glDeleteLists(g_fontBase, 96);
        g_fontBase = 0;
    }
    if (g_font)
    {
        DeleteObject(g_font);
        g_font = nullptr;
    }
}

void Renderer_Render(const Net::StateSnapshotPacket& snapshot, bool connected, int playerId, const std::string& hostIp, int width, int height, HDC hdc)
{
    glViewport(0, 0, width, height);
    glClearColor(0.03f, 0.04f, 0.07f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const float paddleX1 = -0.92f;
    const float paddleX2 = 0.92f;
    const float paddleHalfW = 0.025f;
    const float paddleHalfH = 0.18f;
    const float ballHalf = 0.03f;
    const float time = GetTimeSeconds();

    DrawBackground(time);
    DrawArena(time);

    if (connected)
    {
        const GameObject leftPaddle = MakeCenteredObject(paddleX1, snapshot.player1Y, paddleHalfW, paddleHalfH);
        const GameObject rightPaddle = MakeCenteredObject(paddleX2, snapshot.player2Y, paddleHalfW, paddleHalfH);
        const GameObject ball = MakeCenteredObject(snapshot.ballX, snapshot.ballY, ballHalf, ballHalf);

        UpdateTrail(snapshot.ballX, snapshot.ballY);
        DrawBallTrail(ballHalf);
        DrawPaddle(leftPaddle.X(), leftPaddle.Y(), leftPaddle.HalfWidth(), leftPaddle.HalfHeight(), 0.18f, 0.82f, 0.90f, playerId == 1);
        DrawPaddle(rightPaddle.X(), rightPaddle.Y(), rightPaddle.HalfWidth(), rightPaddle.HalfHeight(), 1.0f, 0.45f, 0.32f, playerId == 2);
        DrawBall(ball.X(), ball.Y(), ball.HalfWidth(), time);
    }
    else
    {
        g_trailReady = false;
    }

    DrawVignette();

    BeginTextPass(width, height);

    char header[256] = {};
    std::snprintf(header, sizeof(header), "LAN Pong | Server %s | Player %d", hostIp.c_str(), playerId);
    DrawCenteredText(width, 30, 0.92f, 0.96f, 1.0f, connected ? header : "Connecting to server...");

    if (connected)
    {
        char score[128] = {};
        std::snprintf(score, sizeof(score), "PLAYER 1   %u        %u   PLAYER 2", snapshot.player1Score, snapshot.player2Score);
        DrawCenteredText(width, 64, 0.95f, 0.98f, 1.0f, score);

        if (snapshot.connectedPlayers == 1)
            DrawCenteredText(width, height - 28, 1.0f, 0.78f, 0.38f, "AI opponent active");

        Net::GamePhase phase = static_cast<Net::GamePhase>(snapshot.phase);
        if (phase == Net::GamePhase::Waiting)
        {
            DrawCenteredText(width, height / 2 - 20, 1.0f, 0.86f, 0.35f, "WAITING FOR PLAYER...");
        }
        else if (phase == Net::GamePhase::GameOver)
        {
            if (snapshot.winner == 1)
                DrawCenteredText(width, height / 2 - 20, 0.18f, 0.82f, 0.90f, "PLAYER 1 WINS!");
            else if (snapshot.winner == 2)
                DrawCenteredText(width, height / 2 - 20, 1.0f, 0.45f, 0.32f, "PLAYER 2 WINS!");
            DrawCenteredText(width, height / 2 + 20, 0.85f, 0.90f, 1.0f, "NEW ROUND STARTS AUTOMATICALLY");
        }
    }
    else
    {
        DrawCenteredText(width, height / 2, 0.95f, 0.30f, 0.30f, "DISCONNECTED FROM SERVER");
    }

    EndTextPass();
    glDisable(GL_BLEND);
    SwapBuffers(hdc);
}
