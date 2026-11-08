#include "arctixc/thread/thread.hpp"

#include <atomic>
#include <chrono>
#include <mutex>
#include <utility>

namespace arctixs::Thread
{

// Private state for Thread (opaque to SDK users).
struct ThreadImpl
{
    std::thread thread{};
    std::mutex mutex{};
    std::atomic<bool> running{ false };
    std::atomic<bool> threw{ false };
};

Thread::Thread()
    : impl_(new ThreadImpl())
{
}

Thread::~Thread()
{
    // Safety net: never silently detach, never terminate. A still-joinable
    // thread is joined here (blocking). Prefer explicit Join()/Detach().
    if (impl_->thread.joinable())
    {
        impl_->thread.join();
    }
    delete impl_;
}

Thread::Thread(Thread&& other) noexcept
    : impl_(other.impl_)
{
    other.impl_ = new ThreadImpl();
}

Thread& Thread::operator=(Thread&& other) noexcept
{
    if (this != &other)
    {
        if (impl_->thread.joinable())
        {
            impl_->thread.join();
        }
        delete impl_;
        impl_ = other.impl_;
        other.impl_ = new ThreadImpl();
    }
    return *this;
}

bool Thread::Start(ThreadFunction function, void* userData)
{
    if (function == nullptr)
    {
        return false;
    }
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (impl_->thread.joinable() || impl_->running.load())
    {
        return false;
    }
    impl_->threw.store(false);
    try
    {
        impl_->thread = std::thread([impl = impl_, function, userData]() {
            impl->running.store(true);
            try
            {
                function(userData);
            }
            catch (...)
            {
                // Never let exceptions escape the thread entry boundary.
                impl->threw.store(true);
            }
            impl->running.store(false);
        });
    }
    catch (...)
    {
        return false;
    }
    return true;
}

bool Thread::Join()
{
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->thread.joinable())
    {
        return false;
    }
    impl_->thread.join();
    return true;
}

bool Thread::Detach()
{
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (!impl_->thread.joinable())
    {
        return false;
    }
    impl_->thread.detach();
    return true;
}

bool Thread::IsJoinable() const
{
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->thread.joinable();
}

bool Thread::IsRunning() const
{
    return impl_->running.load();
}

bool Thread::HasThrown() const
{
    return impl_->threw.load();
}

std::thread::id Thread::GetId() const
{
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->thread.get_id();
}

std::thread::id CurrentThreadId()
{
    return std::this_thread::get_id();
}

void ThreadYield()
{
    std::this_thread::yield();
}

void ThreadSleepFor(std::uint64_t milliseconds)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

unsigned int HardwareConcurrency()
{
    return std::thread::hardware_concurrency();
}

} // namespace arctixs::Thread
