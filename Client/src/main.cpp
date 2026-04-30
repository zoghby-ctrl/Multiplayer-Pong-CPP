#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include "../GameClient.h"

#include <windows.h>
#include <GL/gl.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#pragma comment(lib, "opengl32.lib")

namespace {

constexpr int kWindowWidth = 1120;
constexpr int kWindowHeight = 840;
constexpr int kTrailLength = 18;
constexpr int kCircleSegments = 40;
constexpr float kPi = 3.14159265358979323846f;

constexpr UINT kMenuReset = 1001;
constexpr UINT kMenuDifficultyEasy = 1101;
constexpr UINT kMenuDifficultyNormal = 1102;
constexpr UINT kMenuDifficultyHard = 1103;

struct TrailPoint {
    float x = Protocol::ArenaWidth * 0.5f;
    float y = Protocol::ArenaHeight * 0.5f;
};

class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual bool Create() = 0;
    virtual bool ProcessEvents() = 0;
    virtual void Render(const GameClient& client) = 0;
};

class OpenGLRenderer final : public IRenderer {
public:
    bool Create() override {
        WNDCLASSA wc{};
        wc.style = CS_OWNDC;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandleA(nullptr);
        wc.lpszClassName = "MultiplayerPongOpenGLWindow";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

        if (!RegisterClassA(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
            return false;
        }

        window_ = CreateWindowExA(
            0,
            wc.lpszClassName,
            "Multiplayer Pong",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            kWindowWidth,
            kWindowHeight,
            nullptr,
            nullptr,
            wc.hInstance,
            nullptr
        );

        if (!window_) {
            return false;
        }

        SetWindowLongPtrA(window_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        SetMenu(window_, CreateMenuBar());

        dc_ = GetDC(window_);
        if (!dc_) {
            return false;
        }

        PIXELFORMATDESCRIPTOR pfd{};
        pfd.nSize = sizeof(pfd);
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
        if (!glContext_ || !wglMakeCurrent(dc_, glContext_)) {
            return false;
        }

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        return true;
    }

    bool ConsumeResetRequest() {
        const bool requested = resetRequested_;
        resetRequested_ = false;
        return requested;
    }

    bool ConsumeDifficultyRequest(Protocol::Difficulty& difficulty) {
        if (!difficultyRequested_) {
            return false;
        }

        difficulty = requestedDifficulty_;
        difficultyRequested_ = false;
        return true;
    }

    bool ProcessEvents() override {
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

    void Render(const GameClient& client) override {
        const Protocol::GameState& state = client.GetDisplayState();

        UpdateTrail(state);
        UpdateTitle(client);

        ApplyLetterboxViewport();
        glClearColor(0.005f, 0.009f, 0.018f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0, Protocol::ArenaWidth, Protocol::ArenaHeight, 0.0, -1.0, 1.0);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        DrawBackground();
        DrawCourt();
        DrawScore(state);
        DrawPaddle(state.players[0], client.GetLocalPlayerId() == 0);
        DrawPaddle(state.players[1], client.GetLocalPlayerId() == 1);
        DrawBallTrail();
        DrawBall(state.ball);
        DrawDifficultyIndicator(state.difficulty);

        if (state.status == Protocol::MatchStatus::WaitingForPlayers) {
            DrawWaitingPulse();
        }

        SwapBuffers(dc_);
    }

    ~OpenGLRenderer() override {
        if (glContext_) {
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(glContext_);
        }

        if (dc_ && window_) {
            ReleaseDC(window_, dc_);
        }

        if (window_) {
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

        if (msg == WM_COMMAND) {
            auto* renderer = reinterpret_cast<OpenGLRenderer*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
            if (renderer) {
                renderer->HandleMenuCommand(LOWORD(wParam));
            }
            return 0;
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    static HMENU CreateMenuBar() {
        HMENU menuBar = CreateMenu();
        HMENU gameMenu = CreatePopupMenu();
        HMENU difficultyMenu = CreatePopupMenu();

        AppendMenuA(gameMenu, MF_STRING, kMenuReset, "&Reset\tR");
        AppendMenuA(difficultyMenu, MF_STRING, kMenuDifficultyEasy, "&Easy\t1");
        AppendMenuA(difficultyMenu, MF_STRING, kMenuDifficultyNormal, "&Normal\t2");
        AppendMenuA(difficultyMenu, MF_STRING, kMenuDifficultyHard, "&Hard\t3");

        AppendMenuA(menuBar, MF_POPUP, reinterpret_cast<UINT_PTR>(gameMenu), "&Game");
        AppendMenuA(menuBar, MF_POPUP, reinterpret_cast<UINT_PTR>(difficultyMenu), "&Difficulty");
        return menuBar;
    }

    void HandleMenuCommand(UINT command) {
        switch (command) {
            case kMenuReset:
                resetRequested_ = true;
                break;
            case kMenuDifficultyEasy:
                QueueDifficulty(Protocol::Difficulty::Easy);
                break;
            case kMenuDifficultyNormal:
                QueueDifficulty(Protocol::Difficulty::Normal);
                break;
            case kMenuDifficultyHard:
                QueueDifficulty(Protocol::Difficulty::Hard);
                break;
            default:
                break;
        }
    }

    void QueueDifficulty(Protocol::Difficulty difficulty) {
        requestedDifficulty_ = difficulty;
        difficultyRequested_ = true;
        UpdateMenuCheck(difficulty);
    }

    void UpdateMenuCheck(Protocol::Difficulty difficulty) const {
        HMENU menu = GetMenu(window_);
        if (!menu) {
            return;
        }

        UINT id = kMenuDifficultyNormal;
        if (difficulty == Protocol::Difficulty::Easy) {
            id = kMenuDifficultyEasy;
        } else if (difficulty == Protocol::Difficulty::Hard) {
            id = kMenuDifficultyHard;
        }

        CheckMenuRadioItem(
            menu,
            kMenuDifficultyEasy,
            kMenuDifficultyHard,
            id,
            MF_BYCOMMAND
        );
    }

    void ApplyLetterboxViewport() const {
        RECT rect{};
        GetClientRect(window_, &rect);
        const int width = std::max(1, static_cast<int>(rect.right - rect.left));
        const int height = std::max(1, static_cast<int>(rect.bottom - rect.top));
        const float targetAspect = Protocol::ArenaWidth / Protocol::ArenaHeight;
        const float currentAspect = static_cast<float>(width) / static_cast<float>(height);

        int viewportWidth = width;
        int viewportHeight = height;
        int viewportX = 0;
        int viewportY = 0;

        if (currentAspect > targetAspect) {
            viewportWidth = static_cast<int>(static_cast<float>(height) * targetAspect);
            viewportX = (width - viewportWidth) / 2;
        } else {
            viewportHeight = static_cast<int>(static_cast<float>(width) / targetAspect);
            viewportY = (height - viewportHeight) / 2;
        }

        glViewport(viewportX, viewportY, viewportWidth, viewportHeight);
    }

    void UpdateTitle(const GameClient& client) const {
        std::ostringstream title;
        const Protocol::GameState& state = client.GetDisplayState();
        UpdateMenuCheck(state.difficulty);

        title << "Multiplayer Pong - Player " << static_cast<int>(client.GetLocalPlayerId() + 1)
              << " - " << state.score[0] << ":" << state.score[1]
              << " - " << DifficultyName(state.difficulty);

        if (state.status == Protocol::MatchStatus::WaitingForPlayers) {
            title << " - waiting for player 1";
        } else if (state.status == Protocol::MatchStatus::GameOver) {
            title << " - game over";
        }

        SetWindowTextA(window_, title.str().c_str());
    }

    static const char* DifficultyName(Protocol::Difficulty difficulty) {
        switch (difficulty) {
            case Protocol::Difficulty::Easy:
                return "Easy";
            case Protocol::Difficulty::Hard:
                return "Hard";
            case Protocol::Difficulty::Normal:
            default:
                return "Normal";
        }
    }

    void UpdateTrail(const Protocol::GameState& state) {
        if (state.tick == lastTrailTick_) {
            return;
        }

        lastTrailTick_ = state.tick;
        for (std::size_t i = trail_.size() - 1; i > 0; --i) {
            trail_[i] = trail_[i - 1];
        }

        trail_[0] = {state.ball.x, state.ball.y};
    }

    static void DrawRect(float left, float top, float right, float bottom) {
        glBegin(GL_QUADS);
        glVertex2f(left, top);
        glVertex2f(right, top);
        glVertex2f(right, bottom);
        glVertex2f(left, bottom);
        glEnd();
    }

    static void DrawCircle(float x, float y, float radius) {
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(x, y);
        for (int i = 0; i <= kCircleSegments; ++i) {
            const float angle = (static_cast<float>(i) / static_cast<float>(kCircleSegments)) * 2.0f * kPi;
            glVertex2f(x + std::cos(angle) * radius, y + std::sin(angle) * radius);
        }
        glEnd();
    }

    static void DrawBackground() {
        glBegin(GL_QUADS);
        glColor4f(0.02f, 0.05f, 0.09f, 1.0f);
        glVertex2f(0.0f, 0.0f);
        glVertex2f(Protocol::ArenaWidth, 0.0f);
        glColor4f(0.0f, 0.14f, 0.16f, 1.0f);
        glVertex2f(Protocol::ArenaWidth, Protocol::ArenaHeight);
        glVertex2f(0.0f, Protocol::ArenaHeight);
        glEnd();

        glColor4f(0.03f, 0.34f, 0.36f, 0.18f);
        for (int i = 0; i < 8; ++i) {
            const float y = 78.0f + static_cast<float>(i) * 58.0f;
            DrawRect(0.0f, y, Protocol::ArenaWidth, y + 1.5f);
        }
    }

    static void DrawCourt() {
        glLineWidth(3.0f);
        glColor4f(0.78f, 0.95f, 0.92f, 0.85f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(8.0f, 8.0f);
        glVertex2f(Protocol::ArenaWidth - 8.0f, 8.0f);
        glVertex2f(Protocol::ArenaWidth - 8.0f, Protocol::ArenaHeight - 8.0f);
        glVertex2f(8.0f, Protocol::ArenaHeight - 8.0f);
        glEnd();

        glColor4f(0.55f, 0.92f, 0.88f, 0.45f);
        for (float y = 18.0f; y < Protocol::ArenaHeight - 18.0f; y += 32.0f) {
            DrawRect(
                Protocol::ArenaWidth * 0.5f - 2.0f,
                y,
                Protocol::ArenaWidth * 0.5f + 2.0f,
                y + 16.0f
            );
        }
    }

    static void DrawPaddle(const Protocol::PlayerData& paddle, bool localPlayer) {
        const float glow = localPlayer ? 0.32f : 0.18f;
        glColor4f(0.24f, 0.92f, 0.86f, glow);
        DrawRect(
            paddle.x - Protocol::PaddleHalfWidth - 6.0f,
            paddle.y - Protocol::PaddleHalfHeight - 8.0f,
            paddle.x + Protocol::PaddleHalfWidth + 6.0f,
            paddle.y + Protocol::PaddleHalfHeight + 8.0f
        );

        glColor4f(localPlayer ? 0.96f : 0.72f, 0.98f, localPlayer ? 0.92f : 0.96f, 1.0f);
        DrawRect(
            paddle.x - Protocol::PaddleHalfWidth,
            paddle.y - Protocol::PaddleHalfHeight,
            paddle.x + Protocol::PaddleHalfWidth,
            paddle.y + Protocol::PaddleHalfHeight
        );
    }

    void DrawBallTrail() const {
        for (std::size_t i = trail_.size(); i-- > 1;) {
            const float t = static_cast<float>(i) / static_cast<float>(trail_.size());
            glColor4f(1.0f, 0.52f, 0.20f, 0.05f + (0.18f * (1.0f - t)));
            DrawCircle(trail_[i].x, trail_[i].y, Protocol::BallRadius * (1.0f + t * 0.65f));
        }
    }

    static void DrawBall(const Protocol::BallData& ball) {
        glColor4f(1.0f, 0.70f, 0.20f, 0.25f);
        DrawCircle(ball.x, ball.y, Protocol::BallRadius * 2.1f);
        glColor4f(1.0f, 0.86f, 0.30f, 1.0f);
        DrawCircle(ball.x, ball.y, Protocol::BallRadius);
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

        const float width = 28.0f * scale;
        const float height = 52.0f * scale;
        const float thick = 6.0f * scale;
        const float middleY = y + height * 0.5f - thick * 0.5f;
        const float bottomY = y + height - thick;
        const auto& seg = kSegments[static_cast<std::size_t>(std::clamp(digit, 0, 9))];

        if (seg[0]) DrawSegment(x + thick, y, width - 2.0f * thick, thick);
        if (seg[1]) DrawSegment(x, y + thick, thick, height * 0.5f - thick);
        if (seg[2]) DrawSegment(x + width - thick, y + thick, thick, height * 0.5f - thick);
        if (seg[3]) DrawSegment(x + thick, middleY, width - 2.0f * thick, thick);
        if (seg[4]) DrawSegment(x, middleY + thick, thick, height * 0.5f - thick);
        if (seg[5]) DrawSegment(x + width - thick, middleY + thick, thick, height * 0.5f - thick);
        if (seg[6]) DrawSegment(x + thick, bottomY, width - 2.0f * thick, thick);
    }

    static void DrawScoreValue(uint16_t value, float centerX) {
        value = std::min<uint16_t>(value, 99);
        glColor4f(0.76f, 1.0f, 0.93f, 0.9f);

        if (value >= 10) {
            DrawDigit((value / 10) % 10, centerX - 36.0f, 24.0f, 1.0f);
            DrawDigit(value % 10, centerX + 4.0f, 24.0f, 1.0f);
        } else {
            DrawDigit(value, centerX - 14.0f, 24.0f, 1.0f);
        }
    }

    static void DrawScore(const Protocol::GameState& state) {
        DrawScoreValue(state.score[0], Protocol::ArenaWidth * 0.5f - 92.0f);
        DrawScoreValue(state.score[1], Protocol::ArenaWidth * 0.5f + 92.0f);

        glColor4f(1.0f, 0.70f, 0.22f, 0.9f);
        DrawRect(Protocol::ArenaWidth * 0.5f - 8.0f, 42.0f, Protocol::ArenaWidth * 0.5f + 8.0f, 49.0f);
        DrawRect(Protocol::ArenaWidth * 0.5f - 8.0f, 62.0f, Protocol::ArenaWidth * 0.5f + 8.0f, 69.0f);
    }

    static int DifficultyIndex(Protocol::Difficulty difficulty) {
        switch (difficulty) {
            case Protocol::Difficulty::Easy:
                return 0;
            case Protocol::Difficulty::Hard:
                return 2;
            case Protocol::Difficulty::Normal:
            default:
                return 1;
        }
    }

    static void DrawDifficultyIndicator(Protocol::Difficulty difficulty) {
        const int activeIndex = DifficultyIndex(difficulty);
        const float x = Protocol::ArenaWidth - 96.0f;
        const float y = 30.0f;

        for (int i = 0; i < 3; ++i) {
            const float height = 14.0f + static_cast<float>(i) * 10.0f;
            const float left = x + static_cast<float>(i) * 24.0f;
            const float top = y + (34.0f - height);

            if (i == activeIndex) {
                glColor4f(1.0f, 0.70f, 0.22f, 0.95f);
            } else {
                glColor4f(0.46f, 0.75f, 0.78f, 0.35f);
            }

            DrawRect(left, top, left + 14.0f, y + 34.0f);
        }
    }

    static void DrawWaitingPulse() {
        const float seconds = static_cast<float>(GetTickCount64() % 1600) / 1600.0f;
        const float pulse = 1.0f + std::sin(seconds * 2.0f * kPi) * 0.12f;
        glColor4f(0.30f, 0.95f, 0.88f, 0.22f);
        DrawCircle(Protocol::ArenaWidth * 0.5f, Protocol::ArenaHeight * 0.5f, 52.0f * pulse);
    }

    HWND window_ = nullptr;
    HDC dc_ = nullptr;
    HGLRC glContext_ = nullptr;
    std::array<TrailPoint, kTrailLength> trail_{};
    uint32_t lastTrailTick_ = UINT32_MAX;
    bool resetRequested_ = false;
    bool difficultyRequested_ = false;
    Protocol::Difficulty requestedDifficulty_ = Protocol::Difficulty::Normal;
};

Protocol::PlayerInput ReadKeyboardInput() {
    Protocol::PlayerInput input{};
    input.up = ((GetAsyncKeyState('W') & 0x8000) || (GetAsyncKeyState(VK_UP) & 0x8000)) ? 1 : 0;
    input.down = ((GetAsyncKeyState('S') & 0x8000) || (GetAsyncKeyState(VK_DOWN) & 0x8000)) ? 1 : 0;
    return input;
}

bool WasKeyPressed(int virtualKey) {
    static std::array<bool, 256> wasDown{};
    const bool isDown = (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
    const bool pressed = isDown && !wasDown[static_cast<std::size_t>(virtualKey)];
    wasDown[static_cast<std::size_t>(virtualKey)] = isDown;
    return pressed;
}

}

int main() {
    GameClient client;
    if (!client.Connect()) {
        std::cout << "Could not connect to server: " << client.LastError() << "\n";
        std::cout << "Start Server.exe first, then run Client.exe.\n";
        return 1;
    }

    OpenGLRenderer renderer;
    if (!renderer.Create()) {
        std::cout << "Failed to create OpenGL window.\n";
        return 1;
    }

    while (renderer.ProcessEvents() && client.IsConnected()) {
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            break;
        }

        if (WasKeyPressed('R') || renderer.ConsumeResetRequest()) {
            client.RequestReset();
        }

        Protocol::Difficulty requestedDifficulty{};
        if (WasKeyPressed('1')) {
            client.RequestDifficulty(Protocol::Difficulty::Easy);
        } else if (WasKeyPressed('2')) {
            client.RequestDifficulty(Protocol::Difficulty::Normal);
        } else if (WasKeyPressed('3')) {
            client.RequestDifficulty(Protocol::Difficulty::Hard);
        } else if (renderer.ConsumeDifficultyRequest(requestedDifficulty)) {
            client.RequestDifficulty(requestedDifficulty);
        }

        client.Update(ReadKeyboardInput());
        renderer.Render(client);
        std::this_thread::sleep_for(std::chrono::milliseconds(Protocol::FrameTimeMs));
    }

    client.Disconnect();
    return 0;
}
