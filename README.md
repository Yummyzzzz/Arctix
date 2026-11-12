# Arctix — Two DLLs for Memory, Buffers, Virtual Memory, and Threads

**Arctix** is a C/C++ systems toolkit designed to bring several essential low-level utilities together in one library.

Its core library, **ArctixC**, ships as two standalone DLLs built from the same sources into the same folder: `arctixc.dll` for C++ and `arctixc_c.dll` for C, each with its import library (`arctixc.lib` / `arctixc_c.lib`) plus the public headers. Same namespace (`arctix::`), same headers, same behavior — only the binary you link differs. Link one import library, ship its DLL, and gain access to aligned memory allocation with statistics, O(1) memory pools, owning byte buffers, direct operating-system virtual memory control, and multithreading utilities — including a FIFO thread pool without work stealing.

If you have ever needed multiple small libraries just to allocate aligned memory, manage byte data, and run tasks on threads, ArctixC brings those capabilities together in one tested library.

Arctix is released under the **Arctix License**, allowing use in both open-source and closed-source commercial projects. 
By downloading or using Arctix, you automatically accept the **Arctix License**.

**Official WSD Website:** https://whitepoint.pages.dev/ - This is the valid and real website by Whitepoint Software Development (WSD) for Arctix and other WSD projects.

## What You Get

- **Two standalone DLLs, one for each language.** `arctixc.dll` (currently version 0.2.0) carries the C++ API; `arctixc_c.dll` carries the same implementation plus the C ABI. Both are built from the same sources into the same folder. Link `arctixc.lib` for C++ or `arctixc_c.lib` for C.
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
| Memory utilities | `arctix::Memory::` | `SecureZero`, `MemEqual`, and `Fnv1aHash64` (header-only) |
| Virtual memory | `arctix::Memory::` | Reserve, Commit, Decommit, Release, ProtectMemory, and page helpers |
| Thread | `arctix::Thread::Thread` | Move-only thread with caught worker exceptions |
| Thread pool | `arctix::Thread::ThreadPool` | FIFO task execution, WaitIdle, per-task exception counts, and `ParallelFor` |
| C ABI | `Arctix_*` / `ArctixC_*` | Core functionality for C consumers |

## Quick Start — C++

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

## Quick Start — Pure C

```c
#include <arctixc/c_api/memory.h>

ArctixAllocatorHandle* a = Arctix_CreateAllocator(0); /* NULL = defaults */

void* p = Arctix_Allocate(a, 256, 16);

Arctix_Free(a, p);
Arctix_DestroyAllocator(a);
```

## Pure C++ vs. C++ with ArctixC — Full Example

The same task both ways: build four 1 MB chunks, hash each chunk on a
worker, and print the combined fingerprint. First the plain C++ version
using only the standard library, then the same program with ArctixC.
Both files are complete — includes to `main()` — and print the same result.

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

    // 4. Manual cleanup — every new[] needs its delete[].
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

    // 2. One pool hashes all four chunks — workers stay alive, no re-spawn.
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

### Just using ArctixC (prebuilt DLLs + import libs)

Nothing. No installer, no package manager, no third-party libraries:

- Each DLL links only against Windows system libraries, so there is nothing to install on the target machine.
- C++: copy `arctixc.dll` next to your `.exe`, add `ArctixC/include` to your include path, link `arctixc.lib` — done.
- C: copy `arctixc_c.dll` next to your `.exe`, add `ArctixC/include` to your include path, link `arctixc_c.lib`, include `arctixc/c_api/memory.h` / `arctixc/c_api/thread.h` from pure C — done.

### Compiling ArctixC from source

- **Windows 10/11** with **MSVC** (Visual Studio 2022 or newer, "Desktop development with C++" workload). The virtual-memory layer also has a POSIX backend, but the tested build target is Windows + MSVC.
- **CMake 3.20 or newer** (the copy bundled with Visual Studio works).
- **Ninja** (ships with the Visual Studio CMake install — `build.ps1` sets it up automatically).
- A compiler with **C++17, C++20, or C++23** support (any recent MSVC has all three; pick with `-CppStandard 17|20|23` / `-DARCTIXC_CXX_STANDARD=`).

Then build with a single command (see below).

## Build From Source

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

ArctixC test suites run twice through CTest — once per DLL:

- **MemoryTests / C_MemoryTests** — memory-related functionality (C++ DLL / C DLL).
- **ThreadTests / C_ThreadTests** — thread and thread-pool functionality.
- **ExtendedTests / C_ExtendedTests** — buffers, virtual memory, memory pools, and parallel loops. The C DLL ships its own extended-tests exe too.
- **CApiCheckBuild** — verifies that the C ABI headers compile as actual C, not just C++ (links the C DLL).
- **CApiTests** — pure-C end-to-end runner (allocator, pool, buffer, virtual pages, thread, pool) against `arctixc_c.dll`.

The `ArctixC/examples/` directory contains four C++ examples plus one pure-C demo (`Example_CApiDemo.exe`, links `arctixc_c.lib`).

## Project Structure

```text
Arctix/
  ArctixC/                  The SDK — implemented and tested
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

The project version is defined in exactly one place:

`project(ArctixC VERSION ...)` in `ArctixC/CMakeLists.txt`.

The `ARCTIXC_VERSION_*` macros and the DLL's ProductVersion are generated from that definition during CMake configuration.

This keeps the version consistent across the project and makes version updates a simple, one-line change.

## Project Status

**ArctixC 0.2.0** is built, tested, and documented, with every suite passing against both DLLs.

The companion projects are at different stages of development:

- **ArctixI** — setup wizard installing SDK binaries, the `arctix` command and the uninstaller.
- **ArctixCLI** — interactive `arctix>` shell with built-in commands (`version`, `bench`, `check`, `init`, …) and one-shot mode for CI.
- **ArctixS** — planned graphical workspace and statistics viewer.

These are planned companion projects. Each is intended to use ArctixC as its library rather than reimplementing its systems functionality.

## Community, Feedback & Contributions

Arctix values its community and welcomes ideas, suggestions, improvements, reviews, and general feedback.

**Community ideas may influence the future of Arctix.** Arctix will listen to community feedback and consider suggestions and ideas for future development. Ideas may be reviewed, modified, combined with other suggestions, or implemented when appropriate. However, submitting an idea does not guarantee that it will be accepted or implemented.

**Credit and recognition are not guaranteed.** When a community suggestion influences a change, Arctix may mention in its changelogs that the idea or suggestion came from the community. However, individual attribution is not guaranteed. The original community member's name may not be mentioned in the changelog or credited elsewhere directly.

Arctix genuinely appreciates people who take the time to share ideas, suggest improvements, provide feedback, and write reviews. Constructive feedback — especially practical improvements and thoughtful reviews — is highly valued.

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
