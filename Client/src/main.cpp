#include <iostream>
#include <Windows.h>   // for GetAsyncKeyState
#include "..\\..\\Common\\include\\protocol.h"

using namespace std;

int main()
{
    cout << "Client running...\n";
    bool gameRunning = true;
    int moveDirection = 0; // 0 = no movement
    Protocol::PlayerInput input{};

    DWORD lastSendTime = GetTickCount(); // milliseconds
    const DWORD sendInterval = 50; // send input every 50 ms

    while (gameRunning)
    {
        input.up = input.down = input.left = input.right = 0;

        // Read input: player 1 uses W/S, player 2 uses Up/Down arrows
        if ((GetAsyncKeyState('W') & 0x8000) != 0 || (GetAsyncKeyState(VK_UP) & 0x8000) != 0) {
            input.up = 1;
        } else if ((GetAsyncKeyState('S') & 0x8000) != 0 || (GetAsyncKeyState(VK_DOWN) & 0x8000) != 0) {
            input.down = 1;
        } else if ((GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0) {
            gameRunning = false;
        }

        // Determine movement direction
        if (input.up && !input.down)
            moveDirection = 1;
        else if (input.down && !input.up)
            moveDirection = -1;
        else
            moveDirection = 0;

        // Periodic send logic
        DWORD currentTime = GetTickCount();
        if (currentTime - lastSendTime >= sendInterval)
        {
            lastSendTime = currentTime;
            // TODO: send `input` to server here
        }

        // Debug output
        cout << "Up: " << (int)input.up << " Down: " << (int)input.down << endl;
        cout << "moving direction " << moveDirection << endl;

        Sleep(16); // ~60 FPS loop delay for testing
    }

    return 0;
}