// Example: memory consumer.
// Links ArctixC::ArctixC. Ships with the single ArctixC.dll.

#include <arctixc/memory/allocator.hpp>
#include <arctixc/memory/memory_pool.hpp>

#include <cstdio>

int main()
{
    arctix::Allocator allocator;
    if (!allocator.Initialize(arctix::DefaultConfig()))
    {
        return 1;
    }

    void* block = allocator.Allocate(256, 16);
    if (block == nullptr)
    {
        return 1;
    }
    allocator.Deallocate(block);
    allocator.Shutdown();

    arctix::MemoryPool pool;
    if (!pool.Create(64, 16))
    {
        return 1;
    }
    void* item = pool.Allocate();
    if (item == nullptr)
    {
        return 1;
    }
    pool.Free(item);
    pool.Destroy();

    std::printf("memory-only example OK\n");
    return 0;
}
