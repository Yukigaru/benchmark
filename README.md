# Overview
[![Build Status](https://travis-ci.com/Yukigaru/benchmark.svg?branch=master)](https://travis-ci.com/Yukigaru/benchmark)

`Benchmark` is a lightweight C++ library for reliable benchmarking your code. Features:
- Variable arguments  
- Auto CPU warm up
- "Do not optimize" macro
- CPU frequency scaling detection
- CMake support

Platforms: Linux. Not tested on Windows.

# Quick start

#### Example
```
#include <benchmark/benchmark.h>

BENCHMARK("My vector") {
    std::vector<int> v;
    v.reserve(N);
    
    MEASURE(
        REPEAT(64) { v.push_back(i); }
    )
}

BENCHMARK_MAIN
```

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
