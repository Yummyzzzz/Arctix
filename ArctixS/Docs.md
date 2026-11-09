# ArctixS — Docs

**Component:** ArctixS (Arctix Studio, produces `ArctixS.exe` when implemented)
**Developer:** Whitepoint Software Development (independent team, not a registered company or legal entity) - **Author / Owner:** Yummyzzzz (https://github.com/Yummyzzzz)
**Status:** Planned — documentation placeholder. Not implemented in this task; ArctixC is the active component.

## Purpose

Graphical workspace for Arctix developers:

- Inspect ArctixC SDK versions and installed feature modules.
- Visualize allocator statistics (`ArctixStats`) and thread states.
- Launch and compare benchmarks across feature DLLs.
- Manage ArctixI-installed SDK versions side by side.

## Relationship to ArctixC

ArctixS will be a pure consumer of the ArctixC SDK (`ArctixC::Core` / `ArctixC::Memory` / `ArctixC::Thread`). It will implement no systems functionality itself.

## Build

`ArctixS\build.ps1` builds `ArctixS.exe` once a real CMake target exists. Until then it reports the placeholder status and exits 0.
