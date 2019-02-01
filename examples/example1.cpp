#include <benchmark.h>
#include <atomic>
#include <mutex>

int main() {
    int i1 = 0;
    std::mutex m1;
    std::atomic_int i2(0);
    int i3 = 0;

    Benchmark b1("Mutex");
    b1.run([&](){
        m1.lock();
        i1++;
        m1.unlock();

        benchmark::DoNotOptimize(i1);
    });

    Benchmark b2("Atomic");
    b2.run([&](){
        i2++;

        benchmark::DoNotOptimize(i2);
    });

    Benchmark b3("No-sync");
    b3.run([&](){
        i3++;

        benchmark::DoNotOptimize(i3);
    });
}