#include "arctixc/c_api/memory.h"

#include <new>

#include "arctixc/core/version.hpp"
#include "arctixc/memory/allocator.hpp"
#include "arctixc/memory/buffer.hpp"
#include "arctixc/memory/memory_pool.hpp"
#include "arctixc/memory/virtual_memory.hpp"

// Private implementation-only translation unit: implements the stable C ABI
// declared in include/arctixc/c_api/memory.h. No matching private header.

namespace
{

arctix::ArctixConfig ToCppConfig(const ArctixConfigC* config)
{
    arctix::ArctixConfig out{};
    if (config == nullptr)
    {
        return out;
    }
    out.threadSafe = (config->threadSafe != 0);
    out.enableStatistics = (config->enableStatistics != 0);
    out.defaultAlignment = (config->defaultAlignment == 0) ? 16 : config->defaultAlignment;
    return out;
}

void ToCStats(const arctix::ArctixStats& in, ArctixStatsC* out)
{
    if (out == nullptr)
    {
        return;
    }
    out->totalAllocatedBytes = in.totalAllocatedBytes;
    out->totalFreedBytes = in.totalFreedBytes;
    out->currentUsageBytes = in.currentUsageBytes;
    out->peakUsageBytes = in.peakUsageBytes;
    out->allocationCount = in.allocationCount;
    out->freeCount = in.freeCount;
}

} // namespace

struct ArctixAllocatorHandle
{
    arctix::Allocator allocator;
};

struct ArctixPoolHandle
{
    arctix::MemoryPool pool;
};

struct ArctixBufferHandle
{
    arctix::Buffer buffer;
};

extern "C"
{

ARCTIXC_MEMORY_API ArctixAllocatorHandle* Arctix_CreateAllocator(const ArctixConfigC* config)
{
    ArctixAllocatorHandle* handle = new (std::nothrow) ArctixAllocatorHandle();
    if (handle == nullptr)
    {
        return nullptr;
    }
    if (!handle->allocator.Initialize(ToCppConfig(config)))
    {
        delete handle;
        return nullptr;
    }
    return handle;
}

ARCTIXC_MEMORY_API void Arctix_DestroyAllocator(ArctixAllocatorHandle* handle)
{
    if (handle == nullptr)
    {
        return;
    }
    handle->allocator.Shutdown();
    delete handle;
}

ARCTIXC_MEMORY_API void* Arctix_Allocate(ArctixAllocatorHandle* handle, size_t size, size_t alignment)
{
    if (handle == nullptr)
    {
        return nullptr;
    }
    return handle->allocator.Allocate(size, alignment);
}

ARCTIXC_MEMORY_API void Arctix_Free(ArctixAllocatorHandle* handle, void* pointer)
{
    if (handle == nullptr || pointer == nullptr)
    {
        return;
    }
    handle->allocator.Deallocate(pointer);
}

ARCTIXC_MEMORY_API void Arctix_GetStats(ArctixAllocatorHandle* handle, ArctixStatsC* outStats)
{
    if (handle == nullptr || outStats == nullptr)
    {
        return;
    }
    ToCStats(handle->allocator.GetStatistics(), outStats);
}

ARCTIXC_MEMORY_API ArctixPoolHandle* Arctix_PoolCreate(size_t blockSize, size_t blockCount, size_t alignment)
{
    ArctixPoolHandle* handle = new (std::nothrow) ArctixPoolHandle();
    if (handle == nullptr)
    {
        return nullptr;
    }
    if (!handle->pool.Create(blockSize, blockCount, alignment))
    {
        delete handle;
        return nullptr;
    }
    return handle;
}

ARCTIXC_MEMORY_API void Arctix_PoolDestroy(ArctixPoolHandle* handle)
{
    if (handle == nullptr)
    {
        return;
    }
    handle->pool.Destroy();
    delete handle;
}

ARCTIXC_MEMORY_API void* Arctix_PoolAllocate(ArctixPoolHandle* handle)
{
    if (handle == nullptr)
    {
        return nullptr;
    }
    return handle->pool.Allocate();
}

ARCTIXC_MEMORY_API void Arctix_PoolFree(ArctixPoolHandle* handle, void* pointer)
{
    if (handle == nullptr || pointer == nullptr)
    {
        return;
    }
    handle->pool.Free(pointer);
}

ARCTIXC_MEMORY_API ArctixBufferHandle* Arctix_BufferCreate(size_t size)
{
    ArctixBufferHandle* handle = new (std::nothrow) ArctixBufferHandle();
    if (handle == nullptr)
    {
        return nullptr;
    }
    if (!handle->buffer.Resize(size, 0))
    {
        delete handle;
        return nullptr;
    }
    return handle;
}

ARCTIXC_MEMORY_API void Arctix_BufferDestroy(ArctixBufferHandle* handle)
{
    delete handle;
}

ARCTIXC_MEMORY_API int Arctix_BufferCopyFrom(ArctixBufferHandle* handle, const void* data, size_t size)
{
    if (handle == nullptr)
    {
        return 0;
    }
    return handle->buffer.CopyFrom(data, size) ? 1 : 0;
}

ARCTIXC_MEMORY_API int Arctix_BufferAppend(ArctixBufferHandle* handle, const void* data, size_t size)
{
    if (handle == nullptr)
    {
        return 0;
    }
    return handle->buffer.Append(data, size) ? 1 : 0;
}

ARCTIXC_MEMORY_API int Arctix_BufferResize(ArctixBufferHandle* handle, size_t newSize)
{
    if (handle == nullptr)
    {
        return 0;
    }
    return handle->buffer.Resize(newSize, 0) ? 1 : 0;
}

ARCTIXC_MEMORY_API void Arctix_BufferFill(ArctixBufferHandle* handle, unsigned int value)
{
    if (handle == nullptr)
    {
        return;
    }
    handle->buffer.Fill(static_cast<std::uint8_t>(value & 0xFF));
}

ARCTIXC_MEMORY_API void Arctix_BufferClear(ArctixBufferHandle* handle)
{
    if (handle == nullptr)
    {
        return;
    }
    handle->buffer.Clear();
}

ARCTIXC_MEMORY_API int Arctix_BufferEquals(const ArctixBufferHandle* a, const ArctixBufferHandle* b)
{
    if (a == nullptr || b == nullptr)
    {
        return 0;
    }
    return a->buffer.Equals(b->buffer) ? 1 : 0;
}

ARCTIXC_MEMORY_API unsigned long long Arctix_BufferChecksum(const ArctixBufferHandle* handle)
{
    if (handle == nullptr)
    {
        return 0;
    }
    return handle->buffer.Checksum();
}

ARCTIXC_MEMORY_API unsigned char* Arctix_BufferData(ArctixBufferHandle* handle)
{
    if (handle == nullptr)
    {
        return nullptr;
    }
    return handle->buffer.Data();
}

ARCTIXC_MEMORY_API size_t Arctix_BufferSize(const ArctixBufferHandle* handle)
{
    if (handle == nullptr)
    {
        return 0;
    }
    return handle->buffer.Size();
}

ARCTIXC_MEMORY_API size_t ArctixC_VirtualPageSize(void)
{
    return arctix::VirtualPageSize();
}

ARCTIXC_MEMORY_API void* ArctixC_VirtualReserve(size_t size)
{
    return arctix::VirtualReserve(size);
}

ARCTIXC_MEMORY_API int ArctixC_VirtualCommit(void* address, size_t size)
{
    return arctix::VirtualCommit(address, size) ? 1 : 0;
}

ARCTIXC_MEMORY_API int ArctixC_VirtualDecommit(void* address, size_t size)
{
    return arctix::VirtualDecommit(address, size) ? 1 : 0;
}

ARCTIXC_MEMORY_API int ArctixC_VirtualRelease(void* address, size_t size)
{
    return arctix::VirtualRelease(address, size) ? 1 : 0;
}

ARCTIXC_MEMORY_API int ArctixC_VirtualProtect(void* address, size_t size, int protection)
{
    if (protection < 0 || protection > 4)
    {
        return 0;
    }
    return arctix::VirtualProtectMemory(address, size, static_cast<arctix::MemoryProtection>(protection)) ? 1 : 0;
}

ARCTIXC_MEMORY_API const char* Arctix_GetVersionString(void)
{
    return ARCTIXC_VERSION_STRING;
}

} // extern "C"
