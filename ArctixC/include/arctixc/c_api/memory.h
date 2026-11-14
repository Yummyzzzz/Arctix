#pragma once

// ArctixC.Memory - stable C ABI for arctixc_c.dll (C consumers link arctixc_c.lib).
//
// This header is genuinely C-compatible: it only uses C types and can be
// included from a .c file. Function names keep the historical Arctix_*
// prefix so existing C consumers keep working; only the include path
// changed (arctix/c_api.hpp -> arctixc/c_api/memory.h).
//
// Ownership: every Create function must be paired with its Destroy
// function. Passing NULL where a handle is required is always safe
// (allocation functions return NULL, free functions do nothing).

#include <stddef.h>

#include "arctixc/memory/export.hpp"

#ifdef __cplusplus
extern "C"
{
#endif

// Opaque handles. Never dereference; only pass back to Arctix_* functions.
typedef struct ArctixAllocatorHandle ArctixAllocatorHandle;
typedef struct ArctixPoolHandle ArctixPoolHandle;
typedef struct ArctixBufferHandle ArctixBufferHandle;
typedef struct ArctixRingHandle ArctixRingHandle;

// C-compatible configuration (mirrors arctix::Memory::ArctixConfig).
// Use 0/1 for threadSafe and enableStatistics.
typedef struct ArctixConfigC
{
    int threadSafe;
    int enableStatistics;
    size_t defaultAlignment;
} ArctixConfigC;

// C-compatible statistics snapshot (mirrors arctix::Memory::ArctixStats).
typedef struct ArctixStatsC
{
    size_t totalAllocatedBytes;
    size_t totalFreedBytes;
    size_t currentUsageBytes;
    size_t peakUsageBytes;
    unsigned long long allocationCount;
    unsigned long long freeCount;
} ArctixStatsC;

// --- Allocator ---

// Creates and initializes an allocator. config may be NULL (uses defaults).
// Returns NULL on failure. Free with Arctix_DestroyAllocator().
ARCTIXC_MEMORY_API ArctixAllocatorHandle* Arctix_CreateAllocator(const ArctixConfigC* config);

// Destroys an allocator created by Arctix_CreateAllocator(). No-op on NULL.
ARCTIXC_MEMORY_API void Arctix_DestroyAllocator(ArctixAllocatorHandle* handle);

// Allocates size bytes with the given alignment (0 = default).
// Returns NULL on failure.
ARCTIXC_MEMORY_API void* Arctix_Allocate(ArctixAllocatorHandle* handle, size_t size, size_t alignment);

// Frees a pointer from Arctix_Allocate(). No-op on NULL pointer.
ARCTIXC_MEMORY_API void Arctix_Free(ArctixAllocatorHandle* handle, void* pointer);

// Writes a statistics snapshot into outStats. No-op on NULL args.
ARCTIXC_MEMORY_API void Arctix_GetStats(ArctixAllocatorHandle* handle, ArctixStatsC* outStats);

// --- Memory pool ---

// Creates a fixed-size pool. Returns NULL on invalid args / failure.
ARCTIXC_MEMORY_API ArctixPoolHandle* Arctix_PoolCreate(size_t blockSize, size_t blockCount, size_t alignment);

// Destroys a pool created by Arctix_PoolCreate(). No-op on NULL.
ARCTIXC_MEMORY_API void Arctix_PoolDestroy(ArctixPoolHandle* handle);

// Returns one block or NULL when exhausted / on NULL handle.
ARCTIXC_MEMORY_API void* Arctix_PoolAllocate(ArctixPoolHandle* handle);

// Returns a block. No-op on NULL handle or NULL pointer.
ARCTIXC_MEMORY_API void Arctix_PoolFree(ArctixPoolHandle* handle, void* pointer);

// --- Byte buffer ---
//
// Owning resizable byte array. Create with an initial size (0 = empty),
// mutate with Copy/Append/Resize/Fill, inspect with Data/Size/Checksum.
// Free with Arctix_BufferDestroy(). Passing NULL where a handle is
// required is always safe (queries return 0/NULL, mutators do nothing).

// Creates a buffer holding `size` zeroed bytes. Free with Arctix_BufferDestroy.
ARCTIXC_MEMORY_API ArctixBufferHandle* Arctix_BufferCreate(size_t size);

// Destroys a buffer from Arctix_BufferCreate(). Wipes contents. No-op on NULL.
ARCTIXC_MEMORY_API void Arctix_BufferDestroy(ArctixBufferHandle* handle);

// Replaces contents with `size` bytes from `data`. Returns 1 on success.
ARCTIXC_MEMORY_API int Arctix_BufferCopyFrom(ArctixBufferHandle* handle, const void* data, size_t size);

// Appends `size` bytes from `data`. Returns 1 on success.
ARCTIXC_MEMORY_API int Arctix_BufferAppend(ArctixBufferHandle* handle, const void* data, size_t size);

// Resizes, preserving bytes; grown bytes are zeroed. Returns 1 on success.
ARCTIXC_MEMORY_API int Arctix_BufferResize(ArctixBufferHandle* handle, size_t newSize);

// Sets every byte to `value` (0-255). No-op on NULL/empty.
ARCTIXC_MEMORY_API void Arctix_BufferFill(ArctixBufferHandle* handle, unsigned int value);

// Drops contents (size becomes 0), keeps allocation. No-op on NULL.
ARCTIXC_MEMORY_API void Arctix_BufferClear(ArctixBufferHandle* handle);

// Returns 1 when both buffers hold identical bytes, else 0.
ARCTIXC_MEMORY_API int Arctix_BufferEquals(const ArctixBufferHandle* a, const ArctixBufferHandle* b);

// FNV-1a 64-bit hash of contents (0 for NULL/empty). Non-cryptographic:
// never use for password storage, cryptographic authentication, or
// security-sensitive integrity verification.
ARCTIXC_MEMORY_API unsigned long long Arctix_BufferChecksum(const ArctixBufferHandle* handle);

// Raw bytes (valid until next mutating call), NULL when NULL/empty.
ARCTIXC_MEMORY_API unsigned char* Arctix_BufferData(ArctixBufferHandle* handle);

// Current size in bytes (0 for NULL).
ARCTIXC_MEMORY_API size_t Arctix_BufferSize(const ArctixBufferHandle* handle);

// --- Byte ring buffer ---
//
// Fixed-capacity FIFO of bytes. Write() appends at the tail, Read()
// consumes from the head, both with wrap-around. All operations are
// all-or-nothing: Write() fails when `size` exceeds free space,
// Read()/Peek() fail when `size` exceeds used size. No partial I/O,
// no overwrite mode, no blocking. Free with Arctix_RingDestroy().
// Passing NULL where a handle is required is always safe (queries
// return 0, mutators return 0 / do nothing).

// Creates a ring holding `capacity` bytes (> 0). Free with Arctix_RingDestroy.
ARCTIXC_MEMORY_API ArctixRingHandle* Arctix_RingCreate(size_t capacity);

// Destroys a ring from Arctix_RingCreate(). Wipes unread bytes. No-op on NULL.
ARCTIXC_MEMORY_API void Arctix_RingDestroy(ArctixRingHandle* handle);

// Appends `size` bytes from `data`. Returns 1 on success, 0 when full / bad args.
ARCTIXC_MEMORY_API int Arctix_RingWrite(ArctixRingHandle* handle, const void* data, size_t size);

// Copies `size` bytes out and consumes them. Returns 1 on success.
ARCTIXC_MEMORY_API int Arctix_RingRead(ArctixRingHandle* handle, void* out, size_t size);

// Copies `size` bytes out without consuming them. Returns 1 on success.
ARCTIXC_MEMORY_API int Arctix_RingPeek(const ArctixRingHandle* handle, void* out, size_t size);

// Drops `size` bytes without copying them out. Returns 1 on success.
ARCTIXC_MEMORY_API int Arctix_RingDiscard(ArctixRingHandle* handle, size_t size);

// Drops all unread bytes (size becomes 0), keeps allocation. No-op on NULL.
ARCTIXC_MEMORY_API void Arctix_RingClear(ArctixRingHandle* handle);

// Unread bytes (0 for NULL).
ARCTIXC_MEMORY_API size_t Arctix_RingSize(const ArctixRingHandle* handle);

// Total capacity in bytes (0 for NULL).
ARCTIXC_MEMORY_API size_t Arctix_RingCapacity(const ArctixRingHandle* handle);

// Free space in bytes (0 for NULL).
ARCTIXC_MEMORY_API size_t Arctix_RingFreeSpace(const ArctixRingHandle* handle);

// Returns 1 when empty (or NULL), else 0.
ARCTIXC_MEMORY_API int Arctix_RingIsEmpty(const ArctixRingHandle* handle);

// Returns 1 when full, else 0 (NULL counts as not full).
ARCTIXC_MEMORY_API int Arctix_RingIsFull(const ArctixRingHandle* handle);

// --- Virtual memory ---
//
// OS page functions. Work in whole pages; pair every VirtualReserve with
// exactly one VirtualRelease. Committed pages read as zeros.
// Protection constants for ArctixC_VirtualProtect:
//   0 = no access, 1 = read-only, 2 = read-write, 3 = read-execute,
//   4 = read-write-execute.

// OS page size in bytes. Never returns 0.
ARCTIXC_MEMORY_API size_t ArctixC_VirtualPageSize(void);

// Reserves `size` bytes of address space. Returns NULL on failure.
ARCTIXC_MEMORY_API void* ArctixC_VirtualReserve(size_t size);

// Commits reserved pages. Returns 1 on success, 0 on failure.
ARCTIXC_MEMORY_API int ArctixC_VirtualCommit(void* address, size_t size);

// Returns committed pages to reserved status. Returns 1 on success.
ARCTIXC_MEMORY_API int ArctixC_VirtualDecommit(void* address, size_t size);

// Frees a whole reservation (`address` from Reserve, exact `size`).
// Returns 1 on success. Call exactly once per reservation.
ARCTIXC_MEMORY_API int ArctixC_VirtualRelease(void* address, size_t size);

// Changes access rights on committed pages. Returns 1 on success.
ARCTIXC_MEMORY_API int ArctixC_VirtualProtect(void* address, size_t size, int protection);

// --- Version ---
// Returns e.g. "0.2.0". Never returns NULL.
ARCTIXC_MEMORY_API const char* Arctix_GetVersionString(void);

#ifdef __cplusplus
} // extern "C"
#endif
