# Client Architecture - State Synchronization

## Overview

The improved client implementation provides:
- **Sequence Tracking**: Monitors both client and server sequence numbers
- **Server Authoritative State**: Trusts server as single source of truth
- **Consistent Display**: Always shows most recent valid state received
- **Network Diagnostic Info**: Shows sequence numbers and entity positions

## Architecture Components

### 1. GameClient Class (GameClient.h/cpp)

Client-side game state management:

#### Key Methods
- `SendInput(bool up, bool down)` - Send player input to server
  - Called when keyboard input detected
  - Increments client sequence number
  - Queues packet for network transmission

- `ReceiveStateUpdate(const Packet& state_packet)` - Process incoming state
  - Validates packet type
  - Tracks server sequence number
  - Updates display state with server data

- `GetDisplayState()` - Get current state to render
  - Always returns most recent state from server
  - Safe to call every frame for rendering

- `GetClientSequence()` / `GetServerSequence()` - Diagnostics
  - Used for HUD display of network synchronization

#### State Management
- **Display State**: What's currently being shown on screen
- **Client Sequence**: Count of input packets sent
- **Last Server Sequence**: Most recent state sequence received

### 2. Main Client Loop (main.cpp)

Three-phase processing loop:

```
Phase 1: Send Input to Server
  └─ Create input packet
  └─ Set client sequence number
  └─ Call SendPacketToServer()

Phase 2: Receive State Updates
  └─ Call ReceiveFromServer()
  └─ If state packet: process with GameClient
  └─ Track server sequence for display

Phase 3: Render Display
  └─ Get current display state from GameClient
  └─ Call RenderHUD() with diagnostic info
  └─ Show score, entity positions, sequences

Phase 4: Frame Rate Pacing
  └─ Sleep to maintain 60 Hz (16.67 ms target)
```

## Sequence Tracking

### What Sequences Tell Us

**Client Sequence**
- Incremented: Once per input packet sent
- Helps detect: How many inputs have been sent
- Use case: Diagnostics to verify client-side input generation

**Server Sequence**
- Incremented: Once per state broadcast
- Represents: Server's tick number at broadcast time
- Use case: Detect missed updates, measure update frequency

### Example Sequence Flow
```
Time  Client Action            Server Action            Display
---   ----------------         ----------------         ----------------
0ms   Send Input Seq=0          Process packet
16ms  Send Input Seq=1          Tick, broadcast Seq=1    [Waiting]
32ms  Send Input Seq=2          Tick, broadcast Seq=2    [Update Seq=2]
48ms  Send Input Seq=3          Tick, broadcast Seq=3    [Update Seq=3]
```

## Consistency Under Normal Latency

### Why Both Clients See Same Movement/Score

1. **Server Authority**: Only server simulates physics
   - Ball movement
   - Paddle collisions
   - Score calculations

2. **Synchronized Ticking**: Server ticks at fixed 60 Hz
   - Deterministic input processing
   - Deterministic physics
   - Identical results every time

3. **Fixed Broadcast Cadence**: States sent at regular intervals
   - Both clients receive updates at predictable times
   - Updates contain identical information
   - Order guaranteed by sequence numbers

### Example: Ball Scoring

```
Server                              Client 1                  Client 2
------                              --------                  --------
Score: 0-0                          Display: 0-0              Display: 0-0
Tick 100: Ball hits left bound
Score: 0-1
Send Seq=100
Broadcast: {score: 0-1, tick: 100}
                                    Recv Seq=100
                                    Display: 0-1
                                                              Recv Seq=100
                                                              Display: 0-1
```

Both see same score at same game tick!

## Display and Diagnostics

### HUD Output

```
==============================
       SCORE
  Player 1 : 2   |   Player 2 : 1
==============================

  Connected  |  Tick: 150

Client Seq: 150  |  Server Seq: 150
Ball: (450.5, 300.2)  |  P1: 250.0  P2: 350.0
```

**Diagnostics Shown**
- Score: Current game score (from server state)
- Tick: Server tick counter (when last update sent)
- Client/Server Seq: Network sync indicators
- Ball/Paddle Positions: Entity state verification

## Integration Points

### Network I/O (Stubs to implement)

Located in main.cpp:

```cpp
void SendPacketToServer(const Packet& p)
// Implement: Send input packet to server via UDP/TCP
// Called every frame to send latest input

bool ReceiveFromServer(Packet& p)
// Implement: Receive state update from server
// Called every frame; return true if packet received
```

### Example Integration (UDP)

```cpp
bool ReceiveFromServer(Packet& p) {
    sockaddr_in addr;
    int addr_len = sizeof(addr);
    int n = recvfrom(client_socket, &p, sizeof(p), MSG_DONTWAIT,
                     (struct sockaddr*)&addr, &addr_len);
    return n > 0;
}

void SendPacketToServer(const Packet& p) {
    sendto(client_socket, &p, sizeof(p), 0,
           (struct sockaddr*)&server_addr, sizeof(server_addr));
}
```

## Input Handling

### Current Implementation
- Input automatically sent: `input.down = false` (test mode)
- Modify logic in main.cpp Phase 1 to get actual keyboard input

### Expected Integration
```cpp
// In main.cpp Phase 1, replace with:
InputManager input_mgr;
input_mgr.Update();  // Poll keyboard

input_packet.payload.input.up = input_mgr.IsKeyDown(Key::Up);
input_packet.payload.input.down = input_mgr.IsKeyDown(Key::Down);
```

## Performance

### Frame Budget
- **Target**: 16.67 ms per frame (60 Hz)
- **Rendering**: ~10-12 ms
- **Network Wait**: ~0-5 ms (non-blocking)
- **Available**: Margin for other operations

### Network Overhead
- **Packet Size**: ~60 bytes (Packet struct)
- **Bandwidth**: 60 Hz × 60 bytes × 2 directions = ~14.4 KB/s
- **Very efficient**: Typical broadband easily handles this

## Robustness

### Missing Updates
- If server packet lost: Display shows last known state
- No freezing or stuttering
- Next valid packet updates display

### Out-of-Order Packets
- Tracked via sequence numbers
- Could detect and discard old packets
- Currently accepts all valid packets (assumes in-order delivery)

### Latency
- Input latency: Network RTT (~20-100ms typical)
- Display latency: Network one-way + server tick (~10-50ms typical)
- Total: 30-150ms typical for real players

## Testing

### Verifying Consistency
1. Run two clients connecting to same server
2. Observe server ticks increment in both clients
3. Watch score updates appear simultaneously
4. Verify ball position shown identically on both displays

### Checking Sequences
1. Monitor Client Seq on one client - should increment every frame
2. Monitor Server Seq on both clients - should be identical and incrementing
3. Check sequence gaps indicate missed network packets

### Network Diagnostics
- Add logging to SendPacketToServer: Log every input sent
- Add logging to ReceiveFromServer: Log every state received
- Compare logs from two clients - should see coordinated updates

## Future Enhancements

- **Input Buffering**: Queue inputs during network lag
- **Interpolation**: Smooth position changes between updates
- **Prediction**: Show predicted positions while waiting for update
- **Lag Compensation**: Adjust display based on network statistics
- **Bandwidth Adaptation**: Reduce broadcast rate on slow connections
