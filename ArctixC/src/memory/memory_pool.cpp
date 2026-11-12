#include "arctixc/memory/memory_pool.hpp"

#include "memory/windows_memory.hpp"

#include <limits>
#include <mutex>
#include <vector>

namespace arctix::Memory
{

// Private state for MemoryPool (opaque to SDK users).
struct MemoryPoolImpl
{
    std::size_t blockSize = 0;
    std::size_t blockCount = 0;
    std::size_t alignment = 16;
    bool created = false;
    void* backingMemory = nullptr;
    std::vector<void*> freeList{};
    // In-use bitmap: live[i] == 1 while block i is checked out via
    // Allocate(), 0 while it sits in freeList. Lets Free() reject
    // double-frees in O(1) instead of scanning the free list.
    std::vector<char> live{};
    std::mutex mutex{};
};

MemoryPool::MemoryPool()
    : impl_(new MemoryPoolImpl())
{
}

MemoryPool::~MemoryPool()
{
    Destroy();
    delete impl_;
}

bool MemoryPool::Create(std::size_t blockSize, std::size_t blockCount, std::size_t alignment)
{
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (impl_->created || blockSize == 0 || blockCount == 0)
    {
        return false;
    }

    impl_->alignment = platform::NormalizeAlignment(alignment);

    // Round block size up to alignment so every block stays aligned.
    // Guard the arithmetic: (stride + mask) and (stride * blockCount)
    // must not wrap around, or AlignedAlloc would receive a smaller
    // size than the loop below writes (heap overflow).
    constexpr std::size_t kMax = (std::numeric_limits<std::size_t>::max)();
    const std::size_t mask = impl_->alignment - 1;
    std::size_t stride = blockSize;
    if ((stride & mask) != 0)
    {
        if (stride > kMax - mask)
        {
            return false; // rounding up would overflow
        }
        stride = (stride + mask) & ~mask;
    }
    if (stride > kMax / blockCount)
    {
        return false; // total backing size would overflow
    }

    void* backing = platform::AlignedAlloc(stride * blockCount, impl_->alignment, nullptr);
    if (backing == nullptr)
    {
        return false;
    }

    // Tracking allocations below can throw (bad_alloc) AFTER the OS
    // memory is already held. Roll back on failure so Create() reports
    // false with no leak and no half-created state.
    impl_->backingMemory = backing;
    impl_->blockSize = stride;
    impl_->blockCount = blockCount;
    try
    {
        impl_->freeList.reserve(blockCount);
        impl_->live.assign(blockCount, 0);
        char* base = static_cast<char*>(backing);
        for (std::size_t i = 0; i < blockCount; ++i)
        {
            impl_->freeList.push_back(base + i * stride);
        }
    }
    catch (...)
    {
        platform::AlignedFree(backing);
        impl_->backingMemory = nullptr;
        impl_->freeList.clear();
        impl_->live.clear();
        impl_->blockSize = 0;
        impl_->blockCount = 0;
        return false;
    }
    impl_->created = true;
    return true;
}

void MemoryPool::Destroy()
{
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->created)
    {
        return;
    }
    platform::AlignedFree(impl_->backingMemory);
    impl_->backingMemory = nullptr;
    impl_->freeList.clear();
    impl_->live.clear();
    impl_->blockSize = 0;
    impl_->blockCount = 0;
    impl_->created = false;
}

bool MemoryPool::IsCreated() const
{
    return impl_->created;
}

void* MemoryPool::Allocate()
{
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->created || impl_->freeList.empty())
    {
        return nullptr;
    }
    void* block = impl_->freeList.back();
    impl_->freeList.pop_back();
    // Mark checked out. The block came from our own list, so the index
    // math below cannot fail; guard anyway for hardened builds.
    const char* base = static_cast<const char*>(impl_->backingMemory);
    const std::size_t offset = static_cast<const char*>(block) - base;
    const std::size_t index = offset / impl_->blockSize;
    if (index < impl_->live.size())
    {
        impl_->live[index] = 1;
    }
    return block;
}

void MemoryPool::Free(void* pointer)
{
    if (pointer == nullptr)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->created)
    {
        return;
    }
    // Validate before touching the free list. Unchecked pushes turn
    // caller bugs into pool corruption: a foreign pointer gets handed
    // out later as if it were ours, and a double-free hands the same
    // block to two owners (plus UsedCount() wraps around below).
    const char* base = static_cast<const char*>(impl_->backingMemory);
    const char* target = static_cast<const char*>(pointer);
    // Overflow-safe range check: use offsets, never base + total.
    if (target < base)
    {
        return; // before our backing memory: foreign
    }
    const std::size_t offset = static_cast<std::size_t>(target - base);
    if (offset / impl_->blockSize >= impl_->blockCount)
    {
        return; // past the end: foreign
    }
    if (offset % impl_->blockSize != 0)
    {
        return; // interior pointer, not a block start: foreign
    }
    const std::size_t index = offset / impl_->blockSize;
    if (index >= impl_->live.size() || impl_->live[index] == 0)
    {
        return; // already returned (double-free) or never checked out
    }
    impl_->live[index] = 0;
    // Cannot throw: freeList holds at most blockCount entries and was
    // reserved for blockCount in Create(), so capacity is available.
    impl_->freeList.push_back(pointer);
}

std::size_t MemoryPool::BlockSize() const
{
    return impl_->blockSize;
}

std::size_t MemoryPool::BlockCount() const
{
    return impl_->blockCount;
}

std::size_t MemoryPool::FreeCount() const
{
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->freeList.size();
}

std::size_t MemoryPool::UsedCount() const
{
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->created)
    {
        return 0;
    }
    // Saturate instead of wrapping: with validated Free() the list can
    // never exceed blockCount, but a wrap to SIZE_MAX here would be a
    // far worse bug than a clamped 0.
    if (impl_->freeList.size() >= impl_->blockCount)
    {
        return 0;
    }
    return impl_->blockCount - impl_->freeList.size();
}

} // namespace arctix::Memory
