# ArctixI — Docs

**Component:** ArctixI (Arctix Installer, produces `ArctixI.exe` when implemented)
**Author / Owner:** Yummyzzzz (https://github.com/Yummyzzzz)
**Status:** Planned — documentation placeholder. Not implemented in this task; ArctixC is the active component.

## Purpose

Installs versioned Arctix components on a developer machine:

- Copies the single `arctixc.dll` plus its `arctixc.lib` import library and the `include/arctixc/` SDK headers.
- Installs the exported CMake package (`ArctixCConfig.cmake`, `ArctixCTargets.cmake`) so consumers can `find_package(ArctixC)`.
- Registers install location and version for ArctixCLI / ArctixS to find.
- Supports side-by-side versions; never overwrites a loaded DLL in place.
- Verifies hashes of official binaries before installing.

## What it will install (from ArctixC v0.1.1)

- `arctixc.dll` + `arctixc.lib` (everything: foundation, allocator, pool, buffer, virtual memory, thread, thread pool, C ABIs)
- `include/arctixc/**/*.hpp` + `include/arctixc/c_api/*.h` public SDK headers
- CMake package files under `lib/cmake/ArctixC/`

## Build

`ArctixI\build.ps1` builds `ArctixI.exe` once a real CMake target exists. Until then it reports the placeholder status and exits 0.
