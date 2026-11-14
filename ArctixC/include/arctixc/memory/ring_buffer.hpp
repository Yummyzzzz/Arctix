#pragma once

// ArctixC - fixed-capacity byte ring buffer.
// Declaration header. Implementation lives in src/memory/ring_buffer.cpp
// (that .cpp includes this header).
//
// RingBuffer owns a contiguous fixed-size byte array used as a FIFO:
// Write() appends at the tail, Read() consumes from the head, both with
// wrap-around. All operations are all-or-nothing: Write() fails when
// `size` exceeds FreeSpace(), Read()/Peek() fail when `size` exceeds
// Size(). No partial writes or reads, no overwrite mode, no blocking.
//
// Thread safety: every method is mutex-protected (like MemoryPool), so one
// producer and one consumer (or many threads) may share a RingBuffer
// safely. Individual calls are atomic; multi-call sequences are not.
// Move-only; never copy a ring.

#include "arctixc/memory/export.hpp"

#include <cstddef>

namespace arctix::Buffer
{

// Opaque internal state. Defined privately in src/memory/.
struct RingBufferImpl;

// Fixed-capacity FIFO byte ring.
class ARCTIXC_MEMORY_API RingBuffer
{
public:
    RingBuffer();
    explicit RingBuffer(std::size_t capacity);
    ~RingBuffer();

    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;

    RingBuffer(RingBuffer&& other) noexcept;
    RingBuffer& operator=(RingBuffer&& other) noexcept;

    // Creates the ring with `capacity` bytes. Capacity must be > 0.
    // Returns true on success, false on invalid args / already created
    // / out of memory.
    bool Create(std::size_t capacity);

    // Destroys the ring and releases its backing memory.
    // Unread bytes are wiped. The ring may be Create()d again afterwards.
    void Destroy();

    // Returns true if Create() succeeded and Destroy() was not called.
    bool IsCreated() const;

    // Appends `size` bytes from `data` to the tail.
    // Fails (returns false, ring unchanged) when the ring is not created,
    // when `size` exceeds FreeSpace(), or on invalid args.
    // `size == 0` succeeds as a no-op (`data` may be nullptr then).
    bool Write(const void* data, std::size_t size);

    // Copies `size` bytes from the head into `out` and consumes them.
    // Fails (returns false, ring unchanged) when the ring is not created,
    // when `size` exceeds Size(), or on invalid args.
    // `size == 0` succeeds as a no-op (`out` may be nullptr then).
    bool Read(void* out, std::size_t size);

    // Copies `size` bytes from the head into `out` without consuming them.
    // Same failure rules as Read(), but the ring is never modified.
    bool Peek(void* out, std::size_t size) const;

    // Drops `size` bytes from the head without copying them out.
    // Fails (returns false, ring unchanged) when `size` exceeds Size().
    // `size == 0` succeeds as a no-op.
    bool Discard(std::size_t size);

    // Drops all unread bytes (Size() becomes 0) but keeps the allocation
    // so the ring can be reused without another heap call. Wipes bytes.
    void Clear();

    // Capacity queries (0 when not created, except Capacity()).
    std::size_t Capacity() const;
    std::size_t Size() const;
    std::size_t FreeSpace() const;
    bool Empty() const;
    bool Full() const;

private:
    RingBufferImpl* impl_;
};

} // namespace arctix::Buffer
