#pragma once
#include <cstdint>
#include <iostream>
#include <sstream>
#include "Logger.hpp"
namespace kernel
{
class Logger;
struct Thread
{
    Thread() = default;
    Thread(void (*threadPointer)(void) &) : threadPointer{threadPointer} {}
    Thread(void (*threadPointer)(void) &, uint16_t threadId, Logger* logger)
        : threadPointer{threadPointer}, threadId{threadId}, logger{logger}
    {
    }
    int32_t* stackPtr;
    Thread* nextPtr;
    void (*threadPointer)(void);

    Thread* getNextPtr()
    {
        // logSizeChange();
        // logLocalInfo();
        // nextPtr->logLocalInfo();
        // logger->logDebug("  ");
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
    Logger* logger;
    uint8_t priority{0};
};

std::ostream& operator<<(std::ostream& os, const Thread& thread);
} // namespace kernel