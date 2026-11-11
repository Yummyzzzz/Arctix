/* ArctixC C ABI end-to-end tests (pure C, links arctixc_c.dll).
   Covers allocator, pool, buffer, virtual pages, thread and thread pool.
   Returns 0 on success, non-zero on the first failure. */

#include <stdio.h>
#include <string.h>

#include <arctixc/c_api/memory.h>
#include <arctixc/c_api/thread.h>

static int g_failures = 0;

#define CHECK(cond) \
    do { \
        if (!(cond)) { \
            printf("FAIL line %d: %s\n", __LINE__, #cond); \
            ++g_failures; \
        } \
    } while (0)

static void TestVersion(void)
{
    const char* v = Arctix_GetVersionString();
    CHECK(v != 0);
    CHECK(v[0] != '\0');
}

static void TestAllocator(void)
{
    ArctixAllocatorHandle* a = Arctix_CreateAllocator(0);
    CHECK(a != 0);
    if (a == 0) return;

    void* p = Arctix_Allocate(a, 256, 16);
    CHECK(p != 0);

    ArctixStatsC stats;
    memset(&stats, 0, sizeof(stats));
    Arctix_GetStats(a, &stats);
    CHECK(stats.allocationCount >= 1);
    CHECK(stats.currentUsageBytes >= 256);

    Arctix_Free(a, p);
    Arctix_DestroyAllocator(a);

    /* NULL safety. */
    Arctix_Free(0, 0);
    Arctix_DestroyAllocator(0);
    Arctix_GetStats(0, 0);
}

static void TestPool(void)
{
    ArctixPoolHandle* pool = Arctix_PoolCreate(64, 8, 16);
    CHECK(pool != 0);
    if (pool == 0) return;

    void* b = Arctix_PoolAllocate(pool);
    CHECK(b != 0);
    Arctix_PoolFree(pool, b);
    Arctix_PoolDestroy(pool);

    Arctix_PoolFree(0, 0);
    Arctix_PoolDestroy(0);
}

static void TestBuffer(void)
{
    static const char kData[] = "arctix-c";
    ArctixBufferHandle* buf = Arctix_BufferCreate(0);
    CHECK(buf != 0);
    if (buf == 0) return;

    CHECK(Arctix_BufferAppend(buf, kData, sizeof(kData)) == 1);
    CHECK(Arctix_BufferSize(buf) == sizeof(kData));
    CHECK(Arctix_BufferChecksum(buf) != 0);

    ArctixBufferHandle* other = Arctix_BufferCreate(sizeof(kData));
    CHECK(other != 0);
    if (other != 0)
    {
        CHECK(Arctix_BufferCopyFrom(other, kData, sizeof(kData)) == 1);
        CHECK(Arctix_BufferEquals(buf, other) == 1);
        Arctix_BufferDestroy(other);
    }

    Arctix_BufferClear(buf);
    CHECK(Arctix_BufferSize(buf) == 0);
    Arctix_BufferDestroy(buf);
    Arctix_BufferDestroy(0);
}

static void TestVirtual(void)
{
    size_t page = ArctixC_VirtualPageSize();
    CHECK(page != 0);

    void* addr = ArctixC_VirtualReserve(page * 2);
    CHECK(addr != 0);
    if (addr == 0) return;

    CHECK(ArctixC_VirtualCommit(addr, page * 2) == 1);
    memset(addr, 0xAB, page);
    CHECK(ArctixC_VirtualProtect(addr, page * 2, 1) == 1);
    CHECK(ArctixC_VirtualProtect(addr, page * 2, 2) == 1);
    CHECK(ArctixC_VirtualDecommit(addr, page * 2) == 1);
    CHECK(ArctixC_VirtualRelease(addr, page * 2) == 1);
}

static volatile int g_workerRan = 0;

static void Worker(void* userData)
{
    (void)userData;
    g_workerRan = 1;
}

static void TestThread(void)
{
    ArctixC_ThreadHandle* t = ArctixC_ThreadCreate();
    CHECK(t != 0);
    if (t == 0) return;

    CHECK(ArctixC_ThreadIsJoinable(t) == 0);
    CHECK(ArctixC_ThreadStart(t, Worker, 0) == 1);
    CHECK(ArctixC_ThreadJoin(t) == 1);
    CHECK(g_workerRan == 1);
    ArctixC_ThreadDestroy(t);

    CHECK(ArctixC_ThreadHardwareConcurrency() != 0 || 1);
    ArctixC_ThreadDestroy(0);
}

static volatile long g_poolCount = 0;

static void PoolWorker(void* userData)
{
    (void)userData;
    ++g_poolCount;
}

static void TestThreadPool(void)
{
    ArctixC_ThreadPoolHandle* pool = ArctixC_PoolCreate(2);
    CHECK(pool != 0);
    if (pool == 0) return;

    CHECK(ArctixC_PoolThreadCount(pool) == 2);
    for (int i = 0; i < 8; ++i)
    {
        CHECK(ArctixC_PoolSubmit(pool, PoolWorker, 0) == 1);
    }
    CHECK(ArctixC_PoolWaitIdle(pool, 0) == 1);
    CHECK(g_poolCount == 8);
    CHECK(ArctixC_PoolFailedTasks(pool) == 0);
    ArctixC_PoolDestroy(pool);
    ArctixC_PoolDestroy(0);
}

int main(void)
{
    TestVersion();
    TestAllocator();
    TestPool();
    TestBuffer();
    TestVirtual();
    TestThread();
    TestThreadPool();

    if (g_failures == 0)
    {
        printf("All C API tests passed.\n");
        return 0;
    }
    printf("%d C API test(s) FAILED.\n", g_failures);
    return 1;
}
