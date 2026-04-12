#pragma once
#include <string>
#include <vector>

struct DebugData {
    float fps;
    int ping;
    uint32_t lastSnapshotId;
    std::string activeInputs;
};

class DebugManager {
public:
    DebugManager() : m_visible(false) {}
    
    void Toggle() { m_visible = !m_visible; }
    bool IsVisible() const { return m_visible; }

    // Call this inside your main render loop
    void Draw(const DebugData& data);

private:
    bool m_visible;
};
 