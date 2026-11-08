# Arctix - one DLL for memory, buffers, virtual memory, and threads

Arctix is a C/C++ systems toolkit. Its core library, **ArctixC**, ships as a single
standalone binary - `arctixc.dll` plus `arctixc.lib` and headers. No split modules,
no dependency chain, no installer required. Link one import library, ship one DLL,
and you get an aligned allocator with statistics, O(1) memory pools, owning byte
buffers, direct OS virtual-memory control, and threads with a work-stealing-free
FIFO thread pool.

If you have ever vendored three tiny libraries to cover "allocate aligned memory,
hold some bytes, and run work on threads", ArctixC replaces all three with one
tested binary that has exactly one job: stay out of your way. It is MIT licensed,
so you can use it in open source and closed commercial projects alike.

## What you get

- **Single standalone DLL.** Everything lives in `arctixc.dll` (version 0.1.1).
  The build fails if a split module DLL is ever produced again.
- **Modern namespaced C++ API.** Every call reads `arctixs::<category>::<func>`,
  for example `arctixs::Memory::Allocator::Allocate` or
  `arctixs::Thread::ParallelFor`.
- **Stable C ABI.** The same features are reachable from pure C through
  `arctixc/c_api/memory.h` and `arctixc/c_api/thread.h`, with NULL-safe handles.
- **Embedded version info.** The DLL carries ProductName, ProductVersion,
  CompanyName, and copyright in its VERSIONINFO resource (check Properties -
  Details). The version is defined once in CMake and flows into the headers
  and the DLL automatically.
- **Documented to the last function.** `ArctixC/DOC.html` is a single offline
  file that documents all 121 functions with signature, arguments, return value,
  example call, and explanation, plus a searchable index.

## API map

| Area | C++ spelling | Highlights |
|------|--------------|------------|
| Foundation | `arctixs::Core::` | Version queries, `Status` codes shared by every subsystem |
| Allocator | `arctixs::Memory::Allocator` | Aligned allocation, statistics, resettable counters |
| Memory pool | `arctixs::Memory::MemoryPool` | Fixed-size O(1) blocks for hot objects |
| Buffer | `arctixs::Buffer::Buffer` | Owning resizable byte array: CopyFrom, Append, Resize, Checksum, HexEncode |
| Memory utils | `arctixs::Memory::` | `SecureZero`, `MemEqual`, `Fnv1aHash64` (header-only) |
| Virtual memory | `arctixs::Memory::` | Reserve, Commit, Decommit, Release, ProtectMemory, page helpers |
| Thread | `arctixs::Thread::Thread` | Move-only thread with caught worker exceptions |
| Thread pool | `arctixs::Thread::ThreadPool` | FIFO pool, WaitIdle, per-task exception counts, `ParallelFor` |
| C ABI | `Arctix_*` / `ArctixC_*` | Same features for C consumers |

## Quick start - C++

```cpp
#include <arctixc/memory/allocator.hpp>
#include <arctixc/memory/buffer.hpp>
#include <arctixc/thread/thread_pool.hpp>

// Aligned allocation with statistics.
arctixs::Memory::Allocator alloc;
alloc.Initialize(arctixs::Memory::DefaultConfig());
void* p = alloc.Allocate(256, 16);
// ... use p ...
alloc.Deallocate(p);
alloc.Shutdown();

// An owning byte buffer.
arctixs::Buffer::Buffer buf(1024);
buf.CopyFrom(packet, packetSize);
uint64_t fingerprint = buf.Checksum();

// Run work on a pool.
arctixs::Thread::ThreadPool pool(4);
pool.Submit(Work, &job);
pool.WaitIdle();
pool.Shutdown();
```

## Quick start - pure C

```c
#include <arctixc/c_api/memory.h>

ArctixAllocatorHandle* a = Arctix_CreateAllocator(0);  /* NULL = defaults */
void* p = Arctix_Allocate(a, 256, 16);
Arctix_Free(a, p);
Arctix_DestroyAllocator(a);
```

## Build from source

Requirements: Windows with MSVC, CMake 3.20 or newer. (The virtual-memory layer
also has a POSIX backend, but the tested target is Windows + MSVC + Ninja,
which ships with Visual Studio.)

From the repo root:

```powershell
powershell -ExecutionPolicy Bypass -File ArctixC/build.ps1
```

This configures CMake, builds Release, and runs the test suite. Outputs land in
`ArctixC/build/`: `arctixc.dll`, `arctixc.lib`, the test binaries, and four
runnable examples. Pass `-NoTests` to skip the test run.

## Tests

Four suites run under CTest: `MemoryTests`, `ThreadTests`, `ExtendedTests`
(buffer, virtual memory, pool, parallel loops), and `CApiCheckBuild`, which
proves both C ABI headers compile from real C and not just C++. The
`ArctixC/examples/` folder holds one runnable example per area.

## Project layout

```text
Arctix/
  ArctixC/                 The SDK - the part that is done and tested
    include/arctixc/       Public headers (Core, Memory, Buffer, Thread, C ABI)
    src/                   Implementation (never include directly)
    tests/                 The four CTest suites
    examples/              Four runnable examples
    resources/             Windows VERSIONINFO template for arctixc.dll
    cmake/                 CMake package config template (find_package support)
    DOC.html               The whole manual: 121 functions, one offline file
    Docs.md                Short pointer to DOC.html
    build.ps1              Configure + build + test in one step
  ArctixI/                 Planned: versioned installer for SDK binaries
  ArctixCLI/               Planned: command-line front-end and benchmarks
  ArctixS/                 Planned: graphical workspace and stats viewer
```

## Documentation

Open `ArctixC/DOC.html` in any browser. No server, no internet, no build step.
The Dashboard explains the setup; the Functions tab lists every API entry with
its signature, argument types, return type, an example call, and an explanation.

## Versioning

The version lives in exactly one place: `project(ArctixC VERSION ...)` in
`ArctixC/CMakeLists.txt`. The `ARCTIXC_VERSION_*` macros and the DLL
ProductVersion are generated from it at configure time, so a version bump is a
one-line change that can never drift.

## Status

ArctixC 0.1.1 is built, tested (4/4 suites green), and documented. ArctixI,
ArctixCLI, and ArctixS are planned companions with placeholder docs - each will
consume ArctixC as a library and implement no systems functionality itself.

## Credits

Arctix is owned and developed by **Yummyzzzz** - https://github.com/Yummyzzzz

## License

MIT License - see [LICENSE](LICENSE). Copyright (c) 2026 Yummyzzzz.
