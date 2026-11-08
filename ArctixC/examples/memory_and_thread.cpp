// Example: combined consumer using both Memory and Thread.
// Links ArctixC::ArctixC. Ships with the single arctixc.dll.
// Proves the subsystems compose: a worker allocates from a pool.

#include <arctixc/memory/memory_pool.hpp>
#include <arctixc/thread/thread.hpp>

#include <cstdio>

namespace
{

void Worker(void* userData)
{
    arctixs::Memory::MemoryPool* pool = static_cast<arctixs::Memory::MemoryPool*>(userData);
    void* block = pool->Allocate();
    if (block != nullptr)
    {
        pool->Free(block);
    }
}

} // namespace

int main()
{
    arctixs::Memory::MemoryPool pool;
    if (!pool.Create(128, 8))
    {
        return 1;
    }

    arctixs::Thread::Thread thread;
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
