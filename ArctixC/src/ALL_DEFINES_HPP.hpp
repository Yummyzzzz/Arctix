#pragma once

// ArctixC - ALL_DEFINES_HPP: namespace-stripping convenience header.
//
// Goal: write `Allocator`, `MemoryPool`, `Buffer`, `Thread`, `ThreadPool`,
// `VirtualReserve(...)`, `ParallelFor(...)`, ... instead of spelling
// `arctix::Memory::`, `arctix::Buffer::`, `arctix::Thread::` everywhere.
//
// NOTE on the requested `#define` approach: a macro such as
//   #define MemoryPool::Create arctix::Memory::MemoryPool::Create
// is NOT valid C++ for member functions. Member calls never spell the
// namespace at the call site anyway - you write `pool.Create(...)`, not
// `arctix::Memory::MemoryPool::Create(...)`. The namespace only appears on
// the TYPE (`arctix::Memory::MemoryPool pool;`), and the correct way to
// shorten that is a using-declaration, not #define. That is what this file
// does: every public arctixc symbol is brought into scope, so any file that
// includes this header can drop the `arctix::` prefix safely. No macros,
// no name-mangling surprises, overloads keep working.
//
// Usage (inside ArctixC/src - this folder is on the target's PRIVATE
// include path, so sources here can do):
//   #include "ALL_DEFINES_HPP.hpp"
//   MemoryPool pool; pool.Create(64, 128);   // == arctix::Memory::MemoryPool
//   Buffer buf; buf.Resize(256);              // == arctix::Buffer::Buffer
//   Thread t; t.Start(fn, nullptr);          // == arctix::Thread::Thread
//
// Do NOT install this header with the SDK: it is a private developer
// convenience for sources inside this repo, not part of the public API.

#include "arctixc/arctixc.hpp"

// --- Core ---------------------------------------------------------------
using arctix::Core::Status;
using arctix::Core::StatusToString;

// --- Memory: config / stats ---------------------------------------------
using arctix::Memory::ArctixConfig;
using arctix::Memory::ArctixStats;
using arctix::Memory::DefaultConfig;

// --- Memory: utils (header-only free functions) --------------------------
using arctix::Memory::SecureZero;
using arctix::Memory::MemEqual;
using arctix::Memory::Fnv1aHash64;

// --- Memory: Allocator ----------------------------------------------------
using arctix::Memory::Allocator;
// Allocator members (no namespace at call site, listed for reference):
//   Initialize, Shutdown, IsInitialized,
//   Allocate, Deallocate, GetStatistics, ResetStatistics

// --- Memory: MemoryPool ---------------------------------------------------
using arctix::Memory::MemoryPool;
// MemoryPool members (call as pool.Create(...), pool.Allocate(), ...):
//   Create, Destroy, IsCreated,
//   Allocate, Free,
//   BlockSize, BlockCount, FreeCount, UsedCount

// --- Memory: virtual pages ------------------------------------------------
using arctix::Memory::MemoryProtection;
using arctix::Memory::VirtualPageSize;
using arctix::Memory::IsPageAligned;
using arctix::Memory::RoundUpToPageSize;
using arctix::Memory::VirtualReserve;
using arctix::Memory::VirtualCommit;
using arctix::Memory::VirtualDecommit;
using arctix::Memory::VirtualRelease;
using arctix::Memory::VirtualProtectMemory;

// --- Buffer ----------------------------------------------------------------
using arctix::Buffer::Buffer;
// Buffer members (call as buf.CopyFrom(...), buf.Resize(...), ...):
//   CopyFrom, Append, Resize, Reserve,
//   Fill, Clear, SecureWipe,
//   Equals, Checksum, HexEncode,
//   Data, Size, Capacity, Empty

// --- RingBuffer ------------------------------------------------------------
using arctix::Buffer::RingBuffer;
// RingBuffer members (call as ring.Write(...), ring.Read(...), ...):
//   Create, Destroy, IsCreated,
//   Write, Read, Peek, Discard, Clear,
//   Capacity, Size, FreeSpace, Empty, Full

// --- Thread ----------------------------------------------------------------
using arctix::Thread::ThreadFunction;
using arctix::Thread::Thread;
// Thread members (call as t.Start(...), t.Join(), ...):
//   Start, Join, Detach,
//   IsJoinable, IsRunning, HasThrown, GetId

using arctix::Thread::CurrentThreadId;
using arctix::Thread::ThreadYield;
using arctix::Thread::ThreadSleepFor;
using arctix::Thread::HardwareConcurrency;

// --- ThreadPool --------------------------------------------------------------
using arctix::Thread::ThreadPool;
// ThreadPool members (call as pool.Submit(...), pool.WaitIdle(), ...):
//   IsRunning, ThreadCount,
//   Submit, WaitIdle,
//   PendingTasks, RunningTasks, FailedTasks,
//   Shutdown

using arctix::Thread::ParallelFor;

// NOTE: the stable C ABI (Arctix_CreateAllocator, Arctix_PoolCreate,
// ArctixC_ThreadCreate, ...) already lives at global scope - no
// namespace to strip, so nothing is needed here for those.
