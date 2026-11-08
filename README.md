# Arctix — One DLL for Memory, Buffers, Virtual Memory, and Threads

**Arctix** is a C/C++ systems toolkit designed to bring several essential low-level utilities together in one library.

Its core library, **ArctixC**, is distributed as a single standalone DLL: `arctixc.dll`, accompanied by its import library (`arctixc.lib`) and public headers. No split modules, no unnecessary dependency chain, and no installer required. Link one import library, ship one DLL, and gain access to aligned memory allocation with statistics, O(1) memory pools, owning byte buffers, direct operating-system virtual memory control, and multithreading utilities — including a FIFO thread pool without work stealing.

If you have ever needed multiple small libraries just to allocate aligned memory, manage byte data, and run tasks on threads, ArctixC brings those capabilities together in one tested library.

Arctix is released under the **Arctix License**, allowing use in both open-source and closed-source commercial projects.

## What You Get

- **One standalone DLL.** All compiled functionality lives in `arctixc.dll` (currently version 0.1.1), with the corresponding import library and headers. The build is configured to prevent split-module DLLs from being produced.
- **A modern, namespaced C++ API.** Functions follow a consistent structure, such as `arctixs::Memory::Allocator::Allocate` and `arctixs::Thread::ParallelFor`.
- **A stable C ABI.** The same core functionality is accessible from pure C through headers such as `arctixc/c_api/memory.h` and `arctixc/c_api/thread.h`, including NULL-safe handles.
- **Embedded version information.** The DLL includes ProductName, ProductVersion, CompanyName, and copyright information in its Windows VERSIONINFO resource. The version is defined once in CMake and automatically propagated to the headers and DLL.
- **Complete API documentation.** `ArctixC/Documentation.html` is a standalone, offline manual documenting all 121 functions with their signatures, arguments, return values, example calls, and explanations. It also includes a searchable index.

## API Overview

| Area | C++ API | Highlights |
|------|---------|------------|
| Foundation | `arctixs::Core::` | Version queries and shared `Status` codes |
| Allocator | `arctixs::Memory::Allocator` | Aligned allocation, statistics, and resettable counters |
| Memory pool | `arctixs::Memory::MemoryPool` | Fixed-size O(1) blocks for frequently used objects |
| Buffer | `arctixs::Buffer::Buffer` | Owning, resizable byte array with CopyFrom, Append, Resize, Checksum, and HexEncode |
| Memory utilities | `arctixs::Memory::` | `SecureZero`, `MemEqual`, and `Fnv1aHash64` (header-only) |
| Virtual memory | `arctixs::Memory::` | Reserve, Commit, Decommit, Release, ProtectMemory, and page helpers |
| Thread | `arctixs::Thread::Thread` | Move-only thread with caught worker exceptions |
| Thread pool | `arctixs::Thread::ThreadPool` | FIFO task execution, WaitIdle, per-task exception counts, and `ParallelFor` |
| C ABI | `Arctix_*` / `ArctixC_*` | Core functionality for C consumers |

## Quick Start — C++

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

// Run work on a thread pool.
arctixs::Thread::ThreadPool pool(4);
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

## Build From Source

**Requirements:** Windows with MSVC and CMake 3.20 or newer.

The virtual-memory layer also includes a POSIX backend, but the tested build target is Windows with MSVC and Ninja, which is available through Visual Studio.

From the repository root, run:

```powershell
powershell -ExecutionPolicy Bypass -File ArctixC/build.ps1
```

This script configures CMake, builds the Release configuration, and runs the test suite.

Build outputs are placed in `ArctixC/build/`, including:

- `arctixc.dll`
- `arctixc.lib`
- Test binaries
- Four runnable examples

Pass `-NoTests` to skip the test run.

## Tests

ArctixC currently includes four test suites, executed through CTest:

- **MemoryTests** — memory-related functionality.
- **ThreadTests** — thread and thread-pool functionality.
- **ExtendedTests** — buffers, virtual memory, memory pools, and parallel loops.
- **CApiCheckBuild** — verifies that the C ABI headers compile as actual C, not just C++.

The `ArctixC/examples/` directory also contains four runnable examples, covering the library's main areas.

## Project Structure

```text
Arctix/
  ArctixC/                  The SDK — implemented and tested
    include/arctixc/        Public headers (Core, Memory, Buffer, Thread, C ABI)
    src/                    Implementation (not included directly)
    tests/                  The four CTest suites
    examples/               Four runnable examples
    resources/              Windows VERSIONINFO template for arctixc.dll
    cmake/                  CMake package config template (find_package support)
    Documentation.html      Complete offline manual: 121 functions
    Documentation.md        Markdown version of the manual
    generate-docs-md.js     Regenerates Documentation.md from Documentation.html
    Docs.md                 Short pointer to both manual files
    build.ps1               Configure, build, and test in one step

  ArctixI/                  Planned: versioned installer for SDK binaries
  ArctixCLI/                Planned: command-line front-end and benchmarks
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

**ArctixC 0.1.1** is built, tested, and documented, with all four test suites currently passing.

The companion projects are at different stages of development:

- **ArctixI** — planned versioned installer for SDK binaries.
- **ArctixCLI** — planned command-line front-end and benchmarking tools.
- **ArctixS** — planned graphical workspace and statistics viewer.

These are planned companion projects. Each is intended to use ArctixC as its library rather than reimplementing its systems functionality.

## Community, Feedback & Contributions

Arctix values its community and welcomes ideas, suggestions, improvements, reviews, and general feedback.

**Community ideas may influence the future of Arctix.** Arctix will listen to community feedback and consider suggestions and ideas for future development. Ideas may be reviewed, modified, combined with other suggestions, or implemented when appropriate. However, submitting an idea does not guarantee that it will be accepted or implemented.

**Credit and recognition are not guaranteed.** When a community suggestion influences a change, Arctix may mention in its changelogs that the idea or suggestion came from the community. However, individual attribution is not guaranteed. The original community member's name may not be mentioned in the changelog or credited elsewhere directly.

Arctix genuinely appreciates people who take the time to share ideas, suggest improvements, provide feedback, and write reviews. Constructive feedback — especially practical improvements and thoughtful reviews — is highly valued.

**Contributors:** Arctix may introduce contributor recognition or other ways to acknowledge people who help with the project in the future. However, public contributions are not formally established yet, and there is currently no announced public contributor program. More information may be provided if this changes.

## Credits

Arctix is developed by **Whitepoint Software Development**, an independent Whitepoint currently consisting solely of **Yummyzzzz**. Whitepoint Software Development is a team name, not a registered company or separate legal entity. **Yummyzzzz** is the individual developer and current rights holder of Arctix.

GitHub: [https://github.com/Yummyzzzz](https://github.com/Yummyzzzz)

## License

Arctix is released under **Arctix License 1.0**.

See [LICENSE](LICENSE) for the full license text.

Copyright (c) 2026 Yummyzzzz.
