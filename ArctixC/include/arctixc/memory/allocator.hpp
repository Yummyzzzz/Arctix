#pragma once

// ArctixC.Memory - public general-purpose allocator interface.
// Declaration header. Implementation lives in src/memory/allocator.cpp
// (that .cpp includes this header).
//
// Lifecycle: default-construct, call Initialize() once, use
// Allocate()/Deallocate(), then call Shutdown(). All methods except
// Initialize()/Shutdown() require a prior successful Initialize().

#include "arctixc/memory/export.hpp"
#include "arctixc/memory/config.hpp"
#include "arctixc/memory/statistics.hpp"

#include <cstddef>

namespace arctix
{

// Opaque internal state. Defined privately in src/memory/.
struct AllocatorImpl;

// General-purpose aligned allocator.
//
// Thread safety and statistics behavior are controlled by the
// ArctixConfig passed to Initialize().
class ARCTIXC_MEMORY_API Allocator
{
public:
    Allocator();
    ~Allocator();

    Allocator(const Allocator&) = delete;
    Allocator& operator=(const Allocator&) = delete;

    // Initializes internal state from the given configuration.
    // Returns true on success, false if already initialized.
    bool Initialize(const ArctixConfig& config);

    // Releases internal state. All outstanding allocations must have
    // been freed with Deallocate() before calling this.
    void Shutdown();

    // Returns true if Initialize() succeeded and Shutdown() was not called.
    bool IsInitialized() const;

    // Allocates `size` bytes with the given `alignment` (power of two).
    // If alignment == 0, the config's defaultAlignment is used.
    // Returns nullptr on failure (not initialized, size == 0,
    // or underlying OS allocation failed).
    void* Allocate(std::size_t size, std::size_t alignment = 0);

    // Frees a pointer previously returned by Allocate().
    // Passing nullptr is a no-op. Must only be called with
    // pointers owned by this allocator instance.
    void Deallocate(void* pointer);

    // Returns a snapshot of current statistics.
    ArctixStats GetStatistics() const;

    // Resets cumulative statistics counters to zero.
    void ResetStatistics();

private:
    AllocatorImpl* impl_;
};

} // namespace arctix
