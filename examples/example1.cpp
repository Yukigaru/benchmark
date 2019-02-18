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
    std::vector<int> v;
    MEASURE_ONCE( REPEAT(164){ v.push_back(i); } )

    benchmark::DoNotOptimize(si);
}

BENCHMARK(V2) {
    std::vector<int> v;
    v.reserve(32);
    MEASURE_ONCE( REPEAT(164){ v.push_back(i); } )

    benchmark::DoNotOptimize(si);
}

BENCHMARK(V3) {
    std::vector<int> v;
    v.reserve(64);
    MEASURE_ONCE( REPEAT(164){ v.push_back(i); } )

    benchmark::DoNotOptimize(si);
}

int main() {
}