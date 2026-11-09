# ArctixCLI — Docs

**Component:** ArctixCLI (produces `ArctixCLI.exe` when implemented)
**Developer:** Whitepoint Software Development (independent team, not a registered company or legal entity) - **Author / Owner:** Yummyzzzz (https://github.com/Yummyzzzz)
**Status:** Planned — documentation placeholder. Not implemented in this task; ArctixC is the active component.

## Purpose

Command-line front-end over the ArctixC SDK for users and CI pipelines:

- `ArctixCLI --version` — print ArctixC foundation / feature DLL versions.
- `ArctixCLI --bench` — run allocation benchmarks against system malloc.
- `ArctixCLI --check` — leak / double-free diagnostics for instrumented builds.
- `ArctixCLI init` — scaffold a new project linked against `ArctixC::Core` / `ArctixC::Memory` / `ArctixC::Thread`.

## Relationship to ArctixC

ArctixCLI will contain no allocator or thread logic itself; it will consume the ArctixC SDK via `find_package(ArctixC)` and link only the targets it needs (e.g. `ArctixC::Thread` for thread diagnostics). All behavior lives in `arctixc.dll`.

## Build

`ArctixCLI\build.ps1` builds `ArctixCLI.exe` once a real CMake target exists. Until then it reports the placeholder status and exits 0.
