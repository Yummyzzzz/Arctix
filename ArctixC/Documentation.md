# ArctixC v0.1.1 - SDK Documentation

One standalone `arctixc.dll` + `arctixc.lib` - `arctix::Memory` - `arctix::Buffer` - `arctix::Thread` - `arctix::Core`

This file mirrors `ArctixC/Documentation.html` (same content, same 121 functions).
Regenerate it any time with `node ArctixC/generate-docs-md.js` from the `Arctix/` root.

## Dashboard

ArctixC ships as **one** independent binary - `arctixc.dll` with `arctixc.lib` and the `include/arctixc/` headers. There is a single DLL to ship - no split modules - and every subsystem below lives in it. C++ consumers call `arctix::Memory::`, `arctix::Buffer::`, `arctix::Thread::` and `arctix::Core::` functions; C consumers use the flat `Arctix_*` / `ArctixC_*` ABI from `arctixc/c_api/*.h` (genuinely C-compilable headers).

### Categories

- **Foundation** - `arctix::Core` version queries + `ArctixCStatus` codes shared by every subsystem. Zero dependencies.
- **Memory** - Aligned `arctix::Memory::Allocator` with statistics, O(1) `MemoryPool`, owning `arctix::Buffer::Buffer`, header-only wipes, compares and hashes.
- **Virtual memory** - Direct OS page control under `arctix::Memory`: `VirtualReserve` -> `VirtualCommit` -> `VirtualDecommit` -> `VirtualRelease`, plus `VirtualProtectMemory`.
- **Threads** - Move-only `arctix::Thread::Thread` with caught worker exceptions, a FIFO `ThreadPool`, and `ParallelFor` index loops.

### Use it - C++

1. **Link** `arctixc.lib`, add `include/` to the include path, ship `arctixc.dll` next to the exe.
2. **Include** only what you use, e.g. `#include <arctixc/memory/buffer.hpp>` (or the `arctixc.hpp` umbrella).
3. **Build** bytes in a `Buffer`, allocate with `Allocator`/`MemoryPool`, run work in a `ThreadPool`.

### Use it - pure C

1. **Include** `<arctixc/c_api/memory.h>` and/or `<arctixc/c_api/thread.h>` from any `.c` file.
2. **Create** handles (`Arctix_CreateAllocator`, `Arctix_BufferCreate`, `ArctixC_PoolCreate` ...).
3. **Destroy** every handle you create; `NULL` is always safe to pass.

### Build and ship

From the `Arctix/` root: `powershell -ExecutionPolicy Bypass -File ArctixC\build.ps1` (configures CMake + Ninja, Release). Outputs land in `ArctixC/build/`: `arctixc.dll`, `arctixc.lib`, test and example exes. `ctest -C Release` runs `MemoryTests`, `ThreadTests`, `ExtendedTests` and the pure-C `CApiCheckBuild`. The script fails if a split `*Memory*.dll` / `*Thread*.dll` is ever produced again. `arctixc.dll` carries embedded version info (ProductName `arctixc.dll`, ProductVersion `0.1.1` - see Properties -> Details); it tracks the CMake project version automatically.

### Credits

Arctix is developed by **Whitepoint Software Development** - independent Whitepoint behind Arctix (not a registered company). Owner and sole developer: **Yummyzzzz** (Owner and Developer of Arctix) - https://github.com/Yummyzzzz

## Foundation (7)

### arctix::Core::GetVersionString

Signature:
```cpp
const char* arctix::Core::GetVersionString();
```

Arguments: takes no arguments.

Returns: const char* - e.g. "0.1.1". Never NULL.

Example call:
```cpp
const char* v = arctix::Core::GetVersionString();
```

Explanation: Returns the SDK version string. Valid forever (points at static storage); compare it at startup to fail fast on DLL mix-ups.

### arctix::Core::GetVersion

Signature:
```cpp
void arctix::Core::GetVersion(int* major, int* minor, int* patch);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `int*` | `major` | Receives the major version. NULL = skip. |
| `int*` | `minor` | Receives the minor version. NULL = skip. |
| `int*` | `patch` | Receives the patch version. NULL = skip. |

Returns: void

Example call:
```cpp
int mj, mn, p; arctix::Core::GetVersion(&mj, &mn, &p);
```

Explanation: Numeric form of the version for range checks (e.g. require minor >= 3). Every out-parameter is optional.

### arctix::Core::StatusToString

Signature:
```cpp
const char* arctix::Core::StatusToString(int status);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `int` | `status` | An ArctixCStatus code (0-9). |

Returns: const char* - e.g. "Ok", "Exhausted". Never NULL; unknown codes map to "Unknown".

Example call:
```cpp
puts(arctix::Core::StatusToString(ArctixCStatus_Exhausted));
```

Explanation: Human-readable name for a status code, for logs and error messages. Pure lookup, no allocation, thread-safe.

### arctix::Core::GetVersionMajor

Signature:
```cpp
int arctix::Core::GetVersionMajor();
```

Arguments: takes no arguments.

Returns: int - major version.

Example call:
```cpp
int major = arctix::Core::GetVersionMajor(); // 0
```

Explanation: Major version as a plain int for range checks. Header-only wrapper around ArctixC_GetVersion.

### arctix::Core::GetVersionMinor

Signature:
```cpp
int arctix::Core::GetVersionMinor();
```

Arguments: takes no arguments.

Returns: int - minor version.

Example call:
```cpp
int minor = arctix::Core::GetVersionMinor(); // 1
```

Explanation: Minor version as a plain int. Header-only wrapper around ArctixC_GetVersion.

### arctix::Core::GetVersionPatch

Signature:
```cpp
int arctix::Core::GetVersionPatch();
```

Arguments: takes no arguments.

Returns: int - patch version.

Example call:
```cpp
printf("%d\n", arctix::Core::GetVersionPatch());
```

Explanation: Patch version as a plain int. Header-only wrapper around ArctixC_GetVersion.

### ArctixCStatus codes

Signature:
```cpp
typedef enum ArctixCStatus { Ok=0, InvalidArgument=1, NotInitialized=2, AlreadyInitialized=3, OutOfMemory=4, Exhausted=5, NotJoinable=6, AlreadyRunning=7, WorkerThrew=8, Unknown=9 } ArctixCStatus;
```

Arguments: takes no arguments.

Returns: n/a - type definition (global scope, usable from C and C++).

Example call:
```cpp
if (pool.Create(0, 8)) { /* ... */ }
```

Explanation: Shared result codes. Memory keeps historical bool returns; newer APIs (Thread, ThreadPool, virtual memory) use bools today and these codes document the failure vocabulary. From C++ the arctix::Core::Status alias may be used.

## Allocator (10)

### arctix::Memory::ArctixConfig (struct)

Signature:
```cpp
struct arctix::Memory::ArctixConfig { bool threadSafe = true; bool enableStatistics = true; std::size_t defaultAlignment = 16; };
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `bool` | `threadSafe` | Locking Allocate/Deallocate for multi-thread use. false = faster single-thread path. |
| `bool` | `enableStatistics` | Track GetStatistics() counters. Disable for minimum overhead. |
| `size_t` | `defaultAlignment` | Power-of-two alignment used when Allocate() is called with 0. |

Returns: n/a - plain struct, built with arctix::Memory::DefaultConfig() then tweaked.

Example call:
```cpp
arctix::Memory::ArctixConfig c = arctix::Memory::DefaultConfig(); c.threadSafe = false;
```

Explanation: Allocator configuration. Copy it, tweak fields, pass to Initialize(). Header-only: no DLL call involved.

### DefaultConfig

Signature:
```cpp
arctix::Memory::ArctixConfig arctix::Memory::DefaultConfig();
```

Arguments: takes no arguments.

Returns: arctix::Memory::ArctixConfig - { threadSafe:true, enableStatistics:true, defaultAlignment:16 }.

Example call:
```cpp
allocator.Initialize(arctix::Memory::DefaultConfig());
```

Explanation: Returns a sane default configuration. Inline helper, costs nothing; prefer it over hand-filling the struct.

### arctix::Memory::ArctixStats (struct)

Signature:
```cpp
struct arctix::Memory::ArctixStats { size_t totalAllocatedBytes, totalFreedBytes, currentUsageBytes, peakUsageBytes; unsigned long long allocationCount, freeCount; };
```

Arguments: takes no arguments.

Returns: n/a - snapshot returned by value from GetStatistics().

Example call:
```cpp
arctix::Memory::ArctixStats s = allocator.GetStatistics();
```

Explanation: Allocator telemetry. currentUsageBytes = allocated - freed; peakUsageBytes = high-water mark since Initialize()/ResetStatistics(). Read freely on the calling thread after GetStatistics().

### arctix::Memory::Allocator::Initialize

Signature:
```cpp
bool arctix::Memory::Allocator::Initialize(const arctix::Memory::ArctixConfig& config);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `arctix::Memory::ArctixConfig&` | `config` | Threading/statistics/alignment setup (see DefaultConfig). |

Returns: bool - true on success, false if already initialized.

Example call:
```cpp
arctix::Memory::Allocator a; a.Initialize(arctix::Memory::DefaultConfig());
```

Explanation: Brings the allocator to life. Call exactly once before Allocate(); a second call fails instead of leaking. Pair with Shutdown().

### arctix::Memory::Allocator::Shutdown

Signature:
```cpp
void arctix::Memory::Allocator::Shutdown();
```

Arguments: takes no arguments.

Returns: void

Example call:
```cpp
a.Shutdown();
```

Explanation: Releases internal state. Every outstanding block must be Deallocate()d first. After Shutdown the object may be Initialize()d again.

### arctix::Memory::Allocator::IsInitialized

Signature:
```cpp
bool arctix::Memory::Allocator::IsInitialized() const;
```

Arguments: takes no arguments.

Returns: bool - true between Initialize() and Shutdown().

Example call:
```cpp
if (!a.IsInitialized()) return;
```

Explanation: Lifecycle query. All other methods (except Initialize/Shutdown) require a prior successful Initialize().

### arctix::Memory::Allocator::Allocate

Signature:
```cpp
void* arctix::Memory::Allocator::Allocate(std::size_t size, std::size_t alignment = 0);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `size_t` | `size` | Bytes requested. 0 returns nullptr. |
| `size_t` | `alignment` | Power of two; 0 = config defaultAlignment. |

Returns: void* - aligned block, or nullptr (not initialized, size 0, OS failure).

Example call:
```cpp
void* p = a.Allocate(256, 16);
```

Explanation: General-purpose aligned allocation with statistics. Free every success with Deallocate() on the same instance.

### arctix::Memory::Allocator::Deallocate

Signature:
```cpp
void arctix::Memory::Allocator::Deallocate(void* pointer);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `void*` | `pointer` | Block from Allocate(). nullptr is a safe no-op. |

Returns: void

Example call:
```cpp
a.Deallocate(p);
```

Explanation: Returns a block. Only pass pointers this instance handed out; anything else is undefined behaviour.

### arctix::Memory::Allocator::GetStatistics

Signature:
```cpp
arctix::Memory::ArctixStats arctix::Memory::Allocator::GetStatistics() const;
```

Arguments: takes no arguments.

Returns: arctix::Memory::ArctixStats - snapshot copy (see arctix::Memory::ArctixStats struct).

Example call:
```cpp
auto s = a.GetStatistics(); printf("%zu
", s.currentUsageBytes);
```

Explanation: Point-in-time telemetry. Cheap; call it in debug overlays, leak checks and benchmarks.

### arctix::Memory::Allocator::ResetStatistics

Signature:
```cpp
void arctix::Memory::Allocator::ResetStatistics();
```

Arguments: takes no arguments.

Returns: void

Example call:
```cpp
a.ResetStatistics();
```

Explanation: Zeroes cumulative counters and re-bases the peak. Use between benchmark phases to measure each phase alone.

## Memory Pool (9)

### arctix::Memory::MemoryPool::Create

Signature:
```cpp
bool arctix::Memory::MemoryPool::Create(std::size_t blockSize, std::size_t blockCount, std::size_t alignment = 16);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `size_t` | `blockSize` | Bytes per block (> 0). |
| `size_t` | `blockCount` | Number of blocks (> 0). |
| `size_t` | `alignment` | Power of two; 0 = 16. |

Returns: bool - true on success, false on bad args / already created.

Example call:
```cpp
arctix::Memory::MemoryPool p; p.Create(64, 128);
```

Explanation: Pre-allocates every block up front: Allocate() afterwards is O(1) with zero OS calls. Ideal for entities, particles, hot nodes.

### arctix::Memory::MemoryPool::Destroy

Signature:
```cpp
void arctix::Memory::MemoryPool::Destroy();
```

Arguments: takes no arguments.

Returns: void

Example call:
```cpp
p.Destroy();
```

Explanation: Frees the backing memory. Free every checked-out block first; the pool may be Create()d again afterwards.

### arctix::Memory::MemoryPool::IsCreated

Signature:
```cpp
bool arctix::Memory::MemoryPool::IsCreated() const;
```

Arguments: takes no arguments.

Returns: bool - true between Create() and Destroy().

Example call:
```cpp
if (!p.IsCreated()) return;
```

Explanation: Lifecycle query for the pool.

### arctix::Memory::MemoryPool::Allocate

Signature:
```cpp
void* arctix::Memory::MemoryPool::Allocate();
```

Arguments: takes no arguments.

Returns: void* - one free block, or nullptr when exhausted.

Example call:
```cpp
void* e = p.Allocate();
```

Explanation: Pops a block in O(1). nullptr means the pool is drained - grow the pool or fall back to Allocator.

### arctix::Memory::MemoryPool::Free

Signature:
```cpp
void arctix::Memory::MemoryPool::Free(void* pointer);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `void*` | `pointer` | Block from Allocate(). nullptr is a safe no-op. |

Returns: void

Example call:
```cpp
p.Free(e);
```

Explanation: Returns a block to the pool in O(1). Never free foreign pointers into a pool.

### arctix::Memory::MemoryPool::BlockSize

Signature:
```cpp
std::size_t arctix::Memory::MemoryPool::BlockSize() const;
```

Arguments: takes no arguments.

Returns: size_t - bytes per block (valid after Create).

Example call:
```cpp
size_t b = p.BlockSize();
```

Explanation: Capacity introspection for sizing loops and diagnostics.

### arctix::Memory::MemoryPool::BlockCount

Signature:
```cpp
std::size_t arctix::Memory::MemoryPool::BlockCount() const;
```

Arguments: takes no arguments.

Returns: size_t - total blocks (valid after Create).

Example call:
```cpp
size_t n = p.BlockCount();
```

Explanation: Total capacity of the pool.

### arctix::Memory::MemoryPool::FreeCount

Signature:
```cpp
std::size_t arctix::Memory::MemoryPool::FreeCount() const;
```

Arguments: takes no arguments.

Returns: size_t - blocks currently available.

Example call:
```cpp
if (p.FreeCount() == 0) grow();
```

Explanation: Drain gauge: pair with UsedCount() for pool pressure metrics.

### arctix::Memory::MemoryPool::UsedCount

Signature:
```cpp
std::size_t arctix::Memory::MemoryPool::UsedCount() const;
```

Arguments: takes no arguments.

Returns: size_t - blocks currently checked out.

Example call:
```cpp
printf("in flight: %zu\n", p.UsedCount());
```

Explanation: Checked-out count; must be 0 before Destroy().

## Buffer (15)

### Buffer constructors

Signature:
```cpp
arctix::Buffer::Buffer(); explicit arctix::Buffer::Buffer(std::size_t size, std::uint8_t fill = 0); ~arctix::Buffer::Buffer(); // + move ctor / move assign (copy deleted)
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `size_t` | `size` | Initial size; bytes set to fill. |
| `uint8_t` | `fill` | Fill byte for the initial size. |

Returns: n/a - object construction.

Example call:
```cpp
arctix::Buffer::Buffer b(64, 0);
```

Explanation: Owning resizable byte array - the staging area for data before it goes to the Allocator, pool or pages. Move-only (no accidental copies); the destructor wipes contents.

### arctix::Buffer::Buffer::CopyFrom

Signature:
```cpp
bool arctix::Buffer::Buffer::CopyFrom(const void* data, std::size_t size);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `void*` | `data` | Bytes to copy. NULL valid only with size 0. |
| `size_t` | `size` | Byte count; replaces current contents. |

Returns: bool - false on bad args / out of memory (contents unchanged).

Example call:
```cpp
b.CopyFrom("hello", 5);
```

Explanation: Replaces the whole contents with a copy. Grows the allocation when needed, preserving nothing.

### arctix::Buffer::Buffer::Append

Signature:
```cpp
bool arctix::Buffer::Buffer::Append(const void* data, std::size_t size);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `void*` | `data` | Bytes to append (NULL only with size 0). |
| `size_t` | `size` | Byte count to add. |

Returns: bool - false on bad args / overflow / out of memory (unchanged).

Example call:
```cpp
b.Append(" world", 6);
```

Explanation: Grows end-to-end (doubling strategy from a 64-byte seed). Use to assemble packets, blobs and serialised structs.

### arctix::Buffer::Buffer::Resize

Signature:
```cpp
bool arctix::Buffer::Buffer::Resize(std::size_t newSize, std::uint8_t fill = 0);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `size_t` | `newSize` | Target size; old bytes preserved. |
| `uint8_t` | `fill` | Byte for grown region. |

Returns: bool - false on out of memory (unchanged).

Example call:
```cpp
b.Resize(1024, 0);
```

Explanation: Exact-size control. Shrinking wipes the truncated tail; growing zero-fills by default. Prefer Reserve() when you only want headroom.

### arctix::Buffer::Buffer::Reserve

Signature:
```cpp
bool arctix::Buffer::Buffer::Reserve(std::size_t capacity);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `size_t` | `capacity` | Minimum capacity; Size() is untouched. |

Returns: bool - false on out of memory (unchanged).

Example call:
```cpp
b.Reserve(4096);
```

Explanation: Pre-allocates headroom so a burst of Append() calls never reallocates. Capacity never shrinks except by move.

### arctix::Buffer::Buffer::Fill

Signature:
```cpp
void arctix::Buffer::Buffer::Fill(std::uint8_t value);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `uint8_t` | `value` | Byte written to every element. |

Returns: void

Example call:
```cpp
b.Fill(0xFF);
```

Explanation: Memset-style fill of the live Size() bytes. No-op when empty.

### arctix::Buffer::Buffer::Clear

Signature:
```cpp
void arctix::Buffer::Buffer::Clear();
```

Arguments: takes no arguments.

Returns: void

Example call:
```cpp
b.Clear();
```

Explanation: Wipes contents and sets Size() to 0 while keeping the allocation for reuse - the hot-loop friendly reset.

### arctix::Buffer::Buffer::SecureWipe

Signature:
```cpp
void arctix::Buffer::Buffer::SecureWipe();
```

Arguments: takes no arguments.

Returns: void

Example call:
```cpp
keys.SecureWipe();
```

Explanation: Volatile-pointer wipe (cannot be optimised away) then Clear(). Call before destroying buffers that held keys, passwords or tokens.

### arctix::Buffer::Buffer::Equals

Signature:
```cpp
bool arctix::Buffer::Buffer::Equals(const Buffer& other) const;
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `Buffer&` | `other` | Buffer to compare against. |

Returns: bool - true when sizes and all bytes match.

Example call:
```cpp
if (a.Equals(b)) { /* identical */ }
```

Explanation: Constant-shape comparison (size check first, then bytes). For MAC-style secrets prefer hashing; see Checksum().

### arctix::Buffer::Buffer::Checksum

Signature:
```cpp
std::uint64_t arctix::Buffer::Buffer::Checksum() const;
```

Arguments: takes no arguments.

Returns: uint64_t - FNV-1a hash; 0 for empty.

Example call:
```cpp
uint64_t h = b.Checksum();
```

Explanation: Fast non-cryptographic fingerprint for change detection, dedup keys and test assertions. Not a security hash.

### arctix::Buffer::Buffer::HexEncode

Signature:
```cpp
std::string arctix::Buffer::Buffer::HexEncode() const;
```

Arguments: takes no arguments.

Returns: std::string - uppercase hex, two chars per byte (empty for empty).

Example call:
```cpp
puts(b.HexEncode().c_str()); // "ABAB..."
```

Explanation: Debug rendering of binary contents for logs and DOC-style dumps.

### arctix::Buffer::Buffer::Data

Signature:
```cpp
std::uint8_t* arctix::Buffer::Buffer::Data(); const std::uint8_t* arctix::Buffer::Buffer::Data() const;
```

Arguments: takes no arguments.

Returns: uint8_t* - live bytes; nullptr when empty.

Example call:
```cpp
fwrite(b.Data(), 1, b.Size(), f);
```

Explanation: Raw access for I/O, hashing and hand-off to Allocator/pool/page writes. Invalidated by the next mutating call.

### arctix::Buffer::Buffer::Size

Signature:
```cpp
std::size_t arctix::Buffer::Buffer::Size() const;
```

Arguments: takes no arguments.

Returns: size_t - live byte count.

Example call:
```cpp
for (size_t i = 0; i < b.Size(); ++i) {}
```

Explanation: Logical length (not capacity).

### arctix::Buffer::Buffer::Capacity

Signature:
```cpp
std::size_t arctix::Buffer::Buffer::Capacity() const;
```

Arguments: takes no arguments.

Returns: size_t - allocated bytes (>= Size).

Example call:
```cpp
b.Reserve(b.Size() * 2);
```

Explanation: Physical headroom. Size <= Capacity always holds.

### arctix::Buffer::Buffer::Empty

Signature:
```cpp
bool arctix::Buffer::Buffer::Empty() const;
```

Arguments: takes no arguments.

Returns: bool - true when Size() == 0.

Example call:
```cpp
if (b.Empty()) return;
```

Explanation: Emptiness check; Data() is nullptr exactly when this is true.

## Memory Utils (3)

### SecureZero

Signature:
```cpp
void arctix::Memory::SecureZero(void* data, std::size_t size);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `void*` | `data` | Bytes to wipe. NULL with size 0 is safe. |
| `size_t` | `size` | Byte count. |

Returns: void

Example call:
```cpp
arctix::Memory::SecureZero(password, len);
```

Explanation: Optimiser-proof wipe via a volatile pointer. Header-only (no DLL call). Use on any secret material before free.

### MemEqual

Signature:
```cpp
bool arctix::Memory::MemEqual(const void* a, const void* b, std::size_t size);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `void*` | `a` | First range. |
| `void*` | `b` | Second range. |
| `size_t` | `size` | Bytes to compare (0 = equal). |

Returns: bool - true when all bytes match. NULL only equals NULL at size 0.

Example call:
```cpp
if (arctix::Memory::MemEqual(p, q, n)) {}
```

Explanation: Header-only byte comparison with NULL-safe semantics. Straightforward early-exit loop.

### Fnv1aHash64

Signature:
```cpp
std::uint64_t arctix::Memory::Fnv1aHash64(const void* data, std::size_t size);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `void*` | `data` | Bytes to hash. |
| `size_t` | `size` | Byte count (NULL/0 hashes to 0). |

Returns: uint64_t - 64-bit FNV-1a digest.

Example call:
```cpp
uint64_t h = arctix::Memory::Fnv1aHash64(p, n);
```

Explanation: Fast non-cryptographic hash backing arctix::Buffer::Buffer::Checksum(). Header-only; good for hash tables and change detection.

## Virtual Memory (8)

### VirtualPageSize

Signature:
```cpp
std::size_t arctix::Memory::VirtualPageSize();
```

Arguments: takes no arguments.

Returns: size_t - OS page size (e.g. 4096). Never 0.

Example call:
```cpp
size_t pg = arctix::Memory::VirtualPageSize();
```

Explanation: Quantum of the whole virtual-memory API: reserve/commit/decommit/release sizes should be multiples of this.

### IsPageAligned

Signature:
```cpp
bool arctix::Memory::IsPageAligned(const void* address);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `void*` | `address` | Address to test (NULL = false). |

Returns: bool - true when address is a page multiple.

Example call:
```cpp
assert(arctix::Memory::IsPageAligned(region));
```

Explanation: Alignment probe for addresses returned by arctix::Memory::VirtualReserve() and for validating caller ranges.

### RoundUpToPageSize

Signature:
```cpp
std::size_t arctix::Memory::RoundUpToPageSize(std::size_t size);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `size_t` | `size` | Byte count (0 returns 0). |

Returns: size_t - smallest page multiple >= size.

Example call:
```cpp
void* r = arctix::Memory::VirtualReserve(arctix::Memory::RoundUpToPageSize(n));
```

Explanation: Turns arbitrary byte counts into valid reservation sizes. Always round before Reserve().

### VirtualReserve

Signature:
```cpp
void* arctix::Memory::VirtualReserve(std::size_t size);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `size_t` | `size` | Address-space bytes (0 = nullptr). |

Returns: void* - reserved base, or nullptr on failure. No RAM is used yet.

Example call:
```cpp
void* r = arctix::Memory::VirtualReserve(pg * 2);
```

Explanation: Claims address space with PAGE_NOACCESS. Costs no physical memory until arctix::Memory::VirtualCommit(). Release exactly once with arctix::Memory::VirtualRelease().

### VirtualCommit

Signature:
```cpp
bool arctix::Memory::VirtualCommit(void* address, std::size_t size);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `void*` | `address` | Range inside one reservation. |
| `size_t` | `size` | Bytes to back with RAM. |

Returns: bool - false on bad args / OS refusal.

Example call:
```cpp
ok = arctix::Memory::VirtualCommit(r, pg * 2);
```

Explanation: Backs reserved pages with zero-filled RAM. Commit only what you touch: untouched pages stay free.

### VirtualDecommit

Signature:
```cpp
bool arctix::Memory::VirtualDecommit(void* address, std::size_t size);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `void*` | `address` | Committed range to release. |
| `size_t` | `size` | Byte count. |

Returns: bool - false on bad args / OS refusal.

Example call:
```cpp
arctix::Memory::VirtualDecommit(r, pg);
```

Explanation: Returns pages to reserved status; contents are lost but the reservation survives for a later arctix::Memory::VirtualCommit().

### VirtualRelease

Signature:
```cpp
bool arctix::Memory::VirtualRelease(void* address, std::size_t size);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `void*` | `address` | Exact pointer from arctix::Memory::VirtualReserve(). |
| `size_t` | `size` | Exact reserved size (required on POSIX). |

Returns: bool - false on bad args / OS refusal. Call exactly once.

Example call:
```cpp
arctix::Memory::VirtualRelease(r, pg * 2);
```

Explanation: Frees the whole reservation. Partial release is not supported - design reservations as release units.

### VirtualProtectMemory

Signature:
```cpp
bool arctix::Memory::VirtualProtectMemory(void* address, std::size_t size, arctix::Memory::MemoryProtection protection);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `void*` | `address` | Committed range. |
| `size_t` | `size` | Byte count. |
| `arctix::Memory::MemoryProtection` | `protection` | NoAccess / ReadOnly / ReadWrite / ReadExecute / ReadWriteExecute. |

Returns: bool - false on bad args / OS refusal (old rights kept).

Example call:
```cpp
arctix::Memory::VirtualProtectMemory(r, pg, arctix::Memory::MemoryProtection::ReadOnly);
```

Explanation: Least-privilege page rights (e.g. seal a JIT page read-only after writing). Executable pages may be refused under hardened policies.

## Thread (12)

### Thread constructors

Signature:
```cpp
arctix::Thread::Thread(); ~arctix::Thread::Thread(); arctix::Thread::Thread(Thread&&) noexcept; Thread& operator=(Thread&&) noexcept; // copy deleted
```

Arguments: takes no arguments.

Returns: n/a - object lifecycle.

Example call:
```cpp
arctix::Thread::Thread t;
```

Explanation: Move-only owner of at most one OS thread. The destructor joins a still-joinable thread (blocking safety net) - prefer explicit Join()/Detach().

### arctix::Thread::Thread::Start

Signature:
```cpp
bool arctix::Thread::Thread::Start(arctix::Thread::ThreadFunction function, void* userData);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `arctix::Thread::ThreadFunction` | `function` | void(*)(void*) worker. nullptr fails. |
| `void*` | `userData` | Opaque pointer for the worker (may be NULL). |

Returns: bool - false when function is null or a thread is already running.

Example call:
```cpp
t.Start(Worker, &counter);
```

Explanation: Launches the worker exactly once per object. Ownership of userData stays with the caller; the library never frees it.

### arctix::Thread::Thread::Join

Signature:
```cpp
bool arctix::Thread::Thread::Join();
```

Arguments: takes no arguments.

Returns: bool - false when not joinable (never started / already joined / moved).

Example call:
```cpp
t.Join();
```

Explanation: Waits for completion. After success the object is empty again. Join OR Detach exactly once.

### arctix::Thread::Thread::Detach

Signature:
```cpp
bool arctix::Thread::Thread::Detach();
```

Arguments: takes no arguments.

Returns: bool - false when not joinable.

Example call:
```cpp
t.Detach(); // fire and forget
```

Explanation: Releases ownership; the thread keeps running. Use for background work whose lifetime you manage another way.

### arctix::Thread::Thread::IsJoinable

Signature:
```cpp
bool arctix::Thread::Thread::IsJoinable() const;
```

Arguments: takes no arguments.

Returns: bool - true when Join()/Detach() may be called.

Example call:
```cpp
if (t.IsJoinable()) t.Join();
```

Explanation: Guard before Join/Detach and the canonical "did I start something?" probe.

### arctix::Thread::Thread::IsRunning

Signature:
```cpp
bool arctix::Thread::Thread::IsRunning() const;
```

Arguments: takes no arguments.

Returns: bool - true while the worker executes.

Example call:
```cpp
while (t.IsRunning()) { /* spin */ }
```

Explanation: Liveness flag, flipped at worker entry/exit. For waiting prefer Join(); poll this for progress UIs.

### arctix::Thread::Thread::HasThrown

Signature:
```cpp
bool arctix::Thread::Thread::HasThrown() const;
```

Arguments: takes no arguments.

Returns: bool - true when the finished worker threw.

Example call:
```cpp
t.Join(); if (t.HasThrown()) log();
```

Explanation: Worker exceptions are caught at the thread boundary and recorded here. Meaningful only after Join() on a started thread.

### arctix::Thread::Thread::GetId

Signature:
```cpp
std::thread::id arctix::Thread::Thread::GetId() const;
```

Arguments: takes no arguments.

Returns: thread::id - owned thread id, or default when none.

Example call:
```cpp
auto id = t.GetId();
```

Explanation: Identity of the owned thread for logging and thread-affinity checks.

### CurrentThreadId

Signature:
```cpp
std::thread::id arctix::Thread::CurrentThreadId();
```

Arguments: takes no arguments.

Returns: thread::id - the calling thread.

Example call:
```cpp
auto me = arctix::Thread::CurrentThreadId();
```

Explanation: Free function: who am I. Compare with GetId() to assert "runs on worker X".

### ThreadYield

Signature:
```cpp
void arctix::Thread::ThreadYield();
```

Arguments: takes no arguments.

Returns: void

Example call:
```cpp
arctix::Thread::ThreadYield();
```

Explanation: Hints the OS to reschedule so another thread may run. For polite spin-waits, not for synchronisation.

### ThreadSleepFor

Signature:
```cpp
void arctix::Thread::ThreadSleepFor(std::uint64_t milliseconds);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `uint64_t` | `milliseconds` | Minimum sleep time. |

Returns: void

Example call:
```cpp
arctix::Thread::ThreadSleepFor(10);
```

Explanation: Sleeps the caller. Actual wake-up may be later (OS granularity); never use for precise timing.

### HardwareConcurrency

Signature:
```cpp
unsigned int arctix::Thread::HardwareConcurrency();
```

Arguments: takes no arguments.

Returns: unsigned int - thread count, 0 = unknown.

Example call:
```cpp
unsigned n = arctix::Thread::HardwareConcurrency();
```

Explanation: Sizes pools and ParallelFor chunking. 0 means "ask the OS failed" - treat as 1.

## Thread Pool (10)

### ThreadPool constructor

Signature:
```cpp
explicit ThreadPool(std::size_t threadCount = 0); ~ThreadPool(); // copy/move deleted
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `size_t` | `threadCount` | Workers; 0 = hardware threads (min 1). |

Returns: n/a - check IsRunning() to confirm workers started.

Example call:
```cpp
arctix::Thread::ThreadPool pool(4);
```

Explanation: Fixed FIFO pool: workers start in the ctor, drain the queue, and are joined by Shutdown()/the destructor. Never starts zero workers.

### arctix::Thread::ThreadPool::IsRunning

Signature:
```cpp
bool arctix::Thread::ThreadPool::IsRunning() const;
```

Arguments: takes no arguments.

Returns: bool - true between construction and Shutdown().

Example call:
```cpp
if (!pool.IsRunning()) return;
```

Explanation: Readiness probe. Submit() refuses work once this is false.

### arctix::Thread::ThreadPool::ThreadCount

Signature:
```cpp
std::size_t arctix::Thread::ThreadPool::ThreadCount() const;
```

Arguments: takes no arguments.

Returns: size_t - worker count (0 after Shutdown).

Example call:
```cpp
printf("%zu workers\n", pool.ThreadCount());
```

Explanation: Pool width for chunking math and diagnostics.

### arctix::Thread::ThreadPool::Submit

Signature:
```cpp
bool arctix::Thread::ThreadPool::Submit(arctix::Thread::ThreadFunction function, void* userData);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `arctix::Thread::ThreadFunction` | `function` | void(*)(void*) task. nullptr fails. |
| `void*` | `userData` | Task argument (may be NULL). |

Returns: bool - false when null / shut down / queue alloc failed.

Example call:
```cpp
pool.Submit(Work, &job);
```

Explanation: Enqueues one task; a worker picks it up FIFO. Same worker signature as Thread, so workers are reusable.

### arctix::Thread::ThreadPool::WaitIdle

Signature:
```cpp
bool arctix::Thread::ThreadPool::WaitIdle(std::uint64_t timeoutMs = 0);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `uint64_t` | `timeoutMs` | Max wait; 0 = forever. |

Returns: bool - true when queue empty AND no worker busy; false on timeout.

Example call:
```cpp
pool.WaitIdle();
```

Explanation: Barrier: returns only when every submitted task finished. The standard "submit N, wait, read results" pattern.

### arctix::Thread::ThreadPool::PendingTasks

Signature:
```cpp
std::size_t arctix::Thread::ThreadPool::PendingTasks() const;
```

Arguments: takes no arguments.

Returns: size_t - queued (not yet running) tasks.

Example call:
```cpp
if (pool.PendingTasks() > 1000) throttle();
```

Explanation: Back-pressure gauge: throttle producers when the queue grows.

### arctix::Thread::ThreadPool::RunningTasks

Signature:
```cpp
std::size_t arctix::Thread::ThreadPool::RunningTasks() const;
```

Arguments: takes no arguments.

Returns: size_t - tasks executing right now.

Example call:
```cpp
printf("busy: %zu\n", pool.RunningTasks());
```

Explanation: Live occupancy; Pending + Running = all unfinished work.

### arctix::Thread::ThreadPool::FailedTasks

Signature:
```cpp
unsigned long long arctix::Thread::ThreadPool::FailedTasks() const;
```

Arguments: takes no arguments.

Returns: uint64_t - tasks whose worker threw (only grows).

Example call:
```cpp
if (pool.FailedTasks()) alert();
```

Explanation: Per-task exceptions are caught at the worker boundary and counted here; they never escape and never stop the pool.

### arctix::Thread::ThreadPool::Shutdown

Signature:
```cpp
void arctix::Thread::ThreadPool::Shutdown();
```

Arguments: takes no arguments.

Returns: void (idempotent).

Example call:
```cpp
pool.Shutdown();
```

Explanation: Stops accepting, drains queued tasks, joins every worker. Safe to call twice and after WaitIdle(). The destructor does this automatically.

### ParallelFor

Signature:
```cpp
bool arctix::Thread::ParallelFor(std::size_t begin, std::size_t end, void(*)(std::size_t index, void*) function, void* userData, std::size_t threadCount = 0);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `size_t` | `begin` | First index (inclusive). |
| `size_t` | `end` | One past last (exclusive; end < begin fails). |
| `function` | `function` | fn(index, userData); each index visited exactly once. |
| `void*` | `userData` | Shared argument (synchronise interior writes). |
| `size_t` | `threadCount` | Workers; 0 = hardware threads. |

Returns: bool - false on bad args; worker throws are counted, not propagated.

Example call:
```cpp
arctix::Thread::ParallelFor(0, n, Square, out, 4);
```

Explanation: Data-parallel loop over a private pool that is torn down on return. Empty ranges succeed trivially; exceptions in one index never stop the others.

## C ABI - Memory (27)

### Arctix_CreateAllocator

Signature:
```cpp
ArctixAllocatorHandle* Arctix_CreateAllocator(const ArctixConfigC* config);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `ArctixConfigC*` | `config` | 0/1 threadSafe + enableStatistics, defaultAlignment. NULL = defaults. |

Returns: Handle*, or NULL on failure. Free with Arctix_DestroyAllocator().

Example call:
```cpp
ArctixAllocatorHandle* a = Arctix_CreateAllocator(0);
```

Explanation: C entry point for the C++ Allocator (see arctix::Memory::Allocator::Initialize). NULL config is the common fast path.

### Arctix_DestroyAllocator

Signature:
```cpp
void Arctix_DestroyAllocator(ArctixAllocatorHandle* handle);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `handle` | `handle` | From Arctix_CreateAllocator. NULL = no-op. |

Returns: void

Example call:
```cpp
Arctix_DestroyAllocator(a);
```

Explanation: Shuts down and frees. Free outstanding blocks first, mirroring Shutdown().

### Arctix_Allocate

Signature:
```cpp
void* Arctix_Allocate(ArctixAllocatorHandle* handle, size_t size, size_t alignment);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `handle` | `handle` | Allocator handle (NULL = NULL). |
| `size_t` | `size` | Bytes (0 = NULL). |
| `size_t` | `alignment` | Power of two; 0 = default. |

Returns: void* - block or NULL.

Example call:
```cpp
void* p = Arctix_Allocate(a, 256, 16);
```

Explanation: C mirror of arctix::Memory::Allocator::Allocate.

### Arctix_Free

Signature:
```cpp
void Arctix_Free(ArctixAllocatorHandle* handle, void* pointer);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `handle` | `handle` | Allocator handle. |
| `void*` | `pointer` | Block from Arctix_Allocate (NULL = no-op). |

Returns: void

Example call:
```cpp
Arctix_Free(a, p);
```

Explanation: C mirror of arctix::Memory::Allocator::Deallocate. NULL-safe on both arguments.

### Arctix_GetStats

Signature:
```cpp
void Arctix_GetStats(ArctixAllocatorHandle* handle, ArctixStatsC* outStats);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `handle` | `handle` | Allocator handle. |
| `ArctixStatsC*` | `outStats` | Receives the snapshot (NULL = no-op). |

Returns: void

Example call:
```cpp
ArctixStatsC s; Arctix_GetStats(a, &s);
```

Explanation: C mirror of GetStatistics() into the C-compatible ArctixStatsC struct.

### Arctix_PoolCreate

Signature:
```cpp
ArctixPoolHandle* Arctix_PoolCreate(size_t blockSize, size_t blockCount, size_t alignment);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `size_t` | `blockSize` | Bytes per block. |
| `size_t` | `blockCount` | Block count. |
| `size_t` | `alignment` | 0 = 16. |

Returns: Handle*, or NULL on bad args / failure.

Example call:
```cpp
ArctixPoolHandle* p = Arctix_PoolCreate(64, 128, 16);
```

Explanation: C entry point for arctix::Memory::MemoryPool::Create.

### Arctix_PoolDestroy

Signature:
```cpp
void Arctix_PoolDestroy(ArctixPoolHandle* handle);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `handle` | `handle` | From Arctix_PoolCreate. NULL = no-op. |

Returns: void

Example call:
```cpp
Arctix_PoolDestroy(p);
```

Explanation: C mirror of arctix::Memory::MemoryPool::Destroy plus handle free.

### Arctix_PoolAllocate

Signature:
```cpp
void* Arctix_PoolAllocate(ArctixPoolHandle* handle);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `handle` | `handle` | Pool handle (NULL = NULL). |

Returns: void* - block, or NULL when exhausted.

Example call:
```cpp
void* e = Arctix_PoolAllocate(p);
```

Explanation: C mirror of arctix::Memory::MemoryPool::Allocate.

### Arctix_PoolFree

Signature:
```cpp
void Arctix_PoolFree(ArctixPoolHandle* handle, void* pointer);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `handle` | `handle` | Pool handle. |
| `void*` | `pointer` | Block from Arctix_PoolAllocate. |

Returns: void

Example call:
```cpp
Arctix_PoolFree(p, e);
```

Explanation: C mirror of arctix::Memory::MemoryPool::Free. NULL-safe.

### Arctix_GetVersionString

Signature:
```cpp
const char* Arctix_GetVersionString(void);
```

Arguments: takes no arguments.

Returns: const char* - e.g. "0.1.1". Never NULL.

Example call:
```cpp
printf("%s\n", Arctix_GetVersionString());
```

Explanation: Same value as ArctixC_GetVersionString, reachable with only memory.h included.

### Arctix_BufferCreate

Signature:
```cpp
ArctixBufferHandle* Arctix_BufferCreate(size_t size);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `size_t` | `size` | Initial zeroed size (0 = empty). |

Returns: Handle*, or NULL on failure. Free with Arctix_BufferDestroy.

Example call:
```cpp
ArctixBufferHandle* b = Arctix_BufferCreate(64);
```

Explanation: C entry point for Buffer.

### Arctix_BufferDestroy

Signature:
```cpp
void Arctix_BufferDestroy(ArctixBufferHandle* handle);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `handle` | `handle` | From Arctix_BufferCreate. Wipes; NULL = no-op. |

Returns: void

Example call:
```cpp
Arctix_BufferDestroy(b);
```

Explanation: Wipes and frees the buffer and its handle.

### Arctix_BufferCopyFrom

Signature:
```cpp
int Arctix_BufferCopyFrom(ArctixBufferHandle* handle, const void* data, size_t size);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `handle` | `handle` | Buffer handle. |
| `void*` | `data` | Source bytes. |
| `size_t` | `size` | Count (replaces contents). |

Returns: int - 1 success, 0 failure.

Example call:
```cpp
Arctix_BufferCopyFrom(b, "hi", 2);
```

Explanation: C mirror of arctix::Buffer::Buffer::CopyFrom.

### Arctix_BufferAppend

Signature:
```cpp
int Arctix_BufferAppend(ArctixBufferHandle* handle, const void* data, size_t size);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `handle` | `handle` | Buffer handle. |
| `void*` | `data` | Bytes to add. |
| `size_t` | `size` | Count. |

Returns: int - 1 success, 0 failure.

Example call:
```cpp
Arctix_BufferAppend(b, "!", 1);
```

Explanation: C mirror of arctix::Buffer::Buffer::Append.

### Arctix_BufferResize

Signature:
```cpp
int Arctix_BufferResize(ArctixBufferHandle* handle, size_t newSize);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `handle` | `handle` | Buffer handle. |
| `size_t` | `newSize` | Target size (grown bytes zeroed). |

Returns: int - 1 success, 0 failure.

Example call:
```cpp
Arctix_BufferResize(b, 256);
```

Explanation: C mirror of arctix::Buffer::Buffer::Resize.

### Arctix_BufferFill

Signature:
```cpp
void Arctix_BufferFill(ArctixBufferHandle* handle, unsigned int value);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `handle` | `handle` | Buffer handle. |
| `unsigned int` | `value` | Low byte written everywhere. |

Returns: void

Example call:
```cpp
Arctix_BufferFill(b, 0xFF);
```

Explanation: C mirror of arctix::Buffer::Buffer::Fill.

### Arctix_BufferClear

Signature:
```cpp
void Arctix_BufferClear(ArctixBufferHandle* handle);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `handle` | `handle` | Buffer handle. |

Returns: void

Example call:
```cpp
Arctix_BufferClear(b);
```

Explanation: C mirror of arctix::Buffer::Buffer::Clear (wipe + empty, keeps allocation).

### Arctix_BufferEquals

Signature:
```cpp
int Arctix_BufferEquals(const ArctixBufferHandle* a, const ArctixBufferHandle* b);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `handle` | `a` | First buffer. |
| `handle` | `b` | Second buffer. |

Returns: int - 1 when identical, else 0.

Example call:
```cpp
if (Arctix_BufferEquals(x, y)) {}
```

Explanation: C mirror of arctix::Buffer::Buffer::Equals.

### Arctix_BufferChecksum

Signature:
```cpp
unsigned long long Arctix_BufferChecksum(const ArctixBufferHandle* handle);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `handle` | `handle` | Buffer handle (NULL = 0). |

Returns: uint64_t - FNV-1a digest.

Example call:
```cpp
unsigned long long h = Arctix_BufferChecksum(b);
```

Explanation: C mirror of arctix::Buffer::Buffer::Checksum.

### Arctix_BufferData

Signature:
```cpp
unsigned char* Arctix_BufferData(ArctixBufferHandle* handle);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `handle` | `handle` | Buffer handle (NULL = NULL). |

Returns: unsigned char* - bytes, NULL when empty.

Example call:
```cpp
fwrite(Arctix_BufferData(b), 1, Arctix_BufferSize(b), f);
```

Explanation: C mirror of arctix::Buffer::Buffer::Data.

### Arctix_BufferSize

Signature:
```cpp
size_t Arctix_BufferSize(const ArctixBufferHandle* handle);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `handle` | `handle` | Buffer handle (NULL = 0). |

Returns: size_t - byte count.

Example call:
```cpp
size_t n = Arctix_BufferSize(b);
```

Explanation: C mirror of arctix::Buffer::Buffer::Size.

### ArctixC_VirtualPageSize

Signature:
```cpp
size_t ArctixC_VirtualPageSize(void);
```

Arguments: takes no arguments.

Returns: size_t - OS page size. Never 0.

Example call:
```cpp
size_t pg = ArctixC_VirtualPageSize();
```

Explanation: C mirror of arctix::Memory::VirtualPageSize().

### ArctixC_VirtualReserve

Signature:
```cpp
void* ArctixC_VirtualReserve(size_t size);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `size_t` | `size` | Address-space bytes. |

Returns: void* - base or NULL.

Example call:
```cpp
void* r = ArctixC_VirtualReserve(pg * 2);
```

Explanation: C mirror of arctix::Memory::VirtualReserve().

### ArctixC_VirtualCommit

Signature:
```cpp
int ArctixC_VirtualCommit(void* address, size_t size);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `void*` | `address` | Range in a reservation. |
| `size_t` | `size` | Bytes. |

Returns: int - 1 success, 0 failure.

Example call:
```cpp
ArctixC_VirtualCommit(r, pg * 2);
```

Explanation: C mirror of arctix::Memory::VirtualCommit().

### ArctixC_VirtualDecommit

Signature:
```cpp
int ArctixC_VirtualDecommit(void* address, size_t size);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `void*` | `address` | Committed range. |
| `size_t` | `size` | Bytes. |

Returns: int - 1 success, 0 failure.

Example call:
```cpp
ArctixC_VirtualDecommit(r, pg);
```

Explanation: C mirror of arctix::Memory::VirtualDecommit().

### ArctixC_VirtualRelease

Signature:
```cpp
int ArctixC_VirtualRelease(void* address, size_t size);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `void*` | `address` | Exact reserve pointer. |
| `size_t` | `size` | Exact reserve size. |

Returns: int - 1 success. Call exactly once.

Example call:
```cpp
ArctixC_VirtualRelease(r, pg * 2);
```

Explanation: C mirror of arctix::Memory::VirtualRelease().

### ArctixC_VirtualProtect

Signature:
```cpp
int ArctixC_VirtualProtect(void* address, size_t size, int protection);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `void*` | `address` | Committed range. |
| `size_t` | `size` | Bytes. |
| `int` | `protection` | 0 = none, 1 = read, 2 = read-write, 3 = read-exec, 4 = all. |

Returns: int - 1 success, 0 failure (out-of-range codes fail).

Example call:
```cpp
ArctixC_VirtualProtect(r, pg, 1);
```

Explanation: C mirror of arctix::Memory::VirtualProtectMemory().

## C ABI - Thread (20)

### ArctixC_ThreadCreate

Signature:
```cpp
ArctixC_ThreadHandle* ArctixC_ThreadCreate(void);
```

Arguments: takes no arguments.

Returns: Handle* (empty, non-running), or NULL. Free with ArctixC_ThreadDestroy().

Example call:
```cpp
ArctixC_ThreadHandle* t = ArctixC_ThreadCreate();
```

Explanation: C entry point for arctix::Thread::Thread. One Start per handle, then exactly one Join/Detach.

### ArctixC_ThreadDestroy

Signature:
```cpp
void ArctixC_ThreadDestroy(ArctixC_ThreadHandle* handle);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `handle` | `handle` | From ArctixC_ThreadCreate. Joins when still joinable; NULL = no-op. |

Returns: void

Example call:
```cpp
ArctixC_ThreadDestroy(t);
```

Explanation: Mirrors the C++ destructor (join-if-joinable safety net). Prefer explicit Join first.

### ArctixC_ThreadStart

Signature:
```cpp
int ArctixC_ThreadStart(ArctixC_ThreadHandle* handle, ArctixC_ThreadFunction function, void* userData);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `handle` | `handle` | Thread handle. |
| `function` | `function` | void(*)(void*) worker (NULL fails). |
| `void*` | `userData` | Worker argument (never freed by the lib). |

Returns: int - 1 success, 0 failure.

Example call:
```cpp
ArctixC_ThreadStart(t, Worker, &n);
```

Explanation: C mirror of arctix::Thread::Thread::Start.

### ArctixC_ThreadJoin

Signature:
```cpp
int ArctixC_ThreadJoin(ArctixC_ThreadHandle* handle);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `handle` | `handle` | Thread handle. |

Returns: int - 1 success, 0 when not joinable.

Example call:
```cpp
ArctixC_ThreadJoin(t);
```

Explanation: C mirror of arctix::Thread::Thread::Join.

### ArctixC_ThreadDetach

Signature:
```cpp
int ArctixC_ThreadDetach(ArctixC_ThreadHandle* handle);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `handle` | `handle` | Thread handle. |

Returns: int - 1 success, 0 when not joinable.

Example call:
```cpp
ArctixC_ThreadDetach(t);
```

Explanation: C mirror of arctix::Thread::Thread::Detach.

### ArctixC_ThreadIsJoinable

Signature:
```cpp
int ArctixC_ThreadIsJoinable(const ArctixC_ThreadHandle* handle);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `handle` | `handle` | Thread handle (NULL = 0). |

Returns: int - 1 when Join/Detach may be called.

Example call:
```cpp
if (ArctixC_ThreadIsJoinable(t)) {}
```

Explanation: C mirror of arctix::Thread::Thread::IsJoinable.

### ArctixC_ThreadIsRunning

Signature:
```cpp
int ArctixC_ThreadIsRunning(const ArctixC_ThreadHandle* handle);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `handle` | `handle` | Thread handle (NULL = 0). |

Returns: int - 1 while the worker executes.

Example call:
```cpp
while (ArctixC_ThreadIsRunning(t)) {}
```

Explanation: C mirror of arctix::Thread::Thread::IsRunning.

### ArctixC_ThreadHasThrown

Signature:
```cpp
int ArctixC_ThreadHasThrown(const ArctixC_ThreadHandle* handle);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `handle` | `handle` | Thread handle (NULL = 0). |

Returns: int - 1 when the finished worker threw.

Example call:
```cpp
ArctixC_ThreadJoin(t); if (ArctixC_ThreadHasThrown(t)) {}
```

Explanation: C mirror of arctix::Thread::Thread::HasThrown. Exceptions never cross the C boundary.

### ArctixC_ThreadGetId

Signature:
```cpp
unsigned long long ArctixC_ThreadGetId(const ArctixC_ThreadHandle* handle);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `handle` | `handle` | Thread handle (NULL = 0). |

Returns: uint64_t - hash of the owned id, 0 when none.

Example call:
```cpp
unsigned long long id = ArctixC_ThreadGetId(t);
```

Explanation: C mirror of arctix::Thread::Thread::GetId (hashed to a plain integer).

### ArctixC_ThreadCurrentId

Signature:
```cpp
unsigned long long ArctixC_ThreadCurrentId(void);
```

Arguments: takes no arguments.

Returns: uint64_t - hash of the calling thread id.

Example call:
```cpp
printf("%llu\n", ArctixC_ThreadCurrentId());
```

Explanation: C mirror of CurrentThreadId (hashed).

### ArctixC_ThreadYield

Signature:
```cpp
void ArctixC_ThreadYield(void);
```

Arguments: takes no arguments.

Returns: void

Example call:
```cpp
ArctixC_ThreadYield();
```

Explanation: C mirror of ThreadYield.

### ArctixC_ThreadSleepMs

Signature:
```cpp
void ArctixC_ThreadSleepMs(unsigned long long milliseconds);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `uint64_t` | `milliseconds` | Minimum sleep time. |

Returns: void

Example call:
```cpp
ArctixC_ThreadSleepMs(10);
```

Explanation: C mirror of ThreadSleepFor.

### ArctixC_ThreadHardwareConcurrency

Signature:
```cpp
unsigned int ArctixC_ThreadHardwareConcurrency(void);
```

Arguments: takes no arguments.

Returns: unsigned int - thread count, 0 = unknown.

Example call:
```cpp
unsigned n = ArctixC_ThreadHardwareConcurrency();
```

Explanation: C mirror of HardwareConcurrency.

### ArctixC_PoolCreate

Signature:
```cpp
ArctixC_ThreadPoolHandle* ArctixC_PoolCreate(unsigned long long threadCount);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `uint64_t` | `threadCount` | Workers; 0 = hardware threads (min 1). |

Returns: Handle*, or NULL on failure. Free with ArctixC_PoolDestroy().

Example call:
```cpp
ArctixC_ThreadPoolHandle* p = ArctixC_PoolCreate(4);
```

Explanation: C entry point for ThreadPool with an explicit width.

### ArctixC_PoolDestroy

Signature:
```cpp
void ArctixC_PoolDestroy(ArctixC_ThreadPoolHandle* handle);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `handle` | `handle` | From ArctixC_PoolCreate. Drains + joins; NULL = no-op. |

Returns: void

Example call:
```cpp
ArctixC_PoolDestroy(p);
```

Explanation: C mirror of Shutdown plus handle free.

### ArctixC_PoolSubmit

Signature:
```cpp
int ArctixC_PoolSubmit(ArctixC_ThreadPoolHandle* handle, ArctixC_ThreadFunction function, void* userData);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `handle` | `handle` | Pool handle. |
| `function` | `function` | void(*)(void*) task. |
| `void*` | `userData` | Task argument. |

Returns: int - 1 success, 0 failure.

Example call:
```cpp
ArctixC_PoolSubmit(p, Work, &job);
```

Explanation: C mirror of arctix::Thread::ThreadPool::Submit.

### ArctixC_PoolWaitIdle

Signature:
```cpp
int ArctixC_PoolWaitIdle(ArctixC_ThreadPoolHandle* handle, unsigned long long timeoutMs);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `handle` | `handle` | Pool handle. |
| `uint64_t` | `timeoutMs` | Max wait; 0 = forever. |

Returns: int - 1 when idle, 0 on timeout.

Example call:
```cpp
ArctixC_PoolWaitIdle(p, 0);
```

Explanation: C mirror of arctix::Thread::ThreadPool::WaitIdle.

### ArctixC_PoolPendingTasks

Signature:
```cpp
unsigned long long ArctixC_PoolPendingTasks(const ArctixC_ThreadPoolHandle* handle);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `handle` | `handle` | Pool handle (NULL = 0). |

Returns: uint64_t - queued tasks.

Example call:
```cpp
unsigned long long q = ArctixC_PoolPendingTasks(p);
```

Explanation: C mirror of arctix::Thread::ThreadPool::PendingTasks.

### ArctixC_PoolThreadCount

Signature:
```cpp
unsigned long long ArctixC_PoolThreadCount(const ArctixC_ThreadPoolHandle* handle);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `handle` | `handle` | Pool handle (NULL = 0). |

Returns: uint64_t - worker count.

Example call:
```cpp
printf("%llu\n", ArctixC_PoolThreadCount(p));
```

Explanation: C mirror of arctix::Thread::ThreadPool::ThreadCount.

### ArctixC_PoolFailedTasks

Signature:
```cpp
unsigned long long ArctixC_PoolFailedTasks(const ArctixC_ThreadPoolHandle* handle);
```

Arguments:

| Type | Name | Description |
|------|------|-------------|
| `handle` | `handle` | Pool handle (NULL = 0). |

Returns: uint64_t - tasks whose worker threw.

Example call:
```cpp
if (ArctixC_PoolFailedTasks(p)) {}
```

Explanation: C mirror of arctix::Thread::ThreadPool::FailedTasks.
