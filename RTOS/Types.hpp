#pragma once

#include <stdint.h>

#define SYSPRI3 (*((volatile uint32_t*)0xE000ED20))
#define INTCTRL (*((volatile uint32_t*)0xE000ED04))
namespace core
{
constexpr int stackSize{500};
constexpr uint8_t invalidStackId{255};
constexpr uint8_t invalidThreadId{255};

using threadId = uint8_t;
using threadPriority = uint8_t;
using threadStackSize = uint16_t;
using threadStackPtr = int32_t*;

uint16_t constexpr defaultStackSize{1000u};

enum : uint8_t
{
    SVC_ADDTASK = 0,
    SVC_REMOVETASK = 1,
    SVC_SUSPENDASK = 2,
    SVC_RESUMETASK = 3

};

enum class TaskType : uint8_t
{
    SYSTEM,
    HARD_RT,
    NORMAL
};

enum class ErrorCodes : uint8_t
{
    SUCCESS = 0,
    ERROR = 1,
    TIMEOUT = 2,
    INVALID_PARAMETER = 3,
    OUT_OF_MEMORY = 4,
    COM_FAILURE = 11
};

enum class ThreadState : uint8_t
{
    READY,
    RUNNING,
    BLOCKED,
    SUSPENDED,
    TERMINATED // requires stack allocation to be restored
};

enum class SchedulerType : uint8_t
{
    ROUND_ROBIN, // every thread gets equal time slices - 1 - 10 ms
    PRIORITY_BASED, // higher priority threads get more CPU time
    PRIORITY_WAGED, // calculete priority based on starvation and execution time
    RESOURCE_GRID, // planned execution based on 50ms resource grid
    ADAPTIVE_RESOURCE_GRID // dynamic adjustment of resource grid based on system load or external events
};

} // namespace core
