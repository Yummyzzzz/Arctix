// Example: thread consumer.
// Links ArctixC::ArctixC. Ships with the single arctixc.dll.

#include <arctixc/thread/thread.hpp>

#include <cstdio>

namespace
{

void Worker(void* userData)
{
    int* counter = static_cast<int*>(userData);
    ++(*counter);
}

} // namespace

int main()
{
    arctix::Thread::Thread thread;
    int counter = 0;

    if (!thread.Start(Worker, &counter))
    {
        return 1;
    }
    if (!thread.Join())
    {
        return 1;
    }
    if (counter != 1)
    {
        return 1;
    }

    std::printf("thread-only example OK (counter=%d)\n", counter);
    return 0;
}
