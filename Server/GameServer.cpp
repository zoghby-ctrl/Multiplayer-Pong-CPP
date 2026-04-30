#include "GameServer.h"

#include <algorithm>

GameServer::GameServer()
    : gameMode_(std::make_unique<Pong::ClassicPongMode>()) {
    ResetGame();
}

void GameServer::Tick() {
    gameMode_->Step(state_, inputs_, connectedPlayers_);
    ++state_.tick;
}

void GameServer::ResetGame() {
    inputs_.fill({});
    gameMode_->Reset(state_);
}

void GameServer::SetDifficulty(Protocol::Difficulty difficulty) {
    state_.difficulty = difficulty;
    ResetGame();
}

void GameServer::SetPlayerConnected(std::size_t playerId, bool connected) {
    if (playerId >= Protocol::MaxPlayers) {
        return;
    }

    connectedPlayers_[playerId] = connected;
    inputs_[playerId] = {};

    if (ConnectedPlayerCount() == 0) {
        ResetGame();
    }
}

void GameServer::ProcessClientPacket(std::size_t playerId, const Protocol::Packet& packet) {
    if (playerId >= Protocol::MaxPlayers) {
        return;
    }

    if (packet.header.type == Protocol::PacketType::Disconnect) {
        SetPlayerConnected(playerId, false);
        return;
    }

    if (packet.header.type == Protocol::PacketType::Command) {
        ProcessCommand(packet.payload.command);
        return;
    }

    if (packet.header.type == Protocol::PacketType::Input) {
        inputs_[playerId] = packet.payload.input;
    }
}

void GameServer::ProcessCommand(const Protocol::ClientCommand& command) {
    if (command.type == Protocol::ClientCommandType::ResetMatch) {
        ResetGame();
        return;
    }

    if (command.type == Protocol::ClientCommandType::SetDifficulty) {
        SetDifficulty(command.difficulty);
    }
}

Protocol::Packet GameServer::BuildStatePacket(uint32_t sequence) const {
    Protocol::Packet packet{};
    packet.header.type = Protocol::PacketType::State;
    packet.header.seq = sequence;
    packet.payload.state = state_;
    return packet;
}

Protocol::Packet GameServer::BuildWelcomePacket(std::size_t playerId, uint32_t sequence) const {
    Protocol::Packet packet{};
    packet.header.type = Protocol::PacketType::Welcome;
    packet.header.seq = sequence;
    packet.payload.welcome.playerId = static_cast<uint8_t>(std::min<std::size_t>(playerId, Protocol::MaxPlayers - 1));
    packet.payload.welcome.playerCount = Protocol::MaxPlayers;
    return packet;
}

bool GameServer::IsPlayerConnected(std::size_t playerId) const {
    return playerId < Protocol::MaxPlayers && connectedPlayers_[playerId];
}

std::size_t GameServer::ConnectedPlayerCount() const {
    return static_cast<std::size_t>(std::count(connectedPlayers_.begin(), connectedPlayers_.end(), true));
}
