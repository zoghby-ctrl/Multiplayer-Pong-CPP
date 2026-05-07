#include <algorithm>
#include <string>

#include "NetClient.h"
#include "Platform.h"
#include "Renderer.h"

#pragma comment(lib, "opengl32.lib")

static bool g_running = true;
static bool g_keys[256] = {};
static int g_width = 1280;
static int g_height = 720;
static PlatformContext g_platform;
static NetClient g_client;
static constexpr DWORD kClientFrameSleepMs = 8;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CLOSE:
        g_running = false;
        PostQuitMessage(0);
        return 0;
    case WM_SIZE:
        g_width = LOWORD(lParam);
        g_height = HIWORD(lParam);
        return 0;
    case WM_KEYDOWN:
        if (wParam < 256)
            g_keys[wParam] = true;
        if (wParam == VK_ESCAPE)
        {
            g_running = false;
            PostQuitMessage(0);
        }
        return 0;
    case WM_KEYUP:
        if (wParam < 256)
            g_keys[wParam] = false;
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE /*hPrevInstance*/,
    _In_ LPSTR lpCmdLine,
    _In_ int /*nShowCmd*/
)
{
    const char* hostIp = (lpCmdLine && lpCmdLine[0] != '\0') ? lpCmdLine : "127.0.0.1";

    if (!Platform_CreateOpenGLWindow(hInstance, 1280, 720, "LAN Multiplayer Pong Client", WindowProc, g_platform))
    {
        MessageBoxA(nullptr, "Failed to create OpenGL client window.", "Startup Error", MB_ICONERROR | MB_OK);
        return -1;
    }

    if (!Renderer_Initialize(g_platform.hdc))
    {
        MessageBoxA(nullptr, "Failed to initialize renderer.", "Startup Error", MB_ICONERROR | MB_OK);
        Platform_Cleanup(g_platform);
        return -1;
    }

    if (!g_client.Connect(hostIp, Net::kDefaultPort))
    {
        MessageBoxA(nullptr, "Failed to connect to server. Run Client.exe <server-ip> or use 127.0.0.1 for local test.", "Connection Error", MB_ICONERROR | MB_OK);
        Renderer_Shutdown();
        Platform_Cleanup(g_platform);
        return -1;
    }

    MSG msg = {};
    while (g_running)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        const int playerId = g_client.GetPlayerId();
        const bool moveUp = (playerId == 1) ? g_keys['W'] : g_keys[VK_UP];
        const bool moveDown = (playerId == 1) ? g_keys['S'] : g_keys[VK_DOWN];
        g_client.SendInput(moveUp, moveDown);

        Net::StateSnapshotPacket snapshot = g_client.GetLatestSnapshot();
        Renderer_Render(snapshot, g_client.IsConnected(), g_client.GetPlayerId(), g_client.GetHostString(), g_width, g_height, g_platform.hdc);

        if (!g_client.IsConnected())
            g_running = false;

        Sleep(kClientFrameSleepMs);
    }

    g_client.Disconnect();
    Renderer_Shutdown();
    Platform_Cleanup(g_platform);
    return 0;
}
