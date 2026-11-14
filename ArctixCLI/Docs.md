# ArctixCLI - Docs

**Component:** ArctixCLI (produces `ArctixCLI.exe`)
**Developer:** Whitepoint Software Development (independent team, not a registered company or legal entity) - **Author / Owner:** Yummyzzzz (https://github.com/Yummyzzzz)
**Status:** Implemented. Interactive cmd-style shell + one-shot mode for CI.

## What it is

`ArctixCLI.exe` looks and feels like cmd (banner, `arctix>` prompt,
`help`, `cls`, `echo`, `exit`) but understands **only arctix commands**.
Every command is built into the exe and runs in-process against
`arctixc.dll` - nothing shells out to PowerShell, and nothing depends on
`PATH`. Ship `ArctixCLI.exe` with `arctixc.dll` next to it and it runs
anywhere. Output is colored on a real console (cyan prompt and headings,
green names and `[PASS]`, red errors); piped output, `NO_COLOR`, and
`color off` all disable it.

Start it with no arguments for the shell:

```text
ArctixCLI [Version 0.1.0]
(c) Whitepoint Software Development. Type "help" for arctix commands.
arctix> bench-scale --range 2000000
...
```

Or run one command non-interactively (scripts, CI):

```powershell
ArctixCLI --version
ArctixCLI --check
ArctixCLI bench --size 256 --iters 100000 --threads 4
ArctixCLI init my_app
ArctixCLI help bench
```

`help` groups commands by category, most useful first:

```text
Benchmarks - measure the SDK (allocators, pools, hashes, scaling).
Diagnostics - prove the SDK is healthy (leaks, stress, machine info).
Memory - allocate, pool, inspect and wipe memory.
Hashing - FNV-1a checksums for text and files.
Threading - pools, scaling sweeps and parallel loops.
Project - scaffold apps on ArctixC.
Shell - the shell itself.
```

## Command reference

### Benchmarks

| Command | What it does |
|---------|--------------|
| `bench [--size B] [--iters N] [--threads T]` | ArctixC allocator vs system `malloc` throughput, then thread-pool task throughput. |
| `bench-pool [--size B] [--count N] [--iters N]` | Fixed-size `MemoryPool` alloc/free throughput (ops/s). |
| `bench-hash [--size B] [--iters N]` | FNV-1a hashing throughput in MB/s. |
| `bench-scale [--range N] [--max-workers N]` | Parallel-sum sweep across 1..N workers with a speedup table. |

### Diagnostics

| Command | What it does |
|---------|--------------|
| `check` | Leak / exhaustion / checksum diagnostics. Exit code 1 on failure - CI-ready. |
| `stress [--seconds S] [--threads N]` | Hammers allocator + pool + hash on worker threads, then proves outstanding bytes are zero. |
| `sysinfo` / `info` | Machine facts (OS, page size, RAM, cores) plus SDK version and flavor. |
| `leak <bytes> [count]` | Intentionally leaks blocks so you can watch the detector, then frees them. |

### Memory

| Command | What it does |
|---------|--------------|
| `alloc <bytes> [align] [count]` | Allocate via the SDK `Allocator`, show live statistics, free all. |
| `pool <block-bytes> <block-count>` | Drain a fixed-size `MemoryPool`, report used/free counts. |
| `buffer <size> [fill]` | `Buffer` lifecycle: hex preview, append, resize, `CopyFrom` round-trip, `SecureWipe`. |
| `wipe <bytes>` | `SecureZero` memory and verify every byte reads back zero. |

### Hashing

| Command | What it does |
|---------|--------------|
| `hash <text...>` | FNV-1a 64-bit hash of text (same function the SDK uses). |
| `hash-file <path>` | Streams a file through FNV-1a, prints hash plus byte count. |

FNV-1a is non-cryptographic. Never use `hash` / `hash-file` output for password storage, cryptographic authentication, or security-sensitive integrity verification - they are checksums for change detection and diagnostics only.

### Threading

| Command | What it does |
|---------|--------------|
| `threads [workers]` | Parallel-sum demo on the `ThreadPool`, verifies the result. |
| `cores` | Prints hardware concurrency and verifies `ParallelFor` visits each index exactly once. |
| `parallel <begin> <end> [workers]` | `ParallelFor` sum over any range with timing and Gauss verification. |

### Project

| Command | What it does |
|---------|--------------|
| `init [name] [--path DIR] [--lang cxx\|c] [--flavor cxx\|c]` | Scaffold a project. Bare `init` is interactive: it asks for the project name, the path to create it in, the language (`cxx` or `c`), and the SDK flavor (`arctixc` = C++ DLL, `arctixc_c` = C DLL). Generates `CMakeLists.txt` (`find_package(ArctixC)` + the matching `ArctixC::ArctixC[_C]` target), `main.cpp` / `main.c`, and a `README.md` that tells you which DLL (`arctixc.dll` / `arctixc_c.dll`) and import lib to ship. |
| `sdk` | Both SDK flavors side by side (DLLs, headers, CMake targets) and which one this CLI uses. |

### Shell

| Command | What it does |
|---------|--------------|
| `help [cmd]` | Categorized list, or usage of one command. |
| `version` / `ver` | ArctixCLI + live ArctixC versions. |
| `color [on\|off]` | Toggle colored output. |
| `run <script>` | Execute a text file of arctix commands line by line (`#` = comment, `exit` ends the script). Continues past failures and reports the count. |
| `history` | Numbered list of commands run this interactive session. |
| `echo [text...]` | Print text back. |
| `cls` / `clear` | Clear the screen (WinAPI, no `cmd.exe` involved). |
| `exit` / `quit` | Leave the shell. |

Unknown input is rejected cmd-style:
`'foo' is not recognized as an arctix command. Type "help".`

## Relationship to ArctixC

ArctixCLI contains no allocator or thread logic itself - it consumes the
ArctixC SDK from `../ArctixC` sources at build time and links only
`ArctixC::ArctixC`. All behavior (`Allocator`, `MemoryPool`, `Buffer`,
`ThreadPool`, `ParallelFor`, version queries) lives in `arctixc.dll`.
Run `sdk` inside the shell for the flavor map, or `sysinfo` for machine facts.

## Build

```powershell
powershell -ExecutionPolicy Bypass -File ArctixCLI\build.ps1
```

Output in `ArctixCLI/build/`: `ArctixCLI.exe` + `arctixc.dll` side by side.
The CLI tracks its own version (`project(ArctixCLI VERSION ...)` in
`ArctixCLI/CMakeLists.txt`, embedded in the exe's version resource);
the SDK version is read live from the DLL, never hardcoded.
