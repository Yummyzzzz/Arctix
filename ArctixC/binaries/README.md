# ArctixC binaries

Compiled, shippable ArctixC payload. Unlike `build/` (git-ignored), this folder
**is** committed on purpose: the ArctixI setup wizard installs exactly these
files into `<install>\dependencies\` (or downloads them once the links in
`ArctixI/src/common.hpp` go public).

Refresh by rebuilding ArctixC and copying the outputs here:

```powershell
powershell -ExecutionPolicy Bypass -File ArctixC\build.ps1
Copy-Item ArctixC\build\arctixc.dll, ArctixC\build\arctixc.lib, ArctixC\build\arctixc.exp ArctixC\binaries\
Copy-Item ArctixC\build\arctixc_c.dll, ArctixC\build\arctixc_c.lib, ArctixC\build\arctixc_c.exp ArctixC\binaries\
Copy-Item ArctixC\build\ArctixC_*.exe, ArctixC\build\Example_*.exe ArctixC\binaries\
```

Append `-CppStandard 20` or `-CppStandard 23` to build with C++20/C++23
(the code is C++17-clean, so all three standards build the same API).

## Contents

- `arctixc.dll` — the C++ SDK binary (arctix::Core/Memory/Buffer/Thread)
- `arctixc.lib` — import library for the C++ DLL
- `arctixc.exp` — export file produced alongside the C++ import library
- `arctixc_c.dll` — the C SDK binary (same impl + `Arctix_*`/`ArctixC_*` C ABI)
- `arctixc_c.lib` — import library for the C DLL (C consumers link this one)
- `arctixc_c.exp` — export file produced alongside the C import library
- `ArctixC_MemoryTests.exe`, `ArctixC_ThreadTests.exe`,
  `ArctixC_ExtendedTests.exe` — test programs for the C++ DLL
- `ArctixC_C_MemoryTests.exe`, `ArctixC_C_ThreadTests.exe`,
  `ArctixC_C_ExtendedTests.exe`, `ArctixC_CApiTests.exe` — test programs
  for the C DLL (the C DLL carries the full implementation too, plus the
  pure-C `CApiTests` runner)
- `Example_MemoryOnly.exe`, `Example_ThreadOnly.exe`,
  `Example_MemoryAndThread.exe`, `Example_Extended.exe` — C++ examples
- `Example_CApiDemo.exe` — pure-C demo (links `arctixc_c.lib`)

Only the files in the installer list (`kDependencies` in
`ArctixI/src/common.hpp`) are copied during setup. This README itself is
documentation and is never installed.
