#pragma once
#include <string>

class DebugOverlay {
private:
    bool isVisible;

public:
    DebugOverlay();

    void Toggle();
    bool IsVisible() const;

    void Draw(float fps, int ping, int snapshotId, const std::string& input);
};