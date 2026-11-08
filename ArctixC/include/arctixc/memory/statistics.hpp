#pragma once

// ArctixC.Memory - public allocation statistics.
// Types-only header: no corresponding .cpp.

#include <cstddef>

namespace arctix
{

// Snapshot of allocator activity. Returned by value; safe to read
// from the calling thread after calling Allocator::GetStatistics().
struct ArctixStats
{
    // Total bytes ever handed out by Allocate().
    std::size_t totalAllocatedBytes = 0;

    // Total bytes ever returned via Deallocate().
    std::size_t totalFreedBytes = 0;

    // Currently outstanding bytes (totalAllocatedBytes - totalFreedBytes).
    std::size_t currentUsageBytes = 0;

    // Highest observed currentUsageBytes since Initialize() / ResetStatistics().
    std::size_t peakUsageBytes = 0;

    // Number of successful Allocate() calls.
    unsigned long long allocationCount = 0;

    // Number of Deallocate() calls.
    unsigned long long freeCount = 0;
};

} // namespace arctix
