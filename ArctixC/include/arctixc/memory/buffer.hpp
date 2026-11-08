#pragma once

// ArctixC - owning byte buffer.
// Declaration header. Implementation lives in src/memory/buffer.cpp
// (that .cpp includes this header).
//
// Buffer owns a contiguous, resizable byte array on the heap. It is the
// recommended way to stage data before handing it to the Allocator,
// the MemoryPool, or Virtual* page functions: build bytes here, copy
// them where they need to live. Move-only; never copy a buffer.

#include "arctixc/memory/export.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace arctix::Buffer
{

// Opaque internal state. Defined privately in src/memory/.
struct BufferImpl;

// Resizable owning byte array.
class ARCTIXC_MEMORY_API Buffer
{
public:
    Buffer();
    explicit Buffer(std::size_t size, std::uint8_t fill = 0);
    ~Buffer();

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    // Replaces the contents with `size` bytes copied from `data`.
    // NULL data is valid only when size == 0 (clears the buffer).
    // Returns false on invalid args / out of memory (contents unchanged).
    bool CopyFrom(const void* data, std::size_t size);

    // Appends `size` bytes from `data` to the current contents.
    // Returns false on invalid args / out of memory (contents unchanged).
    bool Append(const void* data, std::size_t size);

    // Resizes to newSize, preserving existing bytes. Grown bytes are
    // filled with `fill`. Returns false on out of memory (unchanged).
    bool Resize(std::size_t newSize, std::uint8_t fill = 0);

    // Ensures room for at least `capacity` bytes without changing Size().
    // Returns false on out of memory (unchanged).
    bool Reserve(std::size_t capacity);

    // Sets every byte to `value`. No-op on an empty buffer.
    void Fill(std::uint8_t value);

    // Drops the contents (Size() becomes 0) but keeps the allocation
    // so the buffer can be reused without another heap call.
    void Clear();

    // Overwrites the contents with zeros through a volatile pointer so
    // the compiler cannot optimize the wipe away, then clears the size.
    // Use before destroying buffers that held keys or passwords.
    void SecureWipe();

    // True when both buffers hold the same size and same bytes.
    bool Equals(const Buffer& other) const;

    // FNV-1a 64-bit hash of the current contents (0 for empty buffer).
    std::uint64_t Checksum() const;

    // Uppercase hex string of the contents, two chars per byte.
    std::string HexEncode() const;

    // Raw access. Valid until the next mutating call. nullptr when empty.
    std::uint8_t* Data();
    const std::uint8_t* Data() const;

    std::size_t Size() const;
    std::size_t Capacity() const;
    bool Empty() const;

private:
    BufferImpl* impl_;
};

} // namespace arctix::Buffer
