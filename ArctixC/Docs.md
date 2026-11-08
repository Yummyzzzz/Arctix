# ArctixC — Docs

**The full SDK manual is now `ArctixC/DOC.html`** — open it in any browser (double-click the file; no server, no internet needed).

`DOC.html` is one standalone file (all CSS/JS inline, black background, white font) with:

- a liquid-glass **Dashboard** (overview, C++ / C usage, build & ship steps),
- a searchable, clickable list of **all 118 functions** across Foundation, Allocator, Memory Pool, Buffer, Memory Utils, Virtual Memory, Thread, Thread Pool and both C ABIs,
- every function shown as **Name / Args (with types) / Returns (with type) / Example call / Explanation**, with syntax-highlighted code.

Build: `powershell -ExecutionPolicy Bypass -File ArctixC\build.ps1` (from the `Arctix/` root). Output: single `arctixc.dll` + `arctixc.lib` in `ArctixC/build/Release/`.
