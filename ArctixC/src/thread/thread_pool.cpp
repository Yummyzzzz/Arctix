#include "arctixc/thread/thread_pool.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <new>
#include <queue>
#include <utility>
#include <vector>

// Private implementation-only translation unit: implements arctix::Thread::ThreadPool
// and arctix::Thread::ParallelFor declared in include/arctixc/thread/thread_pool.hpp.
// No matching private header. Worker exceptions are caught per task and
// counted via FailedTasks(); they never escape the worker thread.

namespace arctix::Thread
{

namespace
{

struct Task
{
    ThreadFunction function = nullptr;
    void* userData = nullptr;
};

} // namespace

struct ThreadPoolImpl
{
    std::mutex mutex;
    std::condition_variable workAvailable;
    std::condition_variable drained;
    std::queue<Task> queue;
    std::vector<std::thread> workers;
    bool accepting = true;
    bool stopping = false;
    std::size_t running = 0;
    unsigned long long failed = 0;
};

static void PoolWorkerMain(ThreadPoolImpl* impl)
{
    for (;;)
    {
        Task task;
        {
            std::unique_lock<std::mutex> lock(impl->mutex);
            impl->workAvailable.wait(lock, [&] { return impl->stopping || !impl->queue.empty(); });
            if (impl->stopping && impl->queue.empty())
            {
                return;
            }
            task = impl->queue.front();
            impl->queue.pop();
            ++impl->running;
        }
        try
        {
            task.function(task.userData);
        }
        catch (...)
        {
            std::lock_guard<std::mutex> lock(impl->mutex);
            ++impl->failed;
        }
        {
            std::lock_guard<std::mutex> lock(impl->mutex);
            --impl->running;
            if (impl->queue.empty() && impl->running == 0)
            {
                impl->drained.notify_all();
            }
        }
    }
}

ThreadPool::ThreadPool(std::size_t threadCount)
    : impl_(new (std::nothrow) ThreadPoolImpl())
{
    if (impl_ == nullptr)
    {
        return;
    }
    if (threadCount == 0)
    {
        threadCount = HardwareConcurrency();
    }
    if (threadCount == 0)
    {
        threadCount = 1;
    }
    try
    {
        for (std::size_t i = 0; i < threadCount; ++i)
        {
            impl_->workers.emplace_back(PoolWorkerMain, impl_);
        }
    }
    catch (...)
    {
        {
            std::lock_guard<std::mutex> lock(impl_->mutex);
            impl_->accepting = false;
            impl_->stopping = true;
        }
        impl_->workAvailable.notify_all();
        for (auto& worker : impl_->workers)
        {
            if (worker.joinable())
            {
                worker.join();
            }
        }
        delete impl_;
        impl_ = nullptr;
    }
}

ThreadPool::~ThreadPool()
{
    Shutdown();
    delete impl_;
}

bool ThreadPool::IsRunning() const
{
    if (impl_ == nullptr)
    {
        return false;
    }
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->accepting;
}

std::size_t ThreadPool::ThreadCount() const
{
    if (impl_ == nullptr)
    {
        return 0;
    }
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->workers.size();
}

bool ThreadPool::Submit(ThreadFunction function, void* userData)
{
    if (impl_ == nullptr || function == nullptr)
    {
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        if (!impl_->accepting || impl_->stopping)
        {
            return false;
        }
        try
        {
            impl_->queue.push(Task{function, userData});
        }
        catch (...)
        {
            return false;
        }
    }
    impl_->workAvailable.notify_one();
    return true;
}

bool ThreadPool::WaitIdle(std::uint64_t timeoutMs)
{
    if (impl_ == nullptr)
    {
        return true;
    }
    std::unique_lock<std::mutex> lock(impl_->mutex);
    const auto isIdle = [&] { return impl_->queue.empty() && impl_->running == 0; };
    if (timeoutMs == 0)
    {
        impl_->drained.wait(lock, isIdle);
        return true;
    }
    return impl_->drained.wait_for(lock, std::chrono::milliseconds(timeoutMs), isIdle);
}

std::size_t ThreadPool::PendingTasks() const
{
    if (impl_ == nullptr)
    {
        return 0;
    }
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->queue.size();
}

std::size_t ThreadPool::RunningTasks() const
{
    if (impl_ == nullptr)
    {
        return 0;
    }
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->running;
}

unsigned long long ThreadPool::FailedTasks() const
{
    if (impl_ == nullptr)
    {
        return 0;
    }
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->failed;
}

void ThreadPool::Shutdown()
{
    if (impl_ == nullptr)
    {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        if (!impl_->accepting && impl_->workers.empty())
        {
            return; // already shut down
        }
        impl_->accepting = false;
        // Queued tasks still run: only stop after the queue drains.
        // Mark stopping once drained by joining below after a drain wait.
    }
    // Wait for the queue to drain, then stop the workers.
    {
        std::unique_lock<std::mutex> lock(impl_->mutex);
        impl_->drained.wait(lock, [&] { return impl_->queue.empty() && impl_->running == 0; });
        impl_->stopping = true;
    }
    impl_->workAvailable.notify_all();
    for (auto& worker : impl_->workers)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->workers.clear();
}

namespace
{

struct ParallelRange
{
    std::size_t begin = 0;
    std::size_t end = 0;
    void (*function)(std::size_t, void*) = nullptr;
    void* userData = nullptr;
    std::atomic<std::size_t>* next = nullptr;
};

void ParallelRangeWorker(void* raw)
{
    auto* range = static_cast<ParallelRange*>(raw);
    for (;;)
    {
        const std::size_t index = range->next->fetch_add(1, std::memory_order_relaxed);
        if (index >= range->end)
        {
            return;
        }
        try
        {
            range->function(index, range->userData);
        }
        catch (...)
        {
            // Counted by the pool as a failed task; other indices continue.
            throw;
        }
    }
}

} // namespace

bool ParallelFor(std::size_t begin, std::size_t end, void (*function)(std::size_t index, void* userData), void* userData, std::size_t threadCount)
{
    if (function == nullptr || end < begin)
    {
        return false;
    }
    if (begin == end)
    {
        return true;
    }
    ThreadPool pool(threadCount == 0 ? HardwareConcurrency() : threadCount);
    if (!pool.IsRunning())
    {
        return false;
    }
    std::atomic<std::size_t> next(begin);
    ParallelRange range{begin, end, function, userData, &next};
    const std::size_t workers = pool.ThreadCount();
    for (std::size_t i = 0; i < workers; ++i)
    {
        if (!pool.Submit(ParallelRangeWorker, &range))
        {
            return false;
        }
    }
    return pool.WaitIdle();
}

} // namespace arctix::Thread
