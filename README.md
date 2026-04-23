# Multiplayer Pong C++

A collaborative C++ client-server Pong project with shared protocol definitions for multiplayer gameplay.

The canonical entrypoint for active development is `MultiplayerPong/MultiplayerPong.sln`.

## Project Overview

The repository is split into three core modules:

- **Client** (`/Client`): handles presentation, HUD rendering, and client-side input/network flow.
- **Server** (`/Server`): runs authoritative game simulation, ball/paddle updates, and scoring.
- **Common** (`/Common`): contains shared protocol/data structures used by both client and server (`Common/include/protocol.h`).

## Current Active State

The active solution currently demonstrates:

- shared packet/types in `Common`
- a console client HUD that renders a scripted server-state feed
- a local server simulation with AI fallback for paddle two

The active `Client` and `Server` projects still use transport stubs. Real socket networking is not implemented in this repo revision yet.

## Active vs Experimental

### Active code

- `MultiplayerPong/MultiplayerPong.sln`
- `Client/`
- `Server/`
- `Common/`
- `Docs/`

### Experimental or reference-only code

- Root-level `main.cpp`, `Game.cpp`, `DebugOverlay.*`, and `Include/*`
- `Solution1/`

These paths are kept for reference or prototyping and are not part of the primary `MultiplayerPong.sln` build.

## Prerequisites

Before building, install:

- **Visual Studio 2022**
- **Desktop development with C++** workload (MSVC v143 toolset)
- **Git**
- **GitHub Desktop** (optional, for GUI-based cloning and branch management)

## Clone the Repository

### Option A: Git (CLI)

```bash
git clone https://github.com/zoghby-ctrl/Multiplayer-Pong-CPP.git
cd Multiplayer-Pong-CPP
```

### Option B: GitHub Desktop (optional)

1. Open **GitHub Desktop**.
2. Go to **File -> Clone repository...**.
3. Select the URL tab and paste: `https://github.com/zoghby-ctrl/Multiplayer-Pong-CPP.git`.
4. Choose a local path and clone.

## Branch Workflow

Use `develop` as the integration branch:

1. Checkout `develop` and pull latest updates.
2. Create a feature branch from `develop` (for example: `feature/readme-update`).
3. Commit and push your branch.
4. Open a Pull Request back into **`develop`**.

Example:

```bash
git checkout develop
git pull
git checkout -b feature/your-task-name
```

## Build (Visual Studio 2022)

1. Open `MultiplayerPong/MultiplayerPong.sln`.
2. Set **Solution Configuration** to `Debug`.
3. Set **Solution Platform** to `x64`.
4. Build the solution with **Ctrl+Shift+B** (or Build -> Build Solution).

## Run

Start the server first, then the client.

### Practical launch flow (recommended)

1. In Solution Explorer, right-click **Server** -> **Set as Startup Project**.
2. Start server (`F5` or **Debug -> Start Debugging**).
3. Start a second instance for the client:
   - right-click **Client** -> **Debug -> Start New Instance**, or
   - stop and switch startup project to **Client**, then run.

This order matches the intended future network flow and keeps the current demo startup sequence consistent.

### What to expect from the active solution

- `Server` runs the authoritative Pong simulation.
- When no second player input is available, the server uses AI fallback for paddle two.
- `Client` renders a demo HUD and consumes a scripted state feed while transport is stubbed.

## Prototype Controls

The standalone root-level prototype (`main.cpp`) supports:

- **F3**: Toggle the experimental debug overlay
- **Q**: Quit the experimental overlay demo

## Repository Structure

- `/MultiplayerPong` - canonical Visual Studio solution for active development
- `/Client` - active client project and source
- `/Server` - active server project and source
- `/Common` - shared protocol definitions and common code
- `/Docs` - architecture and QA documentation
- Root-level `.cpp/.h` files - experimental prototypes kept for reference
- `/Solution1` - placeholder Visual Studio starter projects, not part of the main build

## Troubleshooting

- **Build fails on Win32/x86**: switch platform to **x64** (`Debug | x64` is the expected dev target).
- **`protocol.h` not found**:
  - Verify include path points to `Common/include`.
  - In Visual Studio: **Project Properties -> C/C++ -> General -> Additional Include Directories** should include `..\\..\\Common\\include` for Client/Server projects.
- **Server/client appear local-only**:
  - That is expected in the current active solution.
  - The transport layer is stubbed; the projects currently demonstrate structure and simulation flow rather than live network play.

## Screenshots

Add project screenshots here as they become available.

Example markdown:

```md
![Client HUD while connected](Docs/images/client-hud.png)
![Server console output](Docs/images/server-console.png)
```

Suggested location for image assets: `Docs/images/`.
