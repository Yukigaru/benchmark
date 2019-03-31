#include <benchmark/benchmark.h>
#include <atomic>
#include <mutex>

#ifdef WIN32
#else
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#endif

BENCHMARK(Mutex) {
    static unsigned si = 0;
    std::mutex m;
    MEASURE(
        m.lock();
        si++;
        m.unlock();
    )
    benchmark::DoNotOptimize(si);
    benchmark::DoNotOptimize(m);
}

BENCHMARK(AtomicRelaxed) {
    std::atomic_int i(0);
    MEASURE(i.fetch_add(1, std::memory_order_relaxed);)
    benchmark::DoNotOptimize(i);
}

/*BENCHMARK(Nosync) {
    int i = 0;
    MEASURE(
        i++;
    )
    benchmark::DoNotOptimize(i);
}

BENCHMARK(SyscallClose) {
    MEASURE(
        close(123);
    )
}

BENCHMARK(SyscallGetTime) {
    timespec ts;
    clockid_t cid = CLOCK_PROCESS_CPUTIME_ID;
    MEASURE(
        clock_gettime(cid, &ts);
    )
}*/

int main() {
    RUN_BENCHMARKS
}
