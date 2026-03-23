#pragma once
#include <algorithm>
#include <cstdint>
#include <span>
#include <vector>
#include "RTOS/Thread.hpp"
#include "Types.hpp"

namespace core
{

struct Thread;

// ─── Scheduler config ─────────────────────────────────────────────────────────

struct SchedulerConfig
{
    DeadlineBoostMode deadlineBoostMode = DeadlineBoostMode::STARVATION_FOLD;
    uint8_t deadlinePenalty = 2;
    LowPrioCompeteMode lowPrioCompeteMode = LowPrioCompeteMode::IDLE_FRAMES;
};

// ─── Constants ───────────────────────────────────────────────────────────────

static constexpr uint8_t SLOTS_PER_FRAME = 10;
static constexpr uint8_t FRAMES_PER_LIST = 5;
static constexpr uint8_t RESOURCE_LIST_SIZE = SLOTS_PER_FRAME * FRAMES_PER_LIST; // 50
static constexpr uint8_t SYSTEM_SLOT_IN_FRAME = 9;

static constexpr uint8_t MAX_HARD_RT_TASKS = 2;
static constexpr uint8_t MAX_SOFT_RT_TASKS = 8;
static constexpr uint8_t MAX_NORMAL_TASKS = 14;
static constexpr uint8_t MAX_LOW_PRIO_TASKS = 16;

static constexpr uint8_t HARD_RT_FILL_PRIORITY = 1;

// Idle frame mask — used when LowPrioCompeteMode == IDLE_FRAMES.
// Frames 2 and 4 correspond to idle system slots.
static constexpr bool LOW_PRIO_IDLE_FRAMES[FRAMES_PER_LIST] = {
    false, // frame 0 — scheduler
    false, // frame 1 — tracer A
    true, // frame 2 — idle A
    false, // frame 3 — tracer B
    true, // frame 4 — idle B
};

// System task frame assignments
static constexpr uint8_t SYSFRAME_SCHEDULER = 0;
static constexpr uint8_t SYSFRAME_TRACER_A = 1;
static constexpr uint8_t SYSFRAME_IDLE_A = 2;
static constexpr uint8_t SYSFRAME_TRACER_B = 3;
static constexpr uint8_t SYSFRAME_IDLE_B = 4;

// ─── Scheduler ───────────────────────────────────────────────────────────────

class ModernScheduler
{
public:
    explicit ModernScheduler(SchedulerConfig config = {}) : config{config} {}

    // ── Configuration ────────────────────────────────────────────────────────

    void setDeadlineBoostMode(DeadlineBoostMode mode)
    {
        config.deadlineBoostMode = mode;
        LOG_INFO("Scheduler: DeadlineBoostMode set to %s", deadlineBoostModeStr(mode));
    }

    void setDeadlinePenalty(uint8_t penalty)
    {
        config.deadlinePenalty = penalty;
        LOG_INFO("Scheduler: DeadlinePenalty set to %d", penalty);
    }

    void setLowPrioCompeteMode(LowPrioCompeteMode mode)
    {
        config.lowPrioCompeteMode = mode;
        LOG_INFO("Scheduler: LowPrioCompeteMode set to %s", lowPrioCompeteModeStr(mode));
    }

    void logConfig() const
    {
        LOG_INFO("=== Scheduler Config ===");
        LOG_INFO("  DeadlineBoostMode  : %s", deadlineBoostModeStr(config.deadlineBoostMode));
        LOG_INFO("  DeadlinePenalty    : %d", config.deadlinePenalty);
        LOG_INFO("  LowPrioCompeteMode : %s", lowPrioCompeteModeStr(config.lowPrioCompeteMode));
        LOG_INFO(
            "  Task limits        : HARD_RT=%d SOFT_RT=%d NORMAL=%d LOW_PRIO=%d",
            MAX_HARD_RT_TASKS,
            MAX_SOFT_RT_TASKS,
            MAX_NORMAL_TASKS,
            MAX_LOW_PRIO_TASKS);
        LOG_INFO("========================");
    }

    // ── Task registration ────────────────────────────────────────────────────

    void addTask(Thread* thread)
    {
        LOG_INFO("========================");
        if (thread == nullptr)
        {
            LOG_INFO("=========111===============");
            return;
        }
        LOG_INFO("========================");
        switch (thread->taskType)
        {
            case TaskType::SYSTEM:
                systemTasks.push_back(thread);
                break;
            case TaskType::HARD_RT:
                hardRtTasks.push_back(thread);
                break;
            case TaskType::SOFT_RT:
                softRtTasks.push_back(thread);
                break;
            case TaskType::NORMAL:
                normalTasks.push_back(thread);
                break;
            case TaskType::LOW_PRIO:
                lowPrioTasks.push_back(thread);
                break;
        }
    }

    void removeTask(uint16_t threadId)
    {
        auto removeById = [threadId](std::vector<Thread*>& list)
        {
            list.erase(
                std::remove_if(
                    list.begin(), list.end(), [threadId](Thread* t) { return t && t->getThreadId() == threadId; }),
                list.end());
        };
        removeById(hardRtTasks);
        removeById(softRtTasks);
        removeById(normalTasks);
        removeById(lowPrioTasks);
    }

    // ── Dispatcher interface ─────────────────────────────────────────────────

    Thread* getNextThread(uint8_t slotIndex)
    {
        if (slotIndex >= RESOURCE_LIST_SIZE)
        {
            return nullptr;
        }
        return getCurrentResourceList()[slotIndex].owner;
    }

    void switchResourceList() { isPrimaryListActive = !isPrimaryListActive; }

    // ── Main allocation — called once per resource list in system slot 9 ─────

    bool allocateResourceList()
    {
        std::span<Resource> list = getInactiveResourceList();
        clearResourceList(list);

        updateWagedPriorities();

        sortByWagedPriority(hardRtSorted, hardRtTasks);
        sortByWagedPriority(softRtNormalSorted, softRtTasks, normalTasks);
        sortByWagedPriority(softRtNormalLowSorted, softRtTasks, normalTasks, lowPrioTasks);

        for (uint8_t frame = 0; frame < FRAMES_PER_LIST; ++frame)
        {
            uint8_t base = frame * SLOTS_PER_FRAME;

            placeSystemTask(list, frame, base);
            placeHardRtPass(list, base);

            if (lowPrioInFrame(frame))
                placeMixedPass(list, softRtNormalLowSorted, base);
            else
                placeMixedPass(list, softRtNormalSorted, base);

            resetFrameAllocationFlags();
        }

        clearFillCounts();
        fillEmptySlots(list);

        // printResourceList(list);
        return true;
    }

    // ── Debug ────────────────────────────────────────────────────────────────

    void printResourceList(std::span<Resource> list)
    {
        char buf[128];
        LOG_INFO("=== Resource List ===");
        for (uint8_t frame = 0; frame < FRAMES_PER_LIST; ++frame)
        {
            int offset = 0;
            uint8_t base = frame * SLOTS_PER_FRAME;
            offset += snprintf(buf + offset, sizeof(buf) - offset, "Frame %d: ", frame);
            for (uint8_t slot = 0; slot < SLOTS_PER_FRAME; ++slot)
            {
                uint16_t ownerId = list[base + slot].owner ? list[base + slot].owner->getThreadId() : invalidThreadId;
                offset += snprintf(buf + offset, sizeof(buf) - offset, "[%02d:%02d] ", base + slot, ownerId);
            }
            LOG_INFO("%s", buf);
        }
        LOG_INFO("=====================");
    }

private:
    // ── Sorting ──────────────────────────────────────────────────────────────

    template <uint8_t N>
    struct SortedList
    {
        Thread* tasks[N]{};
        uint8_t count{0};

        void clear() { count = 0; }

        void insert(Thread* t)
        {
            if (count >= N || t == nullptr)
            {
                return;
            }
            uint8_t i = count;
            while (i > 0 && tasks[i - 1]->wagedPriority < t->wagedPriority)
            {
                tasks[i] = tasks[i - 1];
                --i;
            }
            tasks[i] = t;
            ++count;
        }

        Thread** begin() { return tasks; }
        Thread** end() { return tasks + count; }
    };

    template <uint8_t N>
    void sortByWagedPriority(SortedList<N>& sorted, std::vector<Thread*>& source)
    {
        sorted.clear();
        for (auto* t : source)
        {
            sorted.insert(t);
        }
    }

    template <uint8_t N>
    void sortByWagedPriority(SortedList<N>& sorted, std::vector<Thread*>& a, std::vector<Thread*>& b)
    {
        sorted.clear();
        for (auto* t : a)
        {
            sorted.insert(t);
        }
        for (auto* t : b)
        {
            sorted.insert(t);
        }
    }

    template <uint8_t N>
    void sortByWagedPriority(
        SortedList<N>& sorted,
        std::vector<Thread*>& a,
        std::vector<Thread*>& b,
        std::vector<Thread*>& c)
    {
        sorted.clear();
        for (auto* t : a)
        {
            sorted.insert(t);
        }
        for (auto* t : b)
        {
            sorted.insert(t);
        }
        for (auto* t : c)
        {
            sorted.insert(t);
        }
    }

    // ── Policy helpers ───────────────────────────────────────────────────────

    bool lowPrioInFrame(uint8_t frame) const
    {
        switch (config.lowPrioCompeteMode)
        {
            case LowPrioCompeteMode::FILL_ONLY:
                return false;
            case LowPrioCompeteMode::ALL_FRAMES:
                return true;
            case LowPrioCompeteMode::IDLE_FRAMES:
                return LOW_PRIO_IDLE_FRAMES[frame];
        }
        return false;
    }

    static const char* deadlineBoostModeStr(DeadlineBoostMode mode)
    {
        switch (mode)
        {
            case DeadlineBoostMode::NONE:
                return "NONE";
            case DeadlineBoostMode::STARVATION_FOLD:
                return "STARVATION_FOLD";
            case DeadlineBoostMode::ENHANCED:
                return "ENHANCED";
        }
        return "UNKNOWN";
    }

    static const char* lowPrioCompeteModeStr(LowPrioCompeteMode mode)
    {
        switch (mode)
        {
            case LowPrioCompeteMode::FILL_ONLY:
                return "FILL_ONLY";
            case LowPrioCompeteMode::IDLE_FRAMES:
                return "IDLE_FRAMES";
            case LowPrioCompeteMode::ALL_FRAMES:
                return "ALL_FRAMES";
        }
        return "UNKNOWN";
    }

    // ── Phase implementations ────────────────────────────────────────────────

    void placeSystemTask(std::span<Resource> list, uint8_t frame, uint8_t base)
    {
        uint8_t sysSlotIdx = base + SYSTEM_SLOT_IN_FRAME;
        if (frame < static_cast<uint8_t>(systemTasks.size()) && systemTasks[frame] != nullptr)
        {
            list[sysSlotIdx].owner = systemTasks[frame];
            list[sysSlotIdx].slotOccupancy = 1;
        }
    }

    void placeHardRtPass(std::span<Resource> list, uint8_t base)
    {
        for (Thread* task : hardRtSorted)
        {
            if (task == nullptr || task->isAllocated)
            {
                continue;
            }
            for (uint8_t slot = 0; slot < SLOTS_PER_FRAME; ++slot)
            {
                Resource& r = list[base + slot];
                if (r.owner == nullptr)
                {
                    r.owner = task;
                    r.slotOccupancy = 1;
                    task->isAllocated = true;
                    break;
                }
            }
        }
    }

    // Mixed guaranteed pass — soft-RT + normal (+ optionally low-prio).
    // One slot per task per frame. Soft-RT tasks that fail get deadline_missed++.
    template <uint8_t N>
    void placeMixedPass(std::span<Resource> list, SortedList<N>& sorted, uint8_t base)
    {
        for (Thread* task : sorted)
        {
            if (task == nullptr || task->isAllocated)
            {
                continue;
            }

            bool placed = false;
            for (uint8_t slot = 0; slot < SLOTS_PER_FRAME && !placed; ++slot)
            {
                Resource& r = list[base + slot];
                if (r.owner == nullptr)
                {
                    r.owner = task;
                    r.slotOccupancy = 1;
                    task->isAllocated = true;
                    placed = true;
                }
            }

            if (!placed && task->taskType == TaskType::SOFT_RT)
            {
                ++task->deadline_missed;
            }
        }
    }

    // Global fill pass — soft-RT + normal + low-prio with fill penalty,
    // hard-RT at floor priority HARD_RT_FILL_PRIORITY.
    void fillEmptySlots(std::span<Resource> list)
    {
        for (uint8_t i = 0; i < RESOURCE_LIST_SIZE; ++i)
        {
            if (list[i].owner != nullptr)
            {
                continue;
            }

            Thread* best = nullptr;
            uint16_t bestEff = 0;

            for (uint8_t j = 0; j < static_cast<uint8_t>(softRtTasks.size()); ++j)
            {
                Thread* t = softRtTasks[j];
                if (!t)
                {
                    continue;
                }
                uint16_t eff = t->wagedPriority / (softRtFillCount[j] + 1);
                if (eff > bestEff)
                {
                    bestEff = eff;
                    best = t;
                }
            }
            for (uint8_t j = 0; j < static_cast<uint8_t>(normalTasks.size()); ++j)
            {
                Thread* t = normalTasks[j];
                if (!t)
                {
                    continue;
                }
                uint16_t eff = t->wagedPriority / (normalFillCount[j] + 1);
                if (eff > bestEff)
                {
                    bestEff = eff;
                    best = t;
                }
            }
            for (uint8_t j = 0; j < static_cast<uint8_t>(lowPrioTasks.size()); ++j)
            {
                Thread* t = lowPrioTasks[j];
                if (!t)
                {
                    continue;
                }
                uint16_t eff = t->wagedPriority / (lowPrioFillCount[j] + 1);
                if (eff > bestEff)
                {
                    bestEff = eff;
                    best = t;
                }
            }

            if (bestEff < HARD_RT_FILL_PRIORITY)
            {
                for (auto* t : hardRtTasks)
                {
                    if (t)
                    {
                        best = t;
                        break;
                    }
                }
            }

            if (best == nullptr)
            {
                break;
            }

            list[i].owner = best;
            list[i].slotOccupancy = 1;
            incrementFillCount(best);
        }
    }

    // ── Helpers ──────────────────────────────────────────────────────────────

    void updateWagedPriorities()
    {
        auto update = [this](std::vector<Thread*>& list)
        {
            for (auto* t : list)
            {
                if (t) t->calculateWagedPriority(config.deadlineBoostMode, config.deadlinePenalty);
            }
        };
        update(hardRtTasks);
        update(softRtTasks);
        update(normalTasks);
        update(lowPrioTasks);
    }

    void resetFrameAllocationFlags()
    {
        auto reset = [](std::vector<Thread*>& list)
        {
            for (auto* t : list)
            {
                if (t) t->isAllocated = false;
            }
        };
        reset(hardRtTasks);
        reset(softRtTasks);
        reset(normalTasks);
        reset(lowPrioTasks);
    }

    void clearFillCounts()
    {
        for (auto& c : softRtFillCount)
        {
            c = 0;
        }
        for (auto& c : normalFillCount)
        {
            c = 0;
        }
        for (auto& c : lowPrioFillCount)
        {
            c = 0;
        }
    }

    void incrementFillCount(Thread* task)
    {
        for (uint8_t j = 0; j < static_cast<uint8_t>(softRtTasks.size()); ++j)
        {
            if (softRtTasks[j] == task)
            {
                ++softRtFillCount[j];
                return;
            }
        }
        for (uint8_t j = 0; j < static_cast<uint8_t>(normalTasks.size()); ++j)
        {
            if (normalTasks[j] == task)
            {
                ++normalFillCount[j];
                return;
            }
        }
        for (uint8_t j = 0; j < static_cast<uint8_t>(lowPrioTasks.size()); ++j)
        {
            if (lowPrioTasks[j] == task)
            {
                ++lowPrioFillCount[j];
                return;
            }
        }
    }

    void clearResourceList(std::span<Resource> list)
    {
        for (auto& r : list)
        {
            r.owner = nullptr;
            r.slotOccupancy = 0;
        }
    }

public:
    std::span<Resource> getCurrentResourceList()
    {
        return isPrimaryListActive ? std::span<Resource>(resourceListPrimary)
                                   : std::span<Resource>(resourceListSecondary);
    }
    std::span<Thread* const> getHardRtTasks() const { return hardRtTasks; }
    std::span<Thread* const> getSoftRtTasks() const { return softRtTasks; }
    std::span<Thread* const> getNormalTasks() const { return normalTasks; }
    std::span<Thread* const> getLowPrioTasks() const { return lowPrioTasks; }

    // std::span<Resource> getCurrentResourceList()
    // {
    //     return isPrimaryListActive ? std::span<Resource>(resourceListPrimary)
    //                                : std::span<Resource>(resourceListSecondary);
    // }

    const SchedulerConfig& getConfig() const { return config; }

private:
    std::span<Resource> getInactiveResourceList()
    {
        return isPrimaryListActive ? std::span<Resource>(resourceListSecondary)
                                   : std::span<Resource>(resourceListPrimary);
    }

    // ── State ────────────────────────────────────────────────────────────────

    SchedulerConfig config;

    std::vector<Thread*> systemTasks;
    std::vector<Thread*> hardRtTasks;
    std::vector<Thread*> softRtTasks;
    std::vector<Thread*> normalTasks;
    std::vector<Thread*> lowPrioTasks;

    // Pre-sorted scratch lists — built once per allocateResourceList call.
    // softRtNormalSorted:    phase 3 for frames where low-prio is excluded
    // softRtNormalLowSorted: phase 3 for frames where low-prio is included
    SortedList<MAX_HARD_RT_TASKS> hardRtSorted;
    SortedList<MAX_SOFT_RT_TASKS + MAX_NORMAL_TASKS> softRtNormalSorted;
    SortedList<MAX_SOFT_RT_TASKS + MAX_NORMAL_TASKS + MAX_LOW_PRIO_TASKS> softRtNormalLowSorted;

    uint8_t softRtFillCount[MAX_SOFT_RT_TASKS]{};
    uint8_t normalFillCount[MAX_NORMAL_TASKS]{};
    uint8_t lowPrioFillCount[MAX_LOW_PRIO_TASKS]{};

    Resource resourceListPrimary[RESOURCE_LIST_SIZE];
    Resource resourceListSecondary[RESOURCE_LIST_SIZE];
    bool isPrimaryListActive{true};
};

} // namespace core