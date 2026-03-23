#include <gtest/gtest.h>

#define private public
#include "RTOS/ModernScheduler.hpp"
#undef private

#include "RTOS/Thread.hpp"

namespace
{

using namespace core;

// Dummy entry point for threads
void idleTask() {}

class ModernSchedulerTest : public ::testing::Test
{
protected:
    ModernScheduler scheduler;

    // Create a variety of threads to use in tests
    // Entry point, ID, TaskType
    Thread sys1{idleTask, 1, TaskType::SYSTEM};
    Thread sys2{idleTask, 2, TaskType::SYSTEM};
    Thread sys3{idleTask, 3, TaskType::SYSTEM};
    Thread sys4{idleTask, 4, TaskType::SYSTEM};
    Thread sys5{idleTask, 5, TaskType::SYSTEM};

    Thread hrt1{idleTask, 10, TaskType::HARD_RT};
    Thread hrt2{idleTask, 11, TaskType::HARD_RT};

    Thread nrm1{idleTask, 20, TaskType::NORMAL};
    Thread nrm2{idleTask, 21, TaskType::NORMAL};

    Thread low1{idleTask, 30, TaskType::LOW_PRIO};

    void SetUp() override
    {
        // // Reset thread properties before each test
        // sys1 = Thread{idleTask, 1, TaskType::SYSTEM};
        // sys2 = Thread{idleTask, 2, TaskType::SYSTEM};
        // sys3 = Thread{idleTask, 3, TaskType::SYSTEM};
        // sys4 = Thread{idleTask, 4, TaskType::SYSTEM};
        // sys5 = Thread{idleTask, 5, TaskType::SYSTEM};
        // hrt1 = Thread{idleTask, 10, TaskType::HARD_RT};
        // hrt2 = Thread{idleTask, 11, TaskType::HARD_RT};
        // nrm1 = Thread{idleTask, 20, TaskType::NORMAL};
        // nrm2 = Thread{idleTask, 21, TaskType::NORMAL};
        // low1 = Thread{idleTask, 30, TaskType::LOW_PRIO};
    }
};

TEST_F(ModernSchedulerTest, AddTask_AddsTasksToCorrectLists)
{
    scheduler.addTask(&sys1);
    scheduler.addTask(&hrt1);
    scheduler.addTask(&nrm1);
    scheduler.addTask(&low1);

    ASSERT_EQ(scheduler.systemTasks.size(), 1);
    EXPECT_EQ(scheduler.systemTasks[0], &sys1);

    ASSERT_EQ(scheduler.hardRtTasks.size(), 1);
    EXPECT_EQ(scheduler.hardRtTasks[0], &hrt1);

    ASSERT_EQ(scheduler.normalTasks.size(), 1);
    EXPECT_EQ(scheduler.normalTasks[0], &nrm1);

    ASSERT_EQ(scheduler.lowPrioTasks.size(), 1);
    EXPECT_EQ(scheduler.lowPrioTasks[0], &low1);
}

TEST_F(ModernSchedulerTest, RemoveTask_RemovesFromTaskLists)
{
    scheduler.addTask(&hrt1);
    scheduler.addTask(&nrm1);
    scheduler.addTask(&low1);

    scheduler.removeTask(hrt1.getThreadId());
    scheduler.removeTask(nrm1.getThreadId());
    scheduler.removeTask(low1.getThreadId());

    EXPECT_TRUE(scheduler.hardRtTasks.empty());
    EXPECT_TRUE(scheduler.normalTasks.empty());
    EXPECT_TRUE(scheduler.lowPrioTasks.empty());
}

TEST_F(ModernSchedulerTest, AllocateResourceList_SystemTasksPlacedCorrectly)
{
    // Add in a specific order
    scheduler.addTask(&sys1); // SYSFRAME_SCHEDULER
    scheduler.addTask(&sys2); // SYSFRAME_TRACER_A
    scheduler.addTask(&sys3); // SYSFRAME_IDLE_A
    scheduler.addTask(&sys4); // SYSFRAME_TRACER_B
    scheduler.addTask(&sys5); // SYSFRAME_IDLE_B

    scheduler.allocateResourceList();

    auto list = scheduler.getInactiveResourceList();
    EXPECT_EQ(list[9].owner, &sys1);
    EXPECT_EQ(list[19].owner, &sys2);
    EXPECT_EQ(list[29].owner, &sys3);
    EXPECT_EQ(list[39].owner, &sys4);
    EXPECT_EQ(list[49].owner, &sys5);
}

TEST_F(ModernSchedulerTest, AllocateResourceList_GuaranteedPassForHardRT)
{
    hrt1.priority = 1;
    hrt2.priority = 10; // Lower priority

    scheduler.addTask(&hrt1);
    scheduler.addTask(&hrt2);

    scheduler.allocateResourceList();
    auto list = scheduler.getInactiveResourceList();

    // In each frame, the first two non-system slots should go to hrt1 and hrt2
    for (uint8_t frame = 0; frame < FRAMES_PER_LIST; ++frame)
    {
        uint8_t base = frame * SLOTS_PER_FRAME;
        EXPECT_EQ(list[base + 0].owner, &hrt1);
        EXPECT_EQ(list[base + 1].owner, &hrt2);
    }
}

TEST_F(ModernSchedulerTest, AllocateResourceList_DeadlineMissForHardRT)
{
    // Create 9 hard-RT tasks. A frame only has 9 allocatable slots (0-8).
    std::vector<Thread> threads;
    for (uint8_t i = 0; i < 9; ++i)
    {
        threads.emplace_back(idleTask, 100 + i, TaskType::HARD_RT);
    }
    // Add a 10th task that won't fit.
    Thread wont_fit{idleTask, 200, TaskType::HARD_RT};

    for (auto& t : threads)
    {
        t.priority = 5; // Equal priority
        scheduler.addTask(&t);
    }
    wont_fit.priority = 1; // Lowest priority, so it's last
    scheduler.addTask(&wont_fit);

    ASSERT_EQ(wont_fit.deadline_missed, 0);

    scheduler.allocateResourceList();

    // Since it couldn't be placed in the guaranteed pass in any of the 5 frames,
    // its deadline_missed count should be 5.
    EXPECT_EQ(wont_fit.deadline_missed, 5);
}

TEST_F(ModernSchedulerTest, AllocateResourceList_FillPhaseIsFair)
{
    nrm1.priority = 10; // High priority
    nrm2.priority = 9; // Slightly lower priority

    scheduler.addTask(&nrm1);
    scheduler.addTask(&nrm2);

    scheduler.allocateResourceList();
    auto list = scheduler.getInactiveResourceList();

    // In frame 0, nrm1 and nrm2 get slots 0 and 1 in the guaranteed pass.
    // Slots 2-8 are for the fill phase.
    // Let's find the first empty slot after the guaranteed pass and check the fill logic.
    uint8_t firstEmptySlot = 2;
    ASSERT_EQ(list[0].owner, &nrm1);
    ASSERT_EQ(list[1].owner, &nrm2);

    // Slot 2 should be filled by nrm1 (highest wagedPriority)
    EXPECT_EQ(list[firstEmptySlot].owner, &nrm1);

    // Slot 3 should now be filled by nrm2, because nrm1's priority was penalized
    // nrm1 eff: 10 / (1+1) = 5
    // nrm2 eff: 9 / (0+1) = 9
    EXPECT_EQ(list[firstEmptySlot + 1].owner, &nrm2);
}

TEST_F(ModernSchedulerTest, GetNextThread_And_SwitchResourceList)
{
    nrm1.priority = 1;
    scheduler.addTask(&nrm1);

    // Initial state: both lists are empty.
    // getNextThread reads from primary.
    EXPECT_EQ(scheduler.getNextThread(0), nullptr);

    // 1. Allocate. Fills the inactive list (secondary).
    scheduler.allocateResourceList();
    EXPECT_EQ(scheduler.getCurrentResourceList().data(), scheduler.resourceListPrimary);
    EXPECT_EQ(scheduler.getInactiveResourceList().data(), scheduler.resourceListSecondary);
    EXPECT_NE(scheduler.getInactiveResourceList()[0].owner, nullptr); // secondary is full
    EXPECT_EQ(scheduler.getCurrentResourceList()[0].owner, nullptr); // primary is still empty

    // getNextThread still reads from the empty primary list.
    EXPECT_EQ(scheduler.getNextThread(0), nullptr);

    // 2. Switch. Primary becomes secondary, secondary becomes primary.
    scheduler.switchResourceList();
    EXPECT_EQ(scheduler.getCurrentResourceList().data(), scheduler.resourceListSecondary); // now active
    EXPECT_EQ(scheduler.getInactiveResourceList().data(), scheduler.resourceListPrimary);

    // getNextThread now reads from the filled list.
    EXPECT_EQ(scheduler.getNextThread(0), &nrm1);
}

} // namespace
