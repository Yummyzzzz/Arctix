#include "memory/memory_manager.hpp"

namespace arctix::Memory
{

MemoryManager::MemoryManager()
    : threadSafe_(true)
{
}

void MemoryManager::Configure(bool threadSafe, bool enableStatistics)
{
    threadSafe_ = threadSafe;
    stats_.SetEnabled(enableStatistics);
    stats_.Reset();
}

void MemoryManager::ResetStatistics()
{
    if (threadSafe_)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stats_.Reset();
    }
    else
    {
        stats_.Reset();
    }
}

void MemoryManager::Register(void* pointer, std::size_t size)
{
    if (pointer == nullptr)
    {
        return;
    }
    if (threadSafe_)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        live_[pointer] = size;
        stats_.OnAllocate(size);
    }
    else
    {
        live_[pointer] = size;
        stats_.OnAllocate(size);
    }
}

std::size_t MemoryManager::Unregister(void* pointer)
{
    if (pointer == nullptr)
    {
        return 0;
    }
    if (threadSafe_)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto it = live_.find(pointer);
        if (it == live_.end())
        {
            return 0;
        }
        const std::size_t size = it->second;
        live_.erase(it);
        stats_.OnFree(size);
        return size;
    }

    const auto it = live_.find(pointer);
    if (it == live_.end())
    {
        return 0;
    }
    const std::size_t size = it->second;
    live_.erase(it);
    stats_.OnFree(size);
    return size;
}

ArctixStats MemoryManager::GetStatistics() const
{
    if (threadSafe_)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return stats_.Snapshot();
    }
    return stats_.Snapshot();
}

} // namespace arctix::Memory
