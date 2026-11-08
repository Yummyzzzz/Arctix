#pragma once

// ArctixC.Memory (private) - tracks live allocations for one Allocator.
// Declaration header. Implementation in memory_manager.cpp.

#include "memory/statistics_manager.hpp"

#include <cstddef>
#include <mutex>
#include <unordered_map>

namespace arctix::Memory
{

// Records pointer -> size for every live Allocate() so Deallocate()
// can credit the statistics tracker with the correct byte count.
// Locking is enabled/disabled from ArctixConfig::threadSafe.
class MemoryManager
{
public:
    MemoryManager();

    void Configure(bool threadSafe, bool enableStatistics);
    void ResetStatistics();

    // Called after a successful OS allocation.
    void Register(void* pointer, std::size_t size);

    // Called on Deallocate(). Returns the tracked size (0 if unknown).
    // Removes the entry. Unknown pointers return 0 and are NOT freed
    // by the caller... the caller still frees the OS pointer, but no
    // statistics are updated for it.
    std::size_t Unregister(void* pointer);

    ArctixStats GetStatistics() const;

private:
    bool threadSafe_;
    mutable std::mutex mutex_;
    std::unordered_map<void*, std::size_t> live_;
    StatisticsManager stats_;
};

} // namespace arctix::Memory
