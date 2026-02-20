#pragma once
#include <map>
#include <stdint.h>
#include <vector>
#include "Logger.hpp"
#include "MemoryPool.hpp"
#include "RTOS/CircularBuffer.hpp"
#include "Scheduler.hpp"
#include "SysTasks.hpp"
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
    Thread* getThreadById(uint16_t id)
    {
        for (const auto& thread : threadControlBlocks)
        {
            if (thread->getThreadId() == id)
            {
                return thread;
            }
        }
        return nullptr;
    }

    uint16_t add(void (*)());
    void remove(uint16_t);
    void suspend(uint16_t);
    void runAllocator()
    {
        scheduler.allocateResourceList();
        // scheduler.printResourceAllocation();
    }
    void pushLog(std::span<char> data) { loggerBuffer.push(data); }

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
    void addSystemThreads()
    {
        add(sysTasks::idleTask);
        add(sysTasks::eventTracerTask);
        add(sysTasks::resourceAllocatorTask);
        add(sysTasks::resourceUpdateTask);
        add(sysTasks::systemReconfigurationTask);

        idGen.setId(10);
    }

    void changeSliceTime(uint8_t newSliceTime) { sliceTime = newSliceTime; }

    void setSchedulerType(SchedulerType type) { schedulerType = type; }

private:
    MemoryPool memoryPool;
    Scheduler scheduler;
    CircularBuffer loggerBuffer;
    void initializeScheduler();
    uint16_t createThread(void (*)());
    void createStack(uint16_t);

    RTCore(utils::IdGen& idGen) : idGen{idGen}
    {
        LOG_INFO("Initializing core");
        systickPrescaler = utils::getClockFreq() / 1000;
        LOG_INFO("System clock frequency: %d Hz", utils::getClockFreq());
        LOG_INFO("Prescaler: %d", systickPrescaler);
        memoryPool.createPool();
        LOG_INFO("Memory pool initialized");
        threadControlBlocks.reserve(20);
        addSystemThreads();
    }

    utils::IdGen& idGen;

    uint32_t systickPrescaler{};
    std::vector<Thread*> threadControlBlocks; // change to unordered_map [threadId, tcb] or not // no this is dumb
    SchedulerType schedulerType{SchedulerType::ROUND_ROBIN};
    std::vector<Thread*> activeStacks;
    // std::vector<Stack*> mappedStacks;
    uint8_t currentStackIndex{0};

    Thread* getNextThreadRoundRobin();
    Thread* getNextThreadPriorityBased()
    {
        // Implement logic for priority-based scheduling
        return nullptr; // Placeholder
    }
    inline static RTCore* kernelInstance = nullptr;
    uint8_t sliceTime{5};
};

} // namespace core

// tmrrw - finise memory pool with deallocation + test
// next - finish tcb and args passing + starvation handling
// then - scheduler extenstion + more scheduling algorithms
// then - sytem threads ( LOGGER, PLANNER, CONFIGURATOR )