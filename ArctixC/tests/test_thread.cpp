// ArctixC.Thread lifecycle and concurrency tests.
// Links ArctixC::ArctixC.

#include <arctixc/c_api/thread.h>
#include <arctixc/thread/thread.hpp>

#include <atomic>
#include <cstdio>
#include <vector>

static int g_failures = 0;

#define CHECK(cond)                                                                 \
    do                                                                              \
    {                                                                               \
        if (!(cond))                                                                \
        {                                                                           \
            std::printf("FAIL line %d: %s\n", __LINE__, #cond);                      \
            ++g_failures;                                                           \
        }                                                                           \
    } while (0)

static void SetFlag(void* userData)
{
    static_cast<std::atomic<bool>*>(userData)->store(true);
}

static void AddToCounter(void* userData)
{
    static_cast<std::atomic<int>*>(userData)->fetch_add(1);
}

static void ThrowWorker(void*)
{
    throw 42;
}

static void SleepyWorker(void*)
{
    arctix::ThreadSleepFor(50);
}

static void TestStartJoin()
{
    arctix::Thread thread;
    CHECK(!thread.IsJoinable());
    CHECK(!thread.Join());   // invalid lifecycle: join before start fails
    CHECK(!thread.Detach()); // invalid lifecycle: detach before start fails

    std::atomic<bool> ran{ false };
    CHECK(thread.Start(SetFlag, &ran));
    CHECK(thread.IsJoinable());
    CHECK(!thread.Start(SetFlag, &ran)); // second start while running fails
    CHECK(thread.Join());
    CHECK(ran.load());
    CHECK(!thread.IsJoinable());
    CHECK(!thread.Join()); // double join fails gracefully
}

static void TestUserData()
{
    arctix::Thread thread;
    std::atomic<int> counter{ 0 };
    CHECK(thread.Start(AddToCounter, &counter));
    CHECK(thread.Join());
    CHECK(counter.load() == 1);

    CHECK(!thread.Start(nullptr, nullptr)); // null function fails
}

static void TestDetach()
{
    arctix::Thread thread;
    std::atomic<bool> ran{ false };
    CHECK(thread.Start(SetFlag, &ran));
    CHECK(thread.Detach());
    CHECK(!thread.IsJoinable());
    CHECK(!thread.Join()); // join after detach fails gracefully
    // Detached thread may still be running; poll briefly for completion.
    for (int i = 0; i < 100 && !ran.load(); ++i)
    {
        arctix::ThreadSleepFor(10);
    }
    CHECK(ran.load());
}

static void TestIdsAndConcurrency()
{
    std::thread::id mainId = arctix::CurrentThreadId();
    CHECK(mainId != std::thread::id{});

    arctix::Thread thread;
    std::atomic<bool> ran{ false };
    CHECK(thread.Start(SetFlag, &ran));
    CHECK(thread.GetId() != std::thread::id{});
    CHECK(thread.GetId() != mainId);
    CHECK(thread.Join());

    arctix::ThreadYield(); // must not crash
    arctix::ThreadSleepFor(1);
    (void)arctix::HardwareConcurrency(); // informational, no assert on value
}

static void TestConcurrentThreads()
{
    constexpr int kThreads = 8;
    std::atomic<int> counter{ 0 };
    std::vector<arctix::Thread> threads(kThreads);
    for (int i = 0; i < kThreads; ++i)
    {
        CHECK(threads[i].Start(AddToCounter, &counter));
    }
    for (int i = 0; i < kThreads; ++i)
    {
        CHECK(threads[i].Join());
    }
    CHECK(counter.load() == kThreads);
}

static void TestIsRunning()
{
    arctix::Thread thread;
    CHECK(!thread.IsRunning());
    CHECK(thread.Start(SleepyWorker, nullptr));
    // Worker sleeps 50ms; running flag should be observable. Poll briefly.
    bool observed = thread.IsRunning();
    for (int i = 0; i < 50 && !observed; ++i)
    {
        arctix::ThreadSleepFor(1);
        observed = thread.IsRunning();
    }
    CHECK(observed);
    CHECK(thread.Join());
    CHECK(!thread.IsRunning());
}

static void TestExceptionBoundary()
{
    arctix::Thread thread;
    CHECK(thread.Start(ThrowWorker, nullptr));
    CHECK(thread.Join()); // must not terminate the process
    CHECK(thread.HasThrown());
}

static void TestMoveSemantics()
{
    std::atomic<bool> ran{ false };
    arctix::Thread a;
    CHECK(a.Start(SetFlag, &ran));
    arctix::Thread b(std::move(a)); // ownership transfers
    CHECK(!a.IsJoinable());
    CHECK(b.IsJoinable());
    CHECK(b.Join());
    CHECK(ran.load());
}

static void TestCApi()
{
    ArctixC_ThreadHandle* handle = ArctixC_ThreadCreate();
    CHECK(handle != nullptr);
    CHECK(ArctixC_ThreadIsJoinable(handle) == 0);
    CHECK(ArctixC_ThreadJoin(handle) == 0); // invalid: join before start
    CHECK(ArctixC_ThreadStart(nullptr, SetFlag, nullptr) == 0);
    CHECK(ArctixC_ThreadStart(handle, nullptr, nullptr) == 0);

    std::atomic<bool> ran{ false };
    CHECK(ArctixC_ThreadStart(handle, SetFlag, &ran) == 1);
    CHECK(ArctixC_ThreadIsJoinable(handle) == 1);
    CHECK(ArctixC_ThreadJoin(handle) == 1);
    CHECK(ran.load() == true);
    CHECK(ArctixC_ThreadHasThrown(handle) == 0);

    CHECK(ArctixC_ThreadCurrentId() != 0);
    ArctixC_ThreadYield();
    ArctixC_ThreadSleepMs(1);
    (void)ArctixC_ThreadHardwareConcurrency();
    ArctixC_ThreadDestroy(handle);
    ArctixC_ThreadDestroy(nullptr); // NULL-safe
}

int main()
{
    TestStartJoin();
    TestUserData();
    TestDetach();
    TestIdsAndConcurrency();
    TestConcurrentThreads();
    TestIsRunning();
    TestExceptionBoundary();
    TestMoveSemantics();
    TestCApi();
    if (g_failures == 0)
    {
        std::printf("THREAD-TESTS-OK\n");
        return 0;
    }
    std::printf("THREAD-TESTS-FAILED (%d)\n", g_failures);
    return 1;
}
