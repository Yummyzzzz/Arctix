// ArctixCLI - built-in arctix commands. Every command runs in-process on
// top of the ArctixC SDK (arctix::Memory / Buffer / Thread / Core).
// Nothing shells out: no PowerShell, no PATH dependency.

#include "commands.hpp"

#include "console.hpp"
#include "shell.hpp"

#include <arctixc/core/version.hpp>
#include <arctixc/memory/allocator.hpp>
#include <arctixc/memory/buffer.hpp>
#include <arctixc/memory/mem_utils.hpp>
#include <arctixc/memory/memory_pool.hpp>
#include <arctixc/thread/thread_pool.hpp>

#include <atomic>
#include <chrono>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

namespace arctixcli
{
namespace
{

// --- Categories, most useful first ---

const char* kCatBench = "Benchmarks";
const char* kCatDiag = "Diagnostics";
const char* kCatMem = "Memory";
const char* kCatHash = "Hashing";
const char* kCatThread = "Threading";
const char* kCatProject = "Project";
const char* kCatShell = "Shell";

struct CategoryInfo
{
    const char* name;
    const char* blurb;
};

const CategoryInfo kCategoryOrder[] = {
    {kCatBench, "measure the SDK (allocators, pools, hashes, scaling)."},
    {kCatDiag, "prove the SDK is healthy (leaks, stress, machine info)."},
    {kCatMem, "allocate, pool, inspect and wipe memory."},
    {kCatHash, "FNV-1a checksums for text and files."},
    {kCatThread, "pools, scaling sweeps and parallel loops."},
    {kCatProject, "scaffold apps on ArctixC."},
    {kCatShell, "the shell itself."},
};

// --- Small helpers ---

bool EqualsIgnoreCase(const std::string& a, const char* b)
{
    std::size_t n = std::strlen(b);
    if (a.size() != n)
    {
        return false;
    }
    for (std::size_t i = 0; i < n; ++i)
    {
        char ca = a[i];
        char cb = b[i];
        if (ca >= 'A' && ca <= 'Z')
        {
            ca = static_cast<char>(ca + ('a' - 'A'));
        }
        if (cb >= 'A' && cb <= 'Z')
        {
            cb = static_cast<char>(cb + ('a' - 'A'));
        }
        if (ca != cb)
        {
            return false;
        }
    }
    return true;
}

bool ParseSize(const std::string& text, std::size_t& out)
{
    if (text.empty())
    {
        return false;
    }
    // Optional k/m/g suffix: 64k = 65536, 4m, 1g.
    std::size_t mult = 1;
    std::string num = text;
    char last = num.back();
    if (last == 'k' || last == 'K')
    {
        mult = 1024;
        num.pop_back();
    }
    else if (last == 'm' || last == 'M')
    {
        mult = 1024 * 1024;
        num.pop_back();
    }
    else if (last == 'g' || last == 'G')
    {
        mult = 1024 * 1024 * 1024;
        num.pop_back();
    }
    if (num.empty())
    {
        return false;
    }
    unsigned long long value = 0;
    for (char c : num)
    {
        if (c < '0' || c > '9')
        {
            return false;
        }
        value = value * 10 + static_cast<unsigned long long>(c - '0');
        if (value > 1024ULL * 1024 * 1024 * 1024)
        {
            return false;
        }
    }
    out = static_cast<std::size_t>(value * mult);
    return out > 0;
}

// Reads "--key value" / "--key=value" style options. Returns false on
// unknown keys or missing values.
bool ParseOptions(const std::vector<std::string>& argv, std::size_t start,
                  const char* const* keys, std::string* values, std::size_t keyCount,
                  std::vector<std::string>& positionals, std::string& error)
{
    for (std::size_t i = start; i < argv.size(); ++i)
    {
        const std::string& tok = argv[i];
        if (tok.size() > 2 && tok[0] == '-' && tok[1] == '-')
        {
            std::string key = tok.substr(2);
            std::string value;
            std::size_t eq = key.find('=');
            if (eq != std::string::npos)
            {
                value = key.substr(eq + 1);
                key = key.substr(0, eq);
            }
            else if (i + 1 < argv.size())
            {
                value = argv[++i];
            }
            else
            {
                error = "option '--" + key + "' needs a value.";
                return false;
            }
            bool known = false;
            for (std::size_t k = 0; k < keyCount; ++k)
            {
                if (key == keys[k])
                {
                    values[k] = value;
                    known = true;
                    break;
                }
            }
            if (!known)
            {
                error = "unknown option '--" + key + "'.";
                return false;
            }
        }
        else
        {
            positionals.push_back(tok);
        }
    }
    return true;
}

void PrintError(const std::string& message)
{
    console::Scope red(console::Color::Red);
    std::cout << message;
}

void PrintPassFail(bool ok, const std::string& name, const std::string& detail)
{
    std::cout << "  [";
    {
        console::Scope tint(ok ? console::Color::Green : console::Color::Red);
        std::cout << (ok ? "PASS" : "FAIL");
    }
    std::cout << "] " << name;
    if (!detail.empty())
    {
        std::cout << " - " << detail;
    }
    std::cout << "\n";
}

std::size_t HardwareWorkers()
{
    std::size_t workers = arctix::Thread::HardwareConcurrency();
    return workers == 0 ? 4 : workers;
}

// --- help ---

int CmdHelp(const std::vector<std::string>& argv);

int CmdHelp(const std::vector<std::string>& argv)
{
    if (argv.size() > 1)
    {
        const Command* cmd = FindCommand(argv[1]);
        if (cmd == nullptr)
        {
            PrintError("'" + argv[1] + "' is not an arctix command. Type \"help\".\n");
            return 1;
        }
        {
            console::Scope green(console::Color::Green);
            std::cout << cmd->name;
        }
        std::cout << " - " << cmd->summary << "\nUsage: " << cmd->usage << "\n";
        return 0;
    }
    PrintHelpOverview();
    return 0;
}

// --- version / ver ---

int CmdVersion(const std::vector<std::string>& /*argv*/)
{
    int major = 0, minor = 0, patch = 0;
    arctix::Core::GetVersion(&major, &minor, &patch);
    {
        console::Scope cyan(console::Color::Cyan);
        std::cout << "ArctixCLI " << CliVersionString() << "\n";
    }
    std::cout << "ArctixC   " << arctix::Core::GetVersionString();
    std::cout << " (major=" << major << " minor=" << minor << " patch=" << patch << ")\n";
    std::cout << "flavor    C++ SDK on arctixc.dll (C flavor ships as arctixc_c.dll)\n";
    return 0;
}

// --- echo ---

int CmdEcho(const std::vector<std::string>& argv)
{
    for (std::size_t i = 1; i < argv.size(); ++i)
    {
        if (i > 1)
        {
            std::cout << ' ';
        }
        std::cout << argv[i];
    }
    std::cout << "\n";
    return 0;
}

// --- cls / clear ---

int CmdClear(const std::vector<std::string>& /*argv*/)
{
#ifdef _WIN32
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (out != INVALID_HANDLE_VALUE)
    {
        CONSOLE_SCREEN_BUFFER_INFO info;
        if (GetConsoleScreenBufferInfo(out, &info))
        {
            DWORD cells = static_cast<DWORD>(info.dwSize.X) * static_cast<DWORD>(info.dwSize.Y);
            DWORD written = 0;
            COORD origin = {0, 0};
            FillConsoleOutputCharacterA(out, ' ', cells, origin, &written);
            FillConsoleOutputAttribute(out, info.wAttributes, cells, origin, &written);
            SetConsoleCursorPosition(out, origin);
            return 0;
        }
    }
    // Fallback when not attached to a console (piped output).
    std::cout << std::string(50, '\n');
#else
    std::cout << "\x1b[2J\x1b[H";
#endif
    return 0;
}

// --- color ---

int CmdColor(const std::vector<std::string>& argv)
{
    if (argv.size() == 1)
    {
        std::cout << "color: " << (console::IsEnabled() ? "on" : "off") << ".\nUsage: color [on|off]\n";
        return 0;
    }
    if (argv.size() != 2 || (argv[1] != "on" && argv[1] != "off"))
    {
        PrintError("color: use 'color on' or 'color off'.\n");
        return 1;
    }
    console::SetEnabled(argv[1] == "on");
    std::cout << "color: " << argv[1] << ".\n";
    return 0;
}

// --- bench ---

struct BenchTask
{
    std::atomic<unsigned long long>* counter;
};

void BenchWorker(void* userData)
{
    auto* task = static_cast<BenchTask*>(userData);
    task->counter->fetch_add(1, std::memory_order_relaxed);
}

int CmdBench(const std::vector<std::string>& argv)
{
    const char* keys[] = {"size", "iters", "threads"};
    std::string values[] = {"256", "100000", "0"};
    std::vector<std::string> positional;
    std::string error;
    if (!ParseOptions(argv, 1, keys, values, 3, positional, error))
    {
        PrintError("bench: " + error + "\nUsage: bench [--size BYTES] [--iters N] [--threads N]\n");
        return 1;
    }
    if (!positional.empty())
    {
        PrintError("bench: unexpected argument '" + positional[0] + "'.\nUsage: bench [--size BYTES] [--iters N] [--threads N]\n");
        return 1;
    }
    std::size_t blockSize = 0, iters = 0, threads = 0;
    if (!ParseSize(values[0], blockSize) || !ParseSize(values[1], iters))
    {
        PrintError("bench: sizes must be positive numbers (k/m/g suffix allowed).\n");
        return 1;
    }
    if (!values[2].empty() && values[2] != "0" && !ParseSize(values[2], threads))
    {
        PrintError("bench: bad thread count '" + values[2] + "'.\n");
        return 1;
    }
    if (threads == 0)
    {
        threads = HardwareWorkers();
    }

    std::cout << "bench: " << iters << " x " << blockSize << " bytes, " << threads << " threads.\n";

    // Phase 1: ArctixC allocator vs system malloc, single threaded.
    arctix::Memory::Allocator alloc;
    alloc.Initialize(arctix::Memory::DefaultConfig());

    auto arctixStart = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < iters; ++i)
    {
        void* p = alloc.Allocate(blockSize, 16);
        if (p == nullptr)
        {
            PrintError("bench: arctix allocation failed at iteration " + std::to_string(i) + ".\n");
            alloc.Shutdown();
            return 1;
        }
        std::memset(p, 0xAB, blockSize);
        alloc.Deallocate(p);
    }
    auto arctixEnd = std::chrono::steady_clock::now();

    auto mallocStart = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < iters; ++i)
    {
        void* p = std::malloc(blockSize == 0 ? 1 : blockSize);
        if (p == nullptr)
        {
            PrintError("bench: malloc failed at iteration " + std::to_string(i) + ".\n");
            alloc.Shutdown();
            return 1;
        }
        std::memset(p, 0xAB, blockSize);
        std::free(p);
    }
    auto mallocEnd = std::chrono::steady_clock::now();

    double arctixMs = std::chrono::duration<double, std::milli>(arctixEnd - arctixStart).count();
    double mallocMs = std::chrono::duration<double, std::milli>(mallocEnd - mallocStart).count();

    arctix::Memory::ArctixStats stats = alloc.GetStatistics();
    alloc.Shutdown();

    double arctixOps = arctixMs > 0.0 ? iters / (arctixMs / 1000.0) : 0.0;
    double mallocOps = mallocMs > 0.0 ? iters / (mallocMs / 1000.0) : 0.0;
    std::cout << "  arctix alloc/free : " << arctixMs << " ms (" << static_cast<unsigned long long>(arctixOps) << " ops/s)\n";
    std::cout << "  malloc/free       : " << mallocMs << " ms (" << static_cast<unsigned long long>(mallocOps) << " ops/s)\n";
    std::cout << "  allocator stats   : " << stats.allocationCount << " allocs, peak " << stats.peakUsageBytes << " bytes\n";

    // Phase 2: thread-pool throughput.
    std::atomic<unsigned long long> counter{0};
    BenchTask task{&counter};
    arctix::Thread::ThreadPool pool(threads);
    auto t3 = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < iters; ++i)
    {
        pool.Submit(BenchWorker, &task);
    }
    pool.WaitIdle();
    auto t4 = std::chrono::steady_clock::now();
    pool.Shutdown();

    double poolMs = std::chrono::duration<double, std::milli>(t4 - t3).count();
    std::cout << "  pool tasks (" << threads << " workers): " << poolMs << " ms, "
              << counter.load() << "/" << iters << " ran\n";
    return counter.load() == iters ? 0 : 1;
}

// --- bench-pool ---

int CmdBenchPool(const std::vector<std::string>& argv)
{
    const char* keys[] = {"size", "count", "iters"};
    std::string values[] = {"64", "1024", "200000"};
    std::vector<std::string> positional;
    std::string error;
    if (!ParseOptions(argv, 1, keys, values, 3, positional, error))
    {
        PrintError("bench-pool: " + error + "\nUsage: bench-pool [--size BYTES] [--count N] [--iters N]\n");
        return 1;
    }
    if (!positional.empty())
    {
        PrintError("bench-pool: unexpected argument '" + positional[0] + "'.\n");
        return 1;
    }
    std::size_t blockSize = 0, blockCount = 0, iters = 0;
    if (!ParseSize(values[0], blockSize) || !ParseSize(values[1], blockCount) || !ParseSize(values[2], iters))
    {
        PrintError("bench-pool: sizes must be positive numbers (k/m/g suffix allowed).\n");
        return 1;
    }

    arctix::Memory::MemoryPool pool;
    if (!pool.Create(blockSize, blockCount, 16))
    {
        PrintError("bench-pool: pool Create failed (" + std::to_string(blockCount) + " x " +
                   std::to_string(blockSize) + " bytes).\n");
        return 1;
    }
    auto t0 = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < iters; ++i)
    {
        void* p = pool.Allocate();
        if (p == nullptr)
        {
            PrintError("bench-pool: pool exhausted at iteration " + std::to_string(i) +
                       " (pool holds " + std::to_string(pool.FreeCount()) + " free).\n");
            pool.Destroy();
            return 1;
        }
        pool.Free(p);
    }
    auto t1 = std::chrono::steady_clock::now();
    pool.Destroy();

    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double ops = ms > 0.0 ? iters / (ms / 1000.0) : 0.0;
    std::cout << "bench-pool: " << iters << " alloc/free on " << blockCount << " x " << blockSize
              << " bytes: " << ms << " ms (" << static_cast<unsigned long long>(ops) << " ops/s)\n";
    return 0;
}

// --- bench-hash ---

int CmdBenchHash(const std::vector<std::string>& argv)
{
    const char* keys[] = {"size", "iters"};
    std::string values[] = {"4096", "20000"};
    std::vector<std::string> positional;
    std::string error;
    if (!ParseOptions(argv, 1, keys, values, 2, positional, error))
    {
        PrintError("bench-hash: " + error + "\nUsage: bench-hash [--size BYTES] [--iters N]\n");
        return 1;
    }
    if (!positional.empty())
    {
        PrintError("bench-hash: unexpected argument '" + positional[0] + "'.\n");
        return 1;
    }
    std::size_t size = 0, iters = 0;
    if (!ParseSize(values[0], size) || !ParseSize(values[1], iters))
    {
        PrintError("bench-hash: sizes must be positive numbers (k/m/g suffix allowed).\n");
        return 1;
    }

    arctix::Buffer::Buffer buf(size, 0xAB);
    volatile std::uint64_t sink = 0; // keep the loop from optimizing away
    auto t0 = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < iters; ++i)
    {
        sink ^= buf.Checksum();
        sink ^= arctix::Memory::Fnv1aHash64(buf.Data(), buf.Size());
    }
    auto t1 = std::chrono::steady_clock::now();

    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double totalBytes = static_cast<double>(size) * static_cast<double>(iters) * 2.0; // two passes
    double mbPerSec = ms > 0.0 ? (totalBytes / (1024.0 * 1024.0)) / (ms / 1000.0) : 0.0;
    std::cout << "bench-hash: " << iters << " x FNV-1a over " << size << " bytes: " << ms << " ms ("
              << static_cast<unsigned long long>(mbPerSec) << " MB/s, sink=" << sink << ")\n";
    return 0;
}

// --- bench-scale: thread-count scaling sweep ---

struct ScaleSum
{
    std::atomic<unsigned long long>* total;
    std::size_t begin;
    std::size_t end;
};

void ScaleWorker(void* userData)
{
    auto* r = static_cast<ScaleSum*>(userData);
    unsigned long long local = 0;
    for (std::size_t i = r->begin; i < r->end; ++i)
    {
        local += i;
    }
    r->total->fetch_add(local, std::memory_order_relaxed);
}

int CmdBenchScale(const std::vector<std::string>& argv)
{
    const char* keys[] = {"range", "max-workers"};
    std::string values[] = {"2000000", "0"};
    std::vector<std::string> positional;
    std::string error;
    if (!ParseOptions(argv, 1, keys, values, 2, positional, error))
    {
        PrintError("bench-scale: " + error + "\nUsage: bench-scale [--range N] [--max-workers N]\n");
        return 1;
    }
    if (!positional.empty())
    {
        PrintError("bench-scale: unexpected argument '" + positional[0] + "'.\n");
        return 1;
    }
    std::size_t range = 0, maxWorkers = 0;
    if (!ParseSize(values[0], range))
    {
        PrintError("bench-scale: bad range '" + values[0] + "'.\n");
        return 1;
    }
    if (!values[1].empty() && values[1] != "0" && !ParseSize(values[1], maxWorkers))
    {
        PrintError("bench-scale: bad worker count '" + values[1] + "'.\n");
        return 1;
    }
    std::size_t hw = HardwareWorkers();
    if (maxWorkers == 0 || maxWorkers > hw)
    {
        maxWorkers = hw;
    }
    if (maxWorkers > 64)
    {
        maxWorkers = 64;
    }

    unsigned long long expect = range * (range - 1) / 2;
    std::cout << "bench-scale: sum 0.." << (range - 1) << " on 1.." << maxWorkers << " workers.\n";
    std::cout << "  workers | ms       | speedup\n";
    double baseline = 0.0;
    for (std::size_t w = 1; w <= maxWorkers; ++w)
    {
        std::atomic<unsigned long long> total{0};
        std::vector<ScaleSum> ranges(w);
        std::size_t chunk = range / w;
        arctix::Thread::ThreadPool pool(w);
        auto t0 = std::chrono::steady_clock::now();
        for (std::size_t k = 0; k < w; ++k)
        {
            ranges[k].total = &total;
            ranges[k].begin = k * chunk;
            ranges[k].end = (k + 1 == w) ? range : (k + 1) * chunk;
            pool.Submit(ScaleWorker, &ranges[k]);
        }
        pool.WaitIdle();
        auto t1 = std::chrono::steady_clock::now();
        pool.Shutdown();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        if (w == 1)
        {
            baseline = ms;
        }
        double speedup = (ms > 0.0 && baseline > 0.0) ? baseline / ms : 0.0;
        bool ok = total.load() == expect;
        std::cout << "  " << w << (w < 10 ? "       | " : w < 100 ? "      | " : "     | ") << ms
                  << " | " << speedup << "x" << (ok ? "" : "  WRONG SUM") << "\n";
        if (!ok)
        {
            return 1;
        }
    }
    return 0;
}

// --- check ---

int CmdCheck(const std::vector<std::string>& argv)
{
    if (argv.size() > 1)
    {
        PrintError("check: takes no arguments.\nUsage: check\n");
        return 1;
    }
    int failures = 0;
    auto report = [&](const char* name, bool ok, const std::string& detail) {
        PrintPassFail(ok, name, detail);
        if (!ok)
        {
            ++failures;
        }
    };

    arctix::Memory::Allocator alloc;
    arctix::Memory::ArctixConfig cfg = arctix::Memory::DefaultConfig();
    report("allocator initializes", alloc.Initialize(cfg), "");

    // NULL / zero-size edge cases must be safe, never crash.
    alloc.Deallocate(nullptr);
    report("free(NULL) is a no-op", true, "");
    void* zero = alloc.Allocate(0, 16);
    report("allocate(0) returns nullptr", zero == nullptr, "");

    // Leak simulation: free only half, statistics must show the rest.
    constexpr std::size_t kBlocks = 8;
    void* blocks[kBlocks] = {};
    bool allOk = true;
    for (std::size_t i = 0; i < kBlocks; ++i)
    {
        blocks[i] = alloc.Allocate(128, 16);
        allOk = allOk && (blocks[i] != nullptr);
    }
    report("8 x 128-byte allocations succeed", allOk, "");
    for (std::size_t i = 0; i < kBlocks / 2; ++i)
    {
        alloc.Deallocate(blocks[i]);
        blocks[i] = nullptr;
    }
    arctix::Memory::ArctixStats mid = alloc.GetStatistics();
    report("leak detector sees outstanding bytes",
           mid.currentUsageBytes == (kBlocks / 2) * 128,
           std::to_string(mid.currentUsageBytes) + " bytes outstanding, " +
               std::to_string(mid.allocationCount - mid.freeCount) + " blocks");
    for (std::size_t i = kBlocks / 2; i < kBlocks; ++i)
    {
        alloc.Deallocate(blocks[i]);
    }
    arctix::Memory::ArctixStats end = alloc.GetStatistics();
    report("all freed: usage returns to zero", end.currentUsageBytes == 0, "");

    // Pool exhaustion is reported (nullptr), never a crash.
    arctix::Memory::MemoryPool pool;
    bool poolOk = pool.Create(64, 4, 16);
    report("pool creates (64 bytes x 4)", poolOk, "");
    void* p[5] = {};
    for (int i = 0; i < 5; ++i)
    {
        p[i] = pool.Allocate();
    }
    report("5th block from a 4-block pool is nullptr", p[4] == nullptr, "");
    for (int i = 0; i < 4; ++i)
    {
        pool.Free(p[i]);
    }
    report("pool drains back to 4 free", pool.FreeCount() == 4, "");
    pool.Destroy();

    // Buffer checksum is stable across identical contents.
    arctix::Buffer::Buffer a(64, 0xAB), b(64, 0xAB);
    report("identical buffers match (Equals + Checksum)", a.Equals(b) && a.Checksum() == b.Checksum(), "");

    alloc.Shutdown();
    report("allocator shuts down clean", !alloc.IsInitialized(), "");

    if (failures == 0)
    {
        console::Scope green(console::Color::Green);
        std::cout << "check: all diagnostics passed.\n";
    }
    else
    {
        PrintError("check: FAILURES detected.\n");
    }
    return failures == 0 ? 0 : 1;
}

// --- stress: multi-threaded hammering with leak verification ---

struct StressState
{
    std::atomic<bool>* stop;
    std::atomic<unsigned long long>* ops;
    std::atomic<unsigned long long>* errors;
};

void StressWorker(void* userData)
{
    auto* st = static_cast<StressState*>(userData);
    // Each worker owns its allocator + pool: no sharing, pure throughput.
    arctix::Memory::Allocator alloc;
    if (!alloc.Initialize(arctix::Memory::DefaultConfig()))
    {
        st->errors->fetch_add(1, std::memory_order_relaxed);
        return;
    }
    arctix::Memory::MemoryPool pool;
    if (!pool.Create(128, 32, 16))
    {
        st->errors->fetch_add(1, std::memory_order_relaxed);
        alloc.Shutdown();
        return;
    }
    unsigned long long local = 0;
    char text[32];
    while (!st->stop->load(std::memory_order_relaxed))
    {
        void* p = alloc.Allocate(64, 16);
        if (p == nullptr)
        {
            break;
        }
        std::memset(p, 0xCD, 64);
        std::uint64_t h = arctix::Memory::Fnv1aHash64(p, 64);
        (void)h;
        alloc.Deallocate(p);
        void* q = pool.Allocate();
        if (q != nullptr)
        {
            pool.Free(q);
        }
        ++local;
        if ((local & 255) == 0)
        {
            std::snprintf(text, sizeof(text), "stress-%llu", local);
            volatile std::uint64_t s = arctix::Memory::Fnv1aHash64(text, std::strlen(text));
            (void)s;
        }
    }
    pool.Destroy();
    arctix::Memory::ArctixStats stats = alloc.GetStatistics();
    if (stats.currentUsageBytes != 0)
    {
        st->errors->fetch_add(1, std::memory_order_relaxed);
    }
    alloc.Shutdown();
    st->ops->fetch_add(local, std::memory_order_relaxed);
}

int CmdStress(const std::vector<std::string>& argv)
{
    const char* keys[] = {"seconds", "threads"};
    std::string values[] = {"3", "0"};
    std::vector<std::string> positional;
    std::string error;
    if (!ParseOptions(argv, 1, keys, values, 2, positional, error))
    {
        PrintError("stress: " + error + "\nUsage: stress [--seconds S] [--threads N]\n");
        return 1;
    }
    if (!positional.empty())
    {
        PrintError("stress: unexpected argument '" + positional[0] + "'.\n");
        return 1;
    }
    std::size_t seconds = 0, threads = 0;
    if (!ParseSize(values[0], seconds))
    {
        PrintError("stress: bad seconds '" + values[0] + "'.\n");
        return 1;
    }
    if (!values[1].empty() && values[1] != "0" && !ParseSize(values[1], threads))
    {
        PrintError("stress: bad thread count '" + values[1] + "'.\n");
        return 1;
    }
    if (threads == 0)
    {
        threads = HardwareWorkers();
    }
    if (seconds > 3600)
    {
        PrintError("stress: capped at 3600 seconds, really.\n");
        return 1;
    }

    std::atomic<bool> stop{false};
    std::atomic<unsigned long long> ops{0};
    std::atomic<unsigned long long> errors{0};
    StressState st{&stop, &ops, &errors};

    std::cout << "stress: " << threads << " workers hammering allocator+pool+hash for " << seconds << "s...\n";
    arctix::Thread::ThreadPool pool(threads);
    for (std::size_t i = 0; i < threads; ++i)
    {
        pool.Submit(StressWorker, &st);
    }
    arctix::Thread::ThreadSleepFor(seconds * 1000);
    stop.store(true, std::memory_order_relaxed);
    pool.WaitIdle();
    pool.Shutdown();

    unsigned long long total = ops.load();
    unsigned long long errs = errors.load();
    double perSec = seconds > 0 ? static_cast<double>(total) / static_cast<double>(seconds) : 0.0;
    std::cout << "stress: " << total << " mixed ops (" << static_cast<unsigned long long>(perSec) << "/s), "
              << errs << " errors.\n";
    bool ok = (errs == 0);
    PrintPassFail(ok, "no leaks, no errors under load", ok ? "" : std::to_string(errs) + " worker errors");
    return ok ? 0 : 1;
}

// --- sysinfo / info ---

int CmdSysInfo(const std::vector<std::string>& argv)
{
    if (argv.size() > 1)
    {
        PrintError("sysinfo: takes no arguments.\nUsage: sysinfo\n");
        return 1;
    }
    int major = 0, minor = 0, patch = 0;
    arctix::Core::GetVersion(&major, &minor, &patch);

    std::cout << "machine:\n";
#ifdef _WIN32
    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);
    const char* arch = "unknown";
    switch (si.wProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_AMD64: arch = "x64"; break;
    case PROCESSOR_ARCHITECTURE_ARM64: arch = "ARM64"; break;
    case PROCESSOR_ARCHITECTURE_INTEL: arch = "x86"; break;
    default: break;
    }
    MEMORYSTATUSEX mem = {};
    mem.dwLength = sizeof(mem);
    GlobalMemoryStatusEx(&mem);
    std::cout << "  os          Windows (" << arch << ")\n";
    std::cout << "  page size   " << si.dwPageSize << " bytes\n";
    std::cout << "  processors  " << si.dwNumberOfProcessors << "\n";
    std::cout << "  ram total   " << (mem.ullTotalPhys / (1024 * 1024)) << " MB\n";
    std::cout << "  ram free    " << (mem.ullAvailPhys / (1024 * 1024)) << " MB\n";
#else
    std::cout << "  os          POSIX (non-Windows)\n";
    std::cout << "  page size   " << sysconf(_SC_PAGESIZE) << " bytes\n";
    std::cout << "  processors  " << sysconf(_SC_NPROCESSORS_ONLN) << "\n";
#endif
    std::cout << "  hw threads  " << HardwareWorkers() << " (arctix::Thread::HardwareConcurrency)\n";
    std::cout << "sdk:\n";
    std::cout << "  version     " << arctix::Core::GetVersionString() << "\n";
    std::cout << "  runtime     arctixc.dll (C++ API) / arctixc_c.dll (C ABI)\n";
    std::cout << "  cli flavor  C++ on arctixc.dll\n";
    return 0;
}

// --- leak: intentional leak, then prove the detector caught it ---

int CmdLeak(const std::vector<std::string>& argv)
{
    if (argv.size() < 2 || argv.size() > 3)
    {
        PrintError("leak: wrong arguments.\nUsage: leak <bytes> [count]\n");
        return 1;
    }
    std::size_t bytes = 0, count = 4;
    if (!ParseSize(argv[1], bytes) || (argv.size() > 2 && !ParseSize(argv[2], count)))
    {
        PrintError("leak: sizes must be positive numbers.\n");
        return 1;
    }

    arctix::Memory::Allocator alloc;
    alloc.Initialize(arctix::Memory::DefaultConfig());
    std::vector<void*> ptrs;
    for (std::size_t i = 0; i < count; ++i)
    {
        void* p = alloc.Allocate(bytes, 16);
        if (p == nullptr)
        {
            PrintError("leak: allocation failed.\n");
            alloc.Shutdown();
            return 1;
        }
        ptrs.push_back(p);
    }
    arctix::Memory::ArctixStats leaked = alloc.GetStatistics();
    std::cout << "leak: holding " << count << " x " << bytes << " bytes -> outstanding "
              << leaked.currentUsageBytes << " bytes in " << (leaked.allocationCount - leaked.freeCount)
              << " blocks. The detector sees it.\n";
    for (void* p : ptrs)
    {
        alloc.Deallocate(p);
    }
    arctix::Memory::ArctixStats clean = alloc.GetStatistics();
    bool ok = clean.currentUsageBytes == 0;
    PrintPassFail(ok, "freed everything, detector back to zero", "");
    alloc.Shutdown();
    return ok ? 0 : 1;
}

// --- alloc ---

int CmdAlloc(const std::vector<std::string>& argv)
{
    if (argv.size() < 2 || argv.size() > 4)
    {
        PrintError("alloc: wrong arguments.\nUsage: alloc <bytes> [align] [count]\n");
        return 1;
    }
    std::size_t bytes = 0, align = 16, count = 1;
    if (!ParseSize(argv[1], bytes))
    {
        PrintError("alloc: bad size '" + argv[1] + "'.\n");
        return 1;
    }
    if (argv.size() > 2 && !ParseSize(argv[2], align))
    {
        PrintError("alloc: bad alignment '" + argv[2] + "'.\n");
        return 1;
    }
    if (argv.size() > 3 && !ParseSize(argv[3], count))
    {
        PrintError("alloc: bad count '" + argv[3] + "'.\n");
        return 1;
    }

    arctix::Memory::Allocator alloc;
    alloc.Initialize(arctix::Memory::DefaultConfig());
    std::vector<void*> ptrs;
    ptrs.reserve(count);
    for (std::size_t i = 0; i < count; ++i)
    {
        void* p = alloc.Allocate(bytes, align);
        if (p == nullptr)
        {
            PrintError("alloc: allocation " + std::to_string(i) + " failed.\n");
            for (void* q : ptrs)
            {
                alloc.Deallocate(q);
            }
            alloc.Shutdown();
            return 1;
        }
        ptrs.push_back(p);
    }
    arctix::Memory::ArctixStats used = alloc.GetStatistics();
    std::cout << "alloc: " << count << " x " << bytes << " bytes (align " << align << "), outstanding "
              << used.currentUsageBytes << " bytes, peak " << used.peakUsageBytes << " bytes.\n";
    for (void* p : ptrs)
    {
        alloc.Deallocate(p);
    }
    arctix::Memory::ArctixStats freed = alloc.GetStatistics();
    std::cout << "alloc: freed all, outstanding " << freed.currentUsageBytes << " bytes.\n";
    alloc.Shutdown();
    return 0;
}

// --- pool ---

int CmdPool(const std::vector<std::string>& argv)
{
    if (argv.size() != 3)
    {
        PrintError("pool: wrong arguments.\nUsage: pool <block-bytes> <block-count>\n");
        return 1;
    }
    std::size_t blockSize = 0, blockCount = 0;
    if (!ParseSize(argv[1], blockSize) || !ParseSize(argv[2], blockCount))
    {
        PrintError("pool: sizes must be positive numbers.\n");
        return 1;
    }
    arctix::Memory::MemoryPool pool;
    if (!pool.Create(blockSize, blockCount, 16))
    {
        PrintError("pool: Create failed.\n");
        return 1;
    }
    std::vector<void*> held;
    while (void* p = pool.Allocate())
    {
        held.push_back(p);
    }
    std::cout << "pool: " << blockCount << " x " << blockSize << " bytes, handed out " << held.size()
              << ", used " << pool.UsedCount() << ".\n";
    for (void* p : held)
    {
        pool.Free(p);
    }
    std::cout << "pool: freed all, free " << pool.FreeCount() << "/" << pool.BlockCount() << ".\n";
    pool.Destroy();
    return 0;
}

// --- buffer: Buffer lifecycle demo ---

int CmdBuffer(const std::vector<std::string>& argv)
{
    if (argv.size() < 2 || argv.size() > 3)
    {
        PrintError("buffer: wrong arguments.\nUsage: buffer <size> [fill-byte]\n");
        return 1;
    }
    std::size_t size = 0;
    unsigned long fill = 0xAB;
    if (!ParseSize(argv[1], size))
    {
        PrintError("buffer: bad size '" + argv[1] + "'.\n");
        return 1;
    }
    if (argv.size() > 2)
    {
        char* endPtr = nullptr;
        unsigned long v = std::strtoul(argv[2].c_str(), &endPtr, 0);
        if (endPtr == argv[2].c_str() || *endPtr != '\0' || v > 255)
        {
            PrintError("buffer: fill byte must be 0-255 (0x.. allowed).\n");
            return 1;
        }
        fill = v;
    }

    arctix::Buffer::Buffer buf(size, static_cast<std::uint8_t>(fill));
    std::cout << "buffer: size=" << buf.Size() << ", capacity=" << buf.Capacity()
              << ", checksum=" << buf.Checksum() << "\n";
    std::string hex = buf.HexEncode();
    std::cout << "  hex[0.." << (hex.size() > 64 ? 64 : hex.size()) << "]: "
              << hex.substr(0, 64) << (hex.size() > 64 ? "..." : "") << "\n";

    const char tail[] = "arctix";
    if (!buf.Append(tail, sizeof(tail) - 1))
    {
        PrintError("buffer: Append failed.\n");
        return 1;
    }
    std::cout << "  after append \"arctix\": size=" << buf.Size() << ", checksum=" << buf.Checksum() << "\n";

    std::size_t grown = buf.Size() * 2;
    if (!buf.Resize(grown, 0))
    {
        PrintError("buffer: Resize failed.\n");
        return 1;
    }
    std::cout << "  after resize x2: size=" << buf.Size() << ", capacity=" << buf.Capacity() << "\n";

    arctix::Buffer::Buffer copy;
    if (!copy.CopyFrom(buf.Data(), buf.Size()))
    {
        PrintError("buffer: CopyFrom failed.\n");
        return 1;
    }
    bool same = copy.Equals(buf);
    PrintPassFail(same, "CopyFrom round-trip matches (Equals)", "");
    buf.SecureWipe();
    std::cout << "  after SecureWipe: size=" << buf.Size() << " (contents zeroed, allocation kept)\n";
    return same ? 0 : 1;
}

// --- wipe: SecureZero demo ---

int CmdWipe(const std::vector<std::string>& argv)
{
    if (argv.size() != 2)
    {
        PrintError("wipe: wrong arguments.\nUsage: wipe <bytes>\n");
        return 1;
    }
    std::size_t bytes = 0;
    if (!ParseSize(argv[1], bytes))
    {
        PrintError("wipe: bad size '" + argv[1] + "'.\n");
        return 1;
    }
    arctix::Memory::Allocator alloc;
    alloc.Initialize(arctix::Memory::DefaultConfig());
    void* p = alloc.Allocate(bytes, 16);
    if (p == nullptr)
    {
        PrintError("wipe: allocation failed.\n");
        alloc.Shutdown();
        return 1;
    }
    std::memset(p, 0xAB, bytes);
    arctix::Memory::SecureZero(p, bytes);
    bool allZero = true;
    const auto* bytes8 = static_cast<const std::uint8_t*>(p);
    for (std::size_t i = 0; i < bytes; ++i)
    {
        if (bytes8[i] != 0)
        {
            allZero = false;
            break;
        }
    }
    PrintPassFail(allZero, "SecureZero wiped " + std::to_string(bytes) + " bytes", "");
    alloc.Deallocate(p);
    alloc.Shutdown();
    return allZero ? 0 : 1;
}

// --- hash ---

int CmdHash(const std::vector<std::string>& argv)
{
    if (argv.size() < 2)
    {
        PrintError("hash: needs text to hash.\nUsage: hash <text...>\n");
        return 1;
    }
    std::string text;
    for (std::size_t i = 1; i < argv.size(); ++i)
    {
        if (i > 1)
        {
            text += ' ';
        }
        text += argv[i];
    }
    std::uint64_t h = arctix::Memory::Fnv1aHash64(text.data(), text.size());
    char hex[17] = {};
    std::snprintf(hex, sizeof(hex), "%016llX", static_cast<unsigned long long>(h));
    std::cout << "hash: fnv1a64(\"" << text << "\") = 0x" << hex << " (" << h << ")\n";
    return 0;
}

// --- hash-file: stream a file through FNV-1a ---

int CmdHashFile(const std::vector<std::string>& argv)
{
    if (argv.size() != 2)
    {
        PrintError("hash-file: wrong arguments.\nUsage: hash-file <path>\n");
        return 1;
    }
    std::ifstream in(argv[1], std::ios::binary);
    if (!in)
    {
        PrintError("hash-file: cannot open '" + argv[1] + "'.\n");
        return 1;
    }
    constexpr std::size_t kChunk = 65536;
    std::vector<char> chunk(kChunk);
    std::uint64_t h = 14695981039346656037ULL;
    std::size_t total = 0;
    while (in)
    {
        in.read(chunk.data(), static_cast<std::streamsize>(kChunk));
        std::streamsize got = in.gcount();
        const auto* p = reinterpret_cast<const std::uint8_t*>(chunk.data());
        for (std::streamsize i = 0; i < got; ++i)
        {
            h ^= static_cast<std::uint64_t>(p[i]);
            h *= 1099511628211ULL;
        }
        total += static_cast<std::size_t>(got);
    }
    char hex[17] = {};
    std::snprintf(hex, sizeof(hex), "%016llX", static_cast<unsigned long long>(h));
    std::cout << "hash-file: fnv1a64(\"" << argv[1] << "\", " << total << " bytes) = 0x" << hex << " (" << h
              << ")\n";
    return 0;
}

// --- threads ---

struct SumRange
{
    std::atomic<unsigned long long>* total;
    std::size_t begin;
    std::size_t end;
};

void SumWorker(void* userData)
{
    auto* r = static_cast<SumRange*>(userData);
    unsigned long long local = 0;
    for (std::size_t i = r->begin; i < r->end; ++i)
    {
        local += i;
    }
    r->total->fetch_add(local, std::memory_order_relaxed);
}

int CmdThreads(const std::vector<std::string>& argv)
{
    std::size_t workers = HardwareWorkers();
    if (argv.size() > 2)
    {
        PrintError("threads: wrong arguments.\nUsage: threads [worker-count]\n");
        return 1;
    }
    if (argv.size() == 2 && !ParseSize(argv[1], workers))
    {
        PrintError("threads: bad worker count '" + argv[1] + "'.\n");
        return 1;
    }

    constexpr std::size_t kN = 1000000;
    std::atomic<unsigned long long> total{0};
    std::vector<SumRange> ranges(workers);
    std::size_t chunk = kN / workers;

    arctix::Thread::ThreadPool pool(workers);
    auto t0 = std::chrono::steady_clock::now();
    for (std::size_t w = 0; w < workers; ++w)
    {
        ranges[w].total = &total;
        ranges[w].begin = w * chunk;
        ranges[w].end = (w + 1 == workers) ? kN : (w + 1) * chunk;
        pool.Submit(SumWorker, &ranges[w]);
    }
    pool.WaitIdle();
    auto t1 = std::chrono::steady_clock::now();
    pool.Shutdown();

    unsigned long long expect = (kN - 1) * kN / 2;
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool ok = total.load() == expect;
    std::cout << "threads: sum 0.." << (kN - 1) << " on " << workers << " workers = " << total.load();
    if (ok)
    {
        console::Scope green(console::Color::Green);
        std::cout << " (correct)";
    }
    else
    {
        console::Scope red(console::Color::Red);
        std::cout << " (WRONG, expected " << expect << ")";
    }
    std::cout << " in " << ms << " ms.\n";
    return ok ? 0 : 1;
}

// --- cores: verify ParallelFor visits every index exactly once ---

struct CoresState
{
    std::atomic<unsigned long long>* visits;
    std::atomic<unsigned long long>* sum;
};

void CoresFn(std::size_t index, void* userData)
{
    auto* st = static_cast<CoresState*>(userData);
    st->visits->fetch_add(1, std::memory_order_relaxed);
    st->sum->fetch_add(static_cast<unsigned long long>(index), std::memory_order_relaxed);
}

int CmdCores(const std::vector<std::string>& argv)
{
    if (argv.size() > 1)
    {
        PrintError("cores: takes no arguments.\nUsage: cores\n");
        return 1;
    }
    std::size_t hw = HardwareWorkers();
    std::cout << "cores: hardware concurrency = " << hw << "\n";
    constexpr std::size_t kN = 100000;
    std::atomic<unsigned long long> visits{0};
    std::atomic<unsigned long long> sum{0};
    CoresState st{&visits, &sum};
    auto t0 = std::chrono::steady_clock::now();
    bool ran = arctix::Thread::ParallelFor(0, kN, CoresFn, &st, 0);
    auto t1 = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    unsigned long long expect = (kN - 1) * kN / 2;
    bool ok = ran && visits.load() == kN && sum.load() == expect;
    PrintPassFail(ok,
                  "ParallelFor visited " + std::to_string(visits.load()) + "/" + std::to_string(kN) +
                      " indices in " + std::to_string(ms) + " ms",
                  "");
    return ok ? 0 : 1;
}

// --- parallel: ParallelFor sum over an arbitrary range ---

struct ParallelSum
{
    std::atomic<unsigned long long>* total;
};

void ParallelSumFn(std::size_t index, void* userData)
{
    auto* st = static_cast<ParallelSum*>(userData);
    st->total->fetch_add(static_cast<unsigned long long>(index), std::memory_order_relaxed);
}

int CmdParallel(const std::vector<std::string>& argv)
{
    if (argv.size() < 3 || argv.size() > 4)
    {
        PrintError("parallel: wrong arguments.\nUsage: parallel <begin> <end> [workers]\n");
        return 1;
    }
    std::size_t begin = 0, end = 0, workers = HardwareWorkers();
    // begin may be 0, so parse manually instead of ParseSize.
    auto parseNum = [](const std::string& text, std::size_t& out) {
        if (text.empty())
        {
            return false;
        }
        unsigned long long v = 0;
        for (char c : text)
        {
            if (c < '0' || c > '9')
            {
                return false;
            }
            v = v * 10 + static_cast<unsigned long long>(c - '0');
            if (v > 1000000000ULL)
            {
                return false;
            }
        }
        out = static_cast<std::size_t>(v);
        return true;
    };
    if (!parseNum(argv[1], begin) || !parseNum(argv[2], end))
    {
        PrintError("parallel: range must be plain numbers 0..1000000000.\n");
        return 1;
    }
    if (argv.size() == 4 && !ParseSize(argv[3], workers))
    {
        PrintError("parallel: bad worker count '" + argv[3] + "'.\n");
        return 1;
    }
    if (!(begin < end))
    {
        PrintError("parallel: need begin < end.\n");
        return 1;
    }
    std::size_t count = end - begin;
    // Overflow-safe Gauss sum: count*first + count*(count-1)/2, range capped above.
    unsigned long long q = count / 2;
    unsigned long long r = count % 2;
    unsigned long long firstPlusLast = static_cast<unsigned long long>(begin) + static_cast<unsigned long long>(end - 1);
    unsigned long long expect = q * firstPlusLast + r * firstPlusLast / 2;

    std::atomic<unsigned long long> total{0};
    ParallelSum st{&total};
    auto t0 = std::chrono::steady_clock::now();
    bool ran = arctix::Thread::ParallelFor(begin, end, ParallelSumFn, &st, workers);
    auto t1 = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    bool ok = ran && total.load() == expect;
    std::cout << "parallel: sum " << begin << ".." << (end - 1) << " on " << workers << " workers = "
              << total.load();
    if (ok)
    {
        console::Scope green(console::Color::Green);
        std::cout << " (correct)";
    }
    else
    {
        console::Scope red(console::Color::Red);
        std::cout << " (WRONG, expected " << expect << ")";
    }
    std::cout << " in " << ms << " ms.\n";
    return ok ? 0 : 1;
}

// --- init: interactive + non-interactive project scaffolding ---

bool IsValidProjectName(const std::string& name)
{
    if (name.empty() || name.size() > 64)
    {
        return false;
    }
    for (char c : name)
    {
        bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                  c == '_' || c == '-';
        if (!ok)
        {
            return false;
        }
    }
    return true;
}

std::string AskLine(const std::string& prompt, const std::string& def)
{
    std::cout << prompt;
    if (!def.empty())
    {
        std::cout << " [" << def << "]";
    }
    std::cout << ": " << std::flush;
    std::string line;
    if (!std::getline(std::cin, line))
    {
        return def;
    }
    if (!line.empty() && line.back() == '\r')
    {
        line.pop_back();
    }
    // Trim spaces.
    std::size_t b = 0;
    while (b < line.size() && (line[b] == ' ' || line[b] == '\t'))
    {
        ++b;
    }
    std::size_t e = line.size();
    while (e > b && (line[e - 1] == ' ' || line[e - 1] == '\t'))
    {
        --e;
    }
    std::string trimmed = line.substr(b, e - b);
    return trimmed.empty() ? def : trimmed;
}

bool MakeDirs(const std::string& path)
{
    if (path.empty() || path == ".")
    {
        return true;
    }
    std::string norm = path;
    for (char& c : norm)
    {
        if (c == '\\')
        {
            c = '/';
        }
    }
    for (std::size_t i = 0; i <= norm.size(); ++i)
    {
        if (i == norm.size() || norm[i] == '/')
        {
            std::string part = norm.substr(0, i);
            if (part.empty() || part == "." || part.back() == ':')
            {
                continue; // root, cwd, or drive letter ("C:") - nothing to create
            }
#ifdef _WIN32
            if (!CreateDirectoryA(part.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS)
            {
                return false;
            }
#else
            if (mkdir(part.c_str(), 0755) != 0 && errno != EEXIST)
            {
                return false;
            }
#endif
        }
    }
    return true;
}

bool WriteTextFile(const std::string& path, const std::string& content)
{
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out)
    {
        return false;
    }
    out << content;
    return static_cast<bool>(out);
}

int ScaffoldProject(const std::string& name, const std::string& dir, bool useCxx)
{
    const char* flavor = useCxx ? "C++ (arctixc.dll)" : "C (arctixc_c.dll)";
    std::cout << "init: scaffolding '" << name << "' in '" << dir << "' for " << flavor << "...\n";

    if (!MakeDirs(dir))
    {
        PrintError("init: cannot create directory '" + dir + "'.\n");
        return 1;
    }
    std::string sep = "/";
    std::string cmakePath = dir + sep + "CMakeLists.txt";
    std::string srcPath = dir + sep + (useCxx ? "main.cpp" : "main.c");
    std::string readmePath = dir + sep + "README.md";

    const char* dllName = useCxx ? "arctixc.dll" : "arctixc_c.dll";
    const char* libName = useCxx ? "arctixc.lib" : "arctixc_c.lib";
    const char* target = useCxx ? "ArctixC::ArctixC" : "ArctixC::ArctixC_C";
    const char* lang = useCxx ? "CXX" : "C";
    const char* src = useCxx ? "main.cpp" : "main.c";

    std::string cmakeLists = "cmake_minimum_required(VERSION 3.20)\nproject(" + name + " LANGUAGES " + lang +
                             ")\n\nset(CMAKE_" + lang + "_STANDARD " + (useCxx ? "17" : "11") +
                             ")\nset(CMAKE_" + lang + "_STANDARD_REQUIRED ON)\n\n"
                             "# Point CMAKE_PREFIX_PATH at your ArctixC install (or set ArctixC_DIR).\n"
                             "find_package(ArctixC REQUIRED)\n\nadd_executable(" +
                             name + " " + src + ")\ntarget_link_libraries(" + name + " PRIVATE " + target +
                             ")\n";

    std::string mainSrc;
    if (useCxx)
    {
        mainSrc = "#include <cstdio>\n\n#include <arctixc/memory/allocator.hpp>\n"
                  "#include <arctixc/memory/buffer.hpp>\n\nint main()\n{\n"
                  "    arctix::Memory::Allocator alloc;\n"
                  "    alloc.Initialize(arctix::Memory::DefaultConfig());\n\n"
                  "    arctix::Buffer::Buffer buf(256, 0xAB);\n"
                  "    std::printf(\"" + name +
                  ": buffer %llu bytes, checksum %llu\\n\", (unsigned long long)buf.Size(), (unsigned long long)buf.Checksum());\n\n"
                  "    alloc.Shutdown();\n"
                  "    return 0;\n}\n";
    }
    else
    {
        mainSrc = "/* Scaffolded by `ArctixCLI init`. Pure C on arctixc_c.dll. */\n\n"
                  "#include <stdio.h>\n\n#include <arctixc/c_api/memory.h>\n\n"
                  "int main(void)\n{\n"
                  "    ArctixAllocatorHandle* alloc = Arctix_CreateAllocator(0);\n"
                  "    if (alloc == 0)\n    {\n        printf(\"allocator failed\\n\");\n        return 1;\n    }\n\n"
                  "    void* p = Arctix_Allocate(alloc, 256, 16);\n"
                  "    printf(\"" + name + ": %s\\n\", p != 0 ? \"allocated 256 bytes\" : \"allocation failed\");\n"
                  "    Arctix_Free(alloc, p);\n    Arctix_DestroyAllocator(alloc);\n    return 0;\n}\n";
    }

    std::string readme = "# " + name + "\n\nScaffolded by `ArctixCLI init` (" + flavor + ").\n\n"
                         "Build against an installed ArctixC SDK:\n\n```powershell\n"
                         "cmake -S . -B build -G Ninja "
                         "-DCMAKE_PREFIX_PATH=<path-to-ArctixC-install>\ncmake --build build\n```\n\n"
                         "Ship `" + dllName + "` (import library `" + libName + "`) next to your exe.\n";

    bool ok = WriteTextFile(cmakePath, cmakeLists) && WriteTextFile(srcPath, mainSrc) &&
              WriteTextFile(readmePath, readme);
    if (!ok)
    {
        PrintError("init: failed writing scaffold files in '" + dir + "/'.\n");
        return 1;
    }
    std::cout << "init: created '" << dir << "/' with CMakeLists.txt, " << src << ", README.md.\n";
    std::cout << "  link target : " << target << "\n";
    std::cout << "  ship with   : " << dllName << " (import lib " << libName << ")\n";
    return 0;
}

int CmdInit(const std::vector<std::string>& argv)
{
    // Interactive mode: bare `init` asks everything.
    if (argv.size() == 1)
    {
        std::cout << "init: new arctix project - answer the prompts (Enter = default).\n";
        std::string name;
        for (;;)
        {
            name = AskLine("project name", "");
            if (IsValidProjectName(name))
            {
                break;
            }
            PrintError("init: name may only contain letters, digits, '_' and '-'.\n");
        }
        std::string path = AskLine("path to create in", ".");
        std::string lang = AskLine("language (cxx/c)", "cxx");
        while (lang != "cxx" && lang != "c" && lang != "C++" && lang != "C")
        {
            PrintError("init: language must be 'cxx' or 'c'.\n");
            lang = AskLine("language (cxx/c)", "cxx");
        }
        bool useCxx = (lang == "cxx" || lang == "C++");
        std::string defFlavor = useCxx ? "cxx" : "c";
        std::string flavor = AskLine("SDK flavor - arctixc (C++) or arctixc_c (C)", defFlavor);
        while (flavor != "cxx" && flavor != "arctixc" && flavor != "c" && flavor != "arctixc_c")
        {
            PrintError("init: flavor must be 'cxx' (arctixc) or 'c' (arctixc_c).\n");
            flavor = AskLine("SDK flavor", defFlavor);
        }
        bool flavorCxx = (flavor == "cxx" || flavor == "arctixc");
        if (flavorCxx != useCxx)
        {
            PrintError("init: flavor/SDK mismatch - C++ code needs arctixc, C code needs arctixc_c. Using language default.\n");
        }
        std::string dir = path;
        while (!dir.empty() && (dir.back() == '/' || dir.back() == '\\'))
        {
            dir.pop_back();
        }
        if (dir.empty() || dir == ".")
        {
            dir = name;
        }
        else
        {
            dir += "/" + name;
        }
        return ScaffoldProject(name, dir, useCxx);
    }

    // Non-interactive: init <name> [--path P] [--lang cxx|c] [--flavor cxx|c].
    const char* keys[] = {"path", "lang", "flavor"};
    std::string values[] = {".", "cxx", ""};
    std::vector<std::string> positional;
    std::string error;
    if (!ParseOptions(argv, 1, keys, values, 3, positional, error))
    {
        PrintError("init: " + error + "\nUsage: init <name> [--path DIR] [--lang cxx|c] [--flavor cxx|c]\n");
        return 1;
    }
    if (positional.size() != 1)
    {
        PrintError("init: needs exactly one project name.\nUsage: init <name> [--path DIR] [--lang cxx|c] [--flavor cxx|c]\n");
        return 1;
    }
    const std::string& name = positional[0];
    if (!IsValidProjectName(name))
    {
        PrintError("init: name may only contain letters, digits, '_' and '-'.\n");
        return 1;
    }
    if (values[1] != "cxx" && values[1] != "c")
    {
        PrintError("init: --lang must be 'cxx' or 'c'.\n");
        return 1;
    }
    bool useCxx = (values[1] == "cxx");
    if (!values[2].empty())
    {
        if (values[2] != "cxx" && values[2] != "c")
        {
            PrintError("init: --flavor must be 'cxx' (arctixc) or 'c' (arctixc_c).\n");
            return 1;
        }
        bool flavorCxx = (values[2] == "cxx");
        if (flavorCxx != useCxx)
        {
            PrintError("init: flavor/language mismatch - C++ needs arctixc, C needs arctixc_c.\n");
            return 1;
        }
    }
    std::string dir = values[0];
    while (!dir.empty() && (dir.back() == '/' || dir.back() == '\\'))
    {
        dir.pop_back();
    }
    if (dir.empty() || dir == ".")
    {
        dir = name;
    }
    else
    {
        dir += "/" + name;
    }
    return ScaffoldProject(name, dir, useCxx);
}

// --- sdk: show SDK flavors, targets, what this CLI uses ---

int CmdSdk(const std::vector<std::string>& argv)
{
    if (argv.size() > 1)
    {
        PrintError("sdk: takes no arguments.\nUsage: sdk\n");
        return 1;
    }
    int major = 0, minor = 0, patch = 0;
    arctix::Core::GetVersion(&major, &minor, &patch);
    std::cout << "ArctixC SDK " << arctix::Core::GetVersionString() << " ships two flavors:\n\n";
    {
        console::Scope green(console::Color::Green);
        std::cout << "  C++ flavor";
    }
    std::cout << " - arctixc.dll / arctixc.lib\n"
              << "    headers : <arctixc/memory/...> <arctixc/thread/...> <arctixc/core/...>\n"
              << "    cmake   : find_package(ArctixC) + ArctixC::ArctixC\n"
              << "    std     : C++17, C++20, C++23 (ARCTIXC_CXX_STANDARD)\n";
    {
        console::Scope green(console::Color::Green);
        std::cout << "  C flavor";
    }
    std::cout << "   - arctixc_c.dll / arctixc_c.lib\n"
              << "    headers : <arctixc/c_api/memory.h> <arctixc/c_api/thread.h>\n"
              << "    cmake   : find_package(ArctixC) + ArctixC::ArctixC_C\n"
              << "    std     : pure C, stable ABI\n";
    std::cout << "\nthis CLI links the C++ flavor (";
    {
        console::Scope cyan(console::Color::Cyan);
        std::cout << "arctixc.dll";
    }
    std::cout << "). Both DLLs live in the same folder; names differ by the _c suffix.\n";
    return 0;
}

// --- run: execute a script file of arctix commands ---

int CmdRun(const std::vector<std::string>& argv)
{
    if (argv.size() != 2)
    {
        PrintError("run: wrong arguments.\nUsage: run <script-file>\n");
        return 1;
    }
    static int depth = 0;
    if (depth >= 8)
    {
        PrintError("run: scripts nested too deep (max 8, loop?).\n");
        return 1;
    }
    std::ifstream in(argv[1]);
    if (!in)
    {
        PrintError("run: cannot open '" + argv[1] + "'.\n");
        return 1;
    }
    ++depth;
    std::string line;
    int failures = 0;
    int lastCode = 0;
    while (std::getline(in, line))
    {
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
        std::size_t b = 0;
        while (b < line.size() && (line[b] == ' ' || line[b] == '\t'))
        {
            ++b;
        }
        if (b == line.size() || line[b] == '#')
        {
            continue; // blanks + comments
        }
        {
            console::Scope gray(console::Color::Gray);
            std::cout << "arctix> " << line.substr(b) << "\n";
        }
        std::vector<std::string> args = Tokenize(line);
        if (args.empty())
        {
            continue;
        }
        if (args[0] == "exit" || args[0] == "quit")
        {
            break; // ends the script, not the shell
        }
        if (args[0] == "run")
        {
            lastCode = CmdRun(args);
        }
        else
        {
            lastCode = RunCommand(args[0], args);
        }
        if (lastCode != 0)
        {
            ++failures;
        }
    }
    --depth;
    if (failures > 0)
    {
        std::cout << "run: " << failures << " command(s) failed in '" << argv[1] << "'.\n";
        return 1;
    }
    return lastCode;
}

// --- history ---

int CmdHistory(const std::vector<std::string>& argv)
{
    if (argv.size() > 1)
    {
        PrintError("history: takes no arguments.\nUsage: history\n");
        return 1;
    }
    const std::vector<std::string>& hist = ShellHistory();
    if (hist.empty())
    {
        std::cout << "history: no commands yet this session.\n";
        return 0;
    }
    for (std::size_t i = 0; i < hist.size(); ++i)
    {
        std::cout << "  " << (i + 1) << "  " << hist[i] << "\n";
    }
    return 0;
}

// --- exit / quit ---

int CmdExit(const std::vector<std::string>& /*argv*/)
{
    return 0;
}

const Command kCommands[] = {
    // Benchmarks - measure the SDK.
    {"bench", "Allocator vs malloc + thread-pool throughput.", "bench [--size BYTES] [--iters N] [--threads N]", kCatBench, CmdBench},
    {"bench-pool", "Fixed-size pool alloc/free throughput.", "bench-pool [--size BYTES] [--count N] [--iters N]", kCatBench, CmdBenchPool},
    {"bench-hash", "FNV-1a hashing throughput (MB/s).", "bench-hash [--size BYTES] [--iters N]", kCatBench, CmdBenchHash},
    {"bench-scale", "Parallel-sum sweep across 1..N workers.", "bench-scale [--range N] [--max-workers N]", kCatBench, CmdBenchScale},
    // Diagnostics - prove the SDK is healthy.
    {"check", "Leak / exhaustion / checksum diagnostics.", "check", kCatDiag, CmdCheck},
    {"stress", "Hammer allocator+pool+hash on threads, verify no leaks.", "stress [--seconds S] [--threads N]", kCatDiag, CmdStress},
    {"sysinfo", "Machine + SDK facts (RAM, cores, page size).", "sysinfo", kCatDiag, CmdSysInfo},
    {"info", "Alias of sysinfo.", "info", kCatDiag, CmdSysInfo},
    {"leak", "Intentionally leak, watch the detector, then free.", "leak <bytes> [count]", kCatDiag, CmdLeak},
    // Memory - allocate, pool, inspect, wipe.
    {"alloc", "Allocate via the SDK and show statistics.", "alloc <bytes> [align] [count]", kCatMem, CmdAlloc},
    {"pool", "Drain a fixed-size memory pool and report.", "pool <block-bytes> <block-count>", kCatMem, CmdPool},
    {"buffer", "Buffer lifecycle: hex, append, resize, wipe.", "buffer <size> [fill-byte]", kCatMem, CmdBuffer},
    {"wipe", "SecureZero memory and prove it is zeroed.", "wipe <bytes>", kCatMem, CmdWipe},
    // Hashing - checksums.
    {"hash", "FNV-1a 64-bit hash of text.", "hash <text...>", kCatHash, CmdHash},
    {"hash-file", "FNV-1a 64-bit hash of a file's bytes.", "hash-file <path>", kCatHash, CmdHashFile},
    // Threading - pools, scaling, parallelism.
    {"threads", "Parallel sum demo on the thread pool.", "threads [worker-count]", kCatThread, CmdThreads},
    {"cores", "Show cores + verify ParallelFor visits each index once.", "cores", kCatThread, CmdCores},
    {"parallel", "ParallelFor sum over any range with timing.", "parallel <begin> <end> [workers]", kCatThread, CmdParallel},
    // Project - scaffold apps on ArctixC.
    {"init", "Scaffold a project (asks path, language, DLL flavor).", "init [name] [--path DIR] [--lang cxx|c] [--flavor cxx|c]", kCatProject, CmdInit},
    {"sdk", "SDK flavors: DLLs, link targets, what this CLI uses.", "sdk", kCatProject, CmdSdk},
    // Shell - the shell itself.
    {"help", "List commands or show one command's usage.", "help [command]", kCatShell, CmdHelp},
    {"version", "Show ArctixCLI and ArctixC versions.", "version", kCatShell, CmdVersion},
    {"ver", "Alias of version.", "ver", kCatShell, CmdVersion},
    {"color", "Turn colored output on or off.", "color [on|off]", kCatShell, CmdColor},
    {"run", "Run a script file of arctix commands.", "run <script-file>", kCatShell, CmdRun},
    {"history", "Show commands run this session.", "history", kCatShell, CmdHistory},
    {"echo", "Print text back.", "echo [text...]", kCatShell, CmdEcho},
    {"cls", "Clear the screen.", "cls", kCatShell, CmdClear},
    {"clear", "Alias of cls.", "clear", kCatShell, CmdClear},
    {"exit", "Leave the shell.", "exit", kCatShell, CmdExit},
    {"quit", "Alias of exit.", "quit", kCatShell, CmdExit},
};

} // namespace (anonymous)

const Command* FindCommand(const std::string& name)
{
    for (const Command& c : kCommands)
    {
        if (EqualsIgnoreCase(name, c.name))
        {
            return &c;
        }
    }
    return nullptr;
}

std::size_t CommandCount()
{
    return sizeof(kCommands) / sizeof(kCommands[0]);
}

const Command& CommandAt(std::size_t index)
{
    return kCommands[index];
}

int RunCommand(const std::string& name, const std::vector<std::string>& argv)
{
    const Command* cmd = FindCommand(name);
    if (cmd == nullptr)
    {
        PrintError("'" + name + "' is not recognized as an arctix command. Type \"help\".\n");
        return 1;
    }
    return cmd->run(argv);
}

void PrintHelpOverview()
{
    std::cout << "Arctix commands (built in - no PowerShell needed):\n";
    for (const CategoryInfo& cat : kCategoryOrder)
    {
        std::cout << "\n";
        {
            console::Scope cyan(console::Color::Cyan);
            std::cout << cat.name;
        }
        std::cout << " - " << cat.blurb << "\n";
        for (std::size_t i = 0; i < CommandCount(); ++i)
        {
            const Command& c = CommandAt(i);
            if (std::string(c.category) != cat.name)
            {
                continue;
            }
            std::cout << "  ";
            {
                console::Scope green(console::Color::Green);
                std::cout << c.name;
            }
            std::size_t pad = std::strlen(c.name) < 12 ? 12 - std::strlen(c.name) : 1;
            std::cout << std::string(pad, ' ') << c.summary << "\n";
        }
    }
    std::cout << "\nType \"help <command>\" for usage of one command.\n";
}

} // namespace arctixcli