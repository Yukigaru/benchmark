#pragma once
#include <algorithm>
#include <chrono>
#include <vector>
#include <cmath>
#include "detail/program_arguments.h"

/*
Usage:

{
    Benchmark b("_name");

    // _setup env
    b.run([&](){ ... }); // runs the lambda function many times and measures the timings
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

struct BenchmarkSetup {
    enum OutputStyle {
        Table,
        OneLine,
        Lines,
        Nothing
    };

    BenchmarkSetup():
        outputStyle(OutputStyle::Table),
        verbose(false),
        skipWarmup(false)
    {
    }
    
    BenchmarkSetup(int argc, const char **argv):
        BenchmarkSetup()
    {
        ProgramArguments args(argc, argv);
        
        std::string outputStyle = args.after("output");
        if (outputStyle == "lines") {
            outputStyle = OutputStyle::Lines;
        } else if (outputStyle == "oneline") {
            outputStyle = OutputStyle::OneLine;
        } else if (outputStyle == "table") {
            outputStyle = OutputStyle::Table;
        } else {
            fprintf(stderr, "Unexpected value of 'output' argument: %s\n", outputStyle.c_str());
        }
        
        verbose = args.contains("verbose");
        skipWarmup = args.contains("skipWarmup");
    }

    OutputStyle outputStyle;
    bool verbose;
    bool skipWarmup;
};

class Benchmark {
    using duration_mcs_t = std::chrono::steady_clock::rep;
    using duration_t = std::chrono::steady_clock::duration;

    std::string _name;
    BenchmarkSetup _setup;
    
    unsigned _totalIterations;
    unsigned _repeats;
    duration_t _totalTimeRun;
    duration_t _averageTime;
    duration_t _medianTime;
    duration_t _minimalTime;
    duration_t _maximalTime;
    duration_t _standardDeviation;

    std::vector<duration_t> _timeSamples;

public:
    Benchmark(const char *name_ = "")
        : Benchmark(BenchmarkSetup(), name_)
    {
    }
    
    Benchmark(const BenchmarkSetup &setup_, const char *name_ = "")
        : _name(name_)
        , _setup(setup_)
        , _totalIterations(0)
        , _repeats(1)
        , _totalTimeRun(0)
        , _averageTime(0)
        , _medianTime(0)
        , _minimalTime(0)
        , _maximalTime(0)
        , _standardDeviation(0)
    {
    }
    
    void warmupCpu() {
        if (_setup.verbose) {
            printf("Warming up CPU\n");
        }

        static const int WarmupTimeSec = 3;
        
        auto start = std::chrono::steady_clock::now(); // do nothing serious for N seconds cycle
        while (true) {
            int p = rand();
            auto end = std::chrono::steady_clock::now();
            if (end - start > std::chrono::seconds(WarmupTimeSec))
                break;
        }
    }

    // TODO: variadic template args
    // TODO: BENCHMARK macro
    // TODO: cpu core affinity?
    // TODO: remove deviations
    // TODO: unit-tests
    template <typename F>
    void run(F &&func)
    {
        printf("[Benchmark] '%s' started", _name.c_str());

        static constexpr int SamplesNumber = 1000;
        _timeSamples.reserve(256);

        bool firstRun = true;
        
        if (!_setup.skipWarmup) {
            warmupCpu();
        }
        
        auto lastTimeDotPrinted = std::chrono::steady_clock::now();
        
        // TODO: run until deviation is small
        for (unsigned i = 0; i < 15; i++) {
            auto timeStart = std::chrono::steady_clock::now();

            for (unsigned j = 0; j < _repeats; j++)
                func();

            auto timeEnd = std::chrono::steady_clock::now();
            auto sample = std::chrono::duration_cast<std::chrono::nanoseconds>(timeEnd - timeStart);
            if (sample < std::chrono::nanoseconds(1))
                sample = std::chrono::nanoseconds(10);

            if (firstRun && sample < std::chrono::milliseconds(1)) {
                _repeats = (int)((float)(1000 * 1000) / (float)sample.count() + 0.5f);
                i--;
                firstRun = false;
                continue;
            }

            _totalIterations += _repeats;
            firstRun = false;
            _totalTimeRun += sample;
            _timeSamples.push_back(sample);

            if (timeEnd - lastTimeDotPrinted > std::chrono::seconds(3)) {
                lastTimeDotPrinted = timeEnd;
                printf(".");
            }
        }

        if (_timeSamples.size() > SamplesNumber)
            _timeSamples.erase(_timeSamples.begin(), _timeSamples.begin() + (SamplesNumber - _timeSamples.size()));

        printf("\n");

        calculateTimings();
        printResults();
    }

    void debugAddSample(std::chrono::steady_clock::duration sample) {
        _timeSamples.push_back(sample);
        _totalTimeRun += sample;
        _totalIterations++;
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
        } else if (durationMcs < 10) {
            printf("%.2f mcs", (float)durationNs / 1000.0f);
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
        if (_timeSamples.empty())
            return false;

        _averageTime = duration_t(0);
        _minimalTime = _maximalTime = _timeSamples[0];

        // minimum, maximum and average
        for (auto sample : _timeSamples) {
            _averageTime += sample; // TODO: can it overflow?

            if (sample < _minimalTime)
                _minimalTime = sample;
            if (sample > _maximalTime)
                _maximalTime = sample;
        }

        _minimalTime /= _repeats;
        _maximalTime /= _repeats;
        _averageTime /= (_timeSamples.size() * _repeats);

        // standard deviation
        auto averageNs = std::chrono::duration_cast<std::chrono::nanoseconds>(_averageTime).count();
        unsigned long long sumOfSquares = 0;

        for (size_t i = 0; i < _timeSamples.size(); i++) {
            auto sampleNs = _timeSamples[i].count() / _repeats;

            auto d = (sampleNs > averageNs) ? (sampleNs - averageNs) : (averageNs - sampleNs);
            sumOfSquares += d * d;
        }
        sumOfSquares /= _timeSamples.size();
        long long stdDevNs = llround(sqrtl((double)sumOfSquares));
        _standardDeviation = std::chrono::nanoseconds(stdDevNs);

        // median
        std::sort(_timeSamples.begin(), _timeSamples.end());

        _medianTime = _timeSamples[_timeSamples.size() / 2] / _repeats;

        return true;
    }

    void printResults()
    {
        if (_repeats == 1)
            printf("Done %u iters, total ", _totalIterations);
        else
            printf("Done %u iters (%u per sample), total spent ", _totalIterations, _repeats);
        printTime(_totalTimeRun);
        printf("\n");

        printf("Avg    : ");
        printTime(_averageTime);
        if (_averageTime > std::chrono::milliseconds(1))
            printf(" (%.2f fps)\n", 1000.0f / std::chrono::duration_cast<std::chrono::milliseconds>(_averageTime).count());
        else
            printf("\n");

        printf("StdDev : ");
        printTime(_standardDeviation);
        printf("\n");

        printf("Median : ");
        printTime(_medianTime);
        printf("\n");

        printf("Min    : ");
        printTime(_minimalTime);
        printf("\n\n");
    }

    unsigned totalIterations() const {
        return _totalIterations;
    }

    unsigned repeats() const {
        return _repeats;
    }

    duration_t totalTimeRun() const {
        return _totalTimeRun;
    }

    duration_t averageTime() const {
        return _averageTime;
    }

    duration_t medianTime() const {
        return _medianTime;
    }

    duration_t minimalTime() const {
        return _minimalTime;
    }

    duration_t maximalTime() const {
        return _maximalTime;
    }

    duration_t standardDeviation() const {
        return _standardDeviation;
    }
};
