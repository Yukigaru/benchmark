# Overview
Benchmark - a lightweight C++ library for reliable benchmarking your code. Features:  
- Two types of benchmarks: synthetic and inline for real code
- Loaded CPU detection
- Automatic core warmup
- Automatic thread affinity
- Drop cache functionality
- "Do not optimize" macro
- Variable arguments
- CMake support

Platforms: Linux, Windows.

Maintained by Alexander Garustovich. Pull requests are welcome.

# Quick start

#### Example
```
#include <benchmark/benchmark.h>

BENCHMARK_MAIN()

BENCHMARK("My vector").ARGUMENT(N, 2, 4, 8, 16, 32) {
    std::vector<int> v;
    v.reserve(N);
    
    MEASURE {
        REPEAT(64) { v.push_back(i); }
    }
}
```

#### Output:
```
Name                        Iters    Min    Avg    StdDev   StatGood
---------------------------------------------------------------------
My vector                   3528     450ns  621ns  200ns      YES     
```

# Usage
BENCHMARK(){ ... } - macro defines a static benchmark object, might be called outside or inside of a function score
REPEAT(\<I\>) - execute a piece of code I times.

# Examples
```
| // setup BENCHMARK("name") {     // measured actions } // finalize                                      | - The block is called many times, setup and finalization are outside                                     |
|---------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------|
| BENCHMARK("name") {     // setup     MEASURE_ONCE {         // measured actions     }     // finalize } | - The whole block is executed many times - For each iteration setup, action and finalize are called once |
| BENCHMARK("name") {     // setup     MEASURE {         // measured actions     }     // finalize }      | - The whole block is executed many times - But the inside block is executed many times                   |
```

## Real code benchmark
```
ALWAYS_START_BENCHMARK()
HOTKEY_START_BENCHMARK()
MANUAL_START_BENCHMARK()

void MyClass::outerMethod() {
    INLINE_BENCHMARK
    ...
    innerMethod();
}

void MyClass::innerMethod() {
    INLINE_BENCHMARK
    ...
    obj->other();
}

void AnotherClass::other() {
    INLINE_BENCHMARK
    ...
}
```

```---------------------------------------------------------------------------
Name                        Iters    Min    Avg    StdDev   StatGood Total
---------------------------------------------------------------------------
MyClass::outerMethod         430     10ms   11ms    840mcs
  MyClass::innerMethod       200     1.2ms  1.23ms  494mcs
    AnotherClass::other       2      300ns  380ns    40ns
``
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
