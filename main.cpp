// Experimental console overlay prototype.
// This file is retained for reference and is not part of MultiplayerPong.sln.

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

    char readChar = '\0';
    const ssize_t bytesRead = read(STDIN_FILENO, &readChar, 1);
    if (bytesRead <= 0) {
        return false;
    }

    key = readChar;
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
        // Track elapsed time between frames.
        auto currentTime = high_resolution_clock::now();
        float deltaTime = duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        // Update FPS once per second.
        frameCount++;
        timer += deltaTime;

        if (timer >= 1.0f) {
            fps = frameCount / timer;
            frameCount = 0;
            timer = 0.0f;
        }

        // Advance a placeholder snapshot counter.
        snapshotId++;

        // Poll input for the standalone overlay prototype.
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
                key = static_cast<char>(_getch()); // Special keys.
                if (key == 61) { // F3 key.
                    debug.Toggle();
                }
            } else {
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

        // Draw the prototype overlay.
        debug.Draw(fps, ping, snapshotId, lastInput);

        // Simulate roughly 60 FPS.
        this_thread::sleep_for(milliseconds(16));
    }

    return 0;
}
