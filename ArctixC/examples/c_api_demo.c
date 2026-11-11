/* ArctixC C demo (pure C, links arctixc_c.lib, runs on arctixc_c.dll).
   Allocates memory, hashes a buffer, and runs work on a thread pool. */

#include <stdio.h>

#include <arctixc/c_api/memory.h>
#include <arctixc/c_api/thread.h>

static unsigned long long g_hash = 0;
static const char* g_text = "hello from arctixc_c.dll";

static void HashWork(void* userData)
{
    ArctixBufferHandle* buf = (ArctixBufferHandle*)userData;
    g_hash = Arctix_BufferChecksum(buf);
}

int main(void)
{
    printf("ArctixC C demo (arctixc_c.dll %s)\n", Arctix_GetVersionString());

    ArctixAllocatorHandle* alloc = Arctix_CreateAllocator(0);
    if (alloc == 0)
    {
        printf("Arctix_CreateAllocator failed\n");
        return 1;
    }

    ArctixBufferHandle* buf = Arctix_BufferCreate(64);
    if (buf == 0)
    {
        printf("Arctix_BufferCreate failed\n");
        Arctix_DestroyAllocator(alloc);
        return 1;
    }

    Arctix_BufferCopyFrom(buf, g_text, 22);
    printf("buffer size: %u bytes\n", (unsigned)Arctix_BufferSize(buf));

    ArctixC_ThreadPoolHandle* pool = ArctixC_PoolCreate(2);
    if (pool == 0)
    {
        printf("ArctixC_PoolCreate failed\n");
        Arctix_BufferDestroy(buf);
        Arctix_DestroyAllocator(alloc);
        return 1;
    }

    ArctixC_PoolSubmit(pool, HashWork, buf);
    ArctixC_PoolWaitIdle(pool, 0);
    printf("checksum: %llu\n", g_hash);

    ArctixC_PoolDestroy(pool);
    Arctix_BufferDestroy(buf);

    void* p = Arctix_Allocate(alloc, 256, 16);
    printf("allocator probe: %s\n", p != 0 ? "ok" : "failed");
    Arctix_Free(alloc, p);
    Arctix_DestroyAllocator(alloc);
    return 0;
}
