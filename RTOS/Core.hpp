#pragma once
#include <map>
#include <stdint.h>
#include <vector>
#include "Logger.hpp"
#include "MemoryPool.hpp"
#include "ModernScheduler.hpp"
#include "RTOS/CircularBuffer.hpp"
#include "RTOS/Dispatcher.hpp"
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

    uint16_t add(void (*)(), TaskType type = TaskType::NORMAL);
    void remove(uint16_t);
    void suspend(uint16_t);
    void runAllocator()
    {
        modernScheduler.allocateResourceList();
        // scheduler.printResourceAllocation();
    }
    void pushLog(std::span<char> data) { loggerBuffer.push(data); }

    // Thread* getNextThread()
    // {
    //     switch (schedulerType)
    //     {
    //         case SchedulerType::ROUND_ROBIN:
    //             return getNextThreadRoundRobin();
    //         case SchedulerType::PRIORITY_BASED:
    //             return getNextThreadPriorityBased();
    //         default:
    //             return getNextThreadRoundRobin();
    //     }
    // }

    Thread* getNextThread()
    {
        switch (schedulerType)
        {
            case SchedulerType::ROUND_ROBIN:
                return getNextThreadRoundRobin();
            case SchedulerType::PRIORITY_BASED:
                return getNextThreadPriorityBased();
            case SchedulerType::RESOURCE_GRID:
                return getNextThreadModern();
            default:
                return getNextThreadRoundRobin();
        }
    }

    Thread* getNextThreadModern()
    {
        // // wrap — new list becomes active, trigger reallocation for next cycle
        // if (modernSlotIndex >= RESOURCE_LIST_SIZE)
        // {
        //     modernSlotIndex = 0;
        //     modernScheduler.switchResourceList();
        // }

        // Thread* next = modernScheduler.getNextThread(modernSlotIndex);
        // ++modernSlotIndex;

        // // slot 9 is the scheduler system task — it will call allocateResourceList()
        // // internally via its task function, nothing to do here

        // // fallback — if slot is empty (not yet allocated) fall back to RR
        // if (next == nullptr)
        // {
        //     return getNextThreadRoundRobin();
        // }

        // return next;
        Thread* next = dispatcher.getNextThread();

        return next;
    }

    void addSystemThreads()
    {
        add(sysTasks::resourceAllocatorTask, TaskType::SYSTEM);
        add(sysTasks::idleTask, TaskType::SYSTEM);
        add(sysTasks::eventTracerTask, TaskType::SYSTEM);
        add(sysTasks::resourceUpdateTask, TaskType::SYSTEM);
        add(sysTasks::systemReconfigurationTask, TaskType::SYSTEM);

        idGen.setId(10);
    }

    void changeSliceTime(uint8_t newSliceTime) { sliceTime = newSliceTime; }

    void setSchedulerType(SchedulerType type) { schedulerType = type; }

private:
    MemoryPool memoryPool;
    Scheduler scheduler;
    ModernScheduler modernScheduler;
    Dispatcher dispatcher{};
    CircularBuffer loggerBuffer;
    uint8_t modernSlotIndex{0};
    bool nextListReady{false};
    void initializeScheduler();
    uint16_t createThread(void (*)(), TaskType type);
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
        dispatcher.init(&modernScheduler, threadControlBlocks[1]); // idle task
    }

    utils::IdGen& idGen;

    uint32_t systickPrescaler{};
    std::vector<Thread*> threadControlBlocks; // change to unordered_map [threadId, tcb] or not // no this is dumb
    SchedulerType schedulerType{SchedulerType::RESOURCE_GRID};
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