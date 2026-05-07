#pragma once

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <string>

#include "../Common/Protocol.h"

class NetClient
{
public:
    bool Connect(const char* hostIp, unsigned short port);
    void Disconnect();

    void SendInput(bool moveUp, bool moveDown);
    bool IsConnected() const { return m_connected; }
    int GetPlayerId() const { return m_playerId; }
    Net::StateSnapshotPacket GetLatestSnapshot() const;
    std::string GetHostString() const { return m_hostIp; }

private:
    void ReceiveLoop();

private:
    SOCKET m_socket = INVALID_SOCKET;
    std::thread m_recvThread;
    std::atomic_bool m_connected = false;
    std::atomic_int m_playerId = 0;
    mutable std::mutex m_snapshotMutex;
    Net::StateSnapshotPacket m_latestSnapshot = {};
    std::string m_hostIp;
};
