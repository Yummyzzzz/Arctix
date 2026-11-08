# ArctixC binaries

Compiled, shippable ArctixC payload. Unlike `build/` (git-ignored), this folder
**is** committed on purpose: the ArctixI setup wizard installs exactly these
files into `<install>\dependencies\` (or downloads them once the links in
`ArctixI/src/common.hpp` go public).

Refresh by rebuilding ArctixC and copying the outputs here:

```powershell
powershell -ExecutionPolicy Bypass -File ArctixC\build.ps1
Copy-Item ArctixC\build\arctixc.dll, ArctixC\build\arctixc.lib, ArctixC\build\arctixc.exp ArctixC\binaries\
Copy-Item ArctixC\build\ArctixC_*.exe, ArctixC\build\Example_*.exe ArctixC\binaries\
```

## Contents

- `arctixc.dll` — the single standalone SDK binary (everything: core, memory,
  buffer, virtual memory, thread, thread pool, C ABIs)
- `arctixc.lib` — import library for linking against the DLL
- `arctixc.exp` — export file produced alongside the import library; ship it
  with the `.lib`, linkers use it to resolve the DLL exports
- `ArctixC_MemoryTests.exe`, `ArctixC_ThreadTests.exe`,
  `ArctixC_ExtendedTests.exe` — the test programs (run fine from
  `dependencies\`, the DLL sits next to them)
- `Example_MemoryOnly.exe`, `Example_ThreadOnly.exe`,
  `Example_MemoryAndThread.exe`, `Example_Extended.exe` — runnable examples

Only the files in the installer list (`kDependencies` in
`ArctixI/src/common.hpp`) are copied during setup. This README itself is
documentation and is never installed.
