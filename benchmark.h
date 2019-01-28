#pragma once
#include <algorithm>
#include <chrono>
#include <vector>

/*
Usage:

{
    Benchmark b("name");

    // setup env
    b.run([](){ ... }); // runs many times and measures time
    // teardown env
}

*/

#if defined(__GNUC__)
#define BENCHMARK_UNUSED __attribute__((unused))
#define BENCHMARK_ALWAYS_INLINE __attribute__((always_inline))
#elif defined(_MSC_VER) && !defined(__clang__)
#define BENCHMARK_UNUSED
#define BENCHMARK_ALWAYS_INLINE __forceinline
#else
#define BENCHMARK_UNUSED
#define BENCHMARK_ALWAYS_INLINE
#endif

#if (!defined(__GNUC__) && !defined(__clang__)) || defined(__pnacl__) || defined(__EMSCRIPTEN__)
#define BENCHMARK_HAS_NO_INLINE_ASSEMBLY
#endif

namespace benchmark {

void UseCharPointer(char const volatile *);

// The DoNotOptimize(...) function can be used to prevent a value or
// expression from being optimized away by the compiler. This function is
// intended to add little to no overhead.
// See: https://youtu.be/nXaxk27zwlk?t=2441
#ifndef BENCHMARK_HAS_NO_INLINE_ASSEMBLY
template <class Tp>
inline BENCHMARK_ALWAYS_INLINE void DoNotOptimize(Tp const &value)
{
    asm volatile("" : : "r,m"(value) : "memory");
}

template <class Tp>
inline BENCHMARK_ALWAYS_INLINE void DoNotOptimize(Tp &value)
{
#if defined(__clang__)
    asm volatile("" : "+r,m"(value) : : "memory");
#else
    asm volatile("" : "+m,r"(value) : : "memory");
#endif
}

// Force the compiler to flush pending writes to global memory. Acts as an
// effective read/write barrier
inline BENCHMARK_ALWAYS_INLINE void ClobberMemory()
{
    asm volatile("" : : : "memory");
}
#elif defined(_MSC_VER)
template <class Tp>
inline BENCHMARK_ALWAYS_INLINE void DoNotOptimize(Tp const &value)
{
    benchmark::UseCharPointer(&reinterpret_cast<char const volatile &>(value));
    _ReadWriteBarrier();
}

inline BENCHMARK_ALWAYS_INLINE void ClobberMemory()
{
    _ReadWriteBarrier();
}
#else
template <class Tp>
inline BENCHMARK_ALWAYS_INLINE void DoNotOptimize(Tp const &value)
{
    benchmark::UseCharPointer(&reinterpret_cast<char const volatile &>(value));
}
#endif

} // namespace benchmark

class Benchmark {
    using duration_mcs_t = std::chrono::steady_clock::rep;

    const char *name;
    unsigned totalIterations;
    unsigned repeats;
    std::chrono::steady_clock::duration totalTimeRun;
    std::chrono::steady_clock::duration averageTime;
    std::chrono::steady_clock::duration medianTime;
    std::chrono::steady_clock::duration minimalTime;
    std::chrono::steady_clock::duration standardDeviation;
    std::vector<duration_mcs_t> timeSamples;
    

public:
    Benchmark(const char *name_)
        : name(name_)
        , totalIterations(0)
        , repeats(0)
        , totalTimeRun(0)
        , averageTime(0)
        , medianTime(0)
        , minimalTime(0)
        , standardDeviation(0)
    {
    }

    // TODO: CPU heat up before the test
    // TODO: variadic template args
    // TODO: BENCHMARK macro
    // TODO: cpu core affinity?
    // TODO: remove deviations
    template <typename F>
    void run(F &&func)
    {
        printf("[Benchmark] '%s' started", name);

        static constexpr int SamplesNumber = 1000;
        timeSamples.reserve(256);

        auto lastTimeDotPrinted = std::chrono::steady_clock::now();

        repeats = 1;
        bool firstRun = true;

        // TODO: run until deviation is small
        for (unsigned i = 0; i < 15; i++) {
            auto timeStart = std::chrono::steady_clock::now();

            for (unsigned j = 0; j < repeats; j++)
                func();

            auto timeEnd = std::chrono::steady_clock::now();
            auto timeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(timeEnd - timeStart);

            if (firstRun && timeNs < std::chrono::milliseconds(1)) {
                if (timeNs < std::chrono::nanoseconds(10))
                    timeNs = std::chrono::nanoseconds(10);
                repeats = (int)((float)(1000 * 1000) / (float)timeNs.count() + 0.5f);
                i--;
                firstRun = false;
                continue;
            }

            totalIterations += repeats;
            firstRun = false;
            totalTimeRun += timeNs;
            timeSamples.push_back((duration_mcs_t)timeNs.count());

            if (timeEnd - lastTimeDotPrinted > std::chrono::seconds(3)) {
                lastTimeDotPrinted = timeEnd;
                printf(".");
            }
        }

        if (timeSamples.size() > SamplesNumber)
            timeSamples.erase(timeSamples.begin(), timeSamples.begin() + (SamplesNumber - timeSamples.size()));

        printf("\n");
    }

    void printTime(std::chrono::steady_clock::duration duration)
    {
        auto durationNs = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
        auto durationMcs = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
        auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        auto durationSec = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
        auto durationMin = std::chrono::duration_cast<std::chrono::minutes>(duration).count();

        if (durationNs < 1000) {
            printf("%llu ns", (unsigned long long)durationNs);
        } else if (durationMcs < 1000) {
            printf("%llu mcs", (unsigned long long)durationMcs);
        } else if (durationMs < 10) {
            printf("%.2f ms", (float)durationMcs / 1000.0f);
        } else if (durationMs < 1000) {
            printf("%llu ms", (unsigned long long)durationMs);
        } else if (durationSec < 10) {
            printf("%.2f sec", (float)durationMs / 1000.0f);
        } else if (durationMin < 1) {
            printf("%llu sec", (unsigned long long)durationSec);
        } else {
            printf("%.2f min", (float)durationSec / 60.0f);
        }
    }

    bool calculateTimings()
    {
        if (timeSamples.empty())
            return false;

        averageTime = std::chrono::steady_clock::duration(0);
        minimalTime = std::chrono::nanoseconds(timeSamples[0]);

        // minimum and average
        for (size_t i = 0; i < timeSamples.size(); i++) {
            auto timeNs = timeSamples[i];

            averageTime += std::chrono::nanoseconds(timeNs);

            if (timeNs < std::chrono::duration_cast<std::chrono::nanoseconds>(minimalTime).count())
                minimalTime = std::chrono::nanoseconds(timeNs);
        }

        minimalTime /= repeats;
        averageTime /= (timeSamples.size() * repeats);

        // standard deviation
        unsigned long long averageNs = std::chrono::duration_cast<std::chrono::nanoseconds>(averageTime).count();
        unsigned long long sumOfSquares = 0;

        for (size_t i = 0; i < timeSamples.size(); i++) {
            auto timeNs = timeSamples[i];
            timeNs /= repeats;
            auto d = ((unsigned long long)timeNs > averageNs) ? (timeNs - averageNs) : (averageNs - timeNs);

            sumOfSquares += d * d;
        }
        sumOfSquares /= timeSamples.size();
        unsigned long long stdDevNs = (unsigned long long)(sqrtl((double)sumOfSquares) + 0.5);
        standardDeviation = std::chrono::nanoseconds(stdDevNs);

        // median
        std::sort(timeSamples.begin(), timeSamples.end());

        auto medianSample = timeSamples[timeSamples.size() / 2];
        medianTime = std::chrono::nanoseconds(medianSample / repeats);

        return true;
    }

    void printResults()
    {
        if (repeats == 1)
            printf("Done %u iters, total ", totalIterations);
        else
            printf("Done %u iters (%u per sample), total spent ", totalIterations, repeats);
        printTime(totalTimeRun);
        printf("\n");

        printf("Avg    : ");
        printTime(averageTime);
        if (averageTime > std::chrono::milliseconds(1))
            printf(" (%.2f fps)\n", 1000.0f / std::chrono::duration_cast<std::chrono::milliseconds>(averageTime).count());
        else
            printf("\n");

        printf("StdDev : ");
        printTime(standardDeviation);
        printf("\n");

        printf("Median : ");
        printTime(medianTime);
        printf("\n");

        printf("Min    : ");
        printTime(minimalTime);
        printf("\n\n");
    }

    ~Benchmark()
    {
        calculateTimings();
        printResults();
    }
};
