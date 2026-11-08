#pragma once

// ArctixC - OS virtual memory page management.
// Declaration header. Implementation lives in src/memory/virtual_memory.cpp
// (that .cpp includes this header).
//
// These functions talk to the OS pager directly (VirtualAlloc on Windows,
// mmap on POSIX) instead of the C heap. Reserved pages cost address space
// but no RAM until committed; committed pages are zero-filled by the OS.
// Always work in multiples of VirtualPageSize().
//
// Lifecycle: VirtualReserve() -> VirtualCommit() -> use ->
//            VirtualDecommit() (optional, keeps the reservation) ->
//            VirtualRelease() exactly once per reservation.

#include "arctixc/memory/export.hpp"

#include <cstddef>

namespace arctix::Memory
{

// Desired access for VirtualProtectMemory(). Not every combination is
// enforced on every OS; requesting executable pages may fail under
// hardened policies (that is reported as false, never a crash).
enum class MemoryProtection
{
    NoAccess = 0,
    ReadOnly = 1,
    ReadWrite = 2,
    ReadExecute = 3,
    ReadWriteExecute = 4
};

// OS page size in bytes (e.g. 4096). Never returns 0.
ARCTIXC_MEMORY_API std::size_t VirtualPageSize();

// True when `address` is a multiple of VirtualPageSize().
ARCTIXC_MEMORY_API bool IsPageAligned(const void* address);

// Rounds `size` up to a whole number of pages. Returns 0 on size == 0.
ARCTIXC_MEMORY_API std::size_t RoundUpToPageSize(std::size_t size);

// Reserves `size` bytes of address space with no access (no RAM used).
// Returns nullptr on failure (size == 0 or OS refusal).
// Release with VirtualRelease().
ARCTIXC_MEMORY_API void* VirtualReserve(std::size_t size);

// Commits previously reserved pages so they can be read/written.
// The address range must lie inside one VirtualReserve() reservation.
// Newly committed pages read as zeros. Returns false on failure.
ARCTIXC_MEMORY_API bool VirtualCommit(void* address, std::size_t size);

// Returns committed pages to reserved status (contents are lost).
// The reservation stays valid for a later VirtualCommit().
// Returns false on invalid args / OS refusal.
ARCTIXC_MEMORY_API bool VirtualDecommit(void* address, std::size_t size);

// Frees an entire reservation from VirtualReserve(). `address` must be
// the exact pointer returned by VirtualReserve(); `size` must be the
// exact reserved size (required on POSIX, ignored on Windows).
// Returns false on invalid args / OS refusal. Call exactly once.
ARCTIXC_MEMORY_API bool VirtualRelease(void* address, std::size_t size);

// Changes access rights on committed pages. Returns false on invalid
// args / OS refusal (existing protection is left unchanged).
ARCTIXC_MEMORY_API bool VirtualProtectMemory(void* address, std::size_t size, MemoryProtection protection);

} // namespace arctix::Memory
