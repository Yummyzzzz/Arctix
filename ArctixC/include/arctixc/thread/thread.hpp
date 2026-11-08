#pragma once

// ArctixC.Thread - convenient C++ thread creation and lifecycle.
//
// Declaration header. Implementation lives in src/thread/thread.cpp
// (that .cpp includes this header).
//
// Design notes:
// - A Thread object owns at most one OS thread at a time (move-only).
// - The worker signature is void(*)(void*) so user data passing stays
//   explicit and C-ABI compatible. Ownership of userData stays with the
//   caller: the library never frees it.
// - Exceptions thrown by the worker are caught at the thread entry
//   boundary (they never escape into the OS) and recorded. Query them
//   with HasThrown() after Join().
// - Lifecycle: Start() once -> Join() OR Detach() exactly once.
//   Join()/Detach() on a non-joinable thread safely return false.
// - The destructor joins a still-joinable thread (blocking) instead of
//   silently detaching or terminating the process. Prefer an explicit
//   Join()/Detach(); the destructor is a safety net, not the API.
// - This module never touches ArctixC.Memory. Workers may use it, but
//   the Thread implementation itself only uses the C++ standard library
//   plus the ArctixC foundation.

#include "arctixc/thread/export.hpp"

#include <cstdint>
#include <thread>

namespace arctix::Thread
{

// Worker function invoked on the new thread. userData is the pointer
// passed to Start(), may be nullptr. Must not throw out (if it does,
// the library catches everything and records it via HasThrown()).
using ThreadFunction = void (*)(void* userData);

// Opaque internal state. Defined privately in src/thread/.
struct ThreadImpl;

// Move-only owning wrapper around one std::thread with explicit lifecycle.
class ARCTIXC_THREAD_API Thread
{
public:
    Thread();
    ~Thread();

    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;

    Thread(Thread&& other) noexcept;
    Thread& operator=(Thread&& other) noexcept;

    // Starts the thread with the given worker and user data.
    // Returns false (does nothing) when function is nullptr or a
    // thread is already running/started on this object.
    bool Start(ThreadFunction function, void* userData);

    // Waits for the thread to finish. Returns false when the thread
    // is not joinable (never started, already joined/detached, moved).
    // After a successful Join(), IsJoinable() is false.
    bool Join();

    // Detaches the thread (it keeps running without an owner).
    // Returns false when the thread is not joinable.
    bool Detach();

    // True when Join()/Detach() may be called (a live std::thread exists).
    bool IsJoinable() const;

    // True while the worker function is executing on the thread.
    bool IsRunning() const;

    // True when the finished worker threw (caught at the entry boundary).
    // Only meaningful after Join() on a started thread.
    bool HasThrown() const;

    // ID of the owned thread, or a default-constructed id when none.
    std::thread::id GetId() const;

private:
    ThreadImpl* impl_;
};

// --- Current-thread utilities (free functions, no Thread object needed) ---

// ID of the calling thread.
ARCTIXC_THREAD_API std::thread::id CurrentThreadId();

// Suggests the OS reschedule so another thread may run.
ARCTIXC_THREAD_API void ThreadYield();

// Sleeps the calling thread for at least the given milliseconds.
ARCTIXC_THREAD_API void ThreadSleepFor(std::uint64_t milliseconds);

// Number of hardware threads reported by the implementation (0 = unknown).
ARCTIXC_THREAD_API unsigned int HardwareConcurrency();

} // namespace arctix::Thread
