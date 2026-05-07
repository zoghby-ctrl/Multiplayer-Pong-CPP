#pragma once

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

namespace Net
{
    inline bool SendAll(SOCKET socketHandle, const char* data, int length)
    {
        int totalSent = 0;
        while (totalSent < length)
        {
            int sent = send(socketHandle, data + totalSent, length - totalSent, 0);
            if (sent == SOCKET_ERROR || sent == 0)
                return false;
            totalSent += sent;
        }
        return true;
    }

    inline bool RecvAll(SOCKET socketHandle, char* data, int length)
    {
        int totalReceived = 0;
        while (totalReceived < length)
        {
            int received = recv(socketHandle, data + totalReceived, length - totalReceived, 0);
            if (received == SOCKET_ERROR || received == 0)
                return false;
            totalReceived += received;
        }
        return true;
    }

    template <typename T>
    inline bool SendPacket(SOCKET socketHandle, const T& packet)
    {
        return SendAll(socketHandle, reinterpret_cast<const char*>(&packet), static_cast<int>(sizeof(T)));
    }

    template <typename T>
    inline bool RecvPacket(SOCKET socketHandle, T& packet)
    {
        return RecvAll(socketHandle, reinterpret_cast<char*>(&packet), static_cast<int>(sizeof(T)));
    }
}
