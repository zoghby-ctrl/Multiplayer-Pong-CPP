# Multiplayer Pong C++

A playable Windows C++ multiplayer Pong project using:

- `Client`: OpenGL window, keyboard input, TCP client, rendering
- `Server`: authoritative TCP server, player slots, game simulation
- `Common`: shared packet protocol and gameplay model

The canonical solution is `MultiplayerPong/MultiplayerPong.sln`.

## Features

- Real client/server communication over TCP on port `7777`
- One or two clients supported
- Player 2 uses AI fallback until a second client joins
- Reset and difficulty selection from the client menu
- OpenGL client renderer with letterboxed scaling, glow, ball trail, score display, and smooth court visuals
- Shared binary protocol with compile-time layout checks
- OOP structure using abstract interfaces, inheritance, and polymorphism:
  - `IClientTransport` -> TCP client transport
  - `IRenderer` -> OpenGL renderer
  - `IGameMode` -> `ClassicPongMode`
  - `IInputSource` -> human, AI, and idle input sources
  - `IGameObject` -> paddle and ball entities

## Build

1. Open `MultiplayerPong/MultiplayerPong.sln` in Visual Studio 2022.
2. Select `Debug` and `x64`.
3. Build the solution.

The project uses Windows libraries already available with Visual Studio:

- `ws2_32.lib` for WinSock networking
- `opengl32.lib` for OpenGL rendering

## Run

Start the server first:

```powershell
.\MultiplayerPong\x64\Debug\Server.exe
```

Then start one client:

```powershell
.\MultiplayerPong\x64\Debug\Client.exe
```

Start a second client to control player 2. If only one client is connected, the server controls player 2 with AI.

## Controls

- `W` or `Up Arrow`: move your paddle up
- `S` or `Down Arrow`: move your paddle down
- `R`: reset the match
- `1`: easy difficulty
- `2`: normal difficulty
- `3`: hard difficulty
- `Esc`: exit the client

You can also use the client window menu:

- `Game > Reset`
- `Difficulty > Easy / Normal / Hard`

The first connected client is player 1. The second connected client is player 2.
First player to `5` wins.

## Project Layout

```text
Client/
  Client/                 Visual Studio client project
  GameClient.h/.cpp       Client networking and state synchronization
  src/main.cpp            OpenGL window, rendering, and input loop

Common/
  Common.vcxproj          Shared static library project
  include/protocol.h      Packet types, constants, wire layout
  include/gameplay.h      Shared OOP gameplay model

Server/
  Server/                 Visual Studio server project
  GameServer.h/.cpp       Authoritative game state and match coordination
  src/main.cpp            WinSock accept loop and broadcasting
```

## Notes For The Team

- The server owns the truth. Clients send input only.
- Do not change packet layouts casually; `protocol.h` has static asserts to catch mismatches.
- Add future gameplay rules in `Common/include/gameplay.h` or behind `IGameMode`.
- Add future transports behind `IClientTransport` so the client loop stays clean.
