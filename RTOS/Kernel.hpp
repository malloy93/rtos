#pragma once
#include <map>
#include <stdint.h>
#include <vector>
#include "Logger.hpp"
#include "Scheduler.hpp"
#include "Stack.hpp"
#include "Thread.hpp"
#include "Utils.hpp"

#define SYSPRI3 (*((volatile uint32_t*)0xE000ED20))
#define INTCTRL (*((volatile uint32_t*)0xE000ED04))

namespace core
{
// API CALLS
extern "C" void changeContext();

enum : uint8_t
{
    SVC_ADDTASK = 0,
    SVC_REMOVETASK = 1,
    SVC_SUSPENDASK = 2,
    SVC_RESUMETASK = 3

};

using taskP = void (*)(void);

static inline void addTask(taskP taskPointer)
{
    register taskP r0 __asm("r0") = taskPointer; // r0 = adres funkcji (LSB=1 dla Thumb)
    __asm volatile("svc %[imm]"
                   : "+r"(r0) // r0 jest “żywe” po SVC (gdybyś chciał zwrot)
                   : [imm] "I"(SVC_ADDTASK)
                   : "memory");
    // __asm volatile("svc %[imm]" ::[imm] "I"(SVC_ADDTASK) : "memory");
}
static inline void removeTask(uint16_t threadId)
{
    register uint16_t r0 __asm("r0") = threadId; // r0 = adres funkcji (LSB=1 dla Thumb)
    __asm volatile("svc %[imm]"
                   : "+r"(r0) // r0 jest “żywe” po SVC (gdybyś chciał zwrot)
                   : [imm] "I"(SVC_REMOVETASK)
                   : "memory");
    // __asm volatile("svc %[imm]" ::[imm] "I"(SVC_ADDTASK) : "memory");
}

static inline void suspendTask(uint16_t threadId)
{
    register uint16_t r0 __asm("r0") = threadId; // r0 = adres funkcji (LSB=1 dla Thumb)
    __asm volatile("svc %[imm]"
                   : "+r"(r0) // r0 jest “żywe” po SVC (gdybyś chciał zwrot)
                   : [imm] "I"(SVC_SUSPENDASK)
                   : "memory");
    // __asm volatile("svc %[imm]" ::[imm] "I"(SVC_ADDTASK) : "memory");
}
static inline void resumeTask(uint16_t threadId)
{
    register uint16_t r0 __asm("r0") = threadId; // r0 = adres funkcji (LSB=1 dla Thumb)
    __asm volatile("svc %[imm]"
                   : "+r"(r0) // r0 jest “żywe” po SVC (gdybyś chciał zwrot)
                   : [imm] "I"(SVC_RESUMETASK)
                   : "memory");
    // __asm volatile("svc %[imm]" ::[imm] "I"(SVC_ADDTASK) : "memory");
}

// class Scheduler;
constexpr int stackSize{500};

class RTCore
{
public:
    static void init(utils::IdGen&);
    static RTCore* getInstance();

    uint8_t addThreads(std::vector<void (*)()>&);
    void launch(uint32_t);

    void logThreadInfo();

    uint16_t add(void (*)());
    void remove(uint16_t);
    void suspend(uint16_t);

    Thread* getNextThread()
    {
        switch (schedulerType)
        {
            case SchedulerType::ROUND_ROBIN:
                return getNextThreadRoundRobin();
            case SchedulerType::PRIORITY_BASED:
                return getNextThreadPriorityBased();
            default:
                return getNextThreadRoundRobin();
        }
    }

private:
    MemoryPool memoryPool;
    Scheduler scheduler;
    void initializeScheduler();
    uint16_t createThread(void (*)());
    void createStack(uint16_t);

    RTCore(utils::IdGen& idGen) : idGen{idGen}
    {
        LOG_INFO("Initializing core");
        prescaler = utils::getClockFreq() / 1000;
    }

    utils::IdGen& idGen;

    uint32_t prescaler{};
    std::vector<Thread*> threadControlBlocks; // change to unordered_map [threadId, tcb] or not // no this is dumb
    SchedulerType schedulerType{SchedulerType::ROUND_ROBIN};
    std::vector<Thread*> activeStacks;
    std::vector<Stack*> mappedStacks;
    uint8_t currentStackIndex{0};

    Thread* getNextThreadRoundRobin();
    Thread* getNextThreadPriorityBased()
    {
        // Implement logic for priority-based scheduling
        return nullptr; // Placeholder
    }
    inline static RTCore* kernelInstance = nullptr;
};

} // namespace core
