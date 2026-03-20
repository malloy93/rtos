#include <gtest/gtest.h>
#include <array>
#include "MemoryPool.hpp"

namespace {

using namespace core;

// ---------------------------------------------------------------------------
// Fixture — tworzy pulę przed każdym testem
// ---------------------------------------------------------------------------
class MemoryPoolTest : public ::testing::Test
{
protected:
    MemoryPool mp;
    void SetUp() override { mp.createPool(); }
};

// ===========================================================================
// createPool — stan początkowy
// ===========================================================================

TEST_F(MemoryPoolTest, CreatePool_1kBPools_HaveCorrectSize)
{
    for (uint8_t id = 0; id < 24; ++id)
    {
        Stack* s = mp.getStack(id);
        ASSERT_NE(s, nullptr) << "getStack(" << (int)id << ") zwróciło nullptr";
        EXPECT_EQ(s->size, StackSize::SIZE_1kB) << "id=" << (int)id;
    }
}

TEST_F(MemoryPoolTest, CreatePool_2kBPools_HaveCorrectSize)
{
    for (uint8_t id = 24; id < 36; ++id)
    {
        Stack* s = mp.getStack(id);
        ASSERT_NE(s, nullptr) << "getStack(" << (int)id << ") zwróciło nullptr";
        EXPECT_EQ(s->size, StackSize::SIZE_2kB) << "id=" << (int)id;
    }
}

TEST_F(MemoryPoolTest, CreatePool_4kBPools_HaveCorrectSize)
{
    for (uint8_t id = 36; id < 42; ++id)
    {
        Stack* s = mp.getStack(id);
        ASSERT_NE(s, nullptr) << "getStack(" << (int)id << ") zwróciło nullptr";
        EXPECT_EQ(s->size, StackSize::SIZE_4kB) << "id=" << (int)id;
    }
}

TEST_F(MemoryPoolTest, CreatePool_8kBPools_HaveCorrectSize)
{
    for (uint8_t id = 42; id < 45; ++id)
    {
        Stack* s = mp.getStack(id);
        ASSERT_NE(s, nullptr) << "getStack(" << (int)id << ") zwróciło nullptr";
        EXPECT_EQ(s->size, StackSize::SIZE_8kB) << "id=" << (int)id;
    }
}

TEST_F(MemoryPoolTest, CreatePool_AllPoolsHaveNonNullAddresses)
{
    for (uint8_t id = 0; id < 45; ++id)
    {
        Stack* s = mp.getStack(id);
        ASSERT_NE(s, nullptr);
        EXPECT_NE(s->endAddress, nullptr)  << "endAddress null dla id=" << (int)id;
        EXPECT_NE(s->baseAddress, nullptr) << "baseAddress null dla id=" << (int)id;
    }
}

TEST_F(MemoryPoolTest, CreatePool_StackIdsAreSequential)
{
    for (uint8_t id = 0; id < 45; ++id)
    {
        Stack* s = mp.getStack(id);
        ASSERT_NE(s, nullptr);
        EXPECT_EQ(s->stackId, id) << "stackId != index dla id=" << (int)id;
    }
}

TEST_F(MemoryPoolTest, CreatePool_AllPoolsHaveFreeCanary)
{
    for (uint8_t id = 0; id < 45; ++id)
    {
        Stack* s = mp.getStack(id);
        ASSERT_NE(s, nullptr);
        EXPECT_EQ(*(s->endAddress + 1), freeCanaryValue)
            << "Brak freeCanary dla id=" << (int)id;
    }
}

// ===========================================================================
// allocateStack
// ===========================================================================

TEST_F(MemoryPoolTest, AllocateStack_1kB_ReturnsValidId)
{
    StackId id = mp.allocateStack(StackSize::SIZE_1kB);
    EXPECT_NE(id, invalidStackId);
    EXPECT_LT(id, 45u);
}

TEST_F(MemoryPoolTest, AllocateStack_2kB_ReturnsValidId)
{
    StackId id = mp.allocateStack(StackSize::SIZE_2kB);
    EXPECT_NE(id, invalidStackId);
    EXPECT_LT(id, 45u);
}

TEST_F(MemoryPoolTest, AllocateStack_4kB_ReturnsValidId)
{
    StackId id = mp.allocateStack(StackSize::SIZE_4kB);
    EXPECT_NE(id, invalidStackId);
    EXPECT_LT(id, 45u);
}

TEST_F(MemoryPoolTest, AllocateStack_8kB_ReturnsValidId)
{
    StackId id = mp.allocateStack(StackSize::SIZE_8kB);
    EXPECT_NE(id, invalidStackId);
    EXPECT_LT(id, 45u);
}

TEST_F(MemoryPoolTest, AllocateStack_1kB_IdIsIn1kBRange)
{
    StackId id = mp.allocateStack(StackSize::SIZE_1kB);
    ASSERT_NE(id, invalidStackId);
    EXPECT_LT(id, 24u);
}

TEST_F(MemoryPoolTest, AllocateStack_2kB_IdIsIn2kBRange)
{
    StackId id = mp.allocateStack(StackSize::SIZE_2kB);
    ASSERT_NE(id, invalidStackId);
    EXPECT_GE(id, 24u);
    EXPECT_LT(id, 36u);
}

TEST_F(MemoryPoolTest, AllocateStack_4kB_IdIsIn4kBRange)
{
    StackId id = mp.allocateStack(StackSize::SIZE_4kB);
    ASSERT_NE(id, invalidStackId);
    EXPECT_GE(id, 36u);
    EXPECT_LT(id, 42u);
}

TEST_F(MemoryPoolTest, AllocateStack_8kB_IdIsIn8kBRange)
{
    StackId id = mp.allocateStack(StackSize::SIZE_8kB);
    ASSERT_NE(id, invalidStackId);
    EXPECT_GE(id, 42u);
    EXPECT_LT(id, 45u);
}

TEST_F(MemoryPoolTest, AllocateStack_SetsUsedCanary)
{
    StackId id = mp.allocateStack(StackSize::SIZE_1kB);
    ASSERT_NE(id, invalidStackId);
    Stack* s = mp.getStack(id);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(*(s->endAddress + 1), usedCanaryValue);
}

TEST_F(MemoryPoolTest, AllocateStack_Exhaust_1kB_ReturnsInvalidId)
{
    for (int i = 0; i < 24; ++i)
    {
        StackId id = mp.allocateStack(StackSize::SIZE_1kB);
        ASSERT_NE(id, invalidStackId) << "Nieudana alokacja przy i=" << i;
    }
    EXPECT_EQ(mp.allocateStack(StackSize::SIZE_1kB), invalidStackId);
}

TEST_F(MemoryPoolTest, AllocateStack_Exhaust_2kB_ReturnsInvalidId)
{
    for (int i = 0; i < 12; ++i)
    {
        StackId id = mp.allocateStack(StackSize::SIZE_2kB);
        ASSERT_NE(id, invalidStackId) << "Nieudana alokacja przy i=" << i;
    }
    EXPECT_EQ(mp.allocateStack(StackSize::SIZE_2kB), invalidStackId);
}

TEST_F(MemoryPoolTest, AllocateStack_Exhaust_4kB_ReturnsInvalidId)
{
    for (int i = 0; i < 6; ++i)
    {
        StackId id = mp.allocateStack(StackSize::SIZE_4kB);
        ASSERT_NE(id, invalidStackId) << "Nieudana alokacja przy i=" << i;
    }
    EXPECT_EQ(mp.allocateStack(StackSize::SIZE_4kB), invalidStackId);
}

TEST_F(MemoryPoolTest, AllocateStack_Exhaust_8kB_ReturnsInvalidId)
{
    for (int i = 0; i < 3; ++i)
    {
        StackId id = mp.allocateStack(StackSize::SIZE_8kB);
        ASSERT_NE(id, invalidStackId) << "Nieudana alokacja przy i=" << i;
    }
    EXPECT_EQ(mp.allocateStack(StackSize::SIZE_8kB), invalidStackId);
}

// ===========================================================================
// getStack
// ===========================================================================

TEST_F(MemoryPoolTest, GetStack_Id0_ReturnsNonNull)
{
    EXPECT_NE(mp.getStack(0), nullptr);
}

TEST_F(MemoryPoolTest, GetStack_Id44_ReturnsNonNull)
{
    EXPECT_NE(mp.getStack(44), nullptr);
}

TEST_F(MemoryPoolTest, GetStack_Id45_ReturnsNull)
{
    EXPECT_EQ(mp.getStack(45), nullptr);
}

TEST_F(MemoryPoolTest, GetStack_InvalidStackId_ReturnsNull)
{
    EXPECT_EQ(mp.getStack(invalidStackId), nullptr);
}

TEST_F(MemoryPoolTest, GetStack_AfterAllocate_MatchesReturnedId)
{
    StackId id = mp.allocateStack(StackSize::SIZE_2kB);
    ASSERT_NE(id, invalidStackId);
    Stack* s = mp.getStack(id);
    ASSERT_NE(s, nullptr);
    EXPECT_EQ(s->stackId, id);
    EXPECT_EQ(s->size, StackSize::SIZE_2kB);
}

// ===========================================================================
// deallocateStack
// ===========================================================================

TEST_F(MemoryPoolTest, DeallocateStack_ValidId_ReturnsTrue)
{
    StackId id = mp.allocateStack(StackSize::SIZE_1kB);
    ASSERT_NE(id, invalidStackId);
    EXPECT_TRUE(mp.deallocateStack(id));
}

TEST_F(MemoryPoolTest, DeallocateStack_OutOfBoundsId_ReturnsFalse)
{
    EXPECT_FALSE(mp.deallocateStack(45));
    EXPECT_FALSE(mp.deallocateStack(invalidStackId));
}

TEST_F(MemoryPoolTest, DeallocateStack_ResetsCanaryToFreeValue)
{
    StackId id = mp.allocateStack(StackSize::SIZE_1kB);
    ASSERT_NE(id, invalidStackId);

    Stack* s = mp.getStack(id);
    ASSERT_NE(s, nullptr);
    ASSERT_EQ(*(s->endAddress + 1), usedCanaryValue);

    mp.deallocateStack(id);

    EXPECT_EQ(*(s->endAddress + 1), freeCanaryValue);
}

TEST_F(MemoryPoolTest, DeallocateStack_ZeroesStackMemory)
{
    StackId id = mp.allocateStack(StackSize::SIZE_1kB);
    ASSERT_NE(id, invalidStackId);

    Stack* s = mp.getStack(id);
    ASSERT_NE(s, nullptr);

    // Zapisz niezerowe wartości w regionie stosu
    *s->endAddress       = 0xCAFEBABE;
    *(s->endAddress + 2) = 0x12345678;

    mp.deallocateStack(id);

    EXPECT_EQ(*s->endAddress,       0u);
    EXPECT_EQ(*(s->endAddress + 2), 0u);
}

// ===========================================================================
// Testy cyklu życia
// ===========================================================================

TEST_F(MemoryPoolTest, AllocateDeallocate_ThenReallocate_Succeeds)
{
    StackId id1 = mp.allocateStack(StackSize::SIZE_1kB);
    ASSERT_NE(id1, invalidStackId);

    EXPECT_TRUE(mp.deallocateStack(id1));

    StackId id2 = mp.allocateStack(StackSize::SIZE_1kB);
    EXPECT_NE(id2, invalidStackId);
}

TEST_F(MemoryPoolTest, Exhaust_DeallocateAll_Reallocate_All_1kB)
{
    std::array<StackId, 24> ids{};
    for (int i = 0; i < 24; ++i)
    {
        ids[i] = mp.allocateStack(StackSize::SIZE_1kB);
        ASSERT_NE(ids[i], invalidStackId) << "Alokacja " << i << " nie powiodła się";
    }

    EXPECT_EQ(mp.allocateStack(StackSize::SIZE_1kB), invalidStackId);

    for (int i = 0; i < 24; ++i)
        EXPECT_TRUE(mp.deallocateStack(ids[i])) << "Dealokacja " << i << " nie powiodła się";

    for (int i = 0; i < 24; ++i)
    {
        StackId id = mp.allocateStack(StackSize::SIZE_1kB);
        EXPECT_NE(id, invalidStackId) << "Re-alokacja " << i << " nie powiodła się";
    }
}

TEST_F(MemoryPoolTest, Exhaust_1kB_DoesNotAffect_2kBPool)
{
    for (int i = 0; i < 24; ++i)
        mp.allocateStack(StackSize::SIZE_1kB);

    StackId id = mp.allocateStack(StackSize::SIZE_2kB);
    EXPECT_NE(id, invalidStackId);
}

TEST_F(MemoryPoolTest, PoolWithCorruptedCanary_NotAllocated)
{
    // Manualnie ustaw canary na wartość inną niż freeCanaryValue
    Stack* s = mp.getStack(0);
    ASSERT_NE(s, nullptr);
    *(s->endAddress + 1) = corruptedCanaryValue;

    // Pula 0 nie powinna zostać przydzielona — alokator pominie ją
    // i przejdzie do puli 1, więc wynik != 0
    StackId id = mp.allocateStack(StackSize::SIZE_1kB);
    ASSERT_NE(id, invalidStackId); // inne pule wciąż dostępne
    EXPECT_NE(id, 0u);             // pula 0 pominięta
}

} // namespace
