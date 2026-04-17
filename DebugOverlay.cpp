#include "DebugOverlay.h"
#include <iostream>

DebugOverlay::DebugOverlay() {
    isVisible = true;
}

void DebugOverlay::Toggle() {
    isVisible = !isVisible;
}

bool DebugOverlay::IsVisible() const {
    return isVisible;
}

void DebugOverlay::Draw(float fps, int ping, int snapshotId, const std::string& input) {
    if (!isVisible) return;

    std::cout << "\033[2J\033[H";

    std::cout << "===== DEBUG OVERLAY =====" << std::endl;
    std::cout << "FPS: " << fps << std::endl;
    std::cout << "Ping: " << ping << " ms" << std::endl;
    std::cout << "Snapshot ID: " << snapshotId << std::endl;
    std::cout << "Last Input: " << input << std::endl;
    std::cout << "=========================" << std::endl;
}
