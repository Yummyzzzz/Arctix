// ArctixC.Memory regression tests (ported from Core v0.1.0 smoke coverage).
// Links ArctixC::ArctixC. Fails (non-zero exit + message) on regression.

#include <arctixc/c_api/memory.h>
#include <arctixc/memory/allocator.hpp>
#include <arctixc/memory/memory_pool.hpp>

#include <cassert>
#include <cstdint>
#include <cstdio>

static int g_failures = 0;

#define CHECK(cond)                                                                 \
    do                                                                              \
    {                                                                               \
        if (!(cond))                                                                \
        {                                                                           \
            std::printf("FAIL line %d: %s\n", __LINE__, #cond);                      \
            ++g_failures;                                                           \
        }                                                                           \
    } while (0)

static void TestAllocatorBasic()
{
    arctix::Memory::Allocator alloc;
    CHECK(!alloc.IsInitialized());
    CHECK(alloc.Allocate(64) == nullptr); // not initialized -> nullptr
    CHECK(alloc.Initialize(arctix::Memory::DefaultConfig()));
    CHECK(!alloc.Initialize(arctix::Memory::DefaultConfig())); // double init fails

    void* p = alloc.Allocate(128, 16);
    CHECK(p != nullptr);
    CHECK((reinterpret_cast<std::uintptr_t>(p) % 16) == 0); // alignment respected

    arctix::Memory::ArctixStats stats = alloc.GetStatistics();
    CHECK(stats.allocationCount == 1);
    CHECK(stats.currentUsageBytes >= 128);

    alloc.Deallocate(p);
    stats = alloc.GetStatistics();
    CHECK(stats.currentUsageBytes == 0);
    CHECK(stats.freeCount == 1);

    alloc.ResetStatistics();
    stats = alloc.GetStatistics();
    CHECK(stats.allocationCount == 0 && stats.freeCount == 0);

    alloc.Deallocate(nullptr); // no-op, must not crash
    alloc.Shutdown();
    CHECK(!alloc.IsInitialized());
    CHECK(alloc.Allocate(8) == nullptr); // after shutdown -> nullptr
}

static void TestAllocatorAlignment()
{
    arctix::Memory::Allocator alloc;
    CHECK(alloc.Initialize(arctix::Memory::DefaultConfig()));
    const std::size_t alignments[] = { 1, 2, 4, 8, 16, 32, 64 };
    for (std::size_t a : alignments)
    {
        void* p = alloc.Allocate(64, a);
        CHECK(p != nullptr);
        if (p != nullptr)
        {
            const std::size_t effective = (a < 16) ? 16 : a; // platform normalizes to >= 16
            CHECK((reinterpret_cast<std::uintptr_t>(p) % effective) == 0);
            alloc.Deallocate(p);
        }
    }
    CHECK(alloc.Allocate(0) == nullptr); // zero size -> nullptr
    alloc.Shutdown();
}

static void TestMemoryPool()
{
    arctix::Memory::MemoryPool pool;
    CHECK(!pool.IsCreated());
    CHECK(pool.Allocate() == nullptr); // not created -> nullptr
    CHECK(!pool.Create(0, 8));         // invalid args fail
    CHECK(!pool.Create(64, 0));
    CHECK(pool.Create(64, 8));
    CHECK(!pool.Create(64, 8)); // double create fails
    CHECK(pool.IsCreated());
    CHECK(pool.BlockCount() == 8);
    CHECK(pool.FreeCount() == 8);
    CHECK(pool.UsedCount() == 0);

    void* blocks[8] = {};
    for (int i = 0; i < 8; ++i)
    {
        blocks[i] = pool.Allocate();
        CHECK(blocks[i] != nullptr);
    }
    CHECK(pool.Allocate() == nullptr); // exhausted
    CHECK(pool.UsedCount() == 8);

    pool.Free(nullptr); // no-op
    pool.Free(blocks[0]);
    CHECK(pool.UsedCount() == 7);
    blocks[0] = pool.Allocate();
    CHECK(blocks[0] != nullptr);

    for (int i = 0; i < 8; ++i)
    {
        pool.Free(blocks[i]);
    }
    CHECK(pool.UsedCount() == 0);
    pool.Destroy();
    CHECK(!pool.IsCreated());
    pool.Destroy(); // double destroy safe
}

static void TestCApi()
{
    ArctixAllocatorHandle* alloc = Arctix_CreateAllocator(nullptr); // NULL = defaults
    CHECK(alloc != nullptr);
    void* p = Arctix_Allocate(alloc, 64, 0);
    CHECK(p != nullptr);
    ArctixStatsC stats = {};
    Arctix_GetStats(alloc, &stats);
    CHECK(stats.allocationCount == 1 && stats.currentUsageBytes >= 64);
    Arctix_Free(alloc, p);
    Arctix_GetStats(nullptr, nullptr); // NULL-safe no-op
    Arctix_Free(nullptr, nullptr);
    Arctix_DestroyAllocator(alloc);

    ArctixPoolHandle* pool = Arctix_PoolCreate(32, 4, 16);
    CHECK(pool != nullptr);
    void* b = Arctix_PoolAllocate(pool);
    CHECK(b != nullptr);
    Arctix_PoolFree(pool, b);
    Arctix_PoolAllocate(nullptr);
    Arctix_PoolFree(nullptr, nullptr);
    Arctix_PoolDestroy(pool);

    CHECK(Arctix_PoolCreate(0, 4, 16) == nullptr);
    CHECK(Arctix_GetVersionString() != nullptr);
}

int main()
{
    TestAllocatorBasic();
    TestAllocatorAlignment();
    TestMemoryPool();
    TestCApi();
    if (g_failures == 0)
    {
        std::printf("MEMORY-TESTS-OK\n");
        return 0;
    }
    std::printf("MEMORY-TESTS-FAILED (%d)\n", g_failures);
    return 1;
}
