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

namespace kernel
{
// API CALLS
extern "C" void changeContext();
extern "C" void addTask();
extern "C" void removeTask();

// class Scheduler;
constexpr int stackSize{500};

class Kernel
{
public:
    static void init(Logger&, utils::IdGen&);
    static Kernel* getInstance();

    uint8_t addThreads(std::vector<void (*)()>&);
    void launch(uint32_t);

    void logThreadInfo();

    uint16_t add(void (*)());
    void remove(uint16_t);

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

    Kernel(Logger& logger, utils::IdGen& idGen) : logger{logger}, idGen{idGen}
    {
        logger.logInfo("Initializing kernel");
        prescaler = utils::getClockFreq() / 1000;
    }

    Logger& logger;
    utils::IdGen& idGen;

    uint32_t prescaler{};
    std::vector<Thread*> threadControlBlocks; // change to unordered_map [threadId, tcb] or not // no this is dumb
    SchedulerType schedulerType{SchedulerType::ROUND_ROBIN};
    std::vector<Thread*> activeStacks;
    uint8_t currentStackIndex{0};

    Thread* getNextThreadRoundRobin();
    Thread* getNextThreadPriorityBased()
    {
        // Implement logic for priority-based scheduling
        return nullptr; // Placeholder
    }
    inline static Kernel* kernelInstance = nullptr;
};

} // namespace kernel
