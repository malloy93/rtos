#pragma once
#include <cstdint>
#include <span>
#include "RTOS/Thread.hpp"
#include "Types.hpp"

namespace core
{

class ModernScheduler;

// ─── Dispatcher ──────────────────────────────────────────────────────────────
// Owns the slot counter, drives context switches, updates starvation counters.
// Holds a pointer to the ResourceAllocator (ModernScheduler).
// Returns idle task for unoccupied slots — no RR fallback.

class Dispatcher
{
public:
    Dispatcher() = default;

    void init(ModernScheduler* allocator, Thread* idle)
    {
        this->allocator = allocator;
        this->idleTask = idle;
    }

    // ── Main entry point — called from RTCore::getNextThreadModern() ──────────

    Thread* getNextThread()
    {
        Thread* next = allocator->getNextThread(slotIndex);

        if (slotIndex % SLOTS_PER_FRAME == SYSTEM_SLOT_IN_FRAME)
        {
            updateStarvation(slotIndex / SLOTS_PER_FRAME);
        }

        ++slotIndex;

        if (slotIndex >= RESOURCE_LIST_SIZE)
        {
            slotIndex = 0;
            allocator->switchResourceList();
        }
        if (next == nullptr)
        {
            LOG_ERROR("Dispatcher: No thread allocated for slot %d, returning idle task", slotIndex);
        }

        return next != nullptr ? next : idleTask;
    }

private:
    // ── Starvation update ─────────────────────────────────────────────────────

    void updateStarvation(uint8_t frame)
    {
        uint8_t base = frame * SLOTS_PER_FRAME;
        std::span<Resource> list = allocator->getCurrentResourceList();

        updateClass(allocator->getHardRtTasks(), list, base);
        updateClass(allocator->getSoftRtTasks(), list, base);
        updateClass(allocator->getNormalTasks(), list, base);
        updateClass(allocator->getLowPrioTasks(), list, base);
    }

    void updateClass(std::span<Thread* const> tasks, std::span<Resource> list, uint8_t base)
    {
        for (auto* t : tasks)
        {
            if (!t)
            {
                continue;
            }
            bool gotSlot = false;
            for (uint8_t s = 0; s < SLOTS_PER_FRAME && !gotSlot; ++s)
                if (list[base + s].owner == t)
                {
                    gotSlot = true;
                }
            gotSlot ? t->starvationCounter = 0 : ++t->starvationCounter;
        }
    }

    // ── State ─────────────────────────────────────────────────────────────────

    ModernScheduler* allocator{nullptr};
    Thread* idleTask{nullptr};
    uint8_t slotIndex{0};
};

} // namespace core