#include <gtest/gtest.h>
#include <vector>

#define private public
#include "Core.hpp"
#undef private

namespace utils
{
uint32_t getClockFreq()
{
    return 16000000u;
}
} // namespace utils

namespace sysTasks
{
void idleTask() {}
void systemReconfigurationTask() {}
void eventTracerTask() {}
void resourceAllocatorTask() {}
void resourceUpdateTask() {}
} // namespace sysTasks

extern "C" void start_thread_switch() {}

extern "C" void context_change() {}

namespace
{

using namespace core;

void taskA() {}
void taskB() {}

class CoreTest : public ::testing::Test
{
protected:
    utils::IdGen idGen{0};

    void SetUp() override { RTCore::init(idGen); }
};

TEST_F(CoreTest, Init_CreatesSingletonInstance)
{
    ASSERT_NE(RTCore::getInstance(), nullptr);
}

TEST_F(CoreTest, Constructor_AddsFiveSystemThreadsAndMovesIdToTen)
{
    auto* core = RTCore::getInstance();
    ASSERT_NE(core, nullptr);

    EXPECT_EQ(core->threadControlBlocks.size(), 5u);
    EXPECT_EQ(idGen.getId(), 10u);
}

TEST_F(CoreTest, GetThreadById_ReturnsThreadWhenPresent)
{
    auto* core = RTCore::getInstance();
    ASSERT_NE(core, nullptr);

    auto id = core->add(taskA);
    auto* thread = core->getThreadById(id);

    ASSERT_NE(thread, nullptr);
    EXPECT_EQ(thread->getThreadId(), id);
}

TEST_F(CoreTest, GetNextThread_RoundRobinCyclesThroughActiveStacks)
{
    auto* core = RTCore::getInstance();
    ASSERT_NE(core, nullptr);

    core->setSchedulerType(SchedulerType::ROUND_ROBIN);

    auto id1 = core->add(taskA);
    auto id2 = core->add(taskB);
    Thread* t1 = core->getThreadById(id1);
    Thread* t2 = core->getThreadById(id2);
    ASSERT_NE(t1, nullptr);
    ASSERT_NE(t2, nullptr);

    core->currentStackIndex = static_cast<uint8_t>(core->activeStacks.size() - 2);

    EXPECT_EQ(core->getNextThread(), t1);
    EXPECT_EQ(core->getNextThread(), t2);
    EXPECT_EQ(core->getNextThread(), core->activeStacks.front());
}

TEST_F(CoreTest, GetNextThread_PriorityBasedReturnsNull)
{
    auto* core = RTCore::getInstance();
    ASSERT_NE(core, nullptr);

    core->setSchedulerType(SchedulerType::PRIORITY_BASED);
    EXPECT_EQ(core->getNextThread(), nullptr);
}

} // namespace
