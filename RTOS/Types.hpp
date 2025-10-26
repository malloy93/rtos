#pragma once


namespace kernel
{
using threadId = uint8_t;
using threadPriority = uint8_t;
using threadStackSize = uint16_t;
using threadStackPtr = int32_t*;


uint16_t constexpr defaultStackSize{1000u};

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
    PRIORITY_BASED
};





}

