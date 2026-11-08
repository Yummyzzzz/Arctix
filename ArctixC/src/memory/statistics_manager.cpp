#include "memory/statistics_manager.hpp"

namespace arctix
{

StatisticsManager::StatisticsManager()
    : enabled_(true)
{
}

void StatisticsManager::SetEnabled(bool enabled)
{
    enabled_ = enabled;
}

void StatisticsManager::OnAllocate(std::size_t bytes)
{
    if (!enabled_)
    {
        return;
    }
    stats_.totalAllocatedBytes += bytes;
    stats_.currentUsageBytes += bytes;
    stats_.allocationCount += 1;
    if (stats_.currentUsageBytes > stats_.peakUsageBytes)
    {
        stats_.peakUsageBytes = stats_.currentUsageBytes;
    }
}

void StatisticsManager::OnFree(std::size_t bytes)
{
    if (!enabled_)
    {
        return;
    }
    stats_.totalFreedBytes += bytes;
    if (bytes >= stats_.currentUsageBytes)
    {
        stats_.currentUsageBytes = 0;
    }
    else
    {
        stats_.currentUsageBytes -= bytes;
    }
    stats_.freeCount += 1;
}

void StatisticsManager::Reset()
{
    stats_ = ArctixStats{};
}

ArctixStats StatisticsManager::Snapshot() const
{
    return stats_;
}

} // namespace arctix
