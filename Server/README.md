# Server Implementation - Files Overview

This folder contains the authoritative game server implementation with fixed tick pacing and clean state broadcasting.

## File Structure

```
Server/
├── src/
│   └── main.cpp                 # Server main loop (fixed timestep, 4-phase processing)
├── GameServer.h                 # Game logic header (physics simulation)
├── GameServer.cpp               # Game logic implementation
├── ARCHITECTURE.md              # Detailed architecture documentation
├── IMPLEMENTATION_GUIDE.md       # Complete implementation guide with examples
└── Server.vcxproj              # Visual Studio project file
```

## Quick Start

### Build
```bash
cd Server
cl.exe /std:c++latest /O2 src/main.cpp GameServer.cpp
```

### Run
```bash
.\a.exe
# Output:
# ===== Multiplayer Pong Server =====
# Tick Rate: 60 Hz (16.67 ms per tick)
# Fixed Timestep: Enabled
# Server initialized. Waiting for clients...
```

## Key Components

### 1. GameServer Class
**File**: `GameServer.h` / `GameServer.cpp`

Core game simulation engine:
- `Tick()` - Execute one frame of physics
- `ProcessClientPacket()` - Handle incoming input
- `ShouldBroadcastState()` - Determine broadcast cadence
- Physics: ball movement, paddle collision, scoring

### 2. Main Server Loop
**File**: `src/main.cpp`

Four-phase processing loop running at 60 Hz:

**Phase 1: Input Processing**
- Receive packets from all clients
- Extract player input
- Track sequence numbers

**Phase 2: Physics Simulation**
- Call `GameServer::Tick()`
- Update ball position
- Check collisions
- Handle scoring

**Phase 3: State Broadcasting**
- Check if should broadcast this tick
- Create state packet with server tick as sequence
- Send to all clients

**Phase 4: Frame Rate Pacing**
- Sleep to maintain 60 Hz target
- Log frame time violations

## Physics Simulation

### Ball Movement
```cpp
ball.x += ball.vx;
ball.y += ball.vy;
```
Simple linear motion, updated every tick.

### Collisions
1. **Wall Bouncing**: Ball reverses Y velocity at top/bottom
2. **Paddle Collision**: Ball reverses X velocity, gains spin based on hit location
3. **Scoring**: Ball exits left/right boundary → score increment → ball reset

### Determinism
- Same input → same physics output
- No randomness or floating-point errors affect consistency
- Critical for multiplayer synchronization

## Sequence Number Handling

### Client Sequences (Diagnostics)
- Tracked per client
- Incremented each input packet received
- Logged: `[RECV] Client 0 - Input Seq:150`

### Server Sequences (Critical)
- Set to server tick number when broadcasting
- Both clients receive same sequence number
- Enables: Detecting missed updates, ordering verification

Example output:
```
[BCAST] Tick 127 - Score: 0 vs 2 - Ball: (400.5, 300.2)
```
Sequence number = 127 (server tick)

## Broadcasting Cadence

### Configuration
```cpp
static constexpr uint32_t BROADCAST_INTERVAL = 1;  // Broadcast every tick
```

### Currently
- Broadcasts every frame (60 Hz)
- Packet sent to all clients with identical state
- Sequence numbers ensure proper ordering

### Future Optimization
Could set `BROADCAST_INTERVAL = 2` to broadcast every 2nd tick (30 Hz):
- Reduces bandwidth by 50%
- Slight latency increase (~8.3 ms)
- Still smooth visual updates

## Network Integration

### Stubs to Implement

**Receive from Clients**
```cpp
bool TryReceivePacket(uint8_t& client_id, Packet& p) {
    // TODO: Non-blocking UDP receive
    // Fill: client_id, p.header, p.payload.input
    // Return true if packet available
    return false;  // Currently: no packets
}
```

**Send to All Clients**
```cpp
void SendPacketToAll(const Packet& p) {
    // TODO: UDP broadcast to all client addresses
    // p.header.type == PacketType::State
    // p.header.seq == server tick
    // p.payload.state == current GameState
}
```

### Example UDP Implementation
```cpp
bool TryReceivePacket(uint8_t& client_id, Packet& p) {
    sockaddr_in addr;
    int addr_len = sizeof(addr);
    int n = recvfrom(server_socket, &p, sizeof(p), MSG_DONTWAIT,
                     (struct sockaddr*)&addr, &addr_len);
    if (n > 0) {
        client_id = GetClientIdFromAddress(addr);
        return true;
    }
    return false;
}

void SendPacketToAll(const Packet& p) {
    for (const auto& client_addr : connected_clients) {
        sendto(server_socket, &p, sizeof(p), 0,
               (struct sockaddr*)&client_addr, sizeof(client_addr));
    }
}
```

## Console Output

### Startup
```
===== Multiplayer Pong Server =====
Tick Rate: 60 Hz (16.67 ms per tick)
Fixed Timestep: Enabled
====================================

Server initialized. Waiting for clients...
```

### Per-Frame (every 60 broadcasts ≈ 1 second)
```
[RECV] Client 0 - Input Seq:300
[RECV] Client 1 - Input Seq:200
[BCAST] Tick 240 - Score: 1 vs 2 - Ball: (450.5, 300.2)
```

### Warnings (if frame takes >16 ms)
```
[WARN] Frame took 18 ms (target: 16 ms)
```

## Testing Checklist

- [ ] **Build**: Compiles without errors
- [ ] **Run**: Server starts and ticks (watch tick counter)
- [ ] **Timing**: Frame times shown in console <16 ms
- [ ] **Network**: When network integration done, verify both clients see same score
- [ ] **Physics**: Verify ball bounces and paddles collide correctly

## Performance

| Metric | Value |
|--------|-------|
| Tick Rate | 60 Hz |
| Frame Time Budget | 16.67 ms |
| Physics Time | ~1-2 ms |
| Network I/O | <1 ms (non-blocking) |
| Available for I/O | ~14-15 ms |

## Documentation

- **ARCHITECTURE.md**: Deep dive into design decisions
- **IMPLEMENTATION_GUIDE.md**: Complete guide with examples
- **GameServer.h**: Inline comments explaining API
- **src/main.cpp**: Commented 4-phase loop

## Debugging

### Enable Detailed Logging
Add to main loop:
```cpp
static uint32_t frame_count = 0;
if (frame_count++ % 60 == 0) {
    std::cout << "[SERVER] Tick " << game_server.GetServerTick() << std::endl;
}
```

### Check Frame Timing
Console shows frame warnings if >16 ms. Look for patterns:
- Consistent <16 ms: ✓ Good
- Occasional >16 ms: OK (network I/O)
- Frequent >16 ms: Problem (physics too complex)

### Verify Physics
Print ball/paddle positions:
```cpp
std::cout << "Ball: (" << ball.x << ", " << ball.y << ")" << std::endl;
```

## Architecture at a Glance

```
                    Server Tick Loop (16.67 ms)
                            │
            ┌───────────────┼───────────────┐
            │               │               │
        Input           Physics         Broadcast
        Process         Simulate        State
            │               │               │
            ▼               ▼               ▼
    Process Packets   Execute Physics  Create Packet
    Track Sequences   Check Collisions  Set Seq = Tick
    Update Input      Handle Scoring    Send to Clients
            │               │               │
            └───────────────┼───────────────┘
                            │
                    Sleep if time remaining
                            │
                    Repeat every 16.67 ms
```

## Quick Reference

**Starting Server**: Just run executable
**Client Connection**: Implement `TryReceivePacket()` and `SendPacketToAll()`
**Tick Rate**: 60 Hz (16.67 ms), edit `FrameTimeMs` in protocol.h to change
**State Update Rate**: Every tick, can configure `BROADCAST_INTERVAL`
**Physics**: Deterministic, no randomness
**Consistency**: All clients receive identical states via sequence numbers

## Support

Refer to:
1. **ARCHITECTURE.md** - Why design choices
2. **IMPLEMENTATION_GUIDE.md** - How to use and extend
3. **GameServer.h** - API documentation
4. **src/main.cpp** - Implementation details

---

**Status**: Ready for network integration. Stubs located in main.cpp at bottom.
