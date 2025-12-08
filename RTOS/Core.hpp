#pragma once
#include <map>
#include <stdint.h>
#include <vector>
#include "Logger.hpp"
#include "Scheduler.hpp"
#include "Stack.hpp"
#include "Thread.hpp"
#include "Utils.hpp"

namespace core
{
// API CALLS
extern "C" void changeContext();

// class Scheduler;

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

        memoryPool.createPool();
        LOG_INFO("Memory pool initialized");
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

// tmrrw - finise memory pool with deallocation + test
// next - finish tcb and args passing + starvation handling
// then - scheduler extenstion + more scheduling algorithms
// then - sytem threads ( LOGGER, PLANNER, CONFIGURATOR )