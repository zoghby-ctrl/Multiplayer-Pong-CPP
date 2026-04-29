#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h> // for GetAsyncKeyState and Sleep
#include <cstdint>   // for uint8_t
#include "../../Common/include/protocol.h"
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

enum class MatchState
{
    WAITING,
    IN_PROGRESS,
    GAME_OVER,
    RESET
};

int main()
{
    ///////////////////////////////////////////////////////////////////////////////////////////

    MatchState state = MatchState::WAITING; // always initialize

    // MAKES THE GAME ALWAYS IN STANDBY MODE
    // state = MatchState::GAME_OVER; syntax is different from the OG class

    const int WIN_SCORE = 5; // the score from 5

    ///////////////////////////////////////////////////////////////////////////////////////////

    cout << "Client running...\n";

    ///////////////////////////////////////////////////////////////////////////////////////////
    // TCP NETWORKING STARTS HERE
    // WSAStartup = starts Winsock library so sockets can work on Windows

    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        cout << "WSAStartup failed\n";
        return 1;
    }

    // Create TCP socket
    // AF_INET = IPv4
    // SOCK_STREAM = TCP
    // IPPROTO_TCP = TCP protocol

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (clientSocket == INVALID_SOCKET)
    {
        cout << "Socket creation failed. Error: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    // Server address
    // 127.0.0.1 means same PC / localhost
    // لو السيرفر على نفس جهازك خليه كده
    // لو السيرفر على جهاز تاني، اكتب IP بتاع الجهاز التاني

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(Protocol::DefaultPort);

    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    // TCP must connect first before sending anything
    // UDP كان بيستخدم sendto
    // TCP بيستخدم connect مرة واحدة وبعدها send / recv

    cout << "Connecting to server...\n";

    if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
    {
        cout << "Connect failed. Error: " << WSAGetLastError() << "\n";
        cout << "Make sure Server.cpp is running first on port " << Protocol::DefaultPort << "\n";

        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    cout << "Connected to server!\n";

    // Make socket non-blocking after connect
    // يعني البرنامج مش هيقف يستنى رسالة
    // لو مفيش رسالة، يكمل عادي

    u_long mode = 1;
    ioctlsocket(clientSocket, FIONBIO, &mode);

    ///////////////////////////////////////////////////////////////////////////////////////////

    bool gameRunning = true;
    int moveDirection = 0; // the player idle state or no movement

    ///////////////////////////////////////////////////////////////////////////////////////////

    Protocol::PlayerInput input;
    Protocol::GameState SCORE_OF_GAME;

    ///////////////////////////////////////////////////////////////////////////////////////////

    DWORD lastSendTime = GetTickCount(); // gives current time in milliseconds --> THE GetTickCount (premade)
    const DWORD sendInterval = 50;       // البرنامج بيقرأ الزرار فورًا بدون تأخير

    // THATS THE DELAY TO THE SERVER
    // every 50 ms we send the input to the server

    ///////////////////////////////////////////////////////////////////////////////////////////

    while (gameRunning)
    {
        if (state == MatchState::IN_PROGRESS)
        {
            input.up = false;
            input.down = false;

            // so to refresh the keyboard input , good for best performance

            bool wPressed = (GetAsyncKeyState('W') & 0x8000) != 0;
            bool upArrowPressed = (GetAsyncKeyState(VK_UP) & 0x8000) != 0;

            bool sPressed = (GetAsyncKeyState('S') & 0x8000) != 0;
            bool downArrowPressed = (GetAsyncKeyState(VK_DOWN) & 0x8000) != 0;

            if (wPressed || upArrowPressed)
            {
                input.up = true;
            }

            /*
                if pressed W or UP arrow we add hexadecimal called holding behavior.
                there is another code as & 0x0001.

                & 0x8000 means: key is being held down now.
                & 0x0001 means: key was pressed once since last check.

                and if we didnt add it , it may act weird
            */

            else if (sPressed || downArrowPressed)
            {
                input.down = true;
            }
            else if ((GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0)
            {
                gameRunning = false;
            }

            // read keyboard & send input
        }
        else if (state == MatchState::WAITING)
        {
            cout << "Waiting for opponent...\n";

            // Just for testing so you can enter game state
            // Later the server should control this.

            if ((GetAsyncKeyState(VK_SPACE) & 0x8000) != 0) // ده مفتاح البدايه اول ما اللعبه تعمل run
            {
                state = MatchState::IN_PROGRESS;
                cout << "Game started!\n";
            }

            if ((GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0)
            {
                gameRunning = false;
            }
        }
        else if (state == MatchState::GAME_OVER)
        {
            cout << "Game Over! Score: " << SCORE_OF_GAME.leftScore << "\n";
            cout << "Game Over! Score: " << SCORE_OF_GAME.rightScore << "\n";
            cout << "Press R to play again or ESC to quit\n";

            if ((GetAsyncKeyState('R') & 0x8000) != 0)
            {
                // tell server this player wants a rematch

                state = MatchState::RESET;
                // GO TO LINE (state == MatchState::RESET)
            }
            else if ((GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0)
            {
                gameRunning = false;
            }

            // wait for RESET signal from server
        }
        else if (state == MatchState::RESET)
        {
            // clear scores and positions locally

            SCORE_OF_GAME.leftScore = 0;
            SCORE_OF_GAME.rightScore = 0;

            moveDirection = 0;
            input.up = false;
            input.down = false;

            // مفيش حركه فوق او تحت كله ثابت حتى بدايه الجيم الجاي

            cout << "Restarting...\n";

            state = MatchState::WAITING;
        }

        // direction

        if (input.up == true && input.down == false)
        {
            moveDirection = 1; // move up
        }
        else if (input.down == true && input.up == false)
        {
            moveDirection = -1; // move down
        }
        else
        {
            moveDirection = 0; // no movement
        }

        // step 3: send input every 50 ms

        DWORD currentTime = GetTickCount();

        if (currentTime - lastSendTime >= sendInterval)
        {
            lastSendTime = currentTime;

            ///////////////////////////////////////////////////////////////////////////////////
            // REAL TCP SEND
            // UDP used sendto()
            // TCP uses send()
            //
            // هنا بقى بدل comment "Later send input"
            // we send the input packet to the connected server

            Protocol::InputPacket inputPacket;
            inputPacket.type = Protocol::PacketType::Input;
            inputPacket.input = input;

            int bytesSent = send(
                clientSocket,
                reinterpret_cast<const char*>(&inputPacket),
                sizeof(inputPacket),
                0
            );

            if (bytesSent == SOCKET_ERROR)
            {
                int error = WSAGetLastError();

                // WSAEWOULDBLOCK means socket is not ready right now
                // ده عادي مع non-blocking socket

                if (error != WSAEWOULDBLOCK)
                {
                    cout << "send failed. Error: " << error << "\n";
                    gameRunning = false;
                }
            }

            ///////////////////////////////////////////////////////////////////////////////////
        }

        /*
            Purpose of this whole thing:
            To control how often you send input to the server.
        */

        ///////////////////////////////////////////////////////////////////////////////////////
        // REAL TCP RECEIVE
        // If server sends GameState, client receives it here

        Protocol::StatePacket statePacket;

        int bytesReceived = recv(
            clientSocket,
            reinterpret_cast<char*>(&statePacket),
            sizeof(statePacket),
            0
        );

        if (bytesReceived == sizeof(statePacket) &&
            statePacket.type == Protocol::PacketType::State)
        {
            SCORE_OF_GAME = statePacket.state;
        }
        else if (bytesReceived == 0)
        {
            cout << "Server disconnected\n";
            gameRunning = false;
        }
        else if (bytesReceived == SOCKET_ERROR)
        {
            int error = WSAGetLastError();

            // WSAEWOULDBLOCK means no data arrived right now
            // عادي جدا لأننا عاملين socket non-blocking

            if (error != WSAEWOULDBLOCK)
            {
                cout << "recv failed. Error: " << error << "\n";
                gameRunning = false;
            }
        }

        ///////////////////////////////////////////////////////////////////////////////////////

        //////////////////////////////
        // 🔹 Print to test

        cout << "Up: " << input.up
            << " Down: " << input.down
            << " moving direction " << moveDirection
            << " | BallX: " << SCORE_OF_GAME.ballX
            << " BallY: " << SCORE_OF_GAME.ballY
            << " Left Paddle: " << SCORE_OF_GAME.leftPaddleY
            << " Right Paddle: " << SCORE_OF_GAME.rightPaddleY
            << "\n";

        Sleep(16); // slow down output just for testing
    }

    // 1) read keyboard تم
    // 2) update paddle movement immediately تم
    // 3) every fixed interval, send input state to server تم
    // 4) receive game state from server تم
    // 5) render
    //
    //
    // player 1 uses W/S
    // player 2 uses ↑ / ↓

    closesocket(clientSocket);
    WSACleanup();

    return 0;
}

/*
    GetTickCount()
    returns time in milliseconds

    DWORD ≈ unsigned int
*/