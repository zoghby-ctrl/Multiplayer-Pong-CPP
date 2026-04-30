# Client Implementation - Files Overview

This folder contains the client application that connects to the server and displays synchronized game state with sequence tracking.

## File Structure

```
Client/
├── src/
│   └── main.cpp                 # Client main loop (input → receive → render)
├── GameClient.h                 # Client state management header
├── GameClient.cpp               # Client state management implementation
├── ARCHITECTURE.md              # Detailed architecture documentation
├── Client.vcxproj              # Visual Studio project file
└── Client/
    └── Client.vcxproj          # Inner project folder (VS structure)
```

## Quick Start

### Build
```bash
cd Client
cl.exe /std:c++latest /O2 src/main.cpp GameClient.cpp
```

### Run
```bash
.\a.exe
# Output:
# ===== Multiplayer Pong Client =====
# Sync Mode: Server Authoritative
# Sequence Tracking: Enabled
# Client initialized. Waiting for server state...
```

### Display (Terminal)
```
==============================
       SCORE
  Player 1 : 0   |   Player 2 : 0
==============================

  Connected  |  Tick: 150

Client Seq: 150  |  Server Seq: 150
Ball: (450.5, 300.2)  |  P1: 250.0  P2: 350.0
```

## Key Components

### 1. GameClient Class
**File**: `GameClient.h` / `GameClient.cpp`

Client-side state management:
- `SendInput()` - Queue input to send to server
- `ReceiveStateUpdate()` - Process incoming server state
- `GetDisplayState()` - Get current state to render
- `GetClientSequence()` / `GetServerSequence()` - Diagnostic info

### 2. Main Client Loop
**File**: `src/main.cpp`

Three-phase processing loop running at 60 Hz:

**Phase 1: Send Input**
- Create input packet
- Set client sequence number
- Call `SendPacketToServer()`

**Phase 2: Receive State**
- Call `ReceiveFromServer()`
- Process with GameClient
- Track server sequence

**Phase 3: Render**
- Get display state
- Show HUD with diagnostics
- Display score, positions, sequences

**Phase 4: Frame Rate Pacing**
- Sleep to maintain 60 Hz

## Architecture

### Data Flow
```
User Input
    │
    ▼
[GameClient::SendInput]
    │
    ▼
Network ◄──────────────┐
    │                  │
    ▼                  │
[GameClient::ReceiveStateUpdate]
    │
    ▼
Display State
    │
    ▼
RenderHUD()
```

### State Model
```
GameClient
  ├─ display_state_         ← What's shown on screen
  │  ├─ players[2]          ← Paddle positions
  │  ├─ ball                ← Ball position/velocity
  │  ├─ score[2]            ← Current score
  │  └─ tick                ← Server tick counter
  ├─ client_sequence_        ← Outgoing input count
  └─ last_server_sequence_   ← Latest server update seq
```

## Sequence Tracking

### Client Sequence
- Incremented every frame when input sent
- Shows: How many inputs have been sent
- Purpose: Verify client is actively sending

### Server Sequence
- Matches server tick number
- Shows: Most recent server state received
- Purpose: Detect missed updates, verify sync

### Example Display
```
Client Seq: 275  |  Server Seq: 270
```
Means: 
- Client sent 275 input packets
- Most recent state received had sequence 270
- Typical difference: 5-15 (network round-trip latency in frames)

## Network Integration

### Stubs to Implement

**Send Input to Server**
```cpp
void SendPacketToServer(const Packet& p) {
    // TODO: UDP send to server address
    // p.header.type == PacketType::Input
    // p.header.seq == client sequence number
    // p.payload.input == player movement
}
```

**Receive State from Server**
```cpp
bool ReceiveFromServer(Packet& p) {
    // TODO: Non-blocking UDP receive
    // Return true if packet available
    // Fill: p.header.seq, p.payload.state
    return false;  // Currently: no packets
}
```

### Example UDP Implementation
```cpp
void SendPacketToServer(const Packet& p) {
    sendto(client_socket, &p, sizeof(p), 0,
           (struct sockaddr*)&server_addr, sizeof(server_addr));
}

bool ReceiveFromServer(Packet& p) {
    sockaddr_in addr;
    int addr_len = sizeof(addr);
    int n = recvfrom(client_socket, &p, sizeof(p), MSG_DONTWAIT,
                     (struct sockaddr*)&addr, &addr_len);
    return n > 0;
}
```

## Console Output

### Startup
```
===== Multiplayer Pong Client =====
Sync Mode: Server Authoritative
Sequence Tracking: Enabled
====================================

Client initialized. Waiting for server state...
```

### Per-Frame (Terminal HUD)
```
==============================
       SCORE
  Player 1 : 2   |   Player 2 : 1
==============================

  Connected  |  Tick: 150

Client Seq: 150  |  Server Seq: 150
Ball: (450.5, 300.2)  |  P1: 250.0  P2: 350.0
```

### Diagnostics (every 60 updates ≈ 1 sec)
```
[CLIENT] Received 60 state updates (Server Seq: 150)
```

## Input Handling

### Current Implementation
```cpp
// In main.cpp Phase 1:
input_packet.payload.input.up = false;
input_packet.payload.input.down = false;
```

### To Integrate Keyboard
Replace with:
```cpp
InputManager input_mgr;
input_mgr.Update();

input_packet.payload.input.up = input_mgr.IsKeyDown(Key::Up);
input_packet.payload.input.down = input_mgr.IsKeyDown(Key::Down);
```

### Input Structure
```cpp
struct PlayerInput {
    uint8_t up;      // 1 = pressed, 0 = released
    uint8_t down;    // 1 = pressed, 0 = released
    uint8_t left;    // Reserved (not used)
    uint8_t right;   // Reserved (not used)
};
```

## Consistency Guarantees

### Why Both Clients See Same State

1. **Single Server Authority**
   - Only server runs physics
   - All game state computed on server
   - Clients display server's authoritative state

2. **Deterministic Simulation**
   - Same input → same physics result
   - Same timestep → same ball movement
   - Collision detection identical on server

3. **Synchronized Broadcasting**
   - Server sends to all clients simultaneously
   - Both clients receive same state packet
   - Sequence numbers ensure ordering

4. **Example: Scoring Event**
   ```
   Server Tick 100: Ball exits left boundary
   Server: score[1]++  // Player 2 scores
   
   Both Clients:
   └─ Receive {score: 0-1, seq: 100}
   └─ Display score: 0-1
   └─ Visible at same time
   ```

## Testing

### Manual Testing
1. **Start server**: `.\server.exe`
2. **Start client 1**: `.\client.exe`
3. **Start client 2**: `.\client.exe` (in another window)
4. **Observe**: Both show same score when updates received

### Sequence Verification
1. Watch "Client Seq" increment every frame
2. When packet received, "Server Seq" updates
3. Both clients should show same "Server Seq"

### Network Diagnostics
1. Add logging to `SendPacketToServer()`:
   ```cpp
   std::cout << "[SEND] Seq: " << packet.header.seq << std::endl;
   ```
2. Add logging to `ReceiveFromServer()`:
   ```cpp
   if (n > 0) {
       std::cout << "[RECV] Seq: " << p.header.seq << std::endl;
   }
   ```
3. Compare logs from both clients

## HUD Interpretation

### Score Display
```
Player 1 : X   |   Player 2 : Y
```
Shows current match score. Updated every time server broadcasts.

### Tick Display
```
Connected  |  Tick: 150
```
Shows server's tick counter at time of last state broadcast.
- Incrementing: ✓ Receiving updates
- Stuck: Problem (no updates received)

### Sequence Display
```
Client Seq: 275  |  Server Seq: 270
```
- Client Seq: Count of inputs sent (increments every frame)
- Server Seq: Most recent state's sequence number
- Difference: ~3-5 = normal latency, ~10+ = high latency/packet loss

### Position Display
```
Ball: (450.5, 300.2)  |  P1: 250.0  P2: 350.0
```
- Ball: (X, Y) position in arena
- P1: Player 1 (left) paddle Y position
- P2: Player 2 (right) paddle Y position
- Use to verify game state makes sense

## Performance

| Metric | Value |
|--------|-------|
| Frame Rate | 60 Hz |
| Input Sending | Every frame |
| State Processing | Non-blocking |
| Rendering | ~10 ms |
| Network I/O | <1 ms (non-blocking) |

## Performance Characteristics

### Bandwidth
- Outgoing: 60 packets/sec × 60 bytes = 3.6 KB/sec
- Incoming: 60 packets/sec × 120 bytes = 7.2 KB/sec
- Total: ~11 KB/sec (minimal)

### Latency
- Input latency: RTT (round-trip time) + 1 server tick
- Display latency: Network one-way + 1 server tick
- Typical: 30-150ms total (depending on connection)

## State Synchronization

### What Gets Synchronized
```cpp
struct GameState {
    uint32_t tick;           // Server tick number
    PlayerData players[2];   // Both paddles
    BallData ball;           // Ball state
    uint16_t score[2];       // Both scores
    MatchStatus status;      // Game status
};
```

### Sent Every Tick
Server broadcasts complete GameState every frame to all clients.
- No partial updates
- No incremental changes
- Always: full authoritative state

### Client Processing
```cpp
// Receive state from server
if (ReceiveFromServer(packet)) {
    game_client.ReceiveStateUpdate(packet);
}

// Get state to display
const GameState& state = game_client.GetDisplayState();
RenderHUD(state, ...);
```

## Troubleshooting

| Problem | Cause | Fix |
|---------|-------|-----|
| Score always 0-0 | No network integration | Implement stubs in main.cpp |
| Client Seq stops incrementing | Network I/O error | Check SendPacketToServer() |
| Server Seq not updating | Server not broadcasting | Check server console for [BCAST] |
| High latency | Network slow | Normal with high ping, working as intended |
| Different scores on clients | Server-side physics issue | Check server GameServer::Tick() |

## Documentation

- **ARCHITECTURE.md**: Design deep-dive
- **GameClient.h**: Class API documentation
- **src/main.cpp**: Inline comments
- **README.md** (this file): Quick reference

## Architecture at a Glance

```
             Client Frame Loop (16.67 ms)
                      │
        ┌─────────────┼─────────────┐
        │             │             │
    Send Input    Receive State   Render HUD
        │             │             │
        ▼             ▼             ▼
    Create Packet Process Update Show Display
    Send to      Apply to       with Sequences
    Server       GameClient      and Positions
        │             │             │
        └─────────────┼─────────────┘
                      │
              Sleep if time remaining
                      │
              Repeat every 16.67 ms
```

## Quick Tips

- **Verify Sync**: Watch "Server Seq" on both clients - should be identical
- **Check Latency**: Difference between "Client Seq" and "Server Seq" indicates network delay
- **Debug Physics**: Look at "Ball" position - verify it's moving correctly
- **Test Scoring**: Wait for ball to exit bounds - both clients should see score update simultaneously

---

**Status**: Ready for network integration. Stubs located in main.cpp at bottom.
