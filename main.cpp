#include <iostream>
#include <chrono>
#include <thread>
#include <conio.h> // for _kbhit and _getch
#include "DebugOverlay.h"

using namespace std;
using namespace std::chrono;

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
        if (_kbhit()) {
            char key = _getch();

            if (key == 'q') break;

            if (key == 0 || key == -32) {
                key = _getch(); // special keys
                if (key == 61) { // F3 key
                    debug.Toggle();
                }
            } else {
                lastInput = string(1, key);
            }
        }

        // رسم الـ overlay
        debug.Draw(fps, ping, snapshotId, lastInput);

        // simulate frame time
        this_thread::sleep_for(milliseconds(16)); // ~60 FPS
    }

    return 0;
}