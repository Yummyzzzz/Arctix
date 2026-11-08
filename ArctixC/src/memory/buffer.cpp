#include "arctixc/memory/buffer.hpp"

#include <new>

#include "arctixc/memory/mem_utils.hpp"

// Private implementation-only translation unit: implements arctixs::Buffer::Buffer
// declared in include/arctixc/memory/buffer.hpp. No matching private header.

namespace arctixs::Buffer
{

struct BufferImpl
{
    std::uint8_t* data = nullptr;
    std::size_t size = 0;
    std::size_t capacity = 0;
};

static bool GrowTo(BufferImpl* impl, std::size_t newCapacity)
{
    std::uint8_t* fresh = new (std::nothrow) std::uint8_t[newCapacity];
    if (fresh == nullptr)
    {
        return false;
    }
    for (std::size_t i = 0; i < impl->size; ++i)
    {
        fresh[i] = impl->data[i];
    }
    delete[] impl->data;
    impl->data = fresh;
    impl->capacity = newCapacity;
    return true;
}

Buffer::Buffer()
    : impl_(new (std::nothrow) BufferImpl())
{
}

Buffer::Buffer(std::size_t size, std::uint8_t fill)
    : impl_(new (std::nothrow) BufferImpl())
{
    if (impl_ != nullptr)
    {
        (void)Resize(size, fill);
    }
}

Buffer::~Buffer()
{
    if (impl_ != nullptr)
    {
        arctixs::Memory::SecureZero(impl_->data, impl_->size);
        delete[] impl_->data;
        delete impl_;
    }
}

Buffer::Buffer(Buffer&& other) noexcept
    : impl_(other.impl_)
{
    other.impl_ = nullptr;
}

Buffer& Buffer::operator=(Buffer&& other) noexcept
{
    if (this != &other)
    {
        arctixs::Memory::SecureZero(impl_ != nullptr ? impl_->data : nullptr, impl_ != nullptr ? impl_->size : 0);
        delete[] impl_->data;
        delete impl_;
        impl_ = other.impl_;
        other.impl_ = nullptr;
    }
    return *this;
}

bool Buffer::CopyFrom(const void* data, std::size_t size)
{
    if (impl_ == nullptr)
    {
        return false;
    }
    if (size == 0)
    {
        arctixs::Memory::SecureZero(impl_->data, impl_->size);
        impl_->size = 0;
        return true;
    }
    if (data == nullptr)
    {
        return false;
    }
    if (size > impl_->capacity && !GrowTo(impl_, size))
    {
        return false;
    }
    const auto* src = static_cast<const std::uint8_t*>(data);
    for (std::size_t i = 0; i < size; ++i)
    {
        impl_->data[i] = src[i];
    }
    if (impl_->size > size)
    {
        arctixs::Memory::SecureZero(impl_->data + size, impl_->size - size);
    }
    impl_->size = size;
    return true;
}

bool Buffer::Append(const void* data, std::size_t size)
{
    if (impl_ == nullptr)
    {
        return false;
    }
    if (size == 0)
    {
        return true;
    }
    if (data == nullptr)
    {
        return false;
    }
    const std::size_t needed = impl_->size + size;
    if (needed < impl_->size)
    {
        return false; // overflow
    }
    if (needed > impl_->capacity)
    {
        std::size_t grown = impl_->capacity == 0 ? 64 : impl_->capacity;
        while (grown < needed)
        {
            grown *= 2;
        }
        if (!GrowTo(impl_, grown))
        {
            return false;
        }
    }
    const auto* src = static_cast<const std::uint8_t*>(data);
    for (std::size_t i = 0; i < size; ++i)
    {
        impl_->data[impl_->size + i] = src[i];
    }
    impl_->size = needed;
    return true;
}

bool Buffer::Resize(std::size_t newSize, std::uint8_t fill)
{
    if (impl_ == nullptr)
    {
        return false;
    }
    if (newSize > impl_->capacity && !GrowTo(impl_, newSize))
    {
        return false;
    }
    if (newSize > impl_->size)
    {
        for (std::size_t i = impl_->size; i < newSize; ++i)
        {
            impl_->data[i] = fill;
        }
    }
    else if (newSize < impl_->size)
    {
        arctixs::Memory::SecureZero(impl_->data + newSize, impl_->size - newSize);
    }
    impl_->size = newSize;
    return true;
}

bool Buffer::Reserve(std::size_t capacity)
{
    if (impl_ == nullptr)
    {
        return false;
    }
    if (capacity <= impl_->capacity)
    {
        return true;
    }
    return GrowTo(impl_, capacity);
}

void Buffer::Fill(std::uint8_t value)
{
    if (impl_ == nullptr)
    {
        return;
    }
    for (std::size_t i = 0; i < impl_->size; ++i)
    {
        impl_->data[i] = value;
    }
}

void Buffer::Clear()
{
    if (impl_ == nullptr)
    {
        return;
    }
    arctixs::Memory::SecureZero(impl_->data, impl_->size);
    impl_->size = 0;
}

void Buffer::SecureWipe()
{
    Clear();
}

bool Buffer::Equals(const Buffer& other) const
{
    if (impl_ == nullptr || other.impl_ == nullptr)
    {
        return impl_ == other.impl_;
    }
    if (impl_->size != other.impl_->size)
    {
        return false;
    }
    return arctixs::Memory::MemEqual(impl_->data, other.impl_->data, impl_->size);
}

std::uint64_t Buffer::Checksum() const
{
    if (impl_ == nullptr)
    {
        return 0;
    }
    return arctixs::Memory::Fnv1aHash64(impl_->data, impl_->size);
}

std::string Buffer::HexEncode() const
{
    static const char kHex[] = "0123456789ABCDEF";
    std::string out;
    if (impl_ == nullptr || impl_->size == 0)
    {
        return out;
    }
    out.resize(impl_->size * 2);
    for (std::size_t i = 0; i < impl_->size; ++i)
    {
        out[i * 2] = kHex[(impl_->data[i] >> 4) & 0xF];
        out[i * 2 + 1] = kHex[impl_->data[i] & 0xF];
    }
    return out;
}

std::uint8_t* Buffer::Data()
{
    return impl_ != nullptr ? impl_->data : nullptr;
}

const std::uint8_t* Buffer::Data() const
{
    return impl_ != nullptr ? impl_->data : nullptr;
}

std::size_t Buffer::Size() const
{
    return impl_ != nullptr ? impl_->size : 0;
}

std::size_t Buffer::Capacity() const
{
    return impl_ != nullptr ? impl_->capacity : 0;
}

bool Buffer::Empty() const
{
    return Size() == 0;
}

} // namespace arctixs::Buffer
