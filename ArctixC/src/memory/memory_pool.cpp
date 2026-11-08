#include "arctixc/memory/memory_pool.hpp"

#include "memory/windows_memory.hpp"

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
    std::size_t stride = blockSize;
    const std::size_t mask = impl_->alignment - 1;
    if ((stride & mask) != 0)
    {
        stride = (stride + mask) & ~mask;
    }

    void* backing = platform::AlignedAlloc(stride * blockCount, impl_->alignment, nullptr);
    if (backing == nullptr)
    {
        return false;
    }

    impl_->backingMemory = backing;
    impl_->blockSize = stride;
    impl_->blockCount = blockCount;
    impl_->created = true;

    impl_->freeList.reserve(blockCount);
    char* base = static_cast<char*>(backing);
    for (std::size_t i = 0; i < blockCount; ++i)
    {
        impl_->freeList.push_back(base + i * stride);
    }
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
    return impl_->blockCount - impl_->freeList.size();
}

} // namespace arctix::Memory
