#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <thread>

#include "../../Common/include/protocol.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#endif

namespace {
using namespace Protocol;

constexpr float kPaddleHalfWidth = 10.0f;
constexpr float kBallRadius = 8.0f;
constexpr float kDigitWidth = 26.0f;
constexpr float kDigitHeight = 48.0f;
constexpr float kDigitThickness = 6.0f;

struct DemoStateFeed {
    GameState state{};
};

GameState CreateInitialDemoState() {
    GameState state{};
    state.status = MatchStatus::InProgress;
    state.players[0].x = 20.0f;
    state.players[1].x = ArenaWidth - 20.0f;
    state.players[0].y = ArenaHeight * 0.5f;
    state.players[1].y = ArenaHeight * 0.5f;
    state.ball.x = ArenaWidth * 0.5f;
    state.ball.y = ArenaHeight * 0.5f;
    state.ball.vx = BallInitialSpeedX;
    state.ball.vy = BallInitialSpeedY;
    return state;
}

void ResetDemoBall(GameState& state, float directionX) {
    state.ball.x = ArenaWidth * 0.5f;
    state.ball.y = ArenaHeight * 0.5f;
    state.ball.vx = directionX * BallInitialSpeedX;
    state.ball.vy = BallInitialSpeedY;
}

void AdvanceDemoState(DemoStateFeed& feed) {
    if (feed.state.status == MatchStatus::GameOver) {
        return;
    }

    ++feed.state.tick;

    const float paddleTrackSpeed = PaddleSpeed * 0.75f;
    if (feed.state.ball.y < feed.state.players[0].y) {
        feed.state.players[0].y -= paddleTrackSpeed;
    } else {
        feed.state.players[0].y += paddleTrackSpeed;
    }

    if (feed.state.ball.y > feed.state.players[1].y) {
        feed.state.players[1].y += paddleTrackSpeed;
    } else {
        feed.state.players[1].y -= paddleTrackSpeed;
    }

    feed.state.players[0].y = std::clamp(feed.state.players[0].y, PaddleHalfHeight, ArenaHeight - PaddleHalfHeight);
    feed.state.players[1].y = std::clamp(feed.state.players[1].y, PaddleHalfHeight, ArenaHeight - PaddleHalfHeight);

    feed.state.ball.x += feed.state.ball.vx;
    feed.state.ball.y += feed.state.ball.vy;

    if (feed.state.ball.y <= kBallRadius || feed.state.ball.y >= ArenaHeight - kBallRadius) {
        feed.state.ball.vy *= -1.0f;
        feed.state.ball.y = std::clamp(feed.state.ball.y, kBallRadius, ArenaHeight - kBallRadius);
    }

    const auto paddleCollision = [&](std::size_t index) {
        const float paddleLeft = feed.state.players[index].x - kPaddleHalfWidth;
        const float paddleRight = feed.state.players[index].x + kPaddleHalfWidth;
        const float paddleTop = feed.state.players[index].y - PaddleHalfHeight;
        const float paddleBottom = feed.state.players[index].y + PaddleHalfHeight;
        return feed.state.ball.x + kBallRadius >= paddleLeft &&
               feed.state.ball.x - kBallRadius <= paddleRight &&
               feed.state.ball.y + kBallRadius >= paddleTop &&
               feed.state.ball.y - kBallRadius <= paddleBottom;
    };

    if (feed.state.ball.vx < 0.0f && paddleCollision(0)) {
        feed.state.ball.vx = std::abs(feed.state.ball.vx);
        feed.state.ball.x = feed.state.players[0].x + kPaddleHalfWidth + kBallRadius;
    } else if (feed.state.ball.vx > 0.0f && paddleCollision(1)) {
        feed.state.ball.vx = -std::abs(feed.state.ball.vx);
        feed.state.ball.x = feed.state.players[1].x - kPaddleHalfWidth - kBallRadius;
    }

    if (feed.state.ball.x + kBallRadius < 0.0f) {
        ++feed.state.score[1];
        ResetDemoBall(feed.state, 1.0f);
    } else if (feed.state.ball.x - kBallRadius > ArenaWidth) {
        ++feed.state.score[0];
        ResetDemoBall(feed.state, -1.0f);
    }

    if (feed.state.score[0] >= WinningScore || feed.state.score[1] >= WinningScore) {
        feed.state.status = MatchStatus::GameOver;
    }
}

[[maybe_unused]] Packet BuildInputPacket(uint32_t sequence) {
    Packet packet{};
    packet.header.type = PacketType::Input;
    packet.header.seq = sequence;

#ifdef _WIN32
    packet.payload.input.up = (GetAsyncKeyState('W') & 0x8000) ? 1 : 0;
    packet.payload.input.down = (GetAsyncKeyState('S') & 0x8000) ? 1 : 0;
    packet.payload.input.left = (GetAsyncKeyState(VK_UP) & 0x8000) ? 1 : 0;
    packet.payload.input.right = (GetAsyncKeyState(VK_DOWN) & 0x8000) ? 1 : 0;
#endif

    return packet;
}

[[maybe_unused]] void SendPacketToServer(const Packet& packet) {
    (void)packet;
}

[[maybe_unused]] bool ReceiveFromServer(Packet& packet) {
    static DemoStateFeed demoFeed{CreateInitialDemoState()};

    packet = {};
    packet.header.type = PacketType::State;
    packet.header.seq = demoFeed.state.tick;
    packet.payload.state = demoFeed.state;
    AdvanceDemoState(demoFeed);
    return true;
}

#ifdef _WIN32
class OpenGLRenderer {
public:
    bool Create() {
        WNDCLASSA wc{};
        wc.style = CS_OWNDC;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandleA(nullptr);
        wc.lpszClassName = "MultiplayerPongClientWindow";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

        if (!RegisterClassA(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            return false;
        }

        window_ = CreateWindowExA(
            0,
            wc.lpszClassName,
            "Multiplayer Pong Client - OpenGL",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            1024,
            768,
            nullptr,
            nullptr,
            wc.hInstance,
            nullptr
        );

        if (window_ == nullptr) {
            return false;
        }

        dc_ = GetDC(window_);
        if (dc_ == nullptr) {
            return false;
        }

        PIXELFORMATDESCRIPTOR pfd{};
        pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.cDepthBits = 24;
        pfd.iLayerType = PFD_MAIN_PLANE;

        const int pixelFormat = ChoosePixelFormat(dc_, &pfd);
        if (pixelFormat == 0 || !SetPixelFormat(dc_, pixelFormat, &pfd)) {
            return false;
        }

        glContext_ = wglCreateContext(dc_);
        if (glContext_ == nullptr || !wglMakeCurrent(dc_, glContext_)) {
            return false;
        }

        return true;
    }

    bool ProcessEvents() {
        MSG msg{};
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                return false;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return true;
    }

    void Render(const GameState& state) {
        RECT rect{};
        GetClientRect(window_, &rect);
        const float width = static_cast<float>(rect.right - rect.left);
        const float height = static_cast<float>(rect.bottom - rect.top);
        if (width <= 0.0f || height <= 0.0f) {
            return;
        }

        glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
        glClearColor(0.02f, 0.05f, 0.09f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0, ArenaWidth, ArenaHeight, 0.0, -1.0, 1.0);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        DrawCourt();
        DrawPaddle(state.players[0]);
        DrawPaddle(state.players[1]);
        DrawBall(state.ball);
        DrawScore(state.score[0], state.score[1]);

        SwapBuffers(dc_);
    }

    ~OpenGLRenderer() {
        if (glContext_ != nullptr) {
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(glContext_);
        }
        if (dc_ != nullptr && window_ != nullptr) {
            ReleaseDC(window_, dc_);
        }
        if (window_ != nullptr) {
            DestroyWindow(window_);
        }
    }

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (msg == WM_CLOSE) {
            DestroyWindow(hwnd);
            return 0;
        }
        if (msg == WM_DESTROY) {
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    static void DrawRect(float left, float top, float right, float bottom) {
        glBegin(GL_QUADS);
        glVertex2f(left, top);
        glVertex2f(right, top);
        glVertex2f(right, bottom);
        glVertex2f(left, bottom);
        glEnd();
    }

    static void DrawCourt() {
        glColor3f(0.92f, 0.94f, 0.96f);
        glLineWidth(2.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(4.0f, 4.0f);
        glVertex2f(ArenaWidth - 4.0f, 4.0f);
        glVertex2f(ArenaWidth - 4.0f, ArenaHeight - 4.0f);
        glVertex2f(4.0f, ArenaHeight - 4.0f);
        glEnd();

        glBegin(GL_LINES);
        glVertex2f(ArenaWidth * 0.5f, 10.0f);
        glVertex2f(ArenaWidth * 0.5f, ArenaHeight - 10.0f);
        glEnd();
    }

    static void DrawPaddle(const PlayerData& paddle) {
        glColor3f(0.91f, 0.95f, 0.98f);
        DrawRect(
            paddle.x - kPaddleHalfWidth,
            paddle.y - PaddleHalfHeight,
            paddle.x + kPaddleHalfWidth,
            paddle.y + PaddleHalfHeight
        );
    }

    static void DrawBall(const BallData& ball) {
        glColor3f(1.0f, 0.78f, 0.18f);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(ball.x, ball.y);
        for (int i = 0; i <= 24; ++i) {
            const float angle = static_cast<float>(i) * 0.261799f;
            glVertex2f(ball.x + std::cos(angle) * kBallRadius, ball.y + std::sin(angle) * kBallRadius);
        }
        glEnd();
    }

    static void DrawSegment(float x, float y, float width, float height) {
        DrawRect(x, y, x + width, y + height);
    }

    static void DrawDigit(int digit, float x, float y, float scale) {
        static constexpr std::array<std::array<bool, 7>, 10> kSegments = {{
            {{true, true, true, false, true, true, true}},
            {{false, false, true, false, false, true, false}},
            {{true, false, true, true, true, false, true}},
            {{true, false, true, true, false, true, true}},
            {{false, true, true, true, false, true, false}},
            {{true, true, false, true, false, true, true}},
            {{true, true, false, true, true, true, true}},
            {{true, false, true, false, false, true, false}},
            {{true, true, true, true, true, true, true}},
            {{true, true, true, true, false, true, true}},
        }};

        if (digit < 0 || digit > 9) {
            return;
        }

        const float w = kDigitWidth * scale;
        const float h = kDigitHeight * scale;
        const float t = kDigitThickness * scale;
        const float middleY = y + (h * 0.5f) - (t * 0.5f);
        const float bottomY = y + h - t;

        const auto& seg = kSegments[static_cast<std::size_t>(digit)];
        if (seg[0]) DrawSegment(x + t, y, w - (2.0f * t), t);
        if (seg[1]) DrawSegment(x, y + t, t, (h * 0.5f) - t);
        if (seg[2]) DrawSegment(x + w - t, y + t, t, (h * 0.5f) - t);
        if (seg[3]) DrawSegment(x + t, middleY, w - (2.0f * t), t);
        if (seg[4]) DrawSegment(x, middleY + t, t, (h * 0.5f) - t);
        if (seg[5]) DrawSegment(x + w - t, middleY + t, t, (h * 0.5f) - t);
        if (seg[6]) DrawSegment(x + t, bottomY, w - (2.0f * t), t);
    }

    static void DrawScore(uint16_t leftScore, uint16_t rightScore) {
        glColor3f(0.45f, 0.94f, 0.88f);
        const float y = 20.0f;
        DrawDigit(leftScore % 10, (ArenaWidth * 0.5f) - 90.0f, y, 1.0f);
        DrawDigit(rightScore % 10, (ArenaWidth * 0.5f) + 64.0f, y, 1.0f);
        DrawRect((ArenaWidth * 0.5f) - 8.0f, y + 16.0f, (ArenaWidth * 0.5f) + 8.0f, y + 22.0f);
        DrawRect((ArenaWidth * 0.5f) - 8.0f, y + 30.0f, (ArenaWidth * 0.5f) + 8.0f, y + 36.0f);
    }

    HWND window_ = nullptr;
    HDC dc_ = nullptr;
    HGLRC glContext_ = nullptr;
};
#endif
}

int main() {
#ifdef _WIN32
    OpenGLRenderer renderer{};
    if (!renderer.Create()) {
        std::cerr << "Failed to initialize OpenGL window." << std::endl;
        return 1;
    }

    uint32_t nextClientSequence = 0;
    GameState latestState = CreateInitialDemoState();

    while (renderer.ProcessEvents()) {
        SendPacketToServer(BuildInputPacket(nextClientSequence++));

        Packet incomingState{};
        if (ReceiveFromServer(incomingState) && incomingState.header.type == PacketType::State) {
            latestState = incomingState.payload.state;
        }

        renderer.Render(latestState);
        std::this_thread::sleep_for(std::chrono::milliseconds(FrameTimeMs));
    }

    return 0;
#else
    std::cerr << "OpenGL client renderer requires a Windows build environment." << std::endl;
    return 0;
#endif
}
