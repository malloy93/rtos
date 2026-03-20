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

    void addTask(Thread* thread)
    {
        switch (thread->taskType)
        {
            case TaskType::SYSTEM:
                systemTasks.push_back(thread);
                break;
            case TaskType::HARD_RT:
                hardRtTasks.push_back(thread);
                break;
            case TaskType::NORMAL:
                activeTasks.push_back(thread);
                break;
        }
    }
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
            uint8_t startIdx = i * 10; // Make startIdx slice-aware

            // clearNextResourceList() for the 9 normal slots
            for (uint8_t k = 0; k < systemTaskSlot; ++k)
            {
                nextResourceList[startIdx + k].owner = nullptr;
                nextResourceList[startIdx + k].slotOccupancy = 0;
            }

            idx = startIdx; // Start allocation at the beginning of the slice.
            for (uint8_t j = 0; j < systemTaskSlot; ++j)
            {
                findBestTask(nextResourceList, currentSliceOccupancy, idx);
                if (currentSliceOccupancy == systemTaskSlot)
                {
                    break;
                }
            }

            // Add system task to the end of the slice
            if (i < systemTasks.size())
            {
                nextResourceList[startIdx + 9].owner = systemTasks[i];
                nextResourceList[startIdx + 9].slotOccupancy = 1;
            }

            clearAllocatedFlags();
            if (currentSliceOccupancy < systemTaskSlot)
            {
                uint8_t endIdx = startIdx + systemTaskSlot;
                // Iterate over the slice again to fill holes
                for (uint8_t k = startIdx; k < endIdx; ++k)
                {
                    if (nextResourceList[k].owner == nullptr)
                    {
                        uint8_t temp_idx = k;
                        findBestTask(nextResourceList, currentSliceOccupancy, temp_idx, false);
                    }
                }
            }

            printResourceAllocation();
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
        char lineBuffer[128];
        int offset = 0;

        LOG_INFO("--- Resource Map ---");
        for (int i = 0; i < 50; i++)
        {
            uint16_t ownerId =
                (currentList[i].owner != nullptr) ? currentList[i].owner->getThreadId() : invalidThreadId;
            // Format: "ID:Właściciel " np "00:01 "
            offset += snprintf(lineBuffer + offset, sizeof(lineBuffer) - offset, "%02d:%02d ", i, ownerId);

            // Co 10 element wypluj linię i wyczyść bufor
            if ((i + 1) % 10 == 0)
            {
                LOG_INFO("%s", lineBuffer);
                offset = 0;
                lineBuffer[0] = '\0';
            }
        }

        LOG_INFO("--- Resource Map ---");

        for (int i = 0; i < 50; i++)
        {
            uint16_t ownerId =
                (currentList[i].owner != nullptr) ? currentList[i].owner->getThreadId() : invalidThreadId;
            // 1. Jeśli to początek nowej linii (0, 10, 20...), wpisz nagłówek do bufora
            if (i % 10 == 0)
            {
                offset += snprintf(lineBuffer + offset, sizeof(lineBuffer) - offset, "Slots %02d-%02d: ", i, i + 9);
            }

            // 2. Format danych z NAWIASAMI: "[ID:Właściciel] "
            offset += snprintf(lineBuffer + offset, sizeof(lineBuffer) - offset, "[%02d:%02d] ", i, ownerId);

            // 3. Co 10 element (lub na końcu listy) wypluj linię i wyczyść
            if ((i + 1) % 10 == 0)
            {
                LOG_INFO("%s", lineBuffer); // Użyj "%s" dla bezpieczeństwa
                offset = 0;
                lineBuffer[0] = '\0';
            }
        }
        for (int i = 0; i < static_cast<int>(currentList.size()); ++i)
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
    std::vector<Thread*> systemTasks; // List of system tasks
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
