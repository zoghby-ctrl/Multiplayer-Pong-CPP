# Multiplayer Pong C++

A collaborative C++ client-server Pong project with shared protocol definitions for multiplayer gameplay.

## Project Overview

The repository is split into three core modules:

- **Client** (`/Client`): handles presentation, HUD rendering, and client-side input/network flow.
- **Server** (`/Server`): runs authoritative game simulation, ball/paddle updates, and scoring.
- **Common** (`/Common`): contains shared protocol/data structures used by both client and server (`Common/include/protocol.h`).

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

This order helps ensure the client can connect once the server is already running.

## Controls

- **F3**: Toggle debug overlay (FPS, ping, snapshot information)
- **Q**: Quit/close the running console app

## Repository Structure

- `/Client` - client executable/project and source
- `/Server` - server executable/project and source
- `/Common` - shared protocol definitions and common code
- `/Docs` - architecture and QA documentation

## Troubleshooting

- **Build fails on Win32/x86**: switch platform to **x64** (`Debug | x64` is the expected dev target).
- **`protocol.h` not found**:
  - Verify include path points to `Common/include`.
  - In Visual Studio: **Project Properties -> C/C++ -> General -> Additional Include Directories** should include `..\\..\\Common\\include` for Client/Server projects.

## Screenshots

Add project screenshots here as they become available.

Example markdown:

```md
![Client HUD while connected](Docs/images/client-hud.png)
![Server console output](Docs/images/server-console.png)
```

Suggested location for image assets: `Docs/images/`.
