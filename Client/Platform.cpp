#include "Platform.h"

bool Platform_CreateOpenGLWindow(HINSTANCE instance, int width, int height, const char* title, WNDPROC windowProc, PlatformContext& outContext)
{
    WNDCLASSA wc = {};
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = windowProc;
    wc.hInstance = instance;
    wc.lpszClassName = "CppLanPongClientWindow";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    if (!RegisterClassA(&wc))
        return false;

    DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    RECT rect{ 0, 0, width, height };
    AdjustWindowRect(&rect, style, FALSE);

    outContext.hwnd = CreateWindowA(
        wc.lpszClassName,
        title,
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        instance,
        nullptr);

    if (!outContext.hwnd)
        return false;

    outContext.hdc = GetDC(outContext.hwnd);

    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int pixelFormat = ChoosePixelFormat(outContext.hdc, &pfd);
    if (pixelFormat == 0)
        return false;
    if (!SetPixelFormat(outContext.hdc, pixelFormat, &pfd))
        return false;

    outContext.glrc = wglCreateContext(outContext.hdc);
    if (!outContext.glrc)
        return false;
    if (!wglMakeCurrent(outContext.hdc, outContext.glrc))
        return false;

    ShowWindow(outContext.hwnd, SW_SHOW);
    UpdateWindow(outContext.hwnd);
    return true;
}

void Platform_Cleanup(PlatformContext& context)
{
    if (context.glrc)
    {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(context.glrc);
        context.glrc = nullptr;
    }
    if (context.hdc && context.hwnd)
    {
        ReleaseDC(context.hwnd, context.hdc);
        context.hdc = nullptr;
    }
    if (context.hwnd)
    {
        DestroyWindow(context.hwnd);
        context.hwnd = nullptr;
    }
}
