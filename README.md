# Multiplayer Pong CPP

LAN multiplayer Pong in C++ using Win32 sockets and OpenGL.

## Structure

- `Client/` - OpenGL client, rendering, controls, and networking client
- `Server/` - server loop, game logic, collision, scoring, and AI fallback
- `Common/` - shared protocol and socket helpers
- `LanMultiplayerPong1/` - Visual Studio solution and server project
- `TEAM_DIVISION.md` - 9-member team split and OOP concept summary

## Build

Open `LanMultiplayerPong1/LanMultiplayerPong1.sln` in Visual Studio and build `Debug|x64`.

Run the server first, then run one or two clients. Player 1 uses `W/S`; Player 2 uses the arrow keys.
