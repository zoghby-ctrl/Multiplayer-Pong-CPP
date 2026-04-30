# Server Architecture - Tick Pacing and State Broadcasting

## Overview

The improved server implementation provides:
- **Fixed Timestep Physics**: 60 Hz server tick rate (16.67 ms per tick)
- **Sequence Handling**: Tracks client packet sequences and assigns server sequence numbers
- **Clean State Broadcast Cadence**: State broadcasts at fixed intervals synchronized with server ticks
- **Deterministic Physics**: Ball-paddle collision detection and movement calculations

## Architecture Components

### 1. GameServer Class (GameServer.h/cpp)

The core game simulation engine that handles:

#### Key Methods
- `Tick()` - Main physics simulation (called once per frame)
  - Updates player input
  - Simulates ball movement
  - Checks collisions
  - Increments tick counter

- `ProcessClientPacket()` - Handles incoming client packets
  - Tracks client sequence numbers
  - Updates player input state

- `ShouldBroadcastState()` - Determines when to send state updates
  - Returns true when broadcast interval is reached
  - Configurable via BROADCAST_INTERVAL constant

#### Physics Simulation
The server performs:
1. **Player Input Processing** - Applies paddle movement from client inputs
2. **Ball Update** - Updates ball position based on velocity
3. **Collision Detection**:
   - Ball-wall collisions (top/bottom bounce)
   - Ball-paddle collisions (with spin calculation)
   - Out-of-bounds scoring

### 2. Main Server Loop (main.cpp)

Four-phase processing loop:

```
Phase 1: Process Incoming Packets
  └─ Handle client inputs
  └─ Track sequence numbers
  └─ Update GameServer state

Phase 2: Server Tick (Physics)
  └─ Call GameServer::Tick()
  └─ Simulate physics
  └─ Update game state

Phase 3: Broadcast State (Cadence)
  └─ Check ShouldBroadcastState()
  └─ Create state packet with server sequence
  └─ Send to all clients

Phase 4: Frame Rate Pacing
  └─ Sleep to maintain 60 Hz (16.67 ms target)
  └─ Log frame overruns if simulation takes too long
```

## Sequence Number Handling

### Client Sequence Numbers
- **Purpose**: Track client input order for diagnostics and potential client-side prediction
- **Tracking**: Incremented each time client sends an input packet
- **Usage**: Logged on server reception for debugging network order

### Server Sequence Numbers
- **Purpose**: Provide clients with authoritative state ordering
- **Value**: Server tick number at time of broadcast
- **Broadcast**: Included in every GameState packet sent to clients

## State Broadcasting Cadence

### Broadcast Interval
- Currently set to `BROADCAST_INTERVAL = 1` (every tick)
- Configurable based on network bandwidth requirements
- Example: Setting to 3 broadcasts every 3rd tick at ~20 Hz

### Deterministic Ordering
1. Process all pending client packets
2. Execute single physics tick
3. Check if should broadcast (based on interval)
4. Send state packet with current tick number as sequence

## Consistency Guarantees

### Both Clients See Consistent State Because:
1. **Single Authoritative Server**: All physics runs on server
2. **Deterministic Physics**: Same input → same output
3. **Fixed Tick Rate**: Both clients receive updates at predictable intervals
4. **Sequence Numbers**: Clients can detect missed/duplicate updates

### Normal Latency Handling
- Clients display most recent server state received
- Server continues broadcasting at fixed cadence regardless of network conditions
- Client input latency doesn't affect visual consistency of opponent movement

## Integration Points

### Network I/O (Stubs to implement)
Located in main.cpp, these functions need implementation:
- `SendPacketToAll(const Packet& p)` - Send to all connected clients
- `TryReceivePacket(uint8_t& client_id, Packet& p)` - Receive from any client

### Example Integration (UDP)
```cpp
bool TryReceivePacket(uint8_t& client_id, Packet& p) {
    sockaddr_in addr;
    int addr_len = sizeof(addr);
    int n = recvfrom(server_socket, &p, sizeof(p), MSG_DONTWAIT, 
                     (struct sockaddr*)&addr, &addr_len);
    if (n > 0) {
        // Map address to client_id
        client_id = GetClientId(addr);
        return true;
    }
    return false;
}
```

## Performance Characteristics

### Frame Budget
- **Target**: 16.67 ms per frame
- **Physics Time**: ~1-2 ms
- **Available for I/O**: ~14-15 ms
- **Headroom**: Allows 4-5ms network operations without frame drops

### Scalability
- Current design: 2 players
- Physics: O(1) per tick (fixed objects)
- Can extend to N players with minimal overhead

## Debugging

### Console Logging
```
[RECV] Client 0 - Input Seq:5
[BCAST] Tick 127 - Score: 0 vs 2 - Ball: (400.5, 300.2)
[WARN] Frame took 18 ms (target: 16 ms)
```

### Sequence Diagnostics
- Server logs every 60 broadcasts (1 per second approximately)
- Client logs server sequence and displays both seq numbers on screen
- Helps detect packet loss or ordering issues

## Testing

### Verifying Tick Pacing
1. Check console output shows consistent tick increments
2. Verify frame timing logs show <16 ms frame times
3. Monitor broadcast frequency matches expected cadence

### Verifying Consistency
1. Run two clients connected to same server
2. Observe score updates appear simultaneously on both clients
3. Verify paddle positions update at same time on both displays
4. Ball movement should be visually synchronized

## Future Enhancements

- **Interpolation**: Smooth movement between state updates
- **Rollback**: Handle client input corrections
- **Interest Management**: Broadcast only relevant state subsets
- **Compression**: Reduce network bandwidth
- **Clock Synchronization**: NTP-like protocol for client time alignment
