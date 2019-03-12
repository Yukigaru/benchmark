#include <benchmark.h>
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
    ADD_ARG_RANGE(1, 64);

    std::vector<int> v;
    v.reserve(ARG1);
    MEASURE_ONCE( REPEAT(164){ v.push_back(i); } )

    benchmark::DoNotOptimize(si);
}

int main() {
}