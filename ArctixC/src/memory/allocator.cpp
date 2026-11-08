#include "arctixc/memory/allocator.hpp"

#include "memory/memory_manager.hpp"
#include "memory/windows_memory.hpp"

#include <mutex>

namespace arctixs::Memory
{

// Private state for Allocator (opaque to SDK users).
// Defined here, not in any header, so internals never leak into the SDK.
struct AllocatorImpl
{
    ArctixConfig config{};
    MemoryManager manager{};
    bool initialized = false;
    std::mutex initMutex{};
};

Allocator::Allocator()
    : impl_(new AllocatorImpl())
{
}

Allocator::~Allocator()
{
    delete impl_;
}

bool Allocator::Initialize(const ArctixConfig& config)
{
    std::lock_guard<std::mutex> lock(impl_->initMutex);
    if (impl_->initialized)
    {
        return false;
    }
    impl_->config = config;
    if (impl_->config.defaultAlignment == 0)
    {
        impl_->config.defaultAlignment = 16;
    }
    impl_->manager.Configure(impl_->config.threadSafe, impl_->config.enableStatistics);
    impl_->initialized = true;
    return true;
}

void Allocator::Shutdown()
{
    std::lock_guard<std::mutex> lock(impl_->initMutex);
    impl_->initialized = false;
}

bool Allocator::IsInitialized() const
{
    return impl_->initialized;
}

void* Allocator::Allocate(std::size_t size, std::size_t alignment)
{
    if (!impl_->initialized || size == 0)
    {
        return nullptr;
    }

    const std::size_t effectiveAlignment =
        (alignment == 0) ? impl_->config.defaultAlignment : alignment;

    std::size_t usableSize = 0;
    void* pointer = platform::AlignedAlloc(size, effectiveAlignment, &usableSize);
    if (pointer != nullptr)
    {
        impl_->manager.Register(pointer, usableSize);
    }
    return pointer;
}

void Allocator::Deallocate(void* pointer)
{
    if (pointer == nullptr || !impl_->initialized)
    {
        return;
    }
    impl_->manager.Unregister(pointer);
    platform::AlignedFree(pointer);
}

ArctixStats Allocator::GetStatistics() const
{
    return impl_->manager.GetStatistics();
}

void Allocator::ResetStatistics()
{
    impl_->manager.ResetStatistics();
}

} // namespace arctixs::Memory
