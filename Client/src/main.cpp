#include <iostream>
#include <Windows.h>   // for GetAsyncKeyState
#include "../../Common/include/protocol.h"


using namespace std;
int main()
{
  cout << "Client running...\n";
    bool gameRunning = true;
    int moveDirection = 0; // the player idle state or no movement
    Protocol::PlayerInput input;

    DWORD lastSendTime = GetTickCount();//gives current time in milliseconds --> THE GEtTICKCOUNT (premade)
    const DWORD sendInterval = 50; // البرنامج بيقرأ الزرار فورًا بدون تأخير 
    // THATS THE DELAY TO THE SERVER  

    while (gameRunning)
    {
        input.up = false;
        input.down = false;
        // so to refresh the keyboard input , good for best performance
        if (input.up = (GetAsyncKeyState('W') & 0x8000) != 0 || (GetAsyncKeyState(VK_UP) & 0x8000)) {

           input.up = true;

        }
        /* if pressed w we have to add hexadecimal  called holding behavior there is another code as & 0x0001
        and if we didnt add it , it may act weird 
        */
        else if(input.down = (GetAsyncKeyState('S') & 0x8000) != 0 ||( GetAsyncKeyState(VK_DOWN) & 0x8000)) {

            input.down = true;


        }
        else if ((GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0) {

            gameRunning = false;
            //

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
      
        // 1) read keyboard تم
        // 2) update paddle movement immediately تم
        // 3) every fixed interval, send input state to server
        // 4) render
        // 
        // 
        //player 1 uses W/S
        //player 2 uses ↑ / ↓
    }


    return 0;
}





/*GetTickCount()

returns time in milliseconds

DWORD ≈ unsigned int
*/