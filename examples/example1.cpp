#include <atomic>
#include <benchmark/benchmark.h>
#include <mutex>
#include <list>
#include <vector>

#ifdef WIN32
#else
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#endif

BENCHMARK(Mutex)
{
    static unsigned si = 0;
    std::mutex m;
    MEASURE(m.lock(); si++; m.unlock();)
    benchmark::DoNotOptimize(si);
    benchmark::DoNotOptimize(m);
}

BENCHMARK(AtomicRelaxed)
{
    std::atomic_int i(0);
    MEASURE( i.fetch_add(1, std::memory_order_relaxed); )
    benchmark::DoNotOptimize(i);
}

BENCHMARK(SSO)
{
    ADD_ARG_RANGE(4, 32);
    char buf[] = "abcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabcabc";
    MEASURE(
        std::string s(buf, ARG1);
        benchmark::DoNotOptimize(s);
    )
}

BENCHMARK(ListTraversal)
{
    ADD_ARG_RANGE(8, 1024);
    std::list<int> l;
    for (int i = 0; i < ARG1; i++)
        l.push_back(i % 16);

    MEASURE(
        for (auto it = l.begin(), iend = l.end(); it != iend; ++it) {
            int n = *it;
            benchmark::DoNotOptimize(n);
        }
    )
}

BENCHMARK(VectorTraversal)
{
    ADD_ARG_RANGE(8, 1024);
    std::vector<int> v;
    for (int i = 0; i < ARG1; i++)
        v.push_back(i % 16);

    MEASURE(
        for (auto it = v.begin(), iend = v.end(); it != iend; ++it) {
            int n = *it;
            benchmark::DoNotOptimize(n);
        }
    )
}

#ifdef UNIX
BENCHMARK(SyscallGetTime)
{
    timespec ts;
    clockid_t cid = CLOCK_PROCESS_CPUTIME_ID;
    MEASURE(clock_gettime(cid, &ts);)
}
#endif

int main()
{
    RUN_BENCHMARKS
}
