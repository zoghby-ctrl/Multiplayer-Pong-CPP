#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct PlatformContext
{
    HWND hwnd = nullptr;
    HDC hdc = nullptr;
    HGLRC glrc = nullptr;
};

bool Platform_CreateOpenGLWindow(HINSTANCE instance, int width, int height, const char* title, WNDPROC windowProc, PlatformContext& outContext);
void Platform_Cleanup(PlatformContext& context);
