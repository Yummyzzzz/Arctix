# Arctix - Two DLLs for Memory, Buffers, Virtual Memory, and Threads

[![ArctixC version](https://img.shields.io/badge/ArctixC-0.2.0-blue)](ArctixC/CMakeLists.txt) [![License](https://img.shields.io/badge/license-Arctix%20License%201.0-green)](LICENSE.md) [![Build status](https://github.com/Yummyzzzz/Arctix/actions/workflows/build.yml/badge.svg)](https://github.com/Yummyzzzz/Arctix/actions/workflows/build.yml) [![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20MSVC-lightgrey)](#cross-platform-goals) [![C++](https://img.shields.io/badge/C%2B%2B-17%20%7C%2020%20%7C%2023-blue)](#compiling-arctixc-from-source) [![Docs](https://img.shields.io/badge/docs-offline%20manual-informational)](ArctixC/Documentation.html)

**Arctix** is a C/C++ systems toolkit designed to bring several essential low-level utilities together in one library.

Its core library, **ArctixC**, ships as two standalone DLLs built from the same sources into the same folder: `arctixc.dll` for C++ and `arctixc_c.dll` for C, each with its import library (`arctixc.lib` / `arctixc_c.lib`) plus the public headers. Same namespace (`arctix::`), same headers, same behavior - only the binary you link differs. Link one import library, ship its DLL, and gain access to aligned memory allocation with statistics, O(1) memory pools, owning byte buffers, direct operating-system virtual memory control, and multithreading utilities - including a FIFO thread pool without work stealing.

If you have ever needed multiple small libraries just to allocate aligned memory, manage byte data, and run tasks on threads, ArctixC brings those capabilities together in one tested library.

Arctix is released under the **Arctix License**, allowing use in both open-source and closed-source commercial projects. 
By downloading or using Arctix, you automatically accept the **Arctix License**.

**Official WSD Website:** https://whitepoint.pages.dev/ - This is the valid and real website by Whitepoint Software Development (WSD) for Arctix and other WSD projects.

## Why Arctix exists

Every native project eventually re-solves the same five problems: aligned allocation, fast reuse of fixed-size blocks, safe byte buffers, direct virtual-memory control, and a sane way to run work on threads. The usual answer is a handful of micro-libraries - five licenses, five build systems, five APIs that never quite fit together.

ArctixC is the opposite bet: one small, dependency-free toolkit that covers exactly those five problems with one consistent API, one license, and one `sdk/` folder to drop into your project. Nothing more - it is not a framework, not a game engine, not a container library. It does the low-level plumbing so your project does not collect half-maintained utilities.

## Who it is for

- **C++ developers** writing games, engines, emulators, tools, servers, or any native code that allocates a lot and threads a lot.
- **Pure-C developers** who need the same functionality through a stable ABI (`sdk/c/arctixc_c.h` compiles as real C, verified with `cl /TC`).
- **Developers who want observability for free:** the allocator tracks current bytes, peak bytes, and allocation counts out of the box, and the pool reports pending, running, and failed tasks.
- **Indie and commercial teams** that want one permissive license (Arctix License, open- and closed-source friendly) instead of auditing five.

If you only need one `std::vector` and one `std::thread`, ArctixC is overkill. If you are writing your third custom pool allocator this year, it is for you.

## Problems it solves

1. **Dependency sprawl.** One toolkit replaces the typical pile of single-purpose alloc/buffer/thread helpers - zero third-party dependencies, links only against Windows system libraries.
2. **Silent allocators.** `malloc` and `new` tell you nothing. `Allocator::GetStatistics()` tells you live bytes, peak bytes, and counts, and `ResetStatistics()` starts a clean measurement window.
3. **Thread lifecycle boilerplate.** Spawning and joining threads per task wastes time and litters code. `ThreadPool` keeps N workers alive on a single FIFO queue; `ParallelFor` splits a range across workers with one call.
4. **Lost worker exceptions.** An exception escaping a raw thread terminates the process. `Thread` and `ThreadPool` catch worker exceptions at the boundary and record them (`HasThrown()` / `FailedTasks()`), so failures become data instead of crashes.
5. **Virtual-memory `#ifdef` mazes.** `VirtualReserve` / `VirtualCommit` / `VirtualDecommit` / `VirtualRelease` / `VirtualProtectMemory` expose one API with Windows and POSIX backends in-tree - no platform branches in your code.
6. **Packaging confusion.** Static lib, import lib, C ABI, single headers: the `sdk/` folder lays out every flavor after each build, with a documented "which files ship with my .exe" table below.

## What You Get

- **Two standalone DLLs, one for each language - plus a static lib.** `arctixc.dll` (currently version 0.2.0) carries the C++ API; `arctixc_c.dll` carries the same implementation plus the C ABI. Both are built from the same sources into the same folder. Link `arctixc.lib` for C++ or `arctixc_c.lib` for C. Prefer a single `.exe` with no DLL to ship? Link `arctixc_static.lib` instead (see "Using ArctixC in your project" below).
- **One-header option, no include tree needed.** `sdk/cpp/arctixc.hpp` (C++ only), `sdk/cpp/arctixc_all.hpp` (C++ + C ABI), and `sdk/c/arctixc_c.h` (pure-C safe) are generated from the same headers on every build. Drop in one file plus the matching `.lib` / `.dll` - you never need the whole `ArctixC/include/` tree in your own project.
- **A ready-to-copy `sdk/` folder.** After every `ArctixC/build.ps1` run: `sdk/cpp/` (headers + `arctixc.lib` + `arctixc.dll` + `arctixc_static.lib`), `sdk/c/` (`arctixc_c.h` + `arctixc_c.lib` + `arctixc_c.dll`), `sdk/tests/` and `sdk/examples/` (runnable, DLLs already beside them).
- **A modern, namespaced C++ API.** Functions follow a consistent structure, such as `arctix::Memory::Allocator::Allocate` and `arctix::Thread::ParallelFor`. Compiles under **C++17, C++20 and C++23** (`-DARCTIXC_CXX_STANDARD=17|20|23`, default 17).
- **A stable C ABI that fully works from C.** The same core functionality is accessible from pure C through headers such as `arctixc/c_api/memory.h` and `arctixc/c_api/thread.h`, including NULL-safe handles. C consumers link `arctixc_c.lib` and ship `arctixc_c.dll`.
- **Embedded version information.** The DLL includes ProductName, ProductVersion, CompanyName, and copyright information in its Windows VERSIONINFO resource. The version is defined once in CMake and automatically propagated to the headers and DLL.
- **Complete API documentation.** `ArctixC/Documentation.html` is a standalone, offline manual documenting all 121 functions with their signatures, arguments, return values, example calls, and explanations. It also includes a searchable index.

## API Overview

| Area | C++ API | Highlights |
|------|---------|------------|
| Foundation | `arctix::Core::` | Version queries and shared `Status` codes |
| Allocator | `arctix::Memory::Allocator` | Aligned allocation, statistics, and resettable counters |
| Memory pool | `arctix::Memory::MemoryPool` | Fixed-size O(1) blocks for frequently used objects |
| Buffer | `arctix::Buffer::Buffer` | Owning, resizable byte array with CopyFrom, Append, Resize, Checksum, and HexEncode |
| Memory utilities | `arctix::Memory::` | `SecureZero`, `MemEqual`, and `Fnv1aHash64` (header-only, non-cryptographic - not for passwords, auth, or security-sensitive integrity) |
| Virtual memory | `arctix::Memory::` | `VirtualReserve`, `VirtualCommit`, `VirtualDecommit`, `VirtualRelease`, `VirtualProtectMemory`, plus `VirtualPageSize` / `RoundUpToPageSize` helpers |
| Thread | `arctix::Thread::Thread` | Move-only thread with caught worker exceptions |
| Thread pool | `arctix::Thread::ThreadPool` | FIFO task execution from a single shared queue (no work stealing), WaitIdle, pending/running/failed task counts - plus the free function `arctix::Thread::ParallelFor` for range-based loops |
| C ABI | `Arctix_*` / `ArctixC_*` | Core functionality for C consumers |

## Compared to native APIs

What you stop hand-writing when you link ArctixC:

| Instead of... | ArctixC gives you... |
|---|---|
| `malloc` / `free`, `new[]` / `delete[]` | `Allocator`: aligned allocation with live statistics (current bytes, peak bytes, counts), resettable counters, optional thread safety |
| A hand-rolled free list for hot objects | `MemoryPool`: fixed-size O(1) blocks with zero per-allocation bookkeeping in your code |
| Raw `new uint8_t[]` plus a manual length | `Buffer`: owning, resizable, move-only byte array (`CopyFrom`, `Append`, `Resize`, `Checksum`, `HexEncode`) - leaks impossible by construction |
| `VirtualAlloc` on Windows, `mmap` elsewhere, `#ifdef` in your code | `VirtualReserve` / `VirtualCommit` / `VirtualDecommit` / `VirtualRelease` / `VirtualProtectMemory`: one API, both backends in-tree |
| Spawning and joining `std::thread`s per task | `ThreadPool`: fixed workers on one FIFO queue, `WaitIdle`, `PendingTasks` / `RunningTasks` / `FailedTasks` |
| Manual index-splitting across threads | `ParallelFor(begin, end, fn, userData)`: one call spreads a range over the hardware |
| Exceptions escaping a worker and terminating the process | `Thread::HasThrown()` and pool-wide `FailedTasks()`: failures become countable data |

ArctixC does not replace the standard library - it sits next to it for the parts the standard library leaves manual: alignment, statistics, pooling, page control, and observable threading.

## Hello world (minimal)

The smallest program that proves your setup works - one include, one linked lib, prints the library version:

```cpp
#include <cstdio>
#include "arctixc_all.hpp" // sdk/cpp/arctixc_all.hpp

int main() {
    std::printf("Hello from ArctixC %s\n", arctix::Core::GetVersionString());
    return 0;
}
```

Build it static (one `.exe`, no DLL to ship): link `sdk/cpp/arctixc_static.lib`, define `ARCTIXC_STATIC`, and compile. Expected output: `Hello from ArctixC 0.2.0`. If you see `LNK2019 __imp_*` errors, the flavors got mixed - see "Using ArctixC in your project" below.

## Quick Start - C++

```cpp
#include <arctixc/memory/allocator.hpp>
#include <arctixc/memory/buffer.hpp>
#include <arctixc/thread/thread_pool.hpp>

// Aligned allocation with statistics.
arctix::Memory::Allocator alloc;
alloc.Initialize(arctix::Memory::DefaultConfig());

void* p = alloc.Allocate(256, 16);

// ... use p ...

alloc.Deallocate(p);
alloc.Shutdown();

// An owning byte buffer.
arctix::Buffer::Buffer buf(1024);
buf.CopyFrom(packet, packetSize);

uint64_t fingerprint = buf.Checksum();

// Run work on a thread pool.
arctix::Thread::ThreadPool pool(4);
pool.Submit(Work, &job);
pool.WaitIdle();
pool.Shutdown();
```

## Quick Start - Pure C

```c
#include <arctixc/c_api/memory.h>

ArctixAllocatorHandle* a = Arctix_CreateAllocator(0); /* NULL = defaults */

void* p = Arctix_Allocate(a, 256, 16);

Arctix_Free(a, p);
Arctix_DestroyAllocator(a);
```

## Pure C++ vs. C++ with ArctixC - Full Example

The same task both ways: build four 1 MB chunks, hash each chunk on a
worker, and print the combined fingerprint. First the plain C++ version
using only the standard library, then the same program with ArctixC.
Both files are complete - includes to `main()` - and print the same result.

Pure C++ (standard library only):

```cpp
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <thread>
#include <vector>

namespace {
std::uint64_t Fnv1a(const std::uint8_t* data, std::size_t size) {
    std::uint64_t h = 14695981039346656037ULL;
    for (std::size_t i = 0; i < size; ++i) { h ^= data[i]; h *= 1099511628211ULL; }
    return h;
}
} // namespace

int main() {
    constexpr std::size_t kChunks = 4;
    constexpr std::size_t kSize = 1024 * 1024;

    // 1. Allocate + fill.
    std::vector<std::uint8_t*> chunks(kChunks);
    for (auto*& c : chunks) {
        c = new std::uint8_t[kSize];
        for (std::size_t i = 0; i < kSize; ++i) c[i] = static_cast<std::uint8_t>(i & 0xFF);
    }

    // 2. Hash each chunk on its own thread.
    std::uint64_t hashes[kChunks] = {};
    std::vector<std::thread> workers;
    for (std::size_t c = 0; c < kChunks; ++c)
        workers.emplace_back([&, c] { hashes[c] = Fnv1a(chunks[c], kSize); });
    for (auto& t : workers) t.join();

    // 3. Combine + report.
    std::uint64_t fingerprint = 14695981039346656037ULL;
    for (auto h : hashes) { fingerprint ^= h; fingerprint *= 1099511628211ULL; }
    std::printf("fingerprint: %llu\n", (unsigned long long)fingerprint);

    // 4. Manual cleanup - every new[] needs its delete[].
    for (auto* c : chunks) delete[] c;
    return 0;
}
```

The same program with ArctixC:

```cpp
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <arctixc/memory/allocator.hpp>
#include <arctixc/memory/mem_utils.hpp>
#include <arctixc/thread/thread_pool.hpp>

namespace {
struct HashJob {
    const std::uint8_t* data;
    std::size_t size;
    std::uint64_t* out;
};

void HashWork(void* userData) {
    const auto* job = static_cast<const HashJob*>(userData);
    *job->out = arctix::Memory::Fnv1aHash64(job->data, job->size);
}
} // namespace

int main() {
    constexpr std::size_t kChunks = 4;
    constexpr std::size_t kSize = 1024 * 1024;

    // 1. One allocator for all chunks (aligned, statistics included).
    arctix::Memory::Allocator alloc;
    alloc.Initialize(arctix::Memory::DefaultConfig());

    std::uint8_t* chunks[kChunks] = {};
    for (auto*& c : chunks) {
        c = static_cast<std::uint8_t*>(alloc.Allocate(kSize, 16));
        for (std::size_t i = 0; i < kSize; ++i) c[i] = static_cast<std::uint8_t>(i & 0xFF);
    }

    // 2. One pool hashes all four chunks - workers stay alive, no re-spawn.
    arctix::Thread::ThreadPool pool(4);
    std::uint64_t hashes[kChunks] = {};
    HashJob jobs[kChunks];
    for (std::size_t c = 0; c < kChunks; ++c) {
        jobs[c] = HashJob{chunks[c], kSize, &hashes[c]};
        pool.Submit(HashWork, &jobs[c]);
    }
    pool.WaitIdle();

    // 3. Combine + report.
    std::uint64_t fingerprint = 14695981039346656037ULL;
    for (auto h : hashes) { fingerprint ^= h; fingerprint *= 1099511628211ULL; }
    std::printf("fingerprint: %llu\n", (unsigned long long)fingerprint);

    // 4. Cleanup: the same allocator frees, then shuts down.
    for (auto* c : chunks) alloc.Deallocate(c);
    pool.Shutdown();
    alloc.Shutdown();
    return 0;
}
```

What changed: aligned allocation with statistics instead of raw `new[]`,
a reusable 4-worker pool instead of spawning and joining threads by hand,
and the built-in `Fnv1aHash64` instead of a hand-rolled hash. To build the
ArctixC version, add `ArctixC/include` to your include path and link
`arctixc.lib` (built by `ArctixC/build.ps1`).

## Dependencies

### Using ArctixC in your project (pick one flavor)

Nothing to install. No package manager, no third-party libraries - each binary links only against Windows system libraries. The only choice you make is **how the library ships with your `.exe`**. All files below come from `sdk/` (regenerated by `ArctixC/build.ps1`).

```text
What ships with your .exe (pick one row):

  Option A - static C++      myapp.exe                  (nothing else)
                             link: sdk/cpp/arctixc_static.lib + ARCTIXC_STATIC

  Option B - C++ DLL         myapp.exe + arctixc.dll
                             link: sdk/cpp/arctixc.lib (no ARCTIXC_STATIC)

  Option C - pure C DLL      myapp.exe + arctixc_c.dll
                             link: sdk/c/arctixc_c.lib, header sdk/c/arctixc_c.h
                             (there is no static C lib - see Option C below)
```

**You have two `.lib` files in `sdk/cpp/` - they are NOT the same:**

- `arctixc.lib` is an **import library** (a few KB of stubs). It contains no code - it just forwards calls to `arctixc.dll` at runtime. Link it = you must also ship the DLL.
- `arctixc_static.lib` is a **static library** (the full compiled code). Link it = everything is baked into your `.exe`, no DLL needed.

**Option A - Static (simplest: one `.exe`, nothing to ship).**

1. Copy `sdk/cpp/arctixc_all.hpp` (or `arctixc.hpp` for C++-only) into your project.
2. Link `sdk/cpp/arctixc_static.lib`.
3. Define `ARCTIXC_STATIC` (Project Properties → C/C++ → Preprocessor, or `target_compile_definitions(myapp PRIVATE ARCTIXC_STATIC)` in CMake). This switches the API macros from `dllimport` to nothing - forget this and you get unresolved externals.
4. `#include "arctixc_all.hpp"` and build. Ship just your `.exe`.

```cmake
# CMake - static
add_executable(myapp main.cpp)
target_sources(myapp PRIVATE path/to/arctixc_all.hpp)
target_link_libraries(myapp PRIVATE path/to/arctixc_static.lib)
target_compile_definitions(myapp PRIVATE ARCTIXC_STATIC)
# Note: arctixc_static.lib uses /MT - build your exe with /MT too
# (MultiThreaded), or you get LNK2038 mismatches.
```

**Option B - DLL (shared, hot-swappable).**

1. Copy `sdk/cpp/arctixc_all.hpp` (or `arctixc.hpp`) into your project.
2. Link `sdk/cpp/arctixc.lib` - do **not** define `ARCTIXC_STATIC` (default = `dllimport`).
3. Copy `sdk/cpp/arctixc.dll` next to your built `.exe` (same folder - Windows will not find it in `sdk/cpp/` at runtime).
4. `#include "arctixc_all.hpp"` and build. Ship your `.exe` + `arctixc.dll`.

```cmake
# CMake - DLL
add_executable(myapp main.cpp)
target_sources(myapp PRIVATE path/to/arctixc_all.hpp)
target_link_libraries(myapp PRIVATE path/to/arctixc.lib)
# After build: copy path/to/arctixc.dll next to myapp.exe
```

### Option C - Pure C projects (DLL only, separate track)

The C interface has a different distribution arrangement from C++: one header, one import library, one DLL, and no static build. Do not mix these files with the C++ ones above.

Getting started (pure C, DLL):

1. Copy `sdk/c/arctixc_c.h` into your project (compiles as real C - verified with `cl /TC`).
2. `#include "arctixc_c.h"` and link `sdk/c/arctixc_c.lib`.
3. Copy `sdk/c/arctixc_c.dll` next to your built `.exe` (same folder) and ship both.
4. Verify: run `dumpbin /dependents myapp.exe` - it must list `arctixc_c.dll` and must NOT list `arctixc.dll`.

Notes for this track:

- There is no static C lib. If you need a single-file `.exe` from C code, use Option A (static C++) and include `arctixc_all.hpp` - it exposes the same C ABI (`Arctix_*` / `ArctixC_*`) alongside the C++ API.
- The C ABI covers allocator, pool, buffer, virtual memory, thread, and pool handles; NULL handles are always safe (calls no-op or return failure). See `sdk/c/arctixc_c.h` - it is the whole contract.

**How to tell it worked:** run `dumpbin /dependents myapp.exe` - static shows only `KERNEL32.dll` (no `arctixc*.dll`); DLL shows `arctixc.dll` / `arctixc_c.dll`. If Windows reports the DLL missing at launch, it is a staging problem (step 3), not a link problem. If you get `LNK2019 __imp_*` errors, you mixed the flavors (e.g. `ARCTIXC_STATIC` defined while linking `arctixc.lib`, or vice versa).

### Compiling ArctixC from source

- **Windows 10/11** with **MSVC** (Visual Studio 2022 or newer, "Desktop development with C++" workload). The virtual-memory layer also has a POSIX backend, but the tested build target is Windows + MSVC.
- **CMake 3.20 or newer** (the copy bundled with Visual Studio works).
- **Ninja** (ships with the Visual Studio CMake install - `build.ps1` sets it up automatically).
- A compiler with **C++17, C++20, or C++23** support (any recent MSVC has all three; pick with `-CppStandard 17|20|23` / `-DARCTIXC_CXX_STANDARD=`).

Then build with a single command (see below).

## Build From Source

> **Not permitted.** Building from source is not permitted - use only official WSD releases as dependencies. The only exception is compiling the unmodified source on your own machine solely to verify its integrity (reproducing the official binaries and running the test suite). Binaries you build yourself are not licensed Components: do not integrate them into your projects, ship them, or distribute them. See `LICENSE.md` (Section 4.4) and the WSD Universal Additional License (Section 4.1) at https://whitepoint.pages.dev/#copyright.

**Requirements:** Windows with MSVC and CMake 3.20 or newer.

The virtual-memory layer also includes a POSIX backend, but the tested build target is Windows with MSVC and Ninja, which is available through Visual Studio.

From the repository root, run:

```powershell
powershell -ExecutionPolicy Bypass -File ArctixC/build.ps1
```

This script configures CMake, builds the Release configuration, and runs the test suite.

Build outputs are placed in `ArctixC/build/` (same folder for both), including:

- `arctixc.dll` + `arctixc.lib` (C++)
- `arctixc_c.dll` + `arctixc_c.lib` (C)
- Test binaries (each suite built twice: `ArctixC_*Tests.exe` for C++, `ArctixC_C_*Tests.exe` + `ArctixC_CApiTests.exe` for C)
- Five runnable examples (four C++, one pure-C `Example_CApiDemo.exe`)

Pass `-NoTests` to skip the test run. Pass `-CppStandard 20` or `-CppStandard 23` for newer C++ (default 17).

## Tests

ArctixC test suites run twice through CTest - once per DLL:

- **MemoryTests / C_MemoryTests** - memory-related functionality (C++ DLL / C DLL).
- **ThreadTests / C_ThreadTests** - thread and thread-pool functionality.
- **ExtendedTests / C_ExtendedTests** - buffers, virtual memory, memory pools, and parallel loops. The C DLL ships its own extended-tests exe too.
- **CApiCheckBuild** - verifies that the C ABI headers compile as actual C, not just C++ (links the C DLL).
- **CApiTests** - pure-C end-to-end runner (allocator, pool, buffer, virtual pages, thread, pool) against `arctixc_c.dll`.

The `ArctixC/examples/` directory contains four C++ examples plus one pure-C demo (`Example_CApiDemo.exe`, links `arctixc_c.lib`).

## Benchmarks

Benchmarks ship inside ArctixCLI - they measure this SDK on your machine, nothing else. There are four:

| Command (defaults) | What it measures | Unit |
|---|---|---|
| `bench [--size 256] [--iters 100000] [--threads N]` | `Allocator` alloc/free vs system `malloc`/`free` (single-threaded), then thread-pool task throughput (`--threads 0` = hardware thread count) | ops/s |
| `bench-pool --size 64 --count 1024 --iters 200000` | `MemoryPool` alloc/free on a fixed pool | ops/s |
| `bench-hash --size 4096 --iters 20000` | FNV-1a (non-cryptographic) over a buffer, two passes per iteration | MB/s |
| `bench-scale [--range 2000000] [--max-workers N]` | Parallel-sum sweep across 1..N workers (N = hardware thread count, capped at 64) | ms + speedup vs 1 worker |

Run them from the repo root after building (`ArctixC/build.ps1`, then build ArctixCLI):

```powershell
ArctixCLI.exe bench --size 256 --iters 100000
ArctixCLI.exe bench-pool --size 64 --count 1024 --iters 200000
ArctixCLI.exe bench-hash --size 4096 --iters 20000
ArctixCLI.exe bench-scale --range 2000000
```

Credibility policy - read before quoting any number:

- **No faster-than claims.** This README makes no claim that Arctix is faster than the standard library or any other allocator. `bench` prints an allocator-vs-`malloc` comparison for context on your machine only - hardware, build flags, and block sizes change the outcome.
- **Every published number must be reproducible.** Quote the exact command with arguments, the machine facts (`ArctixCLI.exe sysinfo`: OS, CPU, RAM, cores), the compiler flags and C++ standard, the Release configuration, and the commit hash. Otherwise it is an anecdote, not a result.
- **Results table (fill in from your own run - do not copy these headings as claims):**

| Benchmark | Machine | Flags / config | Commit | Result |
|---|---|---|---|---|
| `bench` | (your `sysinfo` here) | (e.g. MSVC Release, C++17) | (hash) | (paste output) |
| `bench-pool` | same | same | same | (paste output) |
| `bench-hash` | same | same | same | (paste output) |
| `bench-scale` | same | same | same | (paste table) |

## Cross-platform goals

Honest status, no overselling:

- **Today: Windows 10/11 with MSVC is the tested target.** Every suite runs green there (each suite runs against both DLLs, plus the pure-C runner). C++17 is the default; C++20 and C++23 build with one flag.
- **The POSIX groundwork is already in-tree.** The virtual-memory layer ships a working `mmap` backend next to the `VirtualAlloc` one - the `#ifdef`s live in ArctixC, not in your code. So the API was designed portable from day one; only the CI-kept promise is Windows so far.
- **The goal: same headers, same behavior, more platforms.** Linux and macOS support means porting the thread and allocator backends the way virtual memory already is - no API redesign. Until a platform is tested in CI, this README will not claim it.

Portability rule for contributors and consumers alike: never let platform branches leak into calling code. If you need an `#ifdef` to use ArctixC, that is a bug - file it.

## Project Structure

```text
Arctix/
  ArctixC/                  The SDK - implemented and tested
    include/arctixc/        Public headers (Core, Memory, Buffer, Thread, C ABI)
    src/                    Implementation (not included directly)
    tests/                  CTest suites (each runs against both DLLs, plus pure-C CApiTests)
    examples/               Four C++ examples + one pure-C demo
    resources/              Windows VERSIONINFO template (one copy per DLL: arctixc.dll + arctixc_c.dll)
    cmake/                  CMake package config template (find_package support)
    Documentation.html      Complete offline manual: 121 functions
    Documentation.md        Markdown version of the manual
    generate-docs-md.js     Regenerates Documentation.md from Documentation.html
    Docs.md                 Short pointer to both manual files
    build.ps1               Configure, build, and test in one step

  ArctixI/                  Setup wizard (arctixi.exe), arctix helper, uninstall.exe
  ArctixCLI/                Interactive shell (ArctixCLI.exe) with built-in arctix commands + benchmarks
  ArctixS/                  Planned: graphical workspace and statistics viewer
```

## Documentation

Open `ArctixC/Documentation.html` in any browser, or read `ArctixC/Documentation.md`.

The documentation is entirely offline. It requires no server, internet connection, or additional build step.

The Dashboard explains the setup, while the Functions tab provides a searchable list of every API entry, including its signature, argument types, return type, example call, and explanation.

## Versioning

The library version is defined in exactly one place:

`project(ArctixC VERSION ...)` in `ArctixC/CMakeLists.txt` (currently 0.2.0).

The `ARCTIXC_VERSION_*` macros and the DLL's ProductVersion are generated from that definition during CMake configuration.

This keeps the version consistent across the project and makes version updates a simple, one-line change.

Two version numbers exist side by side - they name different things:

- **ArctixC library version (0.2.0).** The C/C++ SDK itself: headers, DLLs, static lib, docs. Query it at runtime with `arctix::Core::GetVersionString()` or `Arctix_GetVersionString()`. This is the number that matters when linking or reporting SDK bugs.
- **Arctix Core Package release name (e.g. v2.0.0-beta on GitHub Releases).** The distributable bundle: SDK binaries plus the installer (`ArctixI`), shell (`ArctixCLI`), and tools. Its number tracks the whole package, not the library, so it can run ahead of the library version. The companion installer has its own version too (`ARCTIXI_VERSION`, currently 0.1.0 in `ArctixI/src/common.hpp`).

Rule of thumb: integrating the SDK or reading API docs means the library version; downloading a release means the package name. If the two ever disagree on the website, the library version above wins for code, and the mismatch is a docs bug - please file it.

## Project Status

**ArctixC 0.2.0** is built, tested, and documented, with every suite passing against both DLLs.

The companion projects are at different stages of development:

- **ArctixI** - setup wizard installing SDK binaries, the `arctix` command and the uninstaller.
- **ArctixCLI** - interactive `arctix>` shell with built-in commands (`version`, `bench`, `check`, `init`, …) and one-shot mode for CI.
- **ArctixS** - planned graphical workspace and statistics viewer.

These are planned companion projects. Each is intended to use ArctixC as its library rather than reimplementing its systems functionality.

## Community, Feedback & Contributions

Arctix values its community and welcomes ideas, suggestions, improvements, reviews, and general feedback.

**Community ideas may influence the future of Arctix.** Arctix will listen to community feedback and consider suggestions and ideas for future development. Ideas may be reviewed, modified, combined with other suggestions, or implemented when appropriate. However, submitting an idea does not guarantee that it will be accepted or implemented.

**Credit and recognition are not guaranteed.** When a community suggestion influences a change, Arctix may mention in its changelogs that the idea or suggestion came from the community. However, individual attribution is not guaranteed. The original community member's name may not be mentioned in the changelog or credited elsewhere directly.

Arctix genuinely appreciates people who take the time to share ideas, suggest improvements, provide feedback, and write reviews. Constructive feedback - especially practical improvements and thoughtful reviews - is highly valued.

**Contributors:** Arctix may introduce contributor recognition or other ways to acknowledge people who help with the project in the future. However, public contributions are not formally established yet, and there is currently no announced public contributor program. More information may be provided if this changes.

### Whitepoint Software Development Community Server

**Whitepoint Software Development (WSD) has a Community Server on Discord.**

Join the community to connect with WSD, discuss Arctix and other projects, share ideas, provide feedback, and stay connected.

**Discord Invite:** https://discord.gg/UwDStechem

## Bug & Vulnerabilities Report

Found a bug, issue, or vulnerability in Arctix? Reports are welcome and help improve the quality, reliability, and security of the project.

You can create an issue through the **Issues** tab on the [Arctix GitHub repository.](https://github.com/Yummyzzzz/Arctix/issues/) Please provide a clear description of the problem, including steps to reproduce it, expected behavior, actual behavior, and any relevant error messages or logs.

**Security vulnerabilities:** Please avoid posting sensitive security details publicly. Use a private security reporting option if one is available on the repository, so the issue can be reviewed responsibly.

Every report is appreciated. Reports will be reviewed and considered for investigation and possible fixes, but submitting a report does not guarantee a particular resolution or implementation.

**The exploitation, intentional abuse, or malicious distribution of information regarding any known or undisclosed vulnerabilities or bugs in Arctix or any of its tools and components is strictly prohibited. Any such activity constitutes a violation of the Arctix License and may result in the immediate termination of the license agreement between the responsible party and Arctix. Upon termination, the responsible party must immediately cease all use of Arctix and its tools, libraries, and components.**

**Responsible vulnerability reporting and good-faith security research are not prohibited**, provided they are conducted without exploiting, abusing, or causing harm to Arctix or its users.

## Credits

Arctix is developed by **Whitepoint Software Development**, an independent Whitepoint currently consisting solely of **Yummyzzzz**. Whitepoint Software Development is a team name, not a registered company or legal entity. **Yummyzzzz** is the individual developer and current rights holder of Arctix.

GitHub: [https://github.com/Yummyzzzz](https://github.com/Yummyzzzz)

Website: [https://whitepoint.pages.dev/](https://whitepoint.pages.dev/) - Official and real website by Whitepoint Software Development (WSD).

## License

Arctix is released under **Arctix License 1.0**.

See [LICENSE.md](LICENSE.md) for the full license text.

Copyright (c) 2026 Yummyzzzz (Whitepoint Software Development).
