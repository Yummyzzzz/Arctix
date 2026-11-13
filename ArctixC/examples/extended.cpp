// Example: tour of the v0.2.0 additions - Buffer, virtual memory,
// ThreadPool and ParallelFor, all from the single arctixc.dll.

#include <arctixc/memory/buffer.hpp>
#include <arctixc/memory/virtual_memory.hpp>
#include <arctixc/thread/thread_pool.hpp>

#include <cstdio>

namespace
{

void PrintSquare(std::size_t index, void* userData)
{
    (void)userData;
    std::printf("  square(%zu) = %llu\n", index, static_cast<unsigned long long>(index * index));
}

} // namespace

int main()
{
    // Buffer: stage bytes, hash them, wipe them.
    arctix::Buffer::Buffer buf;
    if (!buf.CopyFrom("ArctixC", 7) || !buf.Append(" v0.2.0", 7))
    {
        return 1;
    }
    std::printf("buffer: %s (checksum %llu)\n", buf.HexEncode().c_str(),
                static_cast<unsigned long long>(buf.Checksum()));
    buf.SecureWipe();

    // Virtual memory: reserve two pages, commit, write, release.
    const std::size_t page = arctix::Memory::VirtualPageSize();
    void* region = arctix::Memory::VirtualReserve(page * 2);
    if (region == nullptr || !arctix::Memory::VirtualCommit(region, page * 2))
    {
        return 1;
    }
    static_cast<char*>(region)[0] = 'A';
    std::printf("virtual memory: page=%zu first byte=%c\n", page, static_cast<char*>(region)[0]);
    if (!arctix::Memory::VirtualRelease(region, page * 2))
    {
        return 1;
    }

    // Thread pool + parallel loop.
    if (!arctix::Thread::ParallelFor(0, 5, PrintSquare, nullptr, 2))
    {
        return 1;
    }

    std::printf("extended example OK\n");
    return 0;
}
