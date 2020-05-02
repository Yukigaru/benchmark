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
#include "detail/chrono_utils.h"

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
            : Benchmark(BenchmarkSetup(), name_) {
    }

    Benchmark(const BenchmarkSetup &setup_, const char *name_ = "")
            : _name(name_), _setup(setup_), _totalIterations(0), Iterations(200) {
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

    // TODO: run until data is statistically significant
    // TODO: uplift own priority
    // TODO: add output styles classes
    // TODO: add output printer: cout, csv
    // TODO: make statistics template generic
    // TODO: set core affinity
    // TODO: recommend disabling turbo boost? detect turbo boost?
    // TODO: recommend disabling hyper-threadgin
    // TODO: recommend disabling address space randomization?
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
            _totalIterations = 0;
            auto startTime = std::chrono::steady_clock::now();

            if (bs.variableArgsMode()) {
                bs.pickNextArgument();
            }
            _stats.clear();

            for (unsigned i = 0; i < Iterations;) {
                benchmark::detail::RunState state(bs, _noopTime);

                func(state);

                if (bs.needRestart()) // needed for ADD_ARG_RANGE functionality
                    break;

                benchmark::duration_t sample = state.getSample();

                _totalIterations++;
                firstRun = false;
                _stats.addSample(sample);
                i++;

                // give other processes chance to do their job, so that the scheduler is less willing to suspend ours
                std::this_thread::sleep_for(std::chrono::milliseconds(10));

                if (std::chrono::steady_clock::now() - startTime > std::chrono::seconds(2))
                    break;

                std::cout << (i % 5 ? "" : ".");
                std::cout.flush();
            }

            if (!_stats.empty()) {
                calculateTimings();

                std::cout << "\r";
                std::cout.flush();

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

    bool calculateTimings() {
        return _stats.calculate();
    }

    void printResults(const int *varg1 = nullptr) {
        auto oldPrecision = std::cout.precision();

        if (_setup.outputStyle == BenchmarkSetup::OutputStyle::Full) {
            if (!varg1) {
                std::cout << "[Benchmark '" << _name << "'] done ";
            } else {
                std::cout << "[Benchmark '" << _name << "' $1=" << *varg1 << "] done ";
            }

            std::cout << benchmark::io::Iterations{_totalIterations} << " iters";
            std::cout << ", total spent " << _stats.totalTimeRun() << "\n";

            std::cout << "Avg    : " << _stats.averageTime();
            if (_stats.averageTime() > std::chrono::milliseconds(1)) {
                std::cout << " (" << std::setprecision(2)
                          << 1000000.0f /
                             std::chrono::duration_cast<std::chrono::microseconds>(_stats.averageTime()).count()
                          << " fps)\n";
            } else {
                std::cout << "\n";
            }

            std::cout << "StdDev : " << benchmark::io::ColoredDuration{_stats.standardDeviation(),
                                                                       _stats.highDeviation()
                                                                       ? benchmark::detail::ColorRed
                                                                       : benchmark::detail::ColorLightGreen};

            if (_stats.standardDeviationLevel() >= 0.01f) {
                std::cout << " (" << (int) (_stats.standardDeviationLevel() * 100.0) << "%)";
            } else {
                std::cout << std::setprecision(1)
                          << " (" << (float) (_stats.standardDeviationLevel() * 100.0) << "%)";
            }
            std::cout << "\n";
            std::cout << "Median : " << _stats.medianTime() << "\n";
            std::cout << "90th   : " << _stats.percentile(90) << "\n";
            std::cout << "Min    : " << _stats.minimalTime() << "\n";
            std::cout << "Max    : " << _stats.maximalTime() << std::endl;

        } else if (_setup.outputStyle == BenchmarkSetup::OutputStyle::OneLine) {
            if (!varg1) {
                std::cout << "[Benchmark '" << _name << "'] ";
            } else {
                std::cout << "[Benchmark '" << _name << "' $1=" << *varg1 << "] ";
            }

            std::cout << benchmark::io::Iterations{_totalIterations} << " iters";

            std::cout << ", avg: " << _stats.averageTime();
            if (_stats.averageTime() > std::chrono::milliseconds(1)) {
                std::cout << " (" << std::setprecision(2)
                          << (1000000.0f /
                              std::chrono::duration_cast<std::chrono::microseconds>(_stats.averageTime()).count())
                          << " fps)";
            }

            std::cout << ", 90th: " << _stats.percentile(90);

            std::cout << ", stddev: " << benchmark::io::ColoredDuration{_stats.standardDeviation(),
                                                                        _stats.highDeviation()
                                                                        ? benchmark::detail::ColorRed
                                                                        : benchmark::detail::ColorLightGreen};
            if (_stats.standardDeviationLevel() >= 0.01f) {
                std::cout << " (" << (int) (_stats.standardDeviationLevel() * 100.0) << "%)";
            } else {
                std::cout << " (" << std::setprecision(1) << (float) (_stats.standardDeviationLevel() * 100.0) << "%)";
            }

            std::cout << ", min: " << _stats.minimalTime() << std::endl;
        }
        std::cout << std::setprecision(oldPrecision);
    }

    void printCPULoad() {
        std::cout << "CPU usage:\n";

        auto cpuLoad = benchmark::detail::getCPULoad();

        for (int i = 0; i < cpuLoad->numCores; i++) {
            float loadRel = cpuLoad->loadByCore[i];

            benchmark::detail::ColorTag color = benchmark::detail::selectColorForCPULoad(loadRel);
            std::cout << "[Core " << i << ": " << color << (int) (loadRel * 100.0f) << "%"
                      << benchmark::detail::ColorReset << "] ";

            if (i % 4 == 0 && i > 0) // split by a column
                std::cout << "\n";
        }
        std::cout << "\n";
        for (int i = 0; i < cpuLoad->numCores; i++) {
            int curFreq = cpuLoad->freqByCore[i].curFreq;
            int maxFreq = cpuLoad->freqByCore[i].maxFreq;

            float freqRel = 0.0f;
            if (curFreq > 0 && maxFreq > 0)
                freqRel = (float) curFreq / (float) maxFreq;

            benchmark::detail::ColorTag color = benchmark::detail::selectColorForCPUFreq(freqRel);
            std::cout << "[Freq " << i << ": " << color << (int) (freqRel * 100.0f) << "%"
                      << benchmark::detail::ColorReset << "] ";

            if (i % 4 == 0 && i > 0) // split by a column
                std::cout << "\n";
        }
        std::cout << "\n\n";
    }

    unsigned totalIterations() const {
        return _totalIterations;
    }

    const TimeStatistics & statistics() const {
        return _stats;
    }
};

class BenchmarkSilo {
    using BenchmarkCont = std::vector<Benchmark *>;
    static BenchmarkCont *benchmarks;

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

#define MEASURE(code) { MEASURE_START; { code; } MEASURE_STOP; }

#define REPEAT(n) for (unsigned i = 0; i < n; ++i)

#define ADD_ARG_RANGE(from, to) if (state.addArgument(from, to)) return; MEASURE_START
#define ARG1 state.arg1()

#define RUN_BENCHMARKS BenchmarkSilo::runAll();
#define BENCHMARK_MAIN int main(int argc, char **argv) { int ret = RUN_BENCHMARKS; BenchmarkSilo::deleteAll(); return ret; }

#define BENCHMARK_STATE benchmark::detail::RunState &state