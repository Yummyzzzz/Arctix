#include "arctixc/memory/ring_buffer.hpp"

#include <mutex>
#include <new>

#include "arctixc/memory/mem_utils.hpp"

// Private implementation-only translation unit: implements
// arctix::Buffer::RingBuffer declared in
// include/arctixc/memory/ring_buffer.hpp. No matching private header.

namespace arctix::Buffer
{

struct RingBufferImpl
{
    std::uint8_t* data = nullptr;
    std::size_t capacity = 0;
    std::size_t head = 0; // read position (offset into data)
    std::size_t tail = 0; // write position (offset into data)
    std::size_t size = 0; // unread bytes
    bool created = false;
    std::mutex mutex{};
};

RingBuffer::RingBuffer()
    : impl_(new (std::nothrow) RingBufferImpl())
{
}

RingBuffer::RingBuffer(std::size_t capacity)
    : impl_(new (std::nothrow) RingBufferImpl())
{
    if (impl_ != nullptr)
    {
        (void)Create(capacity);
    }
}

RingBuffer::~RingBuffer()
{
    if (impl_ != nullptr)
    {
        Destroy();
        delete impl_;
    }
}

RingBuffer::RingBuffer(RingBuffer&& other) noexcept
    : impl_(other.impl_)
{
    other.impl_ = nullptr;
}

RingBuffer& RingBuffer::operator=(RingBuffer&& other) noexcept
{
    if (this != &other)
    {
        if (impl_ != nullptr)
        {
            Destroy();
            delete impl_;
        }
        impl_ = other.impl_;
        other.impl_ = nullptr;
    }
    return *this;
}

bool RingBuffer::Create(std::size_t capacity)
{
    if (impl_ == nullptr)
    {
        return false;
    }
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (impl_->created || capacity == 0)
    {
        return false;
    }
    std::uint8_t* backing = new (std::nothrow) std::uint8_t[capacity];
    if (backing == nullptr)
    {
        return false;
    }
    for (std::size_t i = 0; i < capacity; ++i)
    {
        backing[i] = 0;
    }
    impl_->data = backing;
    impl_->capacity = capacity;
    impl_->head = 0;
    impl_->tail = 0;
    impl_->size = 0;
    impl_->created = true;
    return true;
}

void RingBuffer::Destroy()
{
    if (impl_ == nullptr)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->created)
    {
        return;
    }
    arctix::Memory::SecureZero(impl_->data, impl_->capacity);
    delete[] impl_->data;
    impl_->data = nullptr;
    impl_->capacity = 0;
    impl_->head = 0;
    impl_->tail = 0;
    impl_->size = 0;
    impl_->created = false;
}

bool RingBuffer::IsCreated() const
{
    if (impl_ == nullptr)
    {
        return false;
    }
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->created;
}

bool RingBuffer::Write(const void* data, std::size_t size)
{
    if (impl_ == nullptr)
    {
        return false;
    }
    if (size == 0)
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        return impl_->created;
    }
    if (data == nullptr)
    {
        return false;
    }
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->created || size > impl_->capacity - impl_->size)
    {
        return false;
    }
    const auto* src = static_cast<const std::uint8_t*>(data);
    // First chunk: tail to end of backing, second chunk wraps to start.
    std::size_t first = impl_->capacity - impl_->tail;
    if (first > size)
    {
        first = size;
    }
    for (std::size_t i = 0; i < first; ++i)
    {
        impl_->data[impl_->tail + i] = src[i];
    }
    for (std::size_t i = first; i < size; ++i)
    {
        impl_->data[i - first] = src[i];
    }
    impl_->tail = (impl_->tail + size) % impl_->capacity;
    impl_->size += size;
    return true;
}

bool RingBuffer::Read(void* out, std::size_t size)
{
    if (impl_ == nullptr)
    {
        return false;
    }
    if (size == 0)
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        return impl_->created;
    }
    if (out == nullptr)
    {
        return false;
    }
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->created || size > impl_->size)
    {
        return false;
    }
    auto* dst = static_cast<std::uint8_t*>(out);
    std::size_t first = impl_->capacity - impl_->head;
    if (first > size)
    {
        first = size;
    }
    for (std::size_t i = 0; i < first; ++i)
    {
        dst[i] = impl_->data[impl_->head + i];
    }
    for (std::size_t i = first; i < size; ++i)
    {
        dst[i] = impl_->data[i - first];
    }
    arctix::Memory::SecureZero(impl_->data + impl_->head, first);
    if (size > first)
    {
        arctix::Memory::SecureZero(impl_->data, size - first);
    }
    impl_->head = (impl_->head + size) % impl_->capacity;
    impl_->size -= size;
    return true;
}

bool RingBuffer::Peek(void* out, std::size_t size) const
{
    if (impl_ == nullptr)
    {
        return false;
    }
    if (size == 0)
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        return impl_->created;
    }
    if (out == nullptr)
    {
        return false;
    }
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->created || size > impl_->size)
    {
        return false;
    }
    auto* dst = static_cast<std::uint8_t*>(out);
    std::size_t first = impl_->capacity - impl_->head;
    if (first > size)
    {
        first = size;
    }
    for (std::size_t i = 0; i < first; ++i)
    {
        dst[i] = impl_->data[impl_->head + i];
    }
    for (std::size_t i = first; i < size; ++i)
    {
        dst[i] = impl_->data[i - first];
    }
    return true;
}

bool RingBuffer::Discard(std::size_t size)
{
    if (impl_ == nullptr)
    {
        return false;
    }
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->created || size > impl_->size)
    {
        return false;
    }
    if (size == 0)
    {
        return true;
    }
    std::size_t first = impl_->capacity - impl_->head;
    if (first > size)
    {
        first = size;
    }
    arctix::Memory::SecureZero(impl_->data + impl_->head, first);
    if (size > first)
    {
        arctix::Memory::SecureZero(impl_->data, size - first);
    }
    impl_->head = (impl_->head + size) % impl_->capacity;
    impl_->size -= size;
    return true;
}

void RingBuffer::Clear()
{
    if (impl_ == nullptr)
    {
        return;
    }
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->created)
    {
        return;
    }
    // Wipe only the unread bytes (two segments at most), keep allocation.
    std::size_t remaining = impl_->size;
    std::size_t pos = impl_->head;
    while (remaining > 0)
    {
        std::size_t chunk = impl_->capacity - pos;
        if (chunk > remaining)
        {
            chunk = remaining;
        }
        arctix::Memory::SecureZero(impl_->data + pos, chunk);
        pos = (pos + chunk) % impl_->capacity;
        remaining -= chunk;
    }
    impl_->head = 0;
    impl_->tail = 0;
    impl_->size = 0;
}

std::size_t RingBuffer::Capacity() const
{
    if (impl_ == nullptr)
    {
        return 0;
    }
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->created ? impl_->capacity : 0;
}

std::size_t RingBuffer::Size() const
{
    if (impl_ == nullptr)
    {
        return 0;
    }
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->created ? impl_->size : 0;
}

std::size_t RingBuffer::FreeSpace() const
{
    if (impl_ == nullptr)
    {
        return 0;
    }
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->created ? (impl_->capacity - impl_->size) : 0;
}

bool RingBuffer::Empty() const
{
    return Size() == 0;
}

bool RingBuffer::Full() const
{
    if (impl_ == nullptr)
    {
        return false;
    }
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->created && impl_->size == impl_->capacity;
}

} // namespace arctix::Buffer
