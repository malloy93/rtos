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
    void createPool();

    StackId allocateStack(StackSize size);

    Stack* getStack(StackId id);

    bool deallocateStack(StackId id);

private:
    void fillPool(auto& pool, StackSize size, uint32_t*& currentAddress);
    void zeroStackMemory(StackId id);

    constexpr static MemConfig config{};
    Stack totalPools[45];
    uint32_t pool[config.poolSize] = {0}; // size in kB (96kB)
    bool isCCRAM{false};
    bool isSDRAM{false};
};

} // namespace core