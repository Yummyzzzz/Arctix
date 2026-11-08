#include "arctixc/c_api/thread.h"

#include <functional>
#include <new>

#include "arctixc/thread/thread.hpp"
#include "arctixc/thread/thread_pool.hpp"

// Private implementation-only translation unit: implements the stable C ABI
// declared in include/arctixc/c_api/thread.h. No matching private header.

struct ArctixC_ThreadHandle
{
    arctixs::Thread::Thread thread;
};

struct ArctixC_ThreadPoolHandle
{
    arctixs::Thread::ThreadPool* pool = nullptr;
};

static unsigned long long HashThreadId(const std::thread::id& id)
{
    return static_cast<unsigned long long>(std::hash<std::thread::id>{}(id));
}

extern "C"
{

ARCTIXC_THREAD_API ArctixC_ThreadHandle* ArctixC_ThreadCreate(void)
{
    return new (std::nothrow) ArctixC_ThreadHandle();
}

ARCTIXC_THREAD_API void ArctixC_ThreadDestroy(ArctixC_ThreadHandle* handle)
{
    delete handle;
}

ARCTIXC_THREAD_API int ArctixC_ThreadStart(ArctixC_ThreadHandle* handle, ArctixC_ThreadFunction function, void* userData)
{
    if (handle == nullptr || function == nullptr)
    {
        return 0;
    }
    return handle->thread.Start(function, userData) ? 1 : 0;
}

ARCTIXC_THREAD_API int ArctixC_ThreadJoin(ArctixC_ThreadHandle* handle)
{
    if (handle == nullptr)
    {
        return 0;
    }
    return handle->thread.Join() ? 1 : 0;
}

ARCTIXC_THREAD_API int ArctixC_ThreadDetach(ArctixC_ThreadHandle* handle)
{
    if (handle == nullptr)
    {
        return 0;
    }
    return handle->thread.Detach() ? 1 : 0;
}

ARCTIXC_THREAD_API int ArctixC_ThreadIsJoinable(const ArctixC_ThreadHandle* handle)
{
    if (handle == nullptr)
    {
        return 0;
    }
    return handle->thread.IsJoinable() ? 1 : 0;
}

ARCTIXC_THREAD_API int ArctixC_ThreadIsRunning(const ArctixC_ThreadHandle* handle)
{
    if (handle == nullptr)
    {
        return 0;
    }
    return handle->thread.IsRunning() ? 1 : 0;
}

ARCTIXC_THREAD_API int ArctixC_ThreadHasThrown(const ArctixC_ThreadHandle* handle)
{
    if (handle == nullptr)
    {
        return 0;
    }
    return handle->thread.HasThrown() ? 1 : 0;
}

ARCTIXC_THREAD_API unsigned long long ArctixC_ThreadGetId(const ArctixC_ThreadHandle* handle)
{
    if (handle == nullptr)
    {
        return 0;
    }
    return HashThreadId(handle->thread.GetId());
}

ARCTIXC_THREAD_API unsigned long long ArctixC_ThreadCurrentId(void)
{
    return HashThreadId(arctixs::Thread::CurrentThreadId());
}

ARCTIXC_THREAD_API void ArctixC_ThreadYield(void)
{
    arctixs::Thread::ThreadYield();
}

ARCTIXC_THREAD_API void ArctixC_ThreadSleepMs(unsigned long long milliseconds)
{
    arctixs::Thread::ThreadSleepFor(static_cast<std::uint64_t>(milliseconds));
}

ARCTIXC_THREAD_API unsigned int ArctixC_ThreadHardwareConcurrency(void)
{
    return arctixs::Thread::HardwareConcurrency();
}

ARCTIXC_THREAD_API ArctixC_ThreadPoolHandle* ArctixC_PoolCreate(unsigned long long threadCount)
{
    ArctixC_ThreadPoolHandle* handle = new (std::nothrow) ArctixC_ThreadPoolHandle();
    if (handle == nullptr)
    {
        return nullptr;
    }
    handle->pool = new (std::nothrow) arctixs::Thread::ThreadPool(static_cast<std::size_t>(threadCount));
    if (handle->pool == nullptr || !handle->pool->IsRunning())
    {
        delete handle->pool;
        delete handle;
        return nullptr;
    }
    return handle;
}

ARCTIXC_THREAD_API void ArctixC_PoolDestroy(ArctixC_ThreadPoolHandle* handle)
{
    if (handle == nullptr)
    {
        return;
    }
    if (handle->pool != nullptr)
    {
        handle->pool->Shutdown();
        delete handle->pool;
    }
    delete handle;
}

ARCTIXC_THREAD_API int ArctixC_PoolSubmit(ArctixC_ThreadPoolHandle* handle, ArctixC_ThreadFunction function, void* userData)
{
    if (handle == nullptr || handle->pool == nullptr || function == nullptr)
    {
        return 0;
    }
    return handle->pool->Submit(function, userData) ? 1 : 0;
}

ARCTIXC_THREAD_API int ArctixC_PoolWaitIdle(ArctixC_ThreadPoolHandle* handle, unsigned long long timeoutMs)
{
    if (handle == nullptr || handle->pool == nullptr)
    {
        return 0;
    }
    return handle->pool->WaitIdle(static_cast<std::uint64_t>(timeoutMs)) ? 1 : 0;
}

ARCTIXC_THREAD_API unsigned long long ArctixC_PoolPendingTasks(const ArctixC_ThreadPoolHandle* handle)
{
    if (handle == nullptr || handle->pool == nullptr)
    {
        return 0;
    }
    return static_cast<unsigned long long>(handle->pool->PendingTasks());
}

ARCTIXC_THREAD_API unsigned long long ArctixC_PoolThreadCount(const ArctixC_ThreadPoolHandle* handle)
{
    if (handle == nullptr || handle->pool == nullptr)
    {
        return 0;
    }
    return static_cast<unsigned long long>(handle->pool->ThreadCount());
}

ARCTIXC_THREAD_API unsigned long long ArctixC_PoolFailedTasks(const ArctixC_ThreadPoolHandle* handle)
{
    if (handle == nullptr || handle->pool == nullptr)
    {
        return 0;
    }
    return handle->pool->FailedTasks();
}

} // extern "C"
