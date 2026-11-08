// Example: combined consumer using both Memory and Thread.
// Links ArctixC::ArctixC. Ships with the single ArctixC.dll.
// Proves the subsystems compose: a worker allocates from a pool.

#include <arctixc/memory/memory_pool.hpp>
#include <arctixc/thread/thread.hpp>

#include <cstdio>

namespace
{

void Worker(void* userData)
{
    arctix::MemoryPool* pool = static_cast<arctix::MemoryPool*>(userData);
    void* block = pool->Allocate();
    if (block != nullptr)
    {
        pool->Free(block);
    }
}

} // namespace

int main()
{
    arctix::MemoryPool pool;
    if (!pool.Create(128, 8))
    {
        return 1;
    }

    arctix::Thread thread;
    if (!thread.Start(Worker, &pool))
    {
        return 1;
    }
    if (!thread.Join())
    {
        return 1;
    }

    pool.Destroy();
    std::printf("memory-and-thread example OK\n");
    return 0;
}
