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
#include "detail/variables.h"
#include "detail/cpu_scaling.h"

/*
Usage:

{
    Benchmark b("_name");
    // setup env
    b.run([&](){ ... }); // runs the lambda function many times and measures the timings
    // teardown env
}

BENCHMARK("Name") {
	// << setup here
	
	auto t = benchmark.start();
	...
	t.stop();
	
	// << teardown here
}
*/

class Benchmark {
    using duration_mcs_t = std::chrono::steady_clock::rep;
    using duration_t = std::chrono::steady_clock::duration;

    std::string _name;
    BenchmarkSetup _setup;

    TimeStatistics _stats;
    unsigned _totalIterations;

    unsigned Iterations;
    bool _run_once;

    static std::once_flag warnDebugMode;

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
        , _run_once(false)
    {
    }

    ~Benchmark() {
    }
    
    void warmupCpu() {
        static bool onlyOnce = false;
        if (onlyOnce)
            return;
        onlyOnce = true;

        if (_setup.verbose) {
            printf("Warming up CPU.\n");
        }

        static const int WarmupTimeSec = 4;
        
        auto start = std::chrono::steady_clock::now(); // do nothing serious for N seconds cycle
        while (true) {
            unsigned p = rand();
            benchmark::DoNotOptimize(p);
            auto end = std::chrono::steady_clock::now();
            if (end - start > std::chrono::seconds(WarmupTimeSec))
                break;
        }
    }

    // TODO: remove deviations
    // TODO: unit-tests
    // TODO: get core load and print warning
    // TODO: calculate statistical significance
    // TODO: colorization
    // TODO: do_nothing unit-test
    // TODO: change include/benchmark.h to include/benchmark/benchmark.h
    template <typename F>
    void run(F &&func)
    {
#ifdef _DEBUG
#pragma message("Warning: Benchmark library is being compiled in a Debug configuration.")
        std::call_once(warnDebugMode, [](){ printf("Warning: Running in a Debug configuration\n"; });
#endif

        if (!_setup.skipWarmup) {
            if (isCPUScalingEnabled()) {
                printf("Warning: CPU power-safe mode enabled. Will try to warm up before the benchmark.\n");
                warmupCpu();
            }
        }

        if (_setup.outputStyle == BenchmarkSetup::OutputStyle::Full)
            printf("[Benchmark '%s'] started", _name.c_str());

        if (_run_once) {
            Iterations = 1;
        }

        benchmark::detail::BenchmarkState bs;

        while (bs.running()) {
            bool firstRun = true;
            unsigned repeats = 1;

            if (bs.variableArgsMode()) {
                bs.pickNextArgument();
            }
            _stats.clear();

            // TODO: run until data is statistically significant
            for (unsigned i = 0; i < Iterations;) {
                benchmark::detail::RunState state(bs, repeats);

                state.start();
                func(state);
                state.stop();

                auto sample = state.getSample();

                if (bs.needRestart()) {
                    break;
                }

                if (!_run_once && firstRun && sample < std::chrono::milliseconds(1)) {
                    repeats = (unsigned)lround(1000.0f * 1000.0f / (float)sample.count());
                    _stats.setRepeats(repeats);
                    firstRun = false;
                    continue;
                }

                _totalIterations += _stats.repeats();
                firstRun = false;
                _stats.addSample(sample);
                i++;
            }

            if (!_stats.empty()) {
                printf("\n");
                calculateTimings();

                if (bs.variableArgsMode()) {
                    int varg1 = bs.getArg();
                    printResults(&varg1);
                } else {
                    printResults(nullptr);
                }
            }
        }
    }

    template <typename F>
    void run_once(F &&func) {
        _run_once = true;
        run(func);
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
            if (_stats.repeats() == 1)
                printf("Done %u iters, total ", _totalIterations);
            else
                printf("Done %u iters (%u per sample), total spent ", _totalIterations, _stats.repeats());
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
        }
    }

    unsigned totalIterations() const {
        return _totalIterations;
    }

    unsigned repeats() const {
        return _stats.repeats();
    }

    duration_t totalTimeRun() const {
        return _stats.totalTimeRun();
    }

    duration_t averageTime() const {
        return _stats.averageTime();
    }

    duration_t medianTime() const {
        return _stats.medianTime();
    }

    duration_t minimalTime() const {
        return _stats.minimalTime();
    }

    duration_t maximalTime() const {
        return _stats.maximalTime();
    }

    duration_t standardDeviation() const {
        return _stats.standardDeviation();
    }
};


#define BENCHMARK(Name) \
    static struct Benchmark##Name: public Benchmark { \
        Benchmark##Name(const char *name); \
        static BENCHMARK_ALWAYS_INLINE void testedFunc(benchmark::detail::RunState &); \
    } tmp##Name(#Name); \
    \
    Benchmark##Name::Benchmark##Name(const char *name) : Benchmark(name) { \
        run(&Benchmark##Name::testedFunc); \
    } \
    \
    void BENCHMARK_ALWAYS_INLINE Benchmark##Name::testedFunc(benchmark::detail::RunState &state)

#define MEASURE_START state.start();
#define MEASURE_STOP state.stop();

#define MEASURE(code) MEASURE_START; for (unsigned j = 0; j < state.repeats(); j++){ code; } MEASURE_STOP;
#define MEASURE_ONCE(code) MEASURE_START; { code; } MEASURE_STOP;

#define REPEAT(n) for (unsigned i = 0; i < n; ++i)

#define ADD_ARG_RANGE(from, to) if (state.addArgument(from, to)) return; MEASURE_START
#define ARG1 state.arg1()

#define BENCHMARK_MAIN int main(int argc, char **argv) { return 0; }
