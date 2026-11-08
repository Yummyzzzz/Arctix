#include "memory/windows_memory.hpp"

#include <cstdlib>

namespace arctixs::Memory
{
namespace platform
{

bool IsValidAlignment(std::size_t alignment)
{
    return alignment != 0 && (alignment & (alignment - 1)) == 0;
}

std::size_t NormalizeAlignment(std::size_t alignment)
{
    if (!IsValidAlignment(alignment) || alignment < 16)
    {
        return 16;
    }
    return alignment;
}

void* AlignedAlloc(std::size_t size, std::size_t alignment, std::size_t* outUsableSize)
{
    if (size == 0)
    {
        return nullptr;
    }

    const std::size_t fixedAlignment = NormalizeAlignment(alignment);

    // _aligned_malloc is available on MSVC without extra dependencies
    // and guarantees the requested alignment for the DLL build.
    void* pointer = _aligned_malloc(size, fixedAlignment);
    if (pointer != nullptr && outUsableSize != nullptr)
    {
        *outUsableSize = size;
    }
    return pointer;
}

void AlignedFree(void* pointer)
{
    if (pointer == nullptr)
    {
        return;
    }
    _aligned_free(pointer);
}

} // namespace platform
} // namespace arctixs::Memory
