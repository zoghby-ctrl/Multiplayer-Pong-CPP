# LAN Multiplayer Pong Team Division

This project has enough clear parts for 9 members. The team leader can coordinate, review, and connect the pieces while each member owns a small, understandable area.

## OOP Concepts Used

- Encapsulation: `NetClient`, client `GameObject`, `ServerGame`, `ServerPaddle`, and `ServerBall` keep their data private and expose functions.
- Inheritance: server `ServerPaddle` and `ServerBall` inherit from server `GameObject`.
- Polymorphism: server `GameObject::Update()` is virtual, and the game updates objects through base-class pointers.
- Polymorphism: `PaddleController` has human and AI implementations with the same interface.
- Abstraction: networking, rendering, platform/window creation, and game logic are separated into different files/classes.

## Suggested 9-Member Split

1. Team Leader: coordinates tasks, explains the full architecture, combines final work, and runs the demo.
2. Server Game Logic: owns `ServerGame`, scoring, rounds, win state, and reset behavior.
3. OOP Entities: explains and maintains `GameObject`, `ServerPaddle`, and `ServerBall`.
4. Collision and Physics: owns AABB collision, wall bounces, ball speed, and paddle hit response.
5. AI Player: owns `PaddleController`, `HumanController`, and `AiController`.
6. Networking Server: owns sockets, accepting clients, player slots, and sending snapshots.
7. Networking Client: owns `NetClient`, receiving snapshots, sending input, and connection state.
8. Input and Controls: owns `W/S`, arrow keys, Escape handling, and player-specific movement.
9. Graphics and UI: owns OpenGL rendering, paddles, ball effects, arena visuals, score text, and HUD.

## Short Presentation Flow

1. Team leader introduces LAN Pong and the client/server idea.
2. Server members explain game state, collision, scoring, and AI.
3. Networking members explain packets and snapshots.
4. Client members explain input and rendering.
5. Graphics member shows the OpenGL visual improvements.
6. Team leader runs the demo and closes with the OOP concepts.
