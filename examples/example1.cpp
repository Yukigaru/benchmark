#include <benchmark/benchmark.h>
#include <atomic>
#include <mutex>

static unsigned si = 0;

/*
BENCHMARK(Mutex)
{
    std::mutex m;
    MEASURE(
        m.lock();
        si++;
        m.unlock();
    )
    benchmark::DoNotOptimize(si);
}

BENCHMARK(Atomic) {
    std::atomic_int i = 0;
    MEASURE(
        i++;
    )
    benchmark::DoNotOptimize(i);
}

BENCHMARK(Nosync) {
    int i = 0;
    MEASURE(
        i++;
    )
    benchmark::DoNotOptimize(i);
}*/

BENCHMARK(V1) {
    ADD_ARG_RANGE(8, 2048);

}

BENCHMARK_MAIN