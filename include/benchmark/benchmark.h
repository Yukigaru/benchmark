#pragma once
#include <algorithm>
#include <chrono>
#include <vector>
#include <cmath>
#include <mutex>
#include <thread>
#include <iostream>
#include <iomanip>
#include "detail/config.h"
#include "detail/dont_optimize.h"
#include "detail/benchmark_setup.h"
#include "detail/state.h"
#include "detail/statistics.h"
#include "detail/cpu_info.h"
#include "detail/colorization.h"

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
        , Iterations(200)
    {
        // clock's now() takes longer when called first time
        auto init_timer = benchmark::clock_t::now();
        benchmark::DoNotOptimize(init_timer);

        static bool onlyOnce = false;
        if (!onlyOnce) {
            onlyOnce = true;
            printCPULoad();
        }
    }

    ~Benchmark() {
    }

    void warmupCpu() {
        static bool onlyOnce = false;
        if (onlyOnce) // not supposed to be thread-safe, that's fine
            return;
        onlyOnce = true;

        std::cout << benchmark::detail::ColorLightRed
            << "Warning: CPU power-safe mode enabled. Will try to warm up before the benchmark."
            << benchmark::detail::ColorReset
            << std::endl;

        static const auto WarmupTime = std::chrono::seconds(4);

        auto start = benchmark::clock_t::now(); // do nothing serious for N seconds cycle
        while (true) {
            unsigned p = rand();
            benchmark::DoNotOptimize(p);
            if (benchmark::clock_t::now() - start > WarmupTime)
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

    // TODO: remove deviations
    // TODO: run until data is statistically significant
    // TODO: uplift own priority
    // TODO: add output printer: cout, csv
    template<typename F>
    void run(F &&func) {
#ifdef _DEBUG
#pragma message("Warning: Benchmark library is being compiled in a Debug configuration.")
        static std::once_flag warnDebugMode;
        std::call_once(warnDebugMode, [](){ std::cout << "Warning: Running in a Debug configuration" << std::endl; });
#endif
        if (!_setup.skipWarmup) {
            if (benchmark::detail::isCPUScalingEnabled()) {
                warmupCpu(); // TODO: check if it really works
            }
        }

        findNoopTime();

        if (_setup.outputStyle == BenchmarkSetup::OutputStyle::Full)
            std::cout << "[Benchmark '" << _name << "'] started" << std::endl;

        benchmark::detail::BenchmarkState bs;

        while (bs.running()) {
            bool firstRun = true;
            unsigned repeats = 1;
            _totalIterations = 0;
            auto startTime = std::chrono::steady_clock::now();

            if (bs.variableArgsMode()) {
                bs.pickNextArgument();
            }
            _stats.clear();

            for (unsigned i = 0; i < Iterations;) {
                benchmark::detail::RunState state(bs, repeats, _noopTime);

                func(state);

                if (bs.needRestart()) // needed for ADD_ARG_RANGE functionality
                    break;

                benchmark::duration_t sample = state.getSample();

                _totalIterations += repeats;
                firstRun = false;
                _stats.addSample(sample);
                i++;

                std::this_thread::sleep_for(std::chrono::milliseconds(10)); // give other processes chance to do their job, so that the scheduler is less willing to suspend ours

                if (std::chrono::steady_clock::now() - startTime > std::chrono::seconds(2))
                    break;

                std::cout << (i % 5 ? "" : ".") << std::endl;
            }

            if (!_stats.empty()) {
                calculateTimings();

                std::cout << "\r" << std::endl;

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

    void printTime(std::chrono::steady_clock::duration duration, benchmark::detail::ColorTag colorTag = benchmark::detail::ColorLightGreen) {
        auto durationNs = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
        auto durationMcs = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
        auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        auto durationSec = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
        auto durationMin = std::chrono::duration_cast<std::chrono::minutes>(duration).count();

        auto oldPrecision = std::cout.precision();

        std::cout << colorTag;
        if (durationNs < 1000) {
            std::cout << durationNs << " ns";
        } else if (durationMcs < 10) {
            std::cout << std::setprecision(2) << (float)durationNs / 1000.0f << " μs";
        } else if (durationMcs < 1000) {
            std::cout << durationMcs << " μs";
        } else if (durationMs < 10) {
            std::cout << std::setprecision(2) << (float)durationMcs / 1000.0f << " ms";
        } else if (durationMs < 1000) {
            std::cout << durationMs << " ms";
        } else if (durationSec < 10) {
            std::cout << std::setprecision(2) << (float)durationMs / 1000.0f << " sec";
        } else if (durationMin < 1) {
            std::cout << durationSec << " sec";
        } else {
            std::cout << std::setprecision(2) << (float)durationSec / 60.0f << " min";
        }
        std::cout << benchmark::detail::ColorReset << std::setprecision(oldPrecision) << std::endl;
    }

    bool calculateTimings() {
        return _stats.calculate();
    }

    void printResults(int *varg1 = nullptr) {
        // TODO: return precision
        if (_setup.outputStyle == BenchmarkSetup::OutputStyle::Full) {
            if (!varg1) {
                std::cout << "[Benchmark '" << _name << "'] done ";
            } else {
                std::cout << "[Benchmark '" << _name << "' $1=" << *varg1 << "] done ";
            }

            if (_stats.repeats() == 1) {
                printIterations(_totalIterations, "iters");
            } else {
                printIterations(_totalIterations, "iters");
                std::cout << " (";
                printIterations(_stats.repeats(), "per sample)");
            }
            std::cout << ", total spent ";
            printTime(_stats.totalTimeRun());
            std::cout << "\n";

            std::cout << "Avg    : ";
            printTime(_stats.averageTime());
            if (_stats.averageTime() > std::chrono::milliseconds(1)) {
                std::cout << " (" << std::setprecision(2)
                    << 1000000.0f / std::chrono::duration_cast<std::chrono::microseconds>(_stats.averageTime()).count()
                    << " fps)\n";
            } else {
                std::cout << "\n";
            }

            std::cout << "StdDev : ";
            printTime(_stats.standardDeviation(), _stats.highDeviation() ? benchmark::detail::ColorRed : benchmark::detail::ColorLightGreen);
            if (_stats.standardDeviationLevel() >= 0.01f) {
                std::cout << " (" << (int)(_stats.standardDeviationLevel() * 100.0) << "%)";
            } else {
                std::cout << std::setprecision(1)
                    << " (" << (float)(_stats.standardDeviationLevel() * 100.0) << "%)";
            }
            std::cout <<"\n";

            std::cout << "Median : ";
            printTime(_stats.medianTime());
            std::cout << "\n";

            std::cout << "90th   : ";
            printTime(_stats.percentile(90));
            std::cout << "\n";

            std::cout << "Min    : ";
            printTime(_stats.minimalTime());
            std::cout << "\n";

            std::cout << "Max    : ";
            printTime(_stats.maximalTime());
            std::cout << std::endl;

        } else if (_setup.outputStyle == BenchmarkSetup::OutputStyle::OneLine) {
            if (!varg1) {
                std::cout << "[Benchmark '" << _name << "'] ";
            } else {
                std::cout << "[Benchmark '" << _name << "' $1=" << *varg1 << "] ";
            }

            if (_stats.repeats() == 1) {
                printIterations(_totalIterations, "iters");
            } else {
                printIterations(_totalIterations, "iters");
                std::cout << " (";
                printIterations(_stats.repeats(), "per sample)");
            }

            std::cout << ", avg: ";
            printTime(_stats.averageTime());
            if (_stats.averageTime() > std::chrono::milliseconds(1)) {
                std::cout << " (" << std::setprecision(2)
                       << 1000000.0f / std::chrono::duration_cast<std::chrono::microseconds>(_stats.averageTime()).count()
                       << " fps)";
            }

            std::cout << ", 90th: ";
            printTime(_stats.percentile(90));

            std::cout << ", stddev: ";
            printTime(_stats.standardDeviation(), _stats.highDeviation() ? benchmark::detail::ColorRed : benchmark::detail::ColorLightGreen);
            if (_stats.standardDeviationLevel() >= 0.01f) {
                std::cout << " (" << (int)(_stats.standardDeviationLevel() * 100.0) << "%)";
            } else {
                std::cout << " (" << std::setprecision(1) << (float)(_stats.standardDeviationLevel() * 100.0) << "%)";
            }

            std::cout << ", min: ";
            printTime(_stats.minimalTime());

            std::cout << std::endl;
        }
    }

    void printIterations(size_t number, const char *suffix) {
    // TODO: restore precision
        if (number < 1000) {
            std::cout << number;
        } else if (number < 1000000) {
            if (number % 1000 < 100) {
                std::cout << number / 1000 << "k";
            } else {
                std::cout << std::setprecision(1) << (float)number / 1000.0f << "k";
            }
        } else { // millions
            std::cout << std::setprecision(1) << (float)number / 1000000.0f << "m";
        }
        std::cout << " " << suffix;
    }

    void printCPULoad() {
        std::cout << "CPU usage:\n";

        auto cpuLoad = benchmark::detail::getCPULoad();

        for (int i = 0; i < cpuLoad->numCores; i++) {
            float loadRel = cpuLoad->loadByCore[i];

            benchmark::detail::ColorTag color = benchmark::detail::selectColorForCPULoad(loadRel);
            std::cout << "[Core " << i << ": " << color << (int)(loadRel * 100.0f) << "%" << benchmark::detail::ColorReset << "] ";

            if (i % 4 == 0 && i > 0) // split by a column
                std::cout << "\n";
        }
        std::cout << "\n";
        for (int i = 0; i < cpuLoad->numCores; i++) {
            int curFreq = cpuLoad->freqByCore[i].curFreq;
            int maxFreq = cpuLoad->freqByCore[i].maxFreq;

            float freqRel = 0.0f;
            if (curFreq > 0 && maxFreq > 0)
                freqRel = (float)curFreq / (float)maxFreq;

            benchmark::detail::ColorTag color = benchmark::detail::selectColorForCPUFreq(freqRel);
            std::cout << "[Freq " << i << ": " << color << (int)(freqRel * 100.0f) << "%" << benchmark::detail::ColorReset << "] ";

            if (i % 4 == 0 && i > 0) // split by a column
                std::cout << "\n";
        }
        std::cout << "\n\n";
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
#define MEASURE_ONCE(code) { MEASURE_START; { code; } MEASURE_STOP; repeats = 1; }

#define REPEAT(n) for (unsigned i = 0; i < n; ++i)

#define ADD_ARG_RANGE(from, to) if (state.addArgument(from, to)) return; MEASURE_START
#define ARG1 state.arg1()

#define RUN_BENCHMARKS BenchmarkSilo::runAll();
#define BENCHMARK_MAIN int main(int argc, char **argv) { int ret = RUN_BENCHMARKS; BenchmarkSilo::deleteAll(); return ret; }

#define BENCHMARK_STATE benchmark::detail::RunState &state