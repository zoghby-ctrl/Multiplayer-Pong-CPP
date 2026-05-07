#include "NetClient.h"
#include "../Common/SocketHelpers.h"

bool NetClient::Connect(const char* hostIp, unsigned short port)
{
    WSADATA wsaData = {};
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return false;

    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET)
        return false;

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    inet_pton(AF_INET, hostIp, &serverAddr.sin_addr);
    serverAddr.sin_port = htons(port);

    if (connect(m_socket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
    {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        WSACleanup();
        return false;
    }

    Net::JoinRequestPacket joinRequest = {};
    if (!Net::SendPacket(m_socket, joinRequest))
    {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        WSACleanup();
        return false;
    }

    Net::JoinAcceptPacket acceptPacket = {};
    if (!Net::RecvPacket(m_socket, acceptPacket) || acceptPacket.type != static_cast<std::uint32_t>(Net::PacketType::JoinAccept))
    {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        WSACleanup();
        return false;
    }

    m_playerId = static_cast<int>(acceptPacket.playerId);
    m_connected = true;
    m_hostIp = hostIp;
    m_recvThread = std::thread(&NetClient::ReceiveLoop, this);
    return true;
}

void NetClient::Disconnect()
{
    m_connected = false;
    if (m_socket != INVALID_SOCKET)
    {
        shutdown(m_socket, SD_BOTH);
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
    if (m_recvThread.joinable())
        m_recvThread.join();
    WSACleanup();
}

void NetClient::SendInput(bool moveUp, bool moveDown)
{
    if (!m_connected)
        return;

    Net::InputPacket packet = {};
    packet.playerId = static_cast<std::uint32_t>(m_playerId.load());
    packet.moveUp = moveUp ? 1 : 0;
    packet.moveDown = moveDown ? 1 : 0;

    if (!Net::SendPacket(m_socket, packet))
        m_connected = false;
}

Net::StateSnapshotPacket NetClient::GetLatestSnapshot() const
{
    std::lock_guard<std::mutex> lock(m_snapshotMutex);
    return m_latestSnapshot;
}

void NetClient::ReceiveLoop()
{
    while (m_connected)
    {
        Net::StateSnapshotPacket snapshot = {};
        if (!Net::RecvPacket(m_socket, snapshot) || snapshot.type != static_cast<std::uint32_t>(Net::PacketType::State))
        {
            m_connected = false;
            break;
        }

        std::lock_guard<std::mutex> lock(m_snapshotMutex);
        m_latestSnapshot = snapshot;
    }
}
