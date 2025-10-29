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
    int32_t* stackPtr;
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

    const uint16_t& getThreadId() const { return threadId; }

private:
    uint32_t startPtr;
    const uint16_t threadId{0};
    uint8_t priority{0};
    ThreadState state{ThreadState::READY};
};

std::ostream& operator<<(std::ostream& os, const Thread& thread);
} // namespace core