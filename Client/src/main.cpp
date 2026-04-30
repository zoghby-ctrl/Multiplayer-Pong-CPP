#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h> // for GetAsyncKeyState
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <thread>

#include "../../Common/include/protocol.h"

#ifdef _WIN32
#include <GL/gl.h>
#endif

#pragma comment(lib, "ws2_32.lib")

namespace {
using namespace Protocol;

constexpr float kPaddleHalfWidth = 10.0f;
constexpr float kBallRadius = 8.0f;
constexpr float kDigitWidth = 26.0f;
constexpr float kDigitHeight = 48.0f;
constexpr float kDigitThickness = 6.0f;
constexpr int kBallCircleSegments = 24;
constexpr float kPi = 3.14159265358979323846f;
constexpr float kBallCircleSegmentAngle = (2.0f * kPi) / static_cast<float>(kBallCircleSegments);

// TCP server socket — set in main() after connect()
SOCKET g_serverSocket = INVALID_SOCKET;
bool g_serverDisconnected = false;

GameState CreateInitialState() {
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

// Build an input packet from the current keyboard state.
// W/S   controls player 1 (up/down)
// ↑/↓   controls player 2 (left/right fields, for second paddle)
Packet BuildInputPacket(uint32_t sequence) {
    Packet packet{};
    packet.header.type = PacketType::Input;
    packet.header.seq = sequence;
    packet.payload.input.up    = (GetAsyncKeyState('W')    & 0x8000) ? 1 : 0;
    packet.payload.input.down  = (GetAsyncKeyState('S')    & 0x8000) ? 1 : 0;
    packet.payload.input.left  = (GetAsyncKeyState(VK_UP)  & 0x8000) ? 1 : 0;
    packet.payload.input.right = (GetAsyncKeyState(VK_DOWN)& 0x8000) ? 1 : 0;
    return packet;
}

// Real TCP send to server.
void SendPacketToServer(const Packet& packet) {
    int bytesSent = send(
        g_serverSocket,
        reinterpret_cast<const char*>(&packet),
        sizeof(packet),
        0
    );
    if (bytesSent == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK) {
            std::cout << "send failed. Error: " << error << "\n";
            g_serverDisconnected = true;
        }
    }
}

// Real TCP receive from server.
bool ReceiveFromServer(Packet& packet) {
    packet = {};
    int bytesReceived = recv(
        g_serverSocket,
        reinterpret_cast<char*>(&packet),
        sizeof(packet),
        0
    );
    if (bytesReceived == static_cast<int>(sizeof(packet))) {
        return true;
    }
    if (bytesReceived == 0) {
        std::cout << "Server disconnected\n";
        g_serverDisconnected = true;
    } else if (bytesReceived == SOCKET_ERROR) {
        int error = WSAGetLastError();
        // WSAEWOULDBLOCK is normal on a non-blocking socket with no data.
        if (error != WSAEWOULDBLOCK) {
            std::cout << "recv failed. Error: " << error << "\n";
            g_serverDisconnected = true;
        }
    }
    return false;
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
        for (int i = 0; i <= kBallCircleSegments; ++i) {
            const float angle = static_cast<float>(i) * kBallCircleSegmentAngle;
            glVertex2f(ball.x + std::cos(angle) * kBallRadius, ball.y + std::sin(angle) * kBallRadius);
        }
        glEnd();
    }

    static void DrawSegment(float x, float y, float width, float height) {
        DrawRect(x, y, x + width, y + height);
    }

    static void DrawDigit(int digit, float x, float y, float scale) {
        // Segment order: top, top-left, top-right, middle, bottom-left, bottom-right, bottom.
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

    static void DrawScoreValue(uint16_t score, float centerX, float y) {
        score = std::min<uint16_t>(score, 99);
        if (score >= 10) {
            DrawDigit((score / 10) % 10, centerX - kDigitWidth - 6.0f, y, 1.0f);
            DrawDigit(score % 10, centerX + 6.0f, y, 1.0f);
        } else {
            DrawDigit(score, centerX - (kDigitWidth * 0.5f), y, 1.0f);
        }
    }

    static void DrawScore(uint16_t leftScore, uint16_t rightScore) {
        glColor3f(0.45f, 0.94f, 0.88f);
        const float y = 20.0f;
        DrawScoreValue(leftScore, (ArenaWidth * 0.5f) - 86.0f, y);
        DrawScoreValue(rightScore, (ArenaWidth * 0.5f) + 86.0f, y);
        DrawRect((ArenaWidth * 0.5f) - 8.0f, y + 16.0f, (ArenaWidth * 0.5f) + 8.0f, y + 22.0f);
        DrawRect((ArenaWidth * 0.5f) - 8.0f, y + 30.0f, (ArenaWidth * 0.5f) + 8.0f, y + 36.0f);
    }

    HWND window_ = nullptr;
    HDC dc_ = nullptr;
    HGLRC glContext_ = nullptr;
};
#endif
} // namespace

int main() {
    ///////////////////////////////////////////////////////////////////////////
    // TCP NETWORKING SETUP
    // WSAStartup = starts Winsock library so sockets can work on Windows

    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cout << "WSAStartup failed\n";
        return 1;
    }

    // Create TCP socket
    // AF_INET = IPv4, SOCK_STREAM = TCP

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (clientSocket == INVALID_SOCKET) {
        std::cout << "Socket creation failed. Error: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    // Server address: 127.0.0.1 (localhost) — change to remote IP for LAN play.

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(Protocol::DefaultPort);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    std::cout << "Connecting to server on port " << Protocol::DefaultPort << "...\n";

    if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cout << "Connect failed. Error: " << WSAGetLastError() << "\n";
        std::cout << "Make sure Server is running first on port " << Protocol::DefaultPort << "\n";
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to server!\n";

    // Make socket non-blocking so recv/send don't stall the render loop.
    u_long mode = 1;
    ioctlsocket(clientSocket, FIONBIO, &mode);

    g_serverSocket = clientSocket;

    ///////////////////////////////////////////////////////////////////////////
    // RENDER + INPUT LOOP

#ifdef _WIN32
    OpenGLRenderer renderer{};
    if (!renderer.Create()) {
        std::cerr << "Failed to initialize OpenGL window." << std::endl;
        closesocket(g_serverSocket);
        WSACleanup();
        return 1;
    }

    uint32_t nextClientSequence = 0;
    GameState latestState = CreateInitialState();

    // 1) read keyboard & send input every frame          (W/S = player1, ↑/↓ = player2)
    // 2) receive authoritative game state from server
    // 3) render the received state

    while (renderer.ProcessEvents() && !g_serverDisconnected) {
        SendPacketToServer(BuildInputPacket(nextClientSequence++));

        Packet incomingPacket{};
        if (ReceiveFromServer(incomingPacket) && incomingPacket.header.type == PacketType::State) {
            latestState = incomingPacket.payload.state;
        }

        renderer.Render(latestState);
        std::this_thread::sleep_for(std::chrono::milliseconds(FrameTimeMs));
    }
#else
    std::cerr << "OpenGL client renderer requires a Windows build environment." << std::endl;
#endif

    closesocket(g_serverSocket);
    WSACleanup();

    return 0;
}

/*
    W / S        — move player 1 paddle up / down
    Arrow Up/Down — move player 2 paddle up / down
    Close window  — exit
*/
