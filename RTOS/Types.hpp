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

struct Thread;
// ─── Enums ───────────────────────────────────────────────────────────────────

// Controls how deadline_missed feeds into wagedPriority for soft-RT tasks.
//
//   NONE:            wagedPriority = (10 - priority) * (starvation + 1)
//                    + deadline_missed * deadlinePenalty   (flat additive)
//
//   STARVATION_FOLD: wagedPriority = (10 - priority) * (starvation + deadline_missed + 1)
//                    deadline folds into multiplier — linear recovery
//
//   ENHANCED:        wagedPriority = (10 - priority) * (starvation + 1)
//                    + (deadline_missed * deadline_missed) * deadlinePenalty
//                    quadratic scaling — aggressive recovery under sustained pressure
//
enum class DeadlineBoostMode : uint8_t
{
    NONE,
    STARVATION_FOLD,
    ENHANCED,
};

// Controls which frames low-prio tasks participate in the guaranteed pass (phase 3).
//
//   FILL_ONLY:    low-prio never enters the guaranteed pass — fill phase only
//   IDLE_FRAMES:  guaranteed pass in frames 2 and 4 (idle system slots)
//   ALL_FRAMES:   guaranteed pass in every frame alongside soft-RT and normal
//
enum class LowPrioCompeteMode : uint8_t
{
    FILL_ONLY,
    IDLE_FRAMES,
    ALL_FRAMES,
};

struct Resource
{
    Thread* owner{nullptr};
    uint8_t slotOccupancy{0};
};

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
    SOFT_RT,
    NORMAL,
    LOW_PRIO
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

enum class TaskClass : uint8_t
{
    HARD_RT = 0,
    SYSTEM = 1,
    NORMAL = 2,
    LOW_PRIO = 3,
};

enum class SchedulerMode : uint8_t
{
    AUTONOMOUS,
    SUPERVISED
};

} // namespace core
