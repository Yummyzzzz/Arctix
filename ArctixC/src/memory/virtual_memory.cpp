#include "arctixc/memory/virtual_memory.hpp"

#include <cstdint>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <unistd.h>
#endif

// Private implementation-only translation unit: implements the OS virtual
// memory functions declared in include/arctixc/memory/virtual_memory.hpp.
// No matching private header.

namespace arctix
{

std::size_t VirtualPageSize()
{
#ifdef _WIN32
    SYSTEM_INFO info{};
    GetSystemInfo(&info);
    return info.dwPageSize != 0 ? static_cast<std::size_t>(info.dwPageSize) : 4096;
#else
    const long pages = sysconf(_SC_PAGESIZE);
    return pages > 0 ? static_cast<std::size_t>(pages) : 4096;
#endif
}

bool IsPageAligned(const void* address)
{
    if (address == nullptr)
    {
        return false;
    }
    const auto raw = reinterpret_cast<std::uintptr_t>(address);
    return (raw % VirtualPageSize()) == 0;
}

std::size_t RoundUpToPageSize(std::size_t size)
{
    if (size == 0)
    {
        return 0;
    }
    const std::size_t page = VirtualPageSize();
    return ((size + page - 1) / page) * page;
}

void* VirtualReserve(std::size_t size)
{
    if (size == 0)
    {
        return nullptr;
    }
#ifdef _WIN32
    return VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_NOACCESS);
#else
    void* p = mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    return (p == MAP_FAILED) ? nullptr : p;
#endif
}

bool VirtualCommit(void* address, std::size_t size)
{
    if (address == nullptr || size == 0)
    {
        return false;
    }
#ifdef _WIN32
    return VirtualAlloc(address, size, MEM_COMMIT, PAGE_READWRITE) != nullptr;
#else
    return mprotect(address, size, PROT_READ | PROT_WRITE) == 0;
#endif
}

bool VirtualDecommit(void* address, std::size_t size)
{
    if (address == nullptr || size == 0)
    {
        return false;
    }
#ifdef _WIN32
    return VirtualFree(address, size, MEM_DECOMMIT) != FALSE;
#else
    // Discard contents but keep the mapping reserved.
    void* p = mmap(address, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    return (p != MAP_FAILED);
#endif
}

bool VirtualRelease(void* address, std::size_t size)
{
    if (address == nullptr || size == 0)
    {
        return false;
    }
#ifdef _WIN32
    (void)size; // MEM_RELEASE always frees the whole reservation.
    return VirtualFree(address, 0, MEM_RELEASE) != FALSE;
#else
    return munmap(address, size) == 0;
#endif
}

bool VirtualProtectMemory(void* address, std::size_t size, MemoryProtection protection)
{
    if (address == nullptr || size == 0)
    {
        return false;
    }
#ifdef _WIN32
    DWORD winProtect = PAGE_NOACCESS;
    switch (protection)
    {
    case MemoryProtection::NoAccess:         winProtect = PAGE_NOACCESS;          break;
    case MemoryProtection::ReadOnly:         winProtect = PAGE_READONLY;          break;
    case MemoryProtection::ReadWrite:        winProtect = PAGE_READWRITE;         break;
    case MemoryProtection::ReadExecute:      winProtect = PAGE_EXECUTE_READ;      break;
    case MemoryProtection::ReadWriteExecute: winProtect = PAGE_EXECUTE_READWRITE; break;
    default: return false;
    }
    DWORD oldProtect = 0;
    return VirtualProtect(address, size, winProtect, &oldProtect) != FALSE;
#else
    int prot = PROT_NONE;
    switch (protection)
    {
    case MemoryProtection::NoAccess:         prot = PROT_NONE;                    break;
    case MemoryProtection::ReadOnly:         prot = PROT_READ;                    break;
    case MemoryProtection::ReadWrite:        prot = PROT_READ | PROT_WRITE;       break;
    case MemoryProtection::ReadExecute:      prot = PROT_READ | PROT_EXEC;        break;
    case MemoryProtection::ReadWriteExecute: prot = PROT_READ | PROT_WRITE | PROT_EXEC; break;
    default: return false;
    }
    return mprotect(address, size, prot) == 0;
#endif
}

} // namespace arctix
