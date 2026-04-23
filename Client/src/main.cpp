#include <iostream>
#include <Windows.h>   // for GetAsyncKeyState
#include "../../Common/include/protocol.h"
using namespace std;
enum class MatchState {
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
    //state = MatchState::GAME_OVER; syntax is differ from the the OG class
    
    const int WIN_SCORE = 5;// the score from 5
    ///////////////////////////////////////////////////////////////////////////////////////////
  cout << "Client running...\n";
    bool gameRunning = true;
    int moveDirection = 0; // the player idle state or no movement
    ///////////////////////////////////////////////////////////////////////////////////////////
    Protocol::PlayerInput input;
    Protocol::GameState SCORE_OF_GAME;
    ///////////////////////////////////////////////////////////////////////////////////////////
    DWORD lastSendTime = GetTickCount();//gives current time in milliseconds --> THE GEtTICKCOUNT (premade)
    const DWORD sendInterval = 50; // البرنامج بيقرأ الزرار فورًا بدون تأخير 
    // THATS THE DELAY TO THE SERVER  

    while (gameRunning)
    {
        if (state == MatchState::IN_PROGRESS) {
            input.up = false;
            input.down = false;
            // so to refresh the keyboard input , good for best performance
            if (input.up = (GetAsyncKeyState('W') & 0x8000) != 0 || (GetAsyncKeyState(VK_UP) & 0x8000)) {

                input.up = true;

            }
            /* if pressed w we have to add hexadecimal  called holding behavior there is another code as & 0x0001
            and if we didnt add it , it may act weird
            */
            else if (input.down = (GetAsyncKeyState('S') & 0x8000) != 0 || (GetAsyncKeyState(VK_DOWN) & 0x8000)) {

                input.down = true;


            }
            else if ((GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0) {

                gameRunning = false;
            }

            // read keyboard & send input
            
        }
        else if (state == MatchState::WAITING) {
            cout << "Waiting for opponent...\n";
        }
        else if (state == MatchState::GAME_OVER) {
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
            // مفيش حركه فوق او تحت كله ثابت حتى بدايه القيم الجاي

            cout << "Restarting...\n";

            state = MatchState::WAITING;
        }
      

        // direction 

        if (input.up == true && input.down == false)
        {
            moveDirection = 1;   // move up
        }
        else if (input.down == true && input.up == false)
        {
            moveDirection = -1;  // move down
        }
        else
        {
            moveDirection = 0;   // no movement
        }

        // step 3: send input every 50 ms
        DWORD currentTime = GetTickCount();

        if (currentTime - lastSendTime >= sendInterval)
        {
            lastSendTime = currentTime;
        }

        /* Purpose of this whole thing

 To control how often you send input to the server*/
        //////////////////////////////
        // 🔹 Print to test
        cout << "Up: " << input.up << " Down: " << input.down << endl;
        cout << "moving direction "<< moveDirection;
        Sleep(16); // slow down output (just for testing)
    }
      
        // 1) read keyboard تم
        // 2) update paddle movement immediately تم
        // 3) every fixed interval, send input state to server
        // 4) render
        // 
        // 
        //player 1 uses W/S
        //player 2 uses ↑ / ↓
   


    return 0;
}





/*GetTickCount()

returns time in milliseconds

DWORD ≈ unsigned int
*/