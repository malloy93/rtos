#pragma once
#include <cstdint>
#include <iostream>
#include <sstream>
#include "Logger.hpp"

#include "Types.hpp"

namespace core
{
class Logger;
struct Thread
{
    Thread() = default;
    Thread(void (*threadPointer)(void) &) : threadPointer{threadPointer} {}
    Thread(void (*threadPointer)(void) &, uint16_t threadId) : threadPointer{threadPointer}, threadId{threadId} {}
    uint32_t* stackPtr;
    Thread* nextPtr;
    void (*threadPointer)(void);

    Thread* getNextPtr()
    {
        // logSizeChange();
        // logLocalInfo();
        // nextPtr->logLocalInfo();
        // LOG_DEBUG("  ");
        return nextPtr;
    }

    const char* printThreadInfo();

    void logLocalInfo();
    void setStackPtr(uint32_t ptr) { startPtr = ptr; }
    void logSizeChange();
    void calculateWagedPriority() { wagedPriority = (10 - priority) * (starvationCounter + 1); }

    const uint16_t& getThreadId() const { return threadId; }

    void setStackId(uint8_t id)
    {
        LOG_DEBUG("Stack ID: %d", id);
        allocatedStackId = id;
    }
    uint8_t getStackId() const
    {
        LOG_DEBUG("Stack ID: %d", allocatedStackId);
        return allocatedStackId;
    }
    uint32_t startPtr;
    void* threadArgs{nullptr};

    const uint16_t threadId{0};
    ThreadState state{ThreadState::READY};

    uint8_t priority{1};
    uint16_t starvationCounter{0};
    uint16_t wagedPriority{0};
    uint8_t minResourceSlice{1};
    bool isAllocated{false};

    uint8_t allocatedStackId{invalidStackId};
};

std::ostream& operator<<(std::ostream& os, const Thread& thread);
} // namespace core