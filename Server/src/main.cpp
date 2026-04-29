#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h> // for Sleep
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

    cout << "Server running...\n";

    ///////////////////////////////////////////////////////////////////////////////////////////
    // TCP NETWORKING STARTS HERE
    // WSAStartup = starts Winsock library so sockets can work on Windows

    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        cout << "WSAStartup failed\n";
        return 1;
    }

    // Create TCP listening socket
    // AF_INET = IPv4
    // SOCK_STREAM = TCP
    // IPPROTO_TCP = TCP protocol

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (listenSocket == INVALID_SOCKET)
    {
        cout << "Socket creation failed. Error: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    // Server address
    // INADDR_ANY means accept connections from any IP on this PC

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(Protocol::DefaultPort);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // bind = attach the socket to port 7777
    // يعني السيرفر يسمع على البورت ده

    if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
    {
        cout << "Bind failed. Error: " << WSAGetLastError() << "\n";

        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // listen = start waiting for clients
    // TCP لازم server يعمل listen قبل accept

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        cout << "Listen failed. Error: " << WSAGetLastError() << "\n";

        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    cout << "Waiting for client...\n";

    // accept = wait until client connects
    // هنا البرنامج هيستنى client يعمل connect

    SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);

    if (clientSocket == INVALID_SOCKET)
    {
        cout << "Accept failed. Error: " << WSAGetLastError() << "\n";

        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    cout << "Client connected!\n";

    // We do not need listenSocket anymore after one client connected
    // عشان احنا بنعمل client واحد بس دلوقتي

    closesocket(listenSocket);

    // Make client socket non-blocking
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

    // Initial game state
    SCORE_OF_GAME.ballX = 400;
    SCORE_OF_GAME.ballY = 300;
    SCORE_OF_GAME.leftPaddleY = 250;
    SCORE_OF_GAME.rightPaddleY = 250;
    SCORE_OF_GAME.leftScore = 0;
    SCORE_OF_GAME.rightScore = 0;

    ///////////////////////////////////////////////////////////////////////////////////////////

    DWORD lastSendTime = GetTickCount(); // gives current time in milliseconds --> THE GetTickCount (premade)
    const DWORD sendInterval = 16;       // server sends state about 60 FPS

    ///////////////////////////////////////////////////////////////////////////////////////////

    while (gameRunning)
    {
        ///////////////////////////////////////////////////////////////////////////////////////
        // REAL TCP RECEIVE
        // Server receives input packet from client

        Protocol::InputPacket inputPacket;

        int bytesReceived = recv(
            clientSocket,
            reinterpret_cast<char*>(&inputPacket),
            sizeof(inputPacket),
            0
        );

        if (bytesReceived == sizeof(inputPacket) &&
            inputPacket.type == Protocol::PacketType::Input)
        {
            input = inputPacket.input;

            if (state == MatchState::WAITING)
            {
                state = MatchState::IN_PROGRESS;
                cout << "Game started from server side!\n";
            }
        }
        else if (bytesReceived == 0)
        {
            cout << "Client disconnected\n";
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

        if (state == MatchState::IN_PROGRESS)
        {
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

            // update paddle movement immediately

            if (moveDirection == 1)
            {
                SCORE_OF_GAME.leftPaddleY -= 5;
            }
            else if (moveDirection == -1)
            {
                SCORE_OF_GAME.leftPaddleY += 5;
            }

            // simple boundaries so paddle doesnt leave the screen

            if (SCORE_OF_GAME.leftPaddleY < 0)
            {
                SCORE_OF_GAME.leftPaddleY = 0;
            }

            if (SCORE_OF_GAME.leftPaddleY > 500)
            {
                SCORE_OF_GAME.leftPaddleY = 500;
            }

            // fake ball movement just to prove state packets return to client
            // later replace this with your real pong ball logic

            SCORE_OF_GAME.ballX += 2;

            if (SCORE_OF_GAME.ballX > 800)
            {
                SCORE_OF_GAME.ballX = 0;
            }

            // Game over test

            if (SCORE_OF_GAME.leftScore >= WIN_SCORE || SCORE_OF_GAME.rightScore >= WIN_SCORE)
            {
                state = MatchState::GAME_OVER;
            }
        }
        else if (state == MatchState::GAME_OVER)
        {
            cout << "Game Over! Score: " << SCORE_OF_GAME.leftScore << "\n";
            cout << "Game Over! Score: " << SCORE_OF_GAME.rightScore << "\n";

            // wait for RESET signal from client later
        }
        else if (state == MatchState::RESET)
        {
            // clear scores and positions locally

            SCORE_OF_GAME.leftScore = 0;
            SCORE_OF_GAME.rightScore = 0;

            SCORE_OF_GAME.ballX = 400;
            SCORE_OF_GAME.ballY = 300;
            SCORE_OF_GAME.leftPaddleY = 250;
            SCORE_OF_GAME.rightPaddleY = 250;

            moveDirection = 0;
            input.up = false;
            input.down = false;

            // مفيش حركه فوق او تحت كله ثابت حتى بدايه الجيم الجاي

            cout << "Restarting...\n";

            state = MatchState::WAITING;
        }

        // step 3: send state every 16 ms

        DWORD currentTime = GetTickCount();

        if (currentTime - lastSendTime >= sendInterval)
        {
            lastSendTime = currentTime;

            ///////////////////////////////////////////////////////////////////////////////////
            // REAL TCP SEND
            // Server sends game state back to client

            Protocol::StatePacket statePacket;
            statePacket.type = Protocol::PacketType::State;
            statePacket.state = SCORE_OF_GAME;

            int bytesSent = send(
                clientSocket,
                reinterpret_cast<const char*>(&statePacket),
                sizeof(statePacket),
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
            To control how often you send state to the client.
        */

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

    closesocket(clientSocket);
    WSACleanup();

    return 0;
}

/*
    GetTickCount()
    returns time in milliseconds

    DWORD ≈ unsigned int
*/