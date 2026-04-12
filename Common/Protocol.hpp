#pragma once
#include <cstdint>

namespace PongProtocol {
    struct GameSnapshot {
        uint32_t snapshotId; // Every authoritative update gets a sequence number
        float ballX, ballY;
        float leftPaddleY, rightPaddleY;
        int scoreLeft, scoreRight;
    };
}