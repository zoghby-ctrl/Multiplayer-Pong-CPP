#pragma once

namespace Common
{
    constexpr int DefaultPort = 7777;

    enum class PacketType
    {
        Unknown = 0,
        Input,
        StateSnapshot
    };
}
