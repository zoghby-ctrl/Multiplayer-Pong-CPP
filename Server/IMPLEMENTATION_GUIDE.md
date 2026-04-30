# Multiplayer Pong - Server Tick Pacing and State Broadcasting Implementation

## Summary of Changes

This document describes the implementation of server tick pacing, sequence handling, and clean state broadcast cadence to ensure both clients see consistent movement and score updates under normal latency conditions.

## Files Added/Modified

### Server Folder (`c:\Users\USER\OneDrive\Documents\GitHub\Multiplayer-Pong-CPP\Server\`)

#### New Files:
1. **GameServer.h** - Header file for server game logic class
   - Encapsulates physics simulation and state management
   - Methods: Tick(), ProcessClientPacket(), ShouldBroadcastState()
   - Fixed physics constants (60 Hz tick rate)

2. **GameServer.cpp** - Implementation of game server
   - Physics simulation (ball movement, paddle collision)
   - Ball-paddle collision detection with spin calculation
   - Player input processing from both clients
   - Win condition checking

3. **ARCHITECTURE.md** - Comprehensive server architecture documentation
   - Detailed explanation of tick pacing
   - Sequence number handling
   - Broadcasting cadence
   - Integration points for networking

#### Modified Files:
1. **src/main.cpp** - Replaced with improved server loop
   - Fixed timestep implementation (60 Hz)
   - Four-phase processing: Input → Tick → Broadcast → Pace
   - Sequence tracking and diagnostics logging
   - Network I/O stubs ready for integration

### Client Folder (`c:\Users\USER\OneDrive\Documents\GitHub\Multiplayer-Pong-CPP\Client\`)

#### New Files:
1. **GameClient.h** - Header for client state management
   - Encapsulates local game state display
   - Methods: ReceiveStateUpdate(), SendInput()
   - Sequence number tracking for diagnostics

2. **GameClient.cpp** - Implementation of client
   - Server-authoritative state handling
   - Input packet formation
   - State interpolation placeholder for future enhancement

3. **ARCHITECTURE.md** - Client architecture documentation
   - Sequence tracking explanation
   - Consistency guarantees
   - HUD diagnostics display format
   - Testing procedures

#### Modified Files:
1. **src/main.cpp** - Replaced with improved client loop
   - Three-phase loop: Send Input → Receive State → Render
   - Sequence number display in HUD
   - Ball and paddle position diagnostics
   - Network I/O stubs ready for integration

### Common Folder
No changes - existing protocol.h defines all shared constants and packet structures

## Key Features Implemented

### 1. Server Tick Pacing (Fixed Timestep)
- **60 Hz tick rate**: 16.67 ms per frame
- **Frame budget enforcement**: Logs warnings if simulation exceeds frame time
- **Deterministic physics**: Same input produces same output
- Implementation: [GameServer.cpp](Server/GameServer.cpp) - Tick() method

### 2. Sequence Handling
**Client Sequences**
- Incremented each time client sends input
- Tracked on server for ordering verification
- Logged in console for diagnostics

**Server Sequences**
- Packet sequence = server tick number
- Broadcast with every state packet
- Allows clients to detect missed/duplicate updates

Implementation:
- [main.cpp](Server/src/main.cpp) - Lines 50-60 (PHASE 1)
- [GameClient.cpp](Client/GameClient.cpp) - ReceiveStateUpdate() method

### 3. Clean State Broadcast Cadence
- **Synchronized with ticks**: Broadcast happens after physics simulation
- **Fixed interval**: Configurable via BROADCAST_INTERVAL (currently every tick)
- **Consistent ordering**: Same sequence of updates to all clients
- Implementation: [main.cpp](Server/src/main.cpp) - PHASE 3 (Line 70-85)

### 4. Consistency Guarantees
Both clients see identical movement/score because:
1. Single server processes all physics
2. Deterministic simulation (same seed, same input = same output)
3. Broadcasts use same state to all clients
4. Sequence numbers ensure proper ordering

Example: When ball scores, both clients receive `{score: X-Y, tick: T}` simultaneously.

## Architecture Diagrams

### Server Processing Loop
```
┌─────────────────────────────────────┐
│   PHASE 1: Receive Client Packets   │
│  └─ Process Input (update paddle)   │
│  └─ Track Sequence Numbers          │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│      PHASE 2: Server Tick           │
│  └─ Update Physics (ball velocity)  │
│  └─ Check Collisions                │
│  └─ Increment Tick Counter          │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│   PHASE 3: Broadcast State          │
│  └─ Create Packet (seq = tick)      │
│  └─ Send to All Clients             │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│    PHASE 4: Frame Rate Pacing       │
│  └─ Sleep to 16.67ms frame time     │
│  └─ Log frame overruns              │
└──────────────┬──────────────────────┘
               │
               └─────────── Repeat


Time: 16.67ms per iteration
```

### Client Processing Loop
```
┌──────────────────────────────────────┐
│   PHASE 1: Send Input to Server      │
│  └─ Create Input Packet              │
│  └─ Set Client Sequence Number       │
│  └─ SendPacketToServer()             │
└────────────────┬─────────────────────┘
                 │
┌────────────────▼─────────────────────┐
│  PHASE 2: Receive Server State       │
│  └─ ReceiveFromServer()              │
│  └─ Process with GameClient          │
│  └─ Track Server Sequence            │
└────────────────┬─────────────────────┘
                 │
┌────────────────▼─────────────────────┐
│     PHASE 3: Render HUD              │
│  └─ Get Display State from Client    │
│  └─ Show Score, Positions, Sequences │
│  └─ Diagnostics Display              │
└────────────────┬─────────────────────┘
                 │
┌────────────────▼─────────────────────┐
│   PHASE 4: Frame Rate Pacing         │
│  └─ Sleep to 16.67ms frame time      │
└────────────────┬─────────────────────┘
                 │
                 └─────────── Repeat

Time: 16.67ms per iteration
```

### Consistency Flow
```
Time  Server                  Client 1                Client 2
----  ------                  --------                --------
0ms   Tick 0, Score 0-0       
16ms  Tick 1                  
      Broadcast Seq=1 (0-0)   
                              Recv Seq=1
                              Display 0-0
                                                      Recv Seq=1
                                                      Display 0-0
32ms  Tick 2                  
      Broadcast Seq=2 (0-0)   
                              Recv Seq=2
                              Display 0-0
                                                      Recv Seq=2
                                                      Display 0-0
48ms  Tick 3
      Ball scores
      Score now 1-0
      Broadcast Seq=3 (1-0)
                              Recv Seq=3
                              Display 1-0  ← Both see score
                                                      Recv Seq=3
                                                      Display 1-0  ← Same time!
```

## Network Integration Requirements

To integrate with actual networking, implement these stub functions:

### Server (main.cpp)
```cpp
// Receive a packet from any connected client
bool TryReceivePacket(uint8_t& client_id, Packet& p) {
    // TODO: Non-blocking socket receive from all clients
    // Return true if packet available, false if none
}

// Send packet to all connected clients
void SendPacketToAll(const Packet& p) {
    // TODO: Broadcast packet to all client addresses
}
```

### Client (main.cpp)
```cpp
// Send input packet to server
void SendPacketToServer(const Packet& p) {
    // TODO: Send packet to server address
}

// Receive state update from server
bool ReceiveFromServer(Packet& p) {
    // TODO: Non-blocking socket receive
    // Return true if packet available
}
```

## Build Instructions

### Prerequisites
- Visual Studio 2019+ or compatible C++17 compiler
- CMake 3.15+ (optional, for build generation)

### Building

#### Option 1: Visual Studio
1. Open `MultiplayerPong.sln`
2. Select Server or Client project
3. Build → Build Solution (Ctrl+Shift+B)

#### Option 2: Command Line
```bash
cd Server
g++ -std=c++17 -O2 src/main.cpp GameServer.cpp -o server.exe
g++ -std=c++17 -O2 ../Common/Common.cpp -o common.o

cd ../Client
g++ -std=c++17 -O2 src/main.cpp GameClient.cpp -o client.exe
```

## Testing

### Functional Testing

#### Test 1: Tick Pacing
1. Run server
2. Observe console output
3. **Expected**: "Tick X" increments smoothly, no frame warnings
4. **Verify**: Frame times consistently <16 ms

#### Test 2: State Broadcasting
1. Run server + 2 clients
2. Watch console output
3. **Expected**: Server logs "BCAST Tick X - Score: A vs B"
4. **Verify**: Both clients display same score at same tick

#### Test 3: Sequence Tracking
1. Run server + 1 client
2. Check client HUD
3. **Expected**: "Client Seq" increments every frame
4. **Expected**: "Server Seq" increments every frame
5. **Verify**: Both clients show same "Server Seq"

#### Test 4: Physics Consistency
1. Run server + 2 clients (with network simulation)
2. Add intentional packet loss (simulate 5-10% loss)
3. **Expected**: Both clients still see same final score
4. **Verify**: Movement appears synchronized despite packet loss

### Performance Testing

#### Latency Impact
- Measure time between input and display update
- Expected: ~30-150ms depending on network
- Should be same for both clients

#### Throughput
- Packets/second: 60 (one per tick)
- Bytes/second: 60 × 60 bytes = 3,600 bytes/sec (very low)
- Network bandwidth: <0.1% of typical broadband

### Debugging Helpers

#### Console Output Format
```
[RECV] Client 0 - Input Seq:150
[BCAST] Tick 150 - Score: 2 vs 1 - Ball: (450.5, 300.2)
[WARN] Frame took 17 ms (target: 16 ms)
```

#### HUD Diagnostics
```
Client Seq: 150  |  Server Seq: 150
Ball: (450.5, 300.2)  |  P1: 250.0  P2: 350.0
```
- If Client Seq > Server Seq: Inputs queued, waiting for server
- If Server Seq >> Client Seq: Client may be lagging
- Ball/paddle positions help verify physics correctness

## Physics Implementation Details

### Ball-Paddle Collision
```cpp
// Pseudo-code
if (ball within paddle bounds) {
    // Calculate spin based on hit location
    spin_ratio = (ball.y - paddle.y) / paddle_height
    ball.vy = spin_ratio * max_spin
    ball.vx = -ball.vx  // Reverse direction
}
```

### Wall Collision
```cpp
// Bounce off top/bottom walls
if (ball.y <= 0 || ball.y >= ArenaHeight) {
    ball.vy = -ball.vy
    ball.y = clamp(ball.y, 0, ArenaHeight)
}
```

### Scoring
```cpp
// Out of bounds = scoring
if (ball.x < 0) score[1]++  // Player 2 scores
if (ball.x > ArenaWidth) score[0]++  // Player 1 scores
ResetBall()
```

## Configuration Parameters

Edit in `Common/include/protocol.h`:

```cpp
constexpr uint32_t FrameTimeMs = 16;        // 60 Hz tick rate (1000/60)
constexpr float ArenaWidth = 800.0f;        // Game area width
constexpr float ArenaHeight = 600.0f;       // Game area height
constexpr float PaddleSpeed = 5.0f;         // Units per tick
constexpr float BallInitialSpeedX = 1.5f;   // Ball X velocity
constexpr float BallInitialSpeedY = 1.5f;   // Ball Y velocity
constexpr uint16_t WinningScore = 5;        // Points to win
```

Edit in `Server/GameServer.h`:

```cpp
constexpr uint32_t BROADCAST_INTERVAL = 1; // Broadcast every N ticks
```

## Troubleshooting

### Clients See Different Scores
- **Cause**: Physics not running on server OR asymmetric physics
- **Fix**: Ensure GameServer::Tick() called every frame
- **Check**: Server logs show "BCAST Tick X - Score" incrementing

### High Frame Times (>16 ms)
- **Cause**: Physics too complex OR I/O blocking
- **Fix**: Make network I/O non-blocking
- **Check**: Console shows "[WARN] Frame took X ms"

### Sequence Numbers Not Incrementing
- **Cause**: Client not receiving from server OR not sending input
- **Fix**: Check network stubs are implemented
- **Check**: Verify Packet structures are correct size

### Ball Stuck/Jittering
- **Cause**: Paddle collision bounds incorrect
- **Fix**: Adjust PADDLE_WIDTH/PADDLE_HEIGHT constants
- **Check**: Test with console output showing ball position

## Performance Benchmarks

### Expected Performance
- Physics: <2 ms per frame
- I/O: <1 ms per frame (non-blocking)
- Rendering: ~10-15 ms per frame (terminal)
- **Total**: ~15-18 ms per frame (within budget)

### Scalability
- Current: 2 players (3 objects: 2 paddles, 1 ball)
- Extendable to: 20+ players with same architecture
- Physics remains O(1) per tick

## Future Enhancements

1. **Interpolation**: Smooth movement between updates
2. **Client-Side Prediction**: Show predicted paddle positions
3. **Compression**: Reduce packet size (e.g., quantize positions)
4. **Adaptive Tickrate**: Adjust based on network conditions
5. **Rollback**: Handle client corrections
6. **Bandwidth Limiting**: Skip broadcasts when needed

## Additional Resources

- [Server Architecture](Server/ARCHITECTURE.md)
- [Client Architecture](Client/ARCHITECTURE.md)
- [Protocol Definition](Common/include/protocol.h)
- [QA Checklist](Docs/QA_Checklist.md)

## Contact & Support

For questions about implementation:
1. Check Architecture docs (ARCHITECTURE.md in Server/Client)
2. Review inline comments in source files
3. Run debug builds with logging enabled
4. Check console output for "[WARN]" and "[RECV]" messages
