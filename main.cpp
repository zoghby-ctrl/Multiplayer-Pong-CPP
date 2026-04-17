#include <iostream>
#include <chrono>
#include <thread>
#include <cstdlib>

#ifdef _WIN32
#include <conio.h>
#else
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <cstdio>
#endif

#include "DebugOverlay.h"

using namespace std;
using namespace std::chrono;

#ifndef _WIN32
namespace {
termios g_originalTermios{};
int g_originalFlags = 0;
bool g_terminalConfigured = false;

void RestoreTerminal() {
    if (!g_terminalConfigured) {
        return;
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &g_originalTermios);
    fcntl(STDIN_FILENO, F_SETFL, g_originalFlags);
}

bool TryReadKey(char& key) {
    if (!g_terminalConfigured) {
        if (tcgetattr(STDIN_FILENO, &g_originalTermios) != 0) {
            return false;
        }

        termios raw = g_originalTermios;
        raw.c_lflag &= static_cast<unsigned>(~(ICANON | ECHO));
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);

        g_originalFlags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, g_originalFlags | O_NONBLOCK);
        std::atexit(RestoreTerminal);
        g_terminalConfigured = true;
    }

    const int value = std::getchar();
    if (value == EOF) {
        return false;
    }

    key = static_cast<char>(value);
    return true;
}
}
#endif

int main() {
    DebugOverlay debug;

    float fps = 0.0f;
    int frameCount = 0;
    float timer = 0.0f;

    int ping = 50; // mock ping
    int snapshotId = 0;
    string lastInput = "None";

    auto lastTime = high_resolution_clock::now();

    while (true) {
        // حساب الزمن بين الفريمات
        auto currentTime = high_resolution_clock::now();
        float deltaTime = duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        // FPS
        frameCount++;
        timer += deltaTime;

        if (timer >= 1.0f) {
            fps = frameCount / timer;
            frameCount = 0;
            timer = 0.0f;
        }

        // Snapshot update
        snapshotId++;

        // Input handling
        char key = '\0';
        bool hasKey = false;
#ifdef _WIN32
        if (_kbhit()) {
            hasKey = true;
            key = static_cast<char>(_getch());
        }
#else
        hasKey = TryReadKey(key);
#endif

        if (hasKey) {
            if (key == 'q') break;

#ifdef _WIN32
            if (key == 0 || key == -32) {
                key = static_cast<char>(_getch()); // special keys
                if (key == 61) { // F3 key
                    debug.Toggle();
                }
            }
            else {
                lastInput = string(1, key);
            }
#else
            if (key == '\x1b') {
                debug.Toggle();
            } else {
                lastInput = string(1, key);
            }
#endif
        }

        // رسم الـ overlay
        debug.Draw(fps, ping, snapshotId, lastInput);

        // simulate frame time
        this_thread::sleep_for(milliseconds(16)); // ~60 FPS
    }

    return 0;
}
