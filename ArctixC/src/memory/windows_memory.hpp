#pragma once

// ArctixC.Memory (private) - OS-level aligned allocation helpers.
// Declaration header. Implementation in windows_memory.cpp.

#include <cstddef>

namespace arctix
{
namespace platform
{

// Allocates `size` bytes aligned to `alignment` (power of two).
// Returns nullptr on failure. The returned pointer must be released
// with AlignedFree(). `outUsableSize` optionally receives the size
// that must be passed to the statistics tracker.
void* AlignedAlloc(std::size_t size, std::size_t alignment, std::size_t* outUsableSize);

// Frees a pointer returned by AlignedAlloc(). No-op on nullptr.
void AlignedFree(void* pointer);

// Returns true if `alignment` is a non-zero power of two.
bool IsValidAlignment(std::size_t alignment);

// Rounds `alignment` up to a valid power-of-two alignment (>= 16).
std::size_t NormalizeAlignment(std::size_t alignment);

} // namespace platform
} // namespace arctix
