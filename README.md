# Overview
[![Build Status](https://travis-ci.com/Yukigaru/benchmark.svg?branch=master)](https://travis-ci.com/Yukigaru/benchmark)

`Benchmark` is a lightweight C++ library for reliable benchmarking your code. Features:
- Variable arguments  
- Auto CPU warm up
- "Do not optimize" macro
- CPU frequency scaling detection
- CMake support

Platforms: Linux and macOS. Windows builds without CPU telemetry.

# Quick start

#### Example
```cpp
#include <benchmark/benchmark.h>
#include <vector>

BENCHMARK(MyVector) {
    std::vector<int> v;
    v.reserve(64);
    
    MEASURE(
        REPEAT(64) { v.push_back(i); }
    )
}

BENCHMARK_MAIN
```

`BENCHMARK_MAIN` accepts these runner options:

- `--output full|oneline|nothing`
- `--iterations N`
- `--time-limit-ms N`
- `--warmup-ms N` or `--skip-warmup`
- `--high-priority` (opt-in; may require additional permissions)
- `--no-cpu-info` to skip CPU telemetry collection
- `--verbose`

The same settings are available through `benchmark::BenchmarkSetup` when using
`benchmark::BenchmarkSilo::runAll(setup)` directly.


# Notes
#### Things that may interfere with a benchmark
- Heavy applications such as a browser, IDE, VM. Better to shut those down before running a benchmark.

- Notebook unplugged to power source.

- CPU frequency scaling, powersafe mode
    > sudo cpupower frequency-set --governor performance
    
    You may also know your real current frequency on:
    Linux: i7z
    Windows: Task Manager, Performance tab.

- Swapping.

- Compiler optimizations.
