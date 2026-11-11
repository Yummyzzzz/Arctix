// ArctixC/generate-docs-md.js
//
// Regenerates ArctixC/Documentation.md from ArctixC/Documentation.html.
// The .html file is the single source of truth (all 121 functions live in
// its data array); this script only re-formats that data as Markdown.
//
// Usage (from the Arctix/ root):
//   node ArctixC/generate-docs-md.js
const fs = require('fs');
const path = require('path');

const root = __dirname;
const html = fs.readFileSync(path.join(root, 'Documentation.html'), 'utf8');

const groups = eval('(' + html.match(/const GROUPS=(\[.*?\]);/)[1] + ')');
const m = html.match(/const FUNCTIONS=\[([\s\S]*?)\n\];/);
const F = eval('[' + m[1] + ']');

// Pipes would break Markdown tables, so they are flattened in table cells.
function cell(s) { return String(s).replace(/\|/g, '/'); }

const L = [];
L.push('# ArctixC v0.2.0 - SDK Documentation');
L.push('');
L.push('Two standalone DLLs - `arctixc.dll` + `arctixc.lib` (C++) and `arctixc_c.dll` + `arctixc_c.lib` (C) - `arctix::Memory` - `arctix::Buffer` - `arctix::Thread` - `arctix::Core`');
L.push('');
L.push('This file mirrors `ArctixC/Documentation.html` (same content, same 121 functions).');
L.push('Regenerate it any time with `node ArctixC/generate-docs-md.js` from the `Arctix/` root.');
L.push('');
L.push('## Dashboard');
L.push('');
L.push('ArctixC ships as **two** independent binaries from the same sources - `arctixc.dll` with `arctixc.lib` (C++) and `arctixc_c.dll` with `arctixc_c.lib` (C) plus the `include/arctixc/` headers. C++ consumers call `arctix::Memory::`, `arctix::Buffer::`, `arctix::Thread::` and `arctix::Core::` functions (builds under C++17/20/23); C consumers link `arctixc_c.lib` and use the flat `Arctix_*` / `ArctixC_*` ABI from `arctixc/c_api/*.h` (genuinely C-compilable headers).');
L.push('');
L.push('### Categories');
L.push('');
L.push('- **Foundation** - `arctix::Core` version queries + `ArctixCStatus` codes shared by every subsystem. Zero dependencies.');
L.push('- **Memory** - Aligned `arctix::Memory::Allocator` with statistics, O(1) `MemoryPool`, owning `arctix::Buffer::Buffer`, header-only wipes, compares and hashes.');
L.push('- **Virtual memory** - Direct OS page control under `arctix::Memory`: `VirtualReserve` -> `VirtualCommit` -> `VirtualDecommit` -> `VirtualRelease`, plus `VirtualProtectMemory`.');
L.push('- **Threads** - Move-only `arctix::Thread::Thread` with caught worker exceptions, a FIFO `ThreadPool`, and `ParallelFor` index loops.');
L.push('');
L.push('### Use it - C++');
L.push('');
L.push('1. **Link** `arctixc.lib` (C++) or `arctixc_c.lib` (C), add `include/` to the include path, ship the matching DLL next to the exe.');
L.push('2. **Include** only what you use, e.g. `#include <arctixc/memory/buffer.hpp>` (or the `arctixc.hpp` umbrella).');
L.push('3. **Build** bytes in a `Buffer`, allocate with `Allocator`/`MemoryPool`, run work in a `ThreadPool`.');
L.push('');
L.push('### Use it - pure C');
L.push('');
L.push('1. **Include** `<arctixc/c_api/memory.h>` and/or `<arctixc/c_api/thread.h>` from any `.c` file.');
L.push('2. **Create** handles (`Arctix_CreateAllocator`, `Arctix_BufferCreate`, `ArctixC_PoolCreate` ...).');
L.push('3. **Destroy** every handle you create; `NULL` is always safe to pass.');
L.push('');
L.push('### Build and ship');
L.push('');
L.push('From the `Arctix/` root: `powershell -ExecutionPolicy Bypass -File ArctixC\build.ps1` (Release, C++17; add `-CppStandard 20|23` for newer C++). Outputs land in `ArctixC/build/`: `arctixc.dll` + `arctixc_c.dll`, import libs, test exes (each suite runs against both DLLs) and examples. `ctest` runs `MemoryTests`, `ThreadTests`, `ExtendedTests`, their `C_*` twins, plus pure-C `CApiTests`/`CApiCheckBuild`. Each DLL carries embedded version info matching its file name (ProductVersion `0.2.0`); it tracks the CMake project version automatically.');
L.push('');
L.push('### Credits');
L.push('');
L.push('Arctix is developed by **Whitepoint Software Development** - independent Whitepoint behind Arctix (not a registered company or legal entity). Owner and sole developer: **Yummyzzzz** (Owner and Developer of Arctix) - https://github.com/Yummyzzzz');
L.push('');

for (const g of groups) {
  const inGrp = F.filter(f => f.g === g);
  L.push('## ' + g + ' (' + inGrp.length + ')');
  L.push('');
  for (const fn of inGrp) {
    L.push('### ' + fn.a);
    L.push('');
    L.push('Signature:');
    L.push('```cpp');
    L.push(fn.s);
    L.push('```');
    L.push('');
    if (fn.t && fn.t.length) {
      L.push('Arguments:');
      L.push('');
      L.push('| Type | Name | Description |');
      L.push('|------|------|-------------|');
      for (const t of fn.t) {
        L.push('| `' + cell(t[0]) + '` | `' + cell(t[1]) + '` | ' + cell(t[2]) + ' |');
      }
      L.push('');
    } else {
      L.push('Arguments: takes no arguments.');
      L.push('');
    }
    L.push('Returns: ' + fn.r);
    L.push('');
    L.push('Example call:');
    L.push('```cpp');
    L.push(fn.e);
    L.push('```');
    L.push('');
    L.push('Explanation: ' + fn.d);
    L.push('');
  }
}

fs.writeFileSync(path.join(root, 'Documentation.md'), L.join('\n'));
console.log('wrote Documentation.md with ' + F.length + ' functions in ' + groups.length + ' groups');
