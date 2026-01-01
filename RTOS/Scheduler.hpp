#pragma once
#include <span>
#include "RTOS/Thread.hpp"
#include "Types.hpp"
namespace core
{

struct Thread;

struct Resource
{
    Thread* owner{nullptr};
    uint8_t slotOccupancy{0};
};

uint8_t constexpr systemTaskSlot{9};

class Scheduler
{
public:
    Scheduler() = default;

    void addTask(Thread* thread) { activeTasks.push_back(thread); }
    void removeTask();

    Thread* getNextThread()
    {
        // Implement logic to return the next thread to run
        return nullptr; // Placeholder
    }

    bool allocateResourceList()
    {
        for (auto& task : activeTasks)
        {
            task->calculateWagedPriority();
        }

        std::span<Resource> nextResourceList;
        if (isInitialRun)
        {
            nextResourceList = getCurrentResourceList();
            isInitialRun = false;
        }
        else
        {
            nextResourceList = getInactiveResourceList();
        }
        uint8_t idx = 0;
        for (uint8_t i = 0; i < 5; ++i)
        {
            uint8_t currentSliceOccupancy = 0;
            uint8_t startIdx = idx;

            // clearNextResourceList()
            for (uint8_t k = 0; k < systemTaskSlot; ++k)
            {
                nextResourceList[startIdx + k].owner = nullptr;
                nextResourceList[startIdx + k].slotOccupancy = 0;
            }

            for (uint8_t j = 0; j < systemTaskSlot; ++j)
            {
                findBestTask(nextResourceList, currentSliceOccupancy, idx);
                if (currentSliceOccupancy == systemTaskSlot)
                {
                    break;
                }
            }
            clearAllocatedFlags();
            if (currentSliceOccupancy < systemTaskSlot)
            {
                startIdx = idx;
                uint8_t endIdx = (i * 10) + systemTaskSlot;
                // Iterate over the slice again to fill holes
                for (uint8_t k = startIdx; k < endIdx; ++k)
                {
                    if (nextResourceList[k].owner == nullptr)
                    {
                        findBestTask(nextResourceList, currentSliceOccupancy, idx, false);
                    }
                }
            }
        }
        return true;
    }

    bool updateResourceList()
    {
        return true; // Placeholder
    }

    void switchResourceList() { isPrimaryListActive = not isPrimaryListActive; }

    void printResourceAllocation()
    {
        std::span<Resource> currentList = getCurrentResourceList();
        for (size_t i = 0; i < currentList.size(); ++i)
        {
            if (currentList[i].owner != nullptr)
            {
                LOG_INFO(
                    "Resource Slot %d: Owned by Thread ID %d with Slot Occupancy %d",
                    i,
                    currentList[i].owner->getThreadId(),
                    currentList[i].slotOccupancy);
            }
            else
            {
                LOG_INFO("Resource Slot %d: Unallocated", i);
            }
        }
    }

private:
    void clearAllocatedFlags()
    {
        for (auto& task : activeTasks)
        {
            task->isAllocated = false;
        }
    }
    void findBestTask(
        std::span<Resource>& nextResourceList,
        uint8_t& currentSliceOccupancy,
        uint8_t& idx,
        bool setAllocated = true)
    {
        uint16_t highestPriority = 0;
        uint8_t selectedTaskId = invalidThreadId;
        for (const auto& task : activeTasks)
        {
            if (not task->isAllocated and task->wagedPriority > highestPriority)
            {
                highestPriority = task->wagedPriority;
                selectedTaskId = task->getThreadId();
            }
        }
        if (selectedTaskId != invalidThreadId)
        {
            Thread* selectedTask = getThreadById(selectedTaskId);
            if (setAllocated)
            {
                selectedTask->isAllocated = true;
            }
            nextResourceList[idx].owner = selectedTask;
            nextResourceList[idx].slotOccupancy = 1;
            currentSliceOccupancy += nextResourceList[idx].slotOccupancy;
            ++idx;
        }
    }
    std::span<Resource> getCurrentResourceList()
    {
        if (isPrimaryListActive)
        {
            return std::span<Resource>(resourceListPrimary);
        }
        else
        {
            return std::span<Resource>(resourceListSecondary);
        }
    }

    std::span<Resource> getInactiveResourceList()
    {
        if (isPrimaryListActive)
        {
            return std::span<Resource>(resourceListSecondary);
        }
        else
        {
            return std::span<Resource>(resourceListPrimary);
        }
    }

    std::vector<Thread*> activeTasks; // List of tasks to be scheduled
    std::vector<Thread*> hardRtTasks; // List of hard real-time tasks
    uint8_t ongoingSlotOccupancy{0};
    Resource resourceListPrimary[50];
    Resource resourceListSecondary[50];
    bool isPrimaryListActive{true};
    bool isInitialRun{true};

    Thread* getThreadById(uint16_t id)
    {
        for (const auto& thread : activeTasks)
        {
            if (thread->getThreadId() == id)
            {
                return thread;
            }
        }
        return nullptr;
    }
};

} // namespace core