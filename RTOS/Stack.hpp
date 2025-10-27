#pragma once
#include "Types.hpp"
namespace kernel
{

class MemoryPool
{
};

struct Stack
{
    Stack(const uint16_t stackId) : stackId{stackId}
    {
        stackMemory = new int32_t[stackSize]; // move stack init from constructor into factory method
    }
    Stack(const Stack&) = delete;
    Stack& operator=(const Stack&) = delete;

    int32_t& operator[](int16_t index) { return stackMemory[index]; }

    ~Stack() { delete[] stackMemory; }

private:
    const uint16_t stackId;
    int32_t* stackMemory;
    threadStackSize stackSize{defaultStackSize};
    bool isUsed{false};
};

} // namespace kernel