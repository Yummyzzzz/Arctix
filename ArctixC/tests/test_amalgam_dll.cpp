// ArctixC - amalgamated-header + DLL proof test.
//
// Includes ONLY the single universal header (arctixc_all.hpp, no
// include/ tree) and links the C++ DLL import lib (no ARCTIXC_STATIC,
// so the API macros are dllimport). Proves the one-header setup works
// identically for DLL consumers and static consumers (StaticSmoke
// covers the static side, Arctix_bench covers it out-of-tree).
#include "arctixc_all.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>

int main() {
    // Version through the amalgamated header.
    const char* v = arctix::Core::GetVersionString();
    assert(v && std::strlen(v) > 0);

    // C++ allocator.
    arctix::Memory::Allocator alloc;
    assert(alloc.Initialize(arctix::Memory::DefaultConfig()));
    void* p = alloc.Allocate(128, 16);
    assert(p);
    alloc.Deallocate(p);

    // C++ pool.
    arctix::Memory::MemoryPool pool;
    assert(pool.Create(64, 8));
    void* q = pool.Allocate();
    assert(q);
    pool.Free(q);
    pool.Destroy();

    // C++ buffer.
    arctix::Buffer::Buffer buf(32, 0xAB);
    assert(buf.Size() == 32);

    // C ABI through the same header.
    ArctixAllocatorHandle* h = Arctix_CreateAllocator(nullptr);
    assert(h);
    void* c = Arctix_Allocate(h, 64, 16);
    assert(c);
    Arctix_Free(h, c);
    Arctix_DestroyAllocator(h);

    alloc.Shutdown();
    std::puts("DllAmalgam OK");
    return 0;
}
