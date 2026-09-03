#include <gtest/gtest.h>

#define private public
#include "Scheduler.hpp"
#undef private

namespace
{

using namespace core;

void idleTask() {}

class SchedulerTest : public ::testing::Test
{
protected:
    Scheduler scheduler;

    void (*entry)() = &idleTask;
    Thread t1{entry, 1};
    Thread t2{entry, 2};
    Thread t3{entry, 3};
};

TEST_F(SchedulerTest, AllocateResourceList_CalculatesWagedPriorityForActiveTasks)
{
    t1.priority = 1;
    t1.starvationCounter = 0;
    t2.priority = 4;
    t2.starvationCounter = 2;

    scheduler.addTask(&t1);
    scheduler.addTask(&t2);

    ASSERT_TRUE(scheduler.allocateResourceList());

    EXPECT_EQ(t1.wagedPriority, static_cast<uint16_t>((10 - 1) * (0 + 1)));
    EXPECT_EQ(t2.wagedPriority, static_cast<uint16_t>((10 - 4) * (2 + 1)));
}

TEST_F(SchedulerTest, AllocateResourceList_FirstRunFillsPrimaryAndClearsAllocatedFlags)
{
    t1.priority = 1;
    t2.priority = 2;
    t3.priority = 3;

    scheduler.addTask(&t1);
    scheduler.addTask(&t2);
    scheduler.addTask(&t3);

    ASSERT_TRUE(scheduler.allocateResourceList());

    EXPECT_FALSE(scheduler.isInitialRun);
    EXPECT_EQ(scheduler.resourceListPrimary[0].owner, &t1);
    EXPECT_EQ(scheduler.resourceListPrimary[1].owner, &t2);
    EXPECT_EQ(scheduler.resourceListPrimary[2].owner, &t3);

    EXPECT_FALSE(t1.isAllocated);
    EXPECT_FALSE(t2.isAllocated);
    EXPECT_FALSE(t3.isAllocated);
}

TEST_F(SchedulerTest, AllocateResourceList_TwoTasksFillSliceWithHighestPriorityOnHoles)
{
    t1.priority = 1;
    t2.priority = 9;
    t1.starvationCounter = 0;
    t2.starvationCounter = 0;

    scheduler.addTask(&t1);
    scheduler.addTask(&t2);

    ASSERT_TRUE(scheduler.allocateResourceList());

    EXPECT_EQ(scheduler.resourceListPrimary[0].owner, &t1);
    EXPECT_EQ(scheduler.resourceListPrimary[1].owner, &t2);
    for (uint8_t i = 2; i < systemTaskSlot; ++i)
    {
        EXPECT_EQ(scheduler.resourceListPrimary[i].owner, &t1) << "slot=" << static_cast<int>(i);
    }
}

TEST_F(SchedulerTest, AllocateResourceList_SecondRunWritesInactiveList)
{
    t1.priority = 9;
    scheduler.addTask(&t1);

    ASSERT_TRUE(scheduler.allocateResourceList());
    EXPECT_EQ(scheduler.resourceListPrimary[0].owner, &t1);
    EXPECT_EQ(scheduler.resourceListSecondary[0].owner, nullptr);

    t2.priority = 1;
    scheduler.addTask(&t2);

    ASSERT_TRUE(scheduler.allocateResourceList());
    EXPECT_EQ(scheduler.resourceListPrimary[0].owner, &t1);
    EXPECT_EQ(scheduler.resourceListSecondary[0].owner, &t2);
}

TEST_F(SchedulerTest, AllocateResourceList_SeparatesNormalAndSystemTaskSlots)
{
    Thread a{entry, 1, TaskType::SYSTEM};
    Thread b{entry, 2, TaskType::SYSTEM};
    Thread c{entry, 13, TaskType::NORMAL};
    Thread d{entry, 14, TaskType::NORMAL};
    Thread e{entry, 15, TaskType::NORMAL};
    Thread f{entry, 16, TaskType::NORMAL};
    Thread g{entry, 17, TaskType::NORMAL};
    Thread h{entry, 8, TaskType::SYSTEM};

    a.priority = 1;
    b.priority = 2;
    c.priority = 3;
    d.priority = 4;
    e.priority = 5;
    f.priority = 6;
    g.priority = 7;
    h.priority = 8;

    scheduler.addTask(&a);
    scheduler.addTask(&b);
    scheduler.addTask(&c);
    scheduler.addTask(&d);
    scheduler.addTask(&e);
    scheduler.addTask(&f);
    scheduler.addTask(&g);
    scheduler.addTask(&h);

    ASSERT_TRUE(scheduler.allocateResourceList());

    EXPECT_EQ(scheduler.resourceListPrimary[0].owner, &c);
    EXPECT_EQ(scheduler.resourceListPrimary[1].owner, &d);
    EXPECT_EQ(scheduler.resourceListPrimary[2].owner, &e);
    EXPECT_EQ(scheduler.resourceListPrimary[3].owner, &f);
    EXPECT_EQ(scheduler.resourceListPrimary[4].owner, &g);
    for (uint8_t slot = 5; slot < systemTaskSlot; ++slot)
    {
        EXPECT_EQ(scheduler.resourceListPrimary[slot].owner, &c) << "slot=" << static_cast<int>(slot);
    }
    EXPECT_EQ(scheduler.resourceListPrimary[systemTaskSlot].owner, &a);
}

} // namespace
