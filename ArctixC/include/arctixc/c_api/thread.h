#pragma once

// ArctixC.Thread - stable C ABI for arctixc_c.dll (C consumers link arctixc_c.lib).
//
// This header is genuinely C-compatible: it only uses C types and can be
// included from a .c file. The handle is opaque; the library owns the
// underlying thread object.
//
// Ownership and lifecycle:
// - ArctixC_ThreadCreate() returns an empty (non-running) handle.
// - ArctixC_ThreadStart() runs the worker exactly once per handle.
// - Finish with exactly one of ArctixC_ThreadJoin() / ArctixC_ThreadDetach().
// - ArctixC_ThreadDestroy() on a still-joinable handle joins (blocking),
//   mirroring the C++ destructor. Prefer explicit Join/Detach first.
// - Passing NULL where a handle is required is always safe (start/join/
//   detach return 0, queries return 0, free functions do nothing).
// - userData ownership stays with the caller; the library never frees it.
// - Worker exceptions can never cross this boundary; a throwing worker is
//   reported via ArctixC_ThreadHasThrown() after a join.

#include <stddef.h>

#include "arctixc/thread/export.hpp"

#ifdef __cplusplus
extern "C"
{
#endif

// Opaque handle. Never dereference; only pass back to ArctixC_Thread*().
typedef struct ArctixC_ThreadHandle ArctixC_ThreadHandle;
typedef struct ArctixC_ThreadPoolHandle ArctixC_ThreadPoolHandle;

// Worker invoked on the new thread. May be NULL-checked by Start (NULL fails).
typedef void (*ArctixC_ThreadFunction)(void* userData);

// Creates an empty thread handle. Returns NULL on out-of-memory.
// Free with ArctixC_ThreadDestroy().
ARCTIXC_THREAD_API ArctixC_ThreadHandle* ArctixC_ThreadCreate(void);

// Destroys a handle from ArctixC_ThreadCreate(). Joins when still joinable.
// No-op on NULL.
ARCTIXC_THREAD_API void ArctixC_ThreadDestroy(ArctixC_ThreadHandle* handle);

// Starts the worker once. Returns 1 on success, 0 on failure
// (NULL handle, NULL function, or already started).
ARCTIXC_THREAD_API int ArctixC_ThreadStart(ArctixC_ThreadHandle* handle, ArctixC_ThreadFunction function, void* userData);

// Waits for completion. Returns 1 on success, 0 when not joinable.
ARCTIXC_THREAD_API int ArctixC_ThreadJoin(ArctixC_ThreadHandle* handle);

// Detaches (thread keeps running, handle becomes non-joinable).
// Returns 1 on success, 0 when not joinable.
ARCTIXC_THREAD_API int ArctixC_ThreadDetach(ArctixC_ThreadHandle* handle);

// Returns 1 when Join/Detach may be called, else 0.
ARCTIXC_THREAD_API int ArctixC_ThreadIsJoinable(const ArctixC_ThreadHandle* handle);

// Returns 1 while the worker is executing, else 0.
ARCTIXC_THREAD_API int ArctixC_ThreadIsRunning(const ArctixC_ThreadHandle* handle);

// Returns 1 when the finished worker threw (caught at the entry boundary).
ARCTIXC_THREAD_API int ArctixC_ThreadHasThrown(const ArctixC_ThreadHandle* handle);

// Hash of the owned thread id, or 0 when no thread was started.
ARCTIXC_THREAD_API unsigned long long ArctixC_ThreadGetId(const ArctixC_ThreadHandle* handle);

// Hash of the calling thread id.
ARCTIXC_THREAD_API unsigned long long ArctixC_ThreadCurrentId(void);

// Suggests the OS reschedule so another thread may run.
ARCTIXC_THREAD_API void ArctixC_ThreadYield(void);

// Sleeps the calling thread for at least the given milliseconds.
ARCTIXC_THREAD_API void ArctixC_ThreadSleepMs(unsigned long long milliseconds);

// Number of hardware threads reported by the implementation (0 = unknown).
ARCTIXC_THREAD_API unsigned int ArctixC_ThreadHardwareConcurrency(void);

// --- Thread pool ---
//
// Fixed worker pool over a single shared FIFO queue (no work stealing).
// Create (workers start), Submit()
// tasks, ArctixC_PoolWaitIdle(), ArctixC_PoolShutdown() (or just destroy:
// the destructor shuts down and joins). Worker exceptions are caught per
// task and counted (see ArctixC_PoolFailedTasks). Passing NULL where a
// handle is required is always safe (returns 0, mutators do nothing).

// Creates a pool with threadCount workers (0 = hardware threads, min 1).
// Returns NULL on failure. Free with ArctixC_PoolDestroy().
ARCTIXC_THREAD_API ArctixC_ThreadPoolHandle* ArctixC_PoolCreate(unsigned long long threadCount);

// Shuts down (drains queued tasks, joins workers) and frees the pool.
// No-op on NULL.
ARCTIXC_THREAD_API void ArctixC_PoolDestroy(ArctixC_ThreadPoolHandle* handle);

// Queues one task. Returns 1 on success, 0 on failure.
ARCTIXC_THREAD_API int ArctixC_PoolSubmit(ArctixC_ThreadPoolHandle* handle, ArctixC_ThreadFunction function, void* userData);

// Blocks until queue is empty and no worker is busy.
// timeoutMs == 0 waits forever. Returns 1 when idle, 0 on timeout.
ARCTIXC_THREAD_API int ArctixC_PoolWaitIdle(ArctixC_ThreadPoolHandle* handle, unsigned long long timeoutMs);

// Tasks waiting in the queue (0 for NULL).
ARCTIXC_THREAD_API unsigned long long ArctixC_PoolPendingTasks(const ArctixC_ThreadPoolHandle* handle);

// Worker thread count (0 for NULL).
ARCTIXC_THREAD_API unsigned long long ArctixC_PoolThreadCount(const ArctixC_ThreadPoolHandle* handle);

// Tasks whose worker threw (0 for NULL).
ARCTIXC_THREAD_API unsigned long long ArctixC_PoolFailedTasks(const ArctixC_ThreadPoolHandle* handle);

#ifdef __cplusplus
} // extern "C"
#endif
