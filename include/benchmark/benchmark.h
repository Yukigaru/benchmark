#pragma once

#include <algorithm>
#include <chrono>
#include <cerrno>
#include <cstdlib>
#include <vector>
#include <cmath>
#include <mutex>
#include <thread>
#include <iostream>
#include <iomanip>
#include <memory>
#include <string>
#include <utility>
#include "detail/config.h"
#include "detail/dont_optimize.h"
#include "detail/benchmark_setup.h"
#include "detail/state.h"
#include "detail/statistics.h"
#include "detail/cpu_info.h"
#include "detail/colorization.h"
#include "detail/chrono_utils.h"

#if defined(__unix__) || defined(__APPLE__)
#include <sys/resource.h>
#endif

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

namespace benchmark {
namespace detail {

class ScopedPriority {
#if defined(__unix__) || defined(__APPLE__)
    int _originalPriority{0};
    bool _changed{false};
#endif

public:
    ScopedPriority(bool enabled, bool verbose)
    {
#if defined(__unix__) || defined(__APPLE__)
        if (!enabled)
            return;

        errno = 0;
        _originalPriority = getpriority(PRIO_PROCESS, 0);
        if (errno != 0 || setpriority(PRIO_PROCESS, 0, -20) != 0) {
            if (verbose)
                std::cerr << "Could not raise process priority (code " << errno << ")\n";
            return;
        }
        _changed = true;
#else
        (void)enabled;
        (void)verbose;
#endif
    }

    ~ScopedPriority()
    {
#if defined(__unix__) || defined(__APPLE__)
        if (_changed)
            setpriority(PRIO_PROCESS, 0, _originalPriority);
#endif
    }

    ScopedPriority(const ScopedPriority &) = delete;
    ScopedPriority &operator=(const ScopedPriority &) = delete;
};

} // namespace detail


class Benchmark {
    std::string _name;
    BenchmarkSetup _setup;

    TimeStatistics _stats;
    unsigned _totalIterations;

    benchmark::duration_t _noopTime{0};

public:
    Benchmark(const char *name_ = "")
            : Benchmark(BenchmarkSetup(), name_) {
    }

    Benchmark(const BenchmarkSetup &setup_, const char *name_ = "")
            : _name(name_), _setup(setup_), _totalIterations(0) {
        // clock's now() takes longer when called first time
        auto init_timer = benchmark::clock_t::now();
        benchmark::DoNotOptimize(init_timer);
    }

    virtual ~Benchmark() = default;

    void warmupCpu() {
        static bool onlyOnce = false;
        if (onlyOnce) // not supposed to be thread-safe, that's fine
            return;
        onlyOnce = true;

        std::cout << benchmark::detail::ColorLightRed
                  << "Warning: CPU power-safe mode enabled. Will try to warm up before the benchmark."
                  << benchmark::detail::ColorReset
                  << std::endl;

        auto start = benchmark::clock_t::now(); // do nothing serious for N seconds cycle
        while (true) {
            unsigned p = static_cast<unsigned>(std::rand());
            benchmark::DoNotOptimize(p);
            if (benchmark::clock_t::now() - start > _setup.warmupTime)
                break;
        }
    }

    void findNoopTime() {
        _noopTime = benchmark::duration_t::max();
        for (int i = 0; i < 20; i++) {
            const auto start = benchmark::clock_t::now();
            const auto end = benchmark::clock_t::now();
            const auto d = end - start;
            if (d < _noopTime) {
                _noopTime = d;
            }
        }
    }

    virtual void vrun() {
    }

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


        static bool printedCpuLoad = false;
        if (!printedCpuLoad && _setup.showCpuInfo &&
            _setup.outputStyle != BenchmarkSetup::OutputStyle::Nothing) {
            printedCpuLoad = true;
            printCPULoad();
        }
        benchmark::detail::ScopedPriority priority(_setup.adjustPriority, _setup.verbose);

        findNoopTime();

        if (_setup.outputStyle == BenchmarkSetup::OutputStyle::Full)
            std::cout << "[Benchmark '" << _name << "'] started" << std::endl;

        benchmark::detail::BenchmarkState bs;

        while (bs.running()) {
            _totalIterations = 0;
            auto startTime = std::chrono::steady_clock::now();

            if (bs.variableArgsMode()) {
                bs.pickNextArgument();
            }
            _stats.clear();

            for (unsigned i = 0; i < _setup.iterations;) {
                // Let other processes run
                std::this_thread::sleep_for(std::chrono::milliseconds(10));

                benchmark::detail::RunState state(bs, _noopTime);

                state.start();
                func(state);
                state.stop();

                if (bs.needRestart()) // needed for ADD_ARG_RANGE functionality
                    break;

                benchmark::duration_t sample = state.getSample();

                _totalIterations++;
                _stats.addSample(sample);
                i++;

                if (std::chrono::steady_clock::now() - startTime > _setup.timeLimit)
                    break;

                if (_setup.outputStyle == BenchmarkSetup::OutputStyle::Full) {
                    std::cout << (i % 5 ? "" : ".");
                    std::cout.flush();
                }
            }

            if (!_stats.empty()) {
                calculateTimings();

                if (_setup.outputStyle != BenchmarkSetup::OutputStyle::Nothing) {
                    std::cout << "\r";
                    std::cout.flush();
                }

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
        const std::ios::fmtflags oldFlags = std::cout.flags();
        auto oldPrecision = std::cout.precision();
        std::cout << std::fixed; // disable scientific notation

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
                std::cout << " (" << std::setprecision(3)
                          << 1000000.0 /
                             static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(_stats.averageTime()).count())
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
                std::cout << " (" << std::setprecision(3)
                          << (1000000.0 /
                              static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(_stats.averageTime()).count()))
                          << " fps)";
            }

            std::cout << ", 90th: " << _stats.percentile(90);

            std::cout << ", stddev: " << benchmark::io::ColoredDuration{_stats.standardDeviation(),
                                                                        _stats.highDeviation()
                                                                        ? benchmark::detail::ColorRed
                                                                        : benchmark::detail::ColorReset};
            if (_stats.standardDeviationLevel() >= 0.01f) {
                std::cout << " (" << (int) (_stats.standardDeviationLevel() * 100.0) << "%)";
            } else {
                std::cout << " (" << std::setprecision(1) << (float) (_stats.standardDeviationLevel() * 100.0) << "%)";
            }

            std::cout << ", min: " << _stats.minimalTime() << std::endl;
        }
        std::cout.flags(oldFlags);
        std::cout << std::setprecision(static_cast<int>(oldPrecision));
    }

    void printCPULoad() {
        std::cout << "CPU usage:\n";
        auto cpuLoad = benchmark::detail::getCPULoad();
        std::cout << cpuLoad;
        std::cout << "\n\n";
    }

    unsigned totalIterations() const {
        return _totalIterations;
    }

    const TimeStatistics & statistics() const {
        return _stats;
    }

    void setSetup(const BenchmarkSetup &setup) {
        _setup = setup;
    }
};

class BenchmarkSilo {
    using BenchmarkCont = std::vector<std::unique_ptr<Benchmark>>;

    static BenchmarkCont &benchmarks() {
        static BenchmarkCont result;
        return result;
    }

public:
    static void registerBenchmark(std::unique_ptr<Benchmark> registeredBenchmark) {
        benchmarks().push_back(std::move(registeredBenchmark));
    }

    static int runAll(const BenchmarkSetup &setup = BenchmarkSetup()) {
        for (auto &registeredBenchmark : benchmarks()) {
            registeredBenchmark->setSetup(setup);
            registeredBenchmark->vrun();
        }
        return 0;
    }
};

} // namespace benchmark

#define BENCHMARK(Name) \
    struct Benchmark##Name: public ::benchmark::Benchmark { \
        Benchmark##Name(const char *name) : ::benchmark::Benchmark(name) { \
        } \
        \
        void vrun() override { \
            run(&Benchmark##Name::testedFunc); \
        } \
        static inline BENCHMARK_ALWAYS_INLINE void testedFunc(::benchmark::detail::RunState &); \
    }; \
    struct RegisterBenchmark##Name { \
        RegisterBenchmark##Name() { \
            ::std::unique_ptr<::benchmark::Benchmark> registeredBenchmark(new Benchmark##Name(#Name)); \
            ::benchmark::BenchmarkSilo::registerBenchmark(::std::move(registeredBenchmark)); \
        } \
    } __registerBenchmark##Name; \
    \
    inline void BENCHMARK_ALWAYS_INLINE Benchmark##Name::testedFunc(::benchmark::detail::RunState &state)

#define MEASURE_START state.start();
#define MEASURE_STOP state.stop();

#define MEASURE(code) { MEASURE_START; { code; } MEASURE_STOP; }

#define REPEAT(n) for (unsigned i = 0; i < n; ++i)

#define ADD_ARG_RANGE(from, to) if (state.addArgument(from, to)) return; MEASURE_START
#define ARG1 state.arg1()

#define RUN_BENCHMARKS ::benchmark::BenchmarkSilo::runAll();
#define BENCHMARK_MAIN int main(int argc, char **argv) { return ::benchmark::BenchmarkSilo::runAll(::benchmark::BenchmarkSetup(argc, argv)); }

#define BENCHMARK_STATE benchmark::detail::RunState &state
