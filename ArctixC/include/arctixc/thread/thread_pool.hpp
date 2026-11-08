#pragma once

// ArctixC - fixed worker thread pool.
// Declaration header. Implementation lives in src/thread/thread_pool.cpp
// (that .cpp includes this header).
//
// Owns N worker threads fed by a FIFO task queue. Tasks are the same
// void(*)(void*) worker signature as arctix::Thread::Thread, so thread code and
// pool code share worker functions. Worker exceptions are caught at the
// worker boundary and counted (see FailedTasks()); they never escape.
//
// Lifecycle: construct (workers start) -> Submit() any number of tasks ->
// WaitIdle() -> Shutdown() (or just destroy: the destructor shuts down
// and joins every worker; prefer an explicit Shutdown()).

#include "arctixc/thread/export.hpp"
#include "arctixc/thread/thread.hpp"

#include <cstddef>
#include <cstdint>

namespace arctix::Thread
{

// Opaque internal state. Defined privately in src/thread/.
struct ThreadPoolImpl;

// Fixed-size pool of worker threads draining a shared task queue.
class ARCTIXC_THREAD_API ThreadPool
{
public:
    // Starts threadCount workers. threadCount == 0 means
    // HardwareConcurrency() (at least 1). Never starts 0 workers.
    explicit ThreadPool(std::size_t threadCount = 0);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    // True between construction and Shutdown().
    bool IsRunning() const;

    // Number of worker threads owned by this pool.
    std::size_t ThreadCount() const;

    // Queues one task. Returns false when function is nullptr, the pool
    // is shut down, or the queue allocation failed.
    bool Submit(ThreadFunction function, void* userData);

    // Blocks until the queue is empty AND no worker is busy.
    // timeoutMs == 0 waits forever. Returns true when idle, false on
    // timeout. Safe to call after Shutdown() (returns true).
    bool WaitIdle(std::uint64_t timeoutMs = 0);

    // Tasks currently waiting in the queue (excludes running tasks).
    std::size_t PendingTasks() const;

    // Tasks currently executing on workers.
    std::size_t RunningTasks() const;

    // Tasks whose worker threw (exceptions are caught per task).
    // Only grows; reset by destroying the pool.
    unsigned long long FailedTasks() const;

    // Stops accepting tasks, drains nothing (queued tasks still run),
    // waits for workers to finish, joins them. Idempotent.
    void Shutdown();

private:
    ThreadPoolImpl* impl_;
};

// Blocks until `function` has been called with every index in
// [begin, end), spread across threadCount workers (0 = hardware
// threads). Calls run with signature fn(index, userData); each index
// is visited exactly once. Returns false on invalid args
// (null function, end < begin); worker exceptions are counted, not
// propagated, and do not stop the remaining indices.
ARCTIXC_THREAD_API bool ParallelFor(std::size_t begin, std::size_t end, void (*function)(std::size_t index, void* userData), void* userData, std::size_t threadCount = 0);

} // namespace arctix::Thread
