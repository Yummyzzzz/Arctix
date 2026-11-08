#pragma once

// ArctixC - small header-only memory utilities.
// Types + inline functions only: no .cpp, no DLL boundary crossed.
// Safe to use from any translation unit that links ArctixC.

#include <cstddef>
#include <cstdint>

namespace arctix::Memory
{

// Overwrites `size` bytes at `data` with zeros through a volatile
// pointer so the compiler cannot optimize the wipe away. No-op on
// NULL data or size == 0. Use for keys, passwords, tokens.
inline void SecureZero(void* data, std::size_t size)
{
    if (data == nullptr || size == 0)
    {
        return;
    }
    volatile std::uint8_t* p = static_cast<volatile std::uint8_t*>(data);
    for (std::size_t i = 0; i < size; ++i)
    {
        p[i] = 0;
    }
}

// True when the two ranges hold identical bytes. NULL == NULL only
// when both sizes are 0; any NULL with a nonzero size returns false.
inline bool MemEqual(const void* a, const void* b, std::size_t size)
{
    if (size == 0)
    {
        return true;
    }
    if (a == nullptr || b == nullptr)
    {
        return false;
    }
    const auto* pa = static_cast<const std::uint8_t*>(a);
    const auto* pb = static_cast<const std::uint8_t*>(b);
    for (std::size_t i = 0; i < size; ++i)
    {
        if (pa[i] != pb[i])
        {
            return false;
        }
    }
    return true;
}

// 64-bit FNV-1a hash of `size` bytes. Returns 0 for NULL/empty input.
// Useful as a fast non-cryptographic checksum for buffer contents.
inline std::uint64_t Fnv1aHash64(const void* data, std::size_t size)
{
    if (data == nullptr || size == 0)
    {
        return 0;
    }
    const auto* p = static_cast<const std::uint8_t*>(data);
    std::uint64_t hash = 14695981039346656037ULL;
    for (std::size_t i = 0; i < size; ++i)
    {
        hash ^= static_cast<std::uint64_t>(p[i]);
        hash *= 1099511628211ULL;
    }
    return hash;
}

} // namespace arctix::Memory
