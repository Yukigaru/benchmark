#pragma once
#include <algorithm>
#include <chrono>
#include <vector>
#include <cmath>
#include "detail/config.h"
#include "detail/dont_optimize.h"
#include "detail/benchmark_setup.h"
#include "detail/sample_timer.h"
#include "detail/statistics.h"

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

    IVarInt *variable;

public:
    Benchmark(const char *name_ = "", benchmark::IVarInt* variable = nullptr)
        : Benchmark(BenchmarkSetup(), name_, variable)
    {
    }
    
    Benchmark(const BenchmarkSetup &setup_, const char *name_ = "", benchmark::IVarInt* variable = nullptr)
        : _name(name_)
        , _setup(setup_)
        , _totalIterations(0)
        , Iterations(10)
        , _run_once(false)
        , variable(variable)
    {
    }

    ~Benchmark() {
        if (variable)
            delete variable;
    }
    
    void warmupCpu() {
        static bool onlyOnce = false;
        if (onlyOnce)
            return;
        onlyOnce = true;

        if (_setup.verbose) {
            printf("Warming up CPU\n");
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

    // TODO: variadic template args
    // TODO: variable arguments
    // TODO: cpu core affinity?
    // TODO: remove deviations
    // TODO: unit-tests
    // TODO: get CORE load and print warning
    // TODO: calculate statistical significance
    // TODO: warning in DEBUG
    // TODO: detect CPU scaling
    // TODO: colorization
    // TODO: unit-test do nothing
    template <typename F>
    void run(F &&func)
    {
        if (_setup.outputStyle == BenchmarkSetup::OutputStyle::Full)
            printf("[Benchmark '%s'] started", _name.c_str());

        bool firstRun = true;
        unsigned repeats = 1;

        if (!_setup.skipWarmup) {
            warmupCpu();
        }

        if (_run_once) {
            Iterations = 1;
        }

        // TODO: run until data is statistically significant
        for (unsigned i = 0; i < Iterations; i++) {
            benchmark::detail::SampleTimer st(repeats);
            st.start();
            func(st);
            st.stop();

            auto sample = st.getSample();

            if (!_run_once && firstRun && sample < std::chrono::milliseconds(1)) {
                repeats = (int)(1000.0f * 1000.0f / (float)sample.count() + 0.5f);
                _stats.setRepeats(repeats);
                i--;
                firstRun = false;
                continue;
            }

            _totalIterations += _stats.repeats();
            firstRun = false;
            _stats.addSample(sample);
        }

        printf("\n");
        calculateTimings();
        printResults();
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

    void printResults()
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
            printf("[Benchmark '%s'] %u iters, ", _name.c_str(), _totalIterations);

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

namespace benchmark {
    class IVarInt {
    public:
        using int_type_t = long long;

        virtual ~IVarInt() {}

        virtual bool done() = 0;
        virtual int_type_t getNext() = 0;
    };

    class VarIntLinear: public IVarInt {
        int_type_t _value;
        int_type_t _to;
        int_type_t _step;

    public:
        VarIntLinear(int_type_t from, int_type_t to, int_type_t step):
            _value(from),
            _to(to),
            _step(step)
        {
        }

        bool done() override {
            return _value > _to;
        }

        int_type_t getNext() override {
            int_type_t ret = _value;
            _value += step;
            return ret;
        }
    };

    class VarIntLog2: public IVarInt {
        using int_type_t = long long;
        int_type_t _value;
        int_type_t _to;

    public:
        VarIntLog2(int_type_t from, int_type_t to):
            _value(from),
            _to(to)
        {
        }

        bool done() override {
            return _value > _to;
        }

        int_type_t getNext() override {
            int_type_t ret = _value;
            _value *= 2;
            return ret;
        }
    };

    class VarIntLog10: public IVarInt {
        using int_type_t = long long;
        int_type_t _value;
        int_type_t _to;

    public:
        VarIntLog10(int_type_t from, int_type_t to):
            _value(from),
            _to(to)
        {
        }

        bool done() override {
            return _value > _to;
        }

        int_type_t getNext() override {
            int_type_t ret = _value;
            _value *= 10;
            return ret;
        }
    };

}

#define BENCHMARK(Name, Var1) \
    static struct Benchmark##Name: public Benchmark { \
        Benchmark##Name(const char *name, IVarInt *variable = nullptr); \
        static BENCHMARK_ALWAYS_INLINE void testedFunc(benchmark::detail::SampleTimer &); \
    } tmp##Name(#Name); \
    \
    Benchmark##Name::Benchmark##Name(const char *name, IVarInt *variable) : Benchmark(name, variable) { \
        run(&Benchmark##Name::testedFunc); \
    } \
    \
    void BENCHMARK_ALWAYS_INLINE Benchmark##Name::testedFunc(benchmark::detail::SampleTimer &st)

#define MEASURE_START st.start();
#define MEASURE_STOP st.stop();

#define MEASURE(code) MEASURE_START; for (unsigned j = 0; j < st.repeats(); j++){ code; } MEASURE_STOP;
#define MEASURE_ONCE(code) MEASURE_START; { code; } MEASURE_STOP;

#define REPEAT(n) for (unsigned i = 0; i < n; ++i)

