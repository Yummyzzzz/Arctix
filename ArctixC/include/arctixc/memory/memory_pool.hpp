#pragma once

// ArctixC.Memory - public fixed-size memory pool interface.
// Declaration header. Implementation lives in src/memory/memory_pool.cpp
// (that .cpp includes this header).
//
// Pre-allocates `blockCount` blocks of `blockSize` bytes once, then
// hands them out in O(1) without per-allocation OS calls. Ideal for
// entities, particles, nodes, and other hot fixed-size objects.
// All blocks share the alignment given at creation time.

#include "arctixc/memory/export.hpp"

#include <cstddef>

namespace arctixs::Memory
{

// Opaque internal state. Defined privately in src/memory/.
struct MemoryPoolImpl;

class ARCTIXC_MEMORY_API MemoryPool
{
public:
    MemoryPool();
    ~MemoryPool();

    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    // Creates the pool: blockSize bytes per block, blockCount blocks.
    // alignment must be a power of two (0 = use 16).
    // Returns true on success, false on invalid args / already created.
    bool Create(std::size_t blockSize, std::size_t blockCount, std::size_t alignment = 16);

    // Destroys the pool and releases its backing memory.
    // All blocks must have been freed with Free() before calling this.
    void Destroy();

    // Returns true if Create() succeeded and Destroy() was not called.
    bool IsCreated() const;

    // Returns one free block, or nullptr if the pool is exhausted.
    void* Allocate();

    // Returns a block previously obtained from Allocate().
    // Passing nullptr is a no-op.
    void Free(void* pointer);

    // Capacity queries (valid after Create()).
    std::size_t BlockSize() const;
    std::size_t BlockCount() const;
    std::size_t FreeCount() const;
    std::size_t UsedCount() const;

private:
    MemoryPoolImpl* impl_;
};

} // namespace arctixs::Memory
