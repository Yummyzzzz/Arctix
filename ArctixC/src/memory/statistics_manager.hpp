#pragma once

// ArctixC.Memory (private) - internal statistics tracker.
// Declaration header. Implementation in statistics_manager.cpp.

#include "arctixc/memory/statistics.hpp"

#include <cstddef>

namespace arctix::Memory
{

// Tracks allocation totals. The owner (Allocator / Pool) is responsible
// for locking when thread safety is enabled; this class itself does no
// locking so it stays usable in both locked and unlocked paths.
class StatisticsManager
{
public:
    StatisticsManager();
    void SetEnabled(bool enabled);

    void OnAllocate(std::size_t bytes);
    void OnFree(std::size_t bytes);
    void Reset();

    ArctixStats Snapshot() const;

private:
    bool enabled_;
    ArctixStats stats_;
};

} // namespace arctix::Memory
