// ArctixC extended tests: Buffer, mem utils, virtual memory, ThreadPool,
// ParallelFor. Links ArctixC::ArctixC. Fails (non-zero exit + message) on
// any failure.

#include <atomic>
#include <cstdio>
#include <cstring>

#include "arctixc/memory/allocator.hpp"
#include "arctixc/memory/buffer.hpp"
#include "arctixc/memory/mem_utils.hpp"
#include "arctixc/memory/virtual_memory.hpp"
#include "arctixc/thread/thread_pool.hpp"

static int g_failures = 0;

#define CHECK(cond) \
    do { \
        if (!(cond)) { \
            std::printf("FAIL line %d: %s\n", __LINE__, #cond); \
            ++g_failures; \
        } \
    } while (0)

namespace
{

void IncrementCounter(void* userData)
{
    auto* counter = static_cast<std::atomic<int>*>(userData);
    counter->fetch_add(1, std::memory_order_relaxed);
}

void ThrowWorker(void*)
{
    throw 42;
}

void SquareIndex(std::size_t index, void* userData)
{
    auto* out = static_cast<unsigned long long*>(userData);
    out[index] = static_cast<unsigned long long>(index * index);
}

} // namespace

static void TestBuffer()
{
    arctix::Buffer::Buffer empty;
    CHECK(empty.Empty());
    CHECK(empty.Size() == 0);
    CHECK(empty.Data() == nullptr);
    CHECK(empty.Checksum() == 0);
    CHECK(empty.HexEncode().empty());

    arctix::Buffer::Buffer buf(4, 0xAB);
    CHECK(buf.Size() == 4);
    CHECK(buf.Capacity() >= 4);
    CHECK(buf.Data()[0] == 0xAB && buf.Data()[3] == 0xAB);
    CHECK(buf.HexEncode() == "ABABABAB");

    const char hello[] = "hello";
    CHECK(buf.CopyFrom(hello, 5));
    CHECK(buf.Size() == 5);
    CHECK(std::memcmp(buf.Data(), "hello", 5) == 0);

    CHECK(buf.Append(" world", 6));
    CHECK(buf.Size() == 11);
    CHECK(std::memcmp(buf.Data(), "hello world", 11) == 0);

    arctix::Buffer::Buffer other;
    CHECK(other.CopyFrom("hello world", 11));
    CHECK(buf.Equals(other));
    CHECK(buf.Checksum() == other.Checksum());
    other.Fill(0x00);
    CHECK(!buf.Equals(other));

    CHECK(buf.Resize(4, 0));
    CHECK(buf.Size() == 4);
    CHECK(std::memcmp(buf.Data(), "hell", 4) == 0);
    const std::size_t capBefore = buf.Capacity();
    CHECK(buf.Reserve(capBefore + 1024));
    CHECK(buf.Capacity() >= capBefore + 1024);
    CHECK(buf.Size() == 4);

    buf.Clear();
    CHECK(buf.Empty());
    CHECK(buf.Capacity() >= capBefore + 1024); // allocation kept

    // Move semantics.
    CHECK(other.CopyFrom("moveme", 6));
    arctix::Buffer::Buffer moved(std::move(other));
    CHECK(moved.Size() == 6);
    CHECK(std::memcmp(moved.Data(), "moveme", 6) == 0);

    // Invalid args.
    CHECK(!buf.CopyFrom(nullptr, 8));
    CHECK(!buf.Append(nullptr, 8));

    // SecureWipe zeroes and empties.
    arctix::Buffer::Buffer secret(16, 0xFF);
    secret.SecureWipe();
    CHECK(secret.Empty());
}

static void TestMemUtils()
{
    unsigned char data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    CHECK(arctix::Memory::MemEqual(data, data, 8));
    unsigned char same[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    unsigned char diff[8] = {1, 2, 3, 4, 5, 6, 7, 9};
    CHECK(arctix::Memory::MemEqual(data, same, 8));
    CHECK(!arctix::Memory::MemEqual(data, diff, 8));
    CHECK(arctix::Memory::MemEqual(nullptr, nullptr, 0));
    CHECK(!arctix::Memory::MemEqual(nullptr, data, 8));

    const std::uint64_t h1 = arctix::Memory::Fnv1aHash64(data, 8);
    const std::uint64_t h2 = arctix::Memory::Fnv1aHash64(same, 8);
    const std::uint64_t h3 = arctix::Memory::Fnv1aHash64(diff, 8);
    CHECK(h1 != 0 && h1 == h2 && h1 != h3);
    CHECK(arctix::Memory::Fnv1aHash64(nullptr, 0) == 0);

    arctix::Memory::SecureZero(data, sizeof(data));
    for (unsigned char b : data)
    {
        CHECK(b == 0);
    }
    arctix::Memory::SecureZero(nullptr, 0); // must not crash
}

static void TestVirtualMemory()
{
    const std::size_t page = arctix::Memory::VirtualPageSize();
    CHECK(page >= 4096);
    CHECK(arctix::Memory::RoundUpToPageSize(1) == page);
    CHECK(arctix::Memory::RoundUpToPageSize(page) == page);
    CHECK(arctix::Memory::RoundUpToPageSize(page + 1) == page * 2);
    CHECK(arctix::Memory::RoundUpToPageSize(0) == 0);

    void* region = arctix::Memory::VirtualReserve(page * 2);
    CHECK(region != nullptr);
    CHECK(arctix::Memory::IsPageAligned(region));
    CHECK(!arctix::Memory::IsPageAligned(nullptr));

    CHECK(arctix::Memory::VirtualCommit(region, page * 2));
    auto* bytes = static_cast<unsigned char*>(region);
    for (std::size_t i = 0; i < page * 2; ++i)
    {
        CHECK(bytes[i] == 0); // OS zero-fills
    }
    bytes[0] = 0x42;
    bytes[page * 2 - 1] = 0x24;
    CHECK(bytes[0] == 0x42 && bytes[page * 2 - 1] == 0x24);

    CHECK(arctix::Memory::VirtualProtectMemory(region, page * 2, arctix::Memory::MemoryProtection::ReadOnly));
    CHECK(arctix::Memory::VirtualProtectMemory(region, page * 2, arctix::Memory::MemoryProtection::ReadWrite));
    CHECK(!arctix::Memory::VirtualProtectMemory(nullptr, page, arctix::Memory::MemoryProtection::ReadWrite));

    CHECK(arctix::Memory::VirtualDecommit(region, page * 2));
    // Recommit after decommit must work with the same reservation.
    CHECK(arctix::Memory::VirtualCommit(region, page));
    static_cast<unsigned char*>(region)[0] = 0x11;

    CHECK(arctix::Memory::VirtualRelease(region, page * 2));

    // Invalid args.
    CHECK(arctix::Memory::VirtualReserve(0) == nullptr);
    CHECK(!arctix::Memory::VirtualCommit(nullptr, page));
    CHECK(!arctix::Memory::VirtualRelease(nullptr, page));
}

static void TestThreadPool()
{
    arctix::Thread::ThreadPool pool(4);
    CHECK(pool.IsRunning());
    CHECK(pool.ThreadCount() == 4);
    CHECK(pool.PendingTasks() == 0);

    std::atomic<int> counter(0);
    const int kTasks = 200;
    for (int i = 0; i < kTasks; ++i)
    {
        CHECK(pool.Submit(IncrementCounter, &counter));
    }
    CHECK(pool.WaitIdle());
    CHECK(counter.load() == kTasks);
    CHECK(pool.PendingTasks() == 0);
    CHECK(pool.RunningTasks() == 0);
    CHECK(pool.FailedTasks() == 0);

    CHECK(!pool.Submit(nullptr, nullptr));

    // Worker exceptions are counted, never escape.
    CHECK(pool.Submit(ThrowWorker, nullptr));
    CHECK(pool.WaitIdle());
    CHECK(pool.FailedTasks() == 1);

    // Timeout path: pool is idle so even 1ms succeeds.
    CHECK(pool.WaitIdle(1));

    pool.Shutdown();
    CHECK(!pool.IsRunning());
    CHECK(pool.ThreadCount() == 0);
    CHECK(!pool.Submit(IncrementCounter, &counter)); // shut down: refused
    pool.Shutdown(); // idempotent

    // Default construction uses hardware threads (at least 1).
    arctix::Thread::ThreadPool autoPool;
    CHECK(autoPool.IsRunning());
    CHECK(autoPool.ThreadCount() >= 1);
}

static void TestParallelFor()
{
    constexpr std::size_t kCount = 1000;
    unsigned long long squares[kCount] = {};
    CHECK(arctix::Thread::ParallelFor(0, kCount, SquareIndex, squares, 4));
    for (std::size_t i = 0; i < kCount; ++i)
    {
        CHECK(squares[i] == i * i);
    }
    // Empty range is a no-op success.
    CHECK(arctix::Thread::ParallelFor(5, 5, SquareIndex, squares, 2));
    // Invalid args.
    CHECK(!arctix::Thread::ParallelFor(0, 10, nullptr, squares, 2));
    CHECK(!arctix::Thread::ParallelFor(10, 0, SquareIndex, squares, 2));

    // ParallelFor works alongside the allocator in one process.
    arctix::Memory::Allocator allocator;
    CHECK(allocator.Initialize(arctix::Memory::DefaultConfig()));
    void* block = allocator.Allocate(64, 16);
    CHECK(block != nullptr);
    allocator.Deallocate(block);
    allocator.Shutdown();
}

int main()
{
    TestBuffer();
    TestMemUtils();
    TestVirtualMemory();
    TestThreadPool();
    TestParallelFor();

    if (g_failures == 0)
    {
        std::printf("EXTENDED-TESTS-OK\n");
        return 0;
    }
    std::printf("EXTENDED-TESTS-FAILED (%d)\n", g_failures);
    return 1;
}
