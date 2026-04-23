#include "ServerUpdate.h"
#include <iostream>
using namespace std;

void testScore() {
    ServerUpdate server;

    for (int i = 0; i < 100; i++) {
        server.update(0.1f);
    }

    auto state = server.update(0.1f);

    cout << "Score: "
        << state.score[0]
        << " - "
        << state.score[1]
        << "\n";
}