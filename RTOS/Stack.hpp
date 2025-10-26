#pragma once
#include "Types.hpp"
namespace kernel
{

class MemoryPool
{
};

struct Stack
{
    Stack(Logger& logger, const uint16_t& stackId) : logger{logger}, stackId{stackId}
    {
        // max stacks = 47
        logger.logDebug("Creating new stack memory. Stack ID: %d", stackId);
        stackMemory = new int32_t[stackSize]; // move stack init from constructor into factory method
    }
    Stack(const Stack&) = delete;
    Stack& operator=(const Stack&) = delete;

    int32_t& operator[](int16_t index) { return stackMemory[index]; }

    ~Stack()
    {
        logger.logDebug("Deleting stack memory. Stack ID: %d", stackId);
        delete[] stackMemory;
    }

private:
    Logger& logger;
    const uint16_t& stackId;
    int32_t* stackMemory;
    threadStackSize stackSize{defaultStackSize};
    bool isUsed{false};
};

} // namespace kernel