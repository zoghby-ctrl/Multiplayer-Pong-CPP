# Multiplayer-Pong-CPP

A C++ multiplayer Pong project being developed by a 9-person team.

## Project Structure

- `Client/` - client-side code
- `Server/` - server-side code
- `Common/` - shared protocol and shared definitions
- `Docs/` - architecture notes and project documentation

## Current Status

This repository currently contains the initial project scaffold.
The team will build the game step by step using feature branches from `develop`.

## Planned Architecture

- **Client**: input, rendering, UI, networking client logic
- **Server**: authoritative game logic, collisions, score, match rules
- **Common**: shared structs, packet definitions, constants, and protocol data

## Team Workflow

- `main` = stable baseline
- `develop` = integration branch
- `feature/...` = individual task branches

## Rules

- Do not work directly on `main`
- Do not work directly on `develop`
- Create a feature branch from `develop`
- Commit your changes with a clear message
- Open a pull request into `develop` when your task is ready
