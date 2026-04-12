#include "DebugManager.hpp"
#include <iostream>

// Example using pseudo-code for rendering
void DebugManager::Draw(const DebugData& data) {
    if (!m_visible) return;

    // Use your engine's text rendering here (e.g., DrawText)
    std::string overlayText = 
        std::string("--- DEBUG INFO ---\n") +
        "FPS: " + std::to_string(static_cast<int>(data.fps)) + "\n" +
        "Ping: " + std::to_string(data.ping) + "ms\n" +
        "Snapshot ID: #" + std::to_string(data.lastSnapshotId) + "\n" +
        "Inputs: " + data.activeInputs;

    // Render logic (RenderText(overlayText, x, y, color))
    std::cout << overlayText << std::endl; 
}