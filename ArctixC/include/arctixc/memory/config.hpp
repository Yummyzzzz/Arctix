#pragma once

// ArctixC.Memory - public allocator configuration.
// Types-only header: no corresponding .cpp.

#include <cstddef>

namespace arctixs::Memory
{

// Configuration passed to Allocator::Initialize().
struct ArctixConfig
{
    // When true, Allocate/Deallocate are safe to call from multiple threads.
    // When false, a faster non-locking path is used. Default: true.
    bool threadSafe = true;

    // When true, allocation statistics are tracked (see statistics.hpp).
    // Disable for absolute minimum overhead. Default: true.
    bool enableStatistics = true;

    // Default alignment (in bytes) used when Allocate() is called
    // without an explicit alignment. Must be a power of two.
    // Default: 16 bytes (SSE / general purpose).
    std::size_t defaultAlignment = 16;
};

// Helper that returns a default-initialized configuration.
inline ArctixConfig DefaultConfig()
{
    return ArctixConfig{};
}

} // namespace arctixs::Memory
