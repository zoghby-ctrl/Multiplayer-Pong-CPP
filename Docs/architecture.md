# Architecture Notes

## Core Idea
The client sends intent, the server sends truth.

## Active Product Surface

- Canonical entrypoint: `MultiplayerPong/MultiplayerPong.sln`
- Client: console HUD plus scripted demo state feed
- Server: authoritative simulation plus AI fallback for paddle two
- Common: shared protocol/data definitions and gameplay constants

## Current Constraints

- The active client/server transport layer is still stubbed.
- `Common/include/protocol.h` remains the single source of truth for packet layout and shared constants.
- Packet field order and sizes must stay coordinated between client and server.

## Experimental and Reference Code

- Root-level `main.cpp`, `Game.cpp`, `DebugOverlay.*`, and `Include/*` are retained prototypes and are not wired into `MultiplayerPong.sln`.
- `Solution1/` contains placeholder Visual Studio starter projects and is not part of the primary build.
