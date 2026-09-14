#pragma once

#include <cstdint>

namespace core
{

enum class LogLevel : uint8_t
{
    KERNEL = 0,
    DEBUG = 1,
    INFO = 2,
    ERROR = 3,
    OFF = 4,
};

} // namespace core
