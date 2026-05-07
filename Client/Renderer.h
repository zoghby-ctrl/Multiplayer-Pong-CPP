#pragma once

#include <windows.h>
#include <string>
#include "../Common/Protocol.h"

bool Renderer_Initialize(HDC hdc);
void Renderer_Shutdown();
void Renderer_Render(const Net::StateSnapshotPacket& snapshot, bool connected, int playerId, const std::string& hostIp, int width, int height, HDC hdc);
