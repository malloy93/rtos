#pragma once

#include <cstdint>

namespace telemetry
{

enum class MessageId : uint16_t
{
    INVALID = 0x0000,
    LOG_TEXT = 0x8000,
};

constexpr bool isValidMessageId(MessageId id)
{
    const auto value = static_cast<uint16_t>(id);
    return value != 0x0000U && value != 0xFFFFU;
}

} // namespace telemetry
