#pragma once

namespace core
{
using threadId = uint8_t;
using threadPriority = uint8_t;
using threadStackSize = uint16_t;
using threadStackPtr = int32_t*;

uint16_t constexpr defaultStackSize{1000u};

enum class ErrorCode
{
    SUCCESS = 0,
    ERROR = 1,
    TIMEOUT = 2,
    INVALID_PARAMETER = 3,
    OUT_OF_MEMORY = 4

};

enum class ThreadState
{
    READY,
    RUNNING,
    BLOCKED,
    SUSPENDED,
    TERMINATED
};

enum class SchedulerType
{
    ROUND_ROBIN,
    PRIORITY_BASED,
    PRIORITY_WAGED
};

} // namespace core
