#include <benchmark.h>
#include <atomic>
#include <mutex>

int main() {
    int i1 = 0;
    std::mutex m1;
    std::atomic_int i2(0);
    int i3 = 0;

    Benchmark b1("mutex");
    b1.run([&](){
        m1.lock();
        i1++;
        m1.unlock();

        benchmark::ClobberMemory();
    });

    Benchmark b2("atomic");
    b2.run([&](){
        i2++;

        benchmark::ClobberMemory();
    });

    Benchmark b3("no-sync");
    b3.run([&](){
        i3++;

        benchmark::ClobberMemory();
    });
}