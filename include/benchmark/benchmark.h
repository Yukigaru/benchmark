#pragma once
#include <algorithm>
#include <chrono>
#include <vector>
#include <cmath>
#include <mutex>
#include "detail/config.h"
#include "detail/dont_optimize.h"
#include "detail/benchmark_setup.h"
#include "detail/state.h"
#include "detail/statistics.h"
#include "detail/cpu_scaling.h"

/*
Usage:
{
    Benchmark b("Name");
    // << setup here
    b.run([&](){ ... }); // runs the lambda function many times and measures the timings
    // << teardown here
}

BENCHMARK(Name) {
	// << setup here
	MEASURE(
	    ...
	)
	// << teardown here
}
*/

class Benchmark {
    std::string _name;
    BenchmarkSetup _setup;

    TimeStatistics _stats;
    unsigned _totalIterations;

    unsigned Iterations;

    benchmark::duration_t _noopTime{0};

public:
    Benchmark(const char *name_ = "")
        : Benchmark(BenchmarkSetup(), name_)
    {
    }
    
    Benchmark(const BenchmarkSetup &setup_, const char *name_ = "")
        : _name(name_)
        , _setup(setup_)
        , _totalIterations(0)
        , Iterations(10)
    {
        // it seems that clock's now() takes longer when called first time
        auto init_timer = benchmark::clock_t::now();
        benchmark::DoNotOptimize(init_timer);
    }

    ~Benchmark() {
    }
    
    void warmupCpu() {
        static bool onlyOnce = false;
        if (onlyOnce)
            return;
        onlyOnce = true;

        printf("Warning: CPU power-safe mode enabled. Will try to warm up before the benchmark.\n");

        static const int WarmupTimeSec = 4;
        
        auto start = benchmark::clock_t::now(); // do nothing serious for N seconds cycle
        while (true) {
            unsigned p = rand();
            benchmark::DoNotOptimize(p);
            auto end = benchmark::clock_t::now();
            if (end - start > std::chrono::seconds(WarmupTimeSec))
                break;
        }
    }

    void findNoopTime() {
        _noopTime = std::chrono::nanoseconds(9999);
        for (int i = 0; i < 20; i++) {
            auto d = benchmark::clock_t::now() - benchmark::clock_t::now();
            if (d < _noopTime) {
                _noopTime = d;
            }
        }
    }

    virtual void vrun() {
    }

    // TODO: think out how to do tear-up/down
    // TODO: rework to run tests within main
    // TODO: remove deviations
    // TODO: get core load-average and print warning
    // TODO: calculate statistical significance
    // TODO: run until data is statistically significant
    // TODO: colorization
    // TODO: print (!) if stddev is too high or too low
    // TODO: after the first try, do more tries in order to calculate repeats more precisely
    template <typename F>
    void run(F &&func)
    {
#ifdef _DEBUG
#pragma message("Warning: Benchmark library is being compiled in a Debug configuration.")
        static std::once_flag warnDebugMode;
        std::call_once(warnDebugMode, [](){ printf("Warning: Running in a Debug configuration\n"); });
#endif
        if (!_setup.skipWarmup) {
            if (isCPUScalingEnabled()) {
                warmupCpu();
            }
        }

        findNoopTime();

        if (_setup.outputStyle == BenchmarkSetup::OutputStyle::Full)
            printf("[Benchmark '%s'] started", _name.c_str());

        benchmark::detail::BenchmarkState bs;

        while (bs.running()) {
            bool firstRun = true;
            unsigned repeats = 1;
            _totalIterations = 0;

            if (bs.variableArgsMode()) {
                bs.pickNextArgument();
            }
            _stats.clear();

            for (unsigned i = 0; i < Iterations;) {
                benchmark::detail::RunState state(bs, repeats, _noopTime);

                func(state);

                if (bs.needRestart()) // needed for ADD_ARG_RANGE functionality
                    break;

                auto sample = state.getSample();

                if (firstRun && sample < std::chrono::milliseconds(1)) {
                    //printf("repeats: %d, sample.count(): %d | ", (int)repeats, (int)sample.count());
                    if (sample.count() > 0) {
                        repeats = (unsigned)((1000.0 * 1000.0 * 100.0) / (double)sample.count());
                    } else {
                        repeats = 100000000;
                    }
                    _stats.setRepeats(repeats);
                    firstRun = false;
                    continue;
                }

                _totalIterations += repeats;
                firstRun = false;
                _stats.addSample(sample);
                i++;

                std::this_thread::sleep_for(std::chrono::milliseconds(10)); // give other processes chance to do their job, so that the scheduler is less willing to suspend ours

                printf(".");
                fflush(stdout);
            }

            if (!_stats.empty()) {
                calculateTimings();

                printf("\r");

                if (bs.variableArgsMode()) {
                    int varg1 = bs.getArg();
                    printResults(&varg1);
                } else {
                    printResults(nullptr);
                }
            }
        }
    }

    void debugAddSample(std::chrono::steady_clock::duration sample) {
        _stats.addSample(sample);
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
        return _stats.calculate();
    }

    void printResults(int *varg1 = nullptr)
    {
        if (_setup.outputStyle == BenchmarkSetup::OutputStyle::Full) {
            printf("[Benchmark '%s'] ", _name.c_str());
            if (_stats.repeats() == 1)
                printf("done %u iters, total ", _totalIterations);
            else
                printf("done %u iters (%u per sample), total spent ", _totalIterations, _stats.repeats());
            printTime(_stats.totalTimeRun());
            printf("\n");

            printf("Avg    : ");
            printTime(_stats.averageTime());
            if (_stats.averageTime() > std::chrono::milliseconds(1))
                printf(" (%.2f fps)\n",
                       1000.0f / std::chrono::duration_cast<std::chrono::milliseconds>(_stats.averageTime()).count());
            else
                printf("\n");

            printf("StdDev : ");
            printTime(_stats.standardDeviation());
            printf("\n");

            printf("Median : ");
            printTime(_stats.medianTime());
            printf("\n");

            printf("Min    : ");
            printTime(_stats.minimalTime());
            printf("\n");
        }
        else if (_setup.outputStyle == BenchmarkSetup::OutputStyle::OneLine) {
            if (!varg1) {
                // no variable arguments
                printf("[Benchmark '%s'] %u iters, ", _name.c_str(), _totalIterations);
            } else {
                printf("[Benchmark '%s' $1=%d] %u iters, ", _name.c_str(), *varg1, _totalIterations);
            }

            printf("avg: ");
            printTime(_stats.averageTime());
            if (_stats.averageTime() > std::chrono::milliseconds(1))
                printf(" (%.2f fps)",
                       1000.0f / std::chrono::duration_cast<std::chrono::milliseconds>(_stats.averageTime()).count());

            printf(", stddev: ");
            printTime(_stats.standardDeviation());

            printf(", min: ");
            printTime(_stats.minimalTime());

            printf("\n");
        }
        fflush(stdout);
    }

    unsigned totalIterations() const {
        return _totalIterations;
    }

    unsigned repeats() const {
        return _stats.repeats();
    }

    benchmark::duration_t totalTimeRun() const {
        return _stats.totalTimeRun();
    }

    benchmark::duration_t averageTime() const {
        return _stats.averageTime();
    }

    benchmark::duration_t medianTime() const {
        return _stats.medianTime();
    }

    benchmark::duration_t minimalTime() const {
        return _stats.minimalTime();
    }

    benchmark::duration_t maximalTime() const {
        return _stats.maximalTime();
    }

    benchmark::duration_t standardDeviation() const {
        return _stats.standardDeviation();
    }
};

class BenchmarkSilo {
    using BenchmarkCont = std::vector<Benchmark *>;
    static BenchmarkCont *benchmarks; // TODO: check it's in BSS

public:
    static void registerBenchmark(Benchmark *pb) {
        if (!benchmarks) {
            benchmarks = new BenchmarkCont();
        }
        benchmarks->push_back(pb);
    }

    static int runAll() {
        for (auto benchmark : *benchmarks) {
            benchmark->vrun();
        }
        return 0;
    }

    static void deleteAll() {
        for (auto benchmark : *benchmarks) {
            delete benchmark;
        }
        delete benchmarks;
    }
};

#define BENCHMARK(Name) \
    struct Benchmark##Name: public Benchmark { \
        Benchmark##Name(const char *name) : Benchmark(name) { \
        } \
        \
        void vrun() override { \
            run(&Benchmark##Name::testedFunc); \
        } \
        static BENCHMARK_ALWAYS_INLINE void testedFunc(benchmark::detail::RunState &); \
    }; \
    struct RegisterBenchmark##Name { \
        RegisterBenchmark##Name() { \
            BenchmarkSilo::registerBenchmark(new Benchmark##Name(#Name)); \
        } \
    } __registerBenchmark##Name; \
    \
    void BENCHMARK_ALWAYS_INLINE Benchmark##Name::testedFunc(benchmark::detail::RunState &state)

#define MEASURE_START state.start();
#define MEASURE_STOP state.stop();

#define MEASURE(code) { MEASURE_START; for (unsigned j = 0; j < state.repeats(); j++){ code; } MEASURE_STOP; }
//#define MEASURE_ONCE(code) MEASURE_START; { code; } MEASURE_STOP;

#define REPEAT(n) for (unsigned i = 0; i < n; ++i)

#define ADD_ARG_RANGE(from, to) if (state.addArgument(from, to)) return; MEASURE_START
#define ARG1 state.arg1()

#define RUN_BENCHMARKS BenchmarkSilo::runAll();
#define BENCHMARK_MAIN int main(int argc, char **argv) { int ret = RUN_BENCHMARKS; BenchmarkSilo::deleteAll(); return ret; }

#define BENCHMARK_STATE benchmark::detail::RunState &state