/* ArctixC C-ABI check: compiled as pure C (not C++).
   Proves both C headers are genuinely C-compatible. */

#include <arctixc/c_api/memory.h>
#include <arctixc/c_api/thread.h>

static void Worker(void* userData)
{
    (void)userData;
}

int ArctixC_CApiCheckRun(void)
{
    ArctixAllocatorHandle* alloc = Arctix_CreateAllocator(0);
    if (alloc == 0)
    {
        return 1;
    }
    void* p = Arctix_Allocate(alloc, 32, 0);
    if (p == 0)
    {
        Arctix_DestroyAllocator(alloc);
        return 2;
    }
    Arctix_Free(alloc, p);
    Arctix_DestroyAllocator(alloc);

    ArctixC_ThreadHandle* thread = ArctixC_ThreadCreate();
    if (thread == 0)
    {
        return 3;
    }
    if (ArctixC_ThreadStart(thread, Worker, 0) == 0)
    {
        ArctixC_ThreadDestroy(thread);
        return 4;
    }
    if (ArctixC_ThreadJoin(thread) == 0)
    {
        ArctixC_ThreadDestroy(thread);
        return 5;
    }
    ArctixC_ThreadDestroy(thread);
    return 0;
}
