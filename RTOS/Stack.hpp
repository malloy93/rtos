#pragma once

namespace kernel
{

struct Stack
{
    Stack(Logger& logger, const uint16_t& stackId) : logger{logger}, stackId{stackId}
    {
        logger.logDebug("Creating new stack memory. Stack ID: &d", stackId);
        stackMemory = new int32_t[1000]; // move stack init from constructor into factory method
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
};

} // namespace kernel