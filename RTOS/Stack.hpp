#pragma once
#include "Types.hpp"
namespace core
{

constexpr uint32_t freeCanaryValue{0xDEADBEEF};
constexpr uint32_t usedCanaryValue{0xBEEFDEAD};
constexpr uint32_t corruptedCanaryValue{0xBAADF00D};

using StackId = uint8_t;

struct MemConfig
{
    constexpr static uint8_t _1kBPolls{24};
    constexpr static uint8_t _2kBPolls{12};
    constexpr static uint8_t _4kBPolls{6};
    constexpr static uint8_t _8kBPolls{3};
    constexpr static uint32_t poolSize{24576}; // size in kB (96kB)
    // constexpr static uint32_t poolSize{1024}; // size in kB (96kB)
};

enum class StackSize : uint16_t
{
    SIZE_1kB = 256,
    SIZE_2kB = 512,
    SIZE_4kB = 1024,
    SIZE_8kB = 2048
};

struct Stack
{
    uint32_t* baseAddress{nullptr};
    uint32_t* stackPointer{nullptr};
    uint32_t* endAddress{nullptr};
    uint32_t canaryOffset{0};
    StackSize size{};
    StackId stackId{invalidStackId};
};

class MemoryPool
{
public:
    void createPool()
    {
        uint32_t* currentAddress = pool;
        uint8_t stackId{0};
        for (stackId = 0; stackId < config._1kBPolls; ++stackId)
        {
            LOG_DEBUG("Creating 1kB pool %d, address: %p", stackId, currentAddress);
            fillPool(totalPools[stackId], StackSize::SIZE_1kB, currentAddress);
            totalPools[stackId].stackId = stackId;
        }
        uint8_t secondIndex{stackId};
        for (; stackId < (config._2kBPolls + secondIndex); ++stackId)
        {
            LOG_DEBUG("Creating 2kB pool %d, address: %p", stackId, currentAddress);
            fillPool(totalPools[stackId], StackSize::SIZE_2kB, currentAddress);
            totalPools[stackId].stackId = stackId;
        }
        secondIndex = stackId;
        for (; stackId < (config._4kBPolls + secondIndex); ++stackId)
        {
            LOG_DEBUG("Creating 4kB pool %d, address: %p", stackId, currentAddress);
            fillPool(totalPools[stackId], StackSize::SIZE_4kB, currentAddress);
            totalPools[stackId].stackId = stackId;
        }
        secondIndex = stackId;
        for (; stackId < (config._8kBPolls + secondIndex); ++stackId)
        {
            LOG_DEBUG("Creating 8kB pool %d, address: %p", stackId, currentAddress);
            fillPool(totalPools[stackId], StackSize::SIZE_8kB, currentAddress);
            totalPools[stackId].stackId = stackId;
        }
    }

    StackId allocateStack(StackSize size)
    {
        // NewStack candidateStack = 0;
        for (auto& pool : totalPools)
        {
            if (pool.size == size and (*(pool.endAddress + 1) == freeCanaryValue))
            {
                LOG_DEBUG("Allocating stack at address: %p, stack ID: %d", pool.baseAddress, pool.stackId);
                *(pool.endAddress + 1) = usedCanaryValue;

                // candidateStack = pool.stackPointer;
                return pool.stackId;
            }
        }
        return invalidStackId;
    }

    Stack* getStack(StackId id)
    {
        if (id >= 45) return nullptr;
        return &totalPools[id];
    }

    bool deallocateStack(StackId id)
    {
        LOG_DEBUG("Deallocating stack with ID: %d", id);
        if (id >= 45) return false;
        Stack& pool = totalPools[id];
        LOG_DEBUG("Deallocating stack at address: %p", pool.baseAddress);
        // zero out stack memory
        zeroStackMemory(id);
        *(pool.endAddress + 1) = freeCanaryValue;
        pool.stackPointer = pool.endAddress; // reset stackPointer do początku regionu
        return true;
    }

private:
    void fillPool(auto& pool, StackSize size, uint32_t*& currentAddress)
    {
        pool.endAddress = currentAddress;
        pool.baseAddress = currentAddress + static_cast<uint32_t>(size); // za duzy o 1
        pool.stackPointer = currentAddress;
        pool.canaryOffset = 1;
        pool.size = size;
        *(pool.endAddress + 1) = freeCanaryValue;
        currentAddress += static_cast<uint32_t>(size);
    }

    void zeroStackMemory(StackId id)
    {
        LOG_DEBUG("Zeroing stack memory for stack ID: %d", id);
        if (id >= 45) return;
        Stack& pool = totalPools[id];
        if (pool.endAddress == nullptr || pool.baseAddress == nullptr) return;

        uint32_t* start = pool.endAddress;
        uint32_t* end = pool.baseAddress;

        // iteruj od endAddress do baseAddress i zeruj każde słowo
        while (start <= end)
        {
            *start = 0;
            ++start;
        }
        LOG_DEBUG("Stack memory zeroed for stack ID: %d", id);
    }

    constexpr static MemConfig config{};
    Stack totalPools[45];
    uint32_t pool[config.poolSize] = {0}; // size in kB (96kB)
    bool isCCRAM{false};
    bool isSDRAM{false};
};

} // namespace core