#pragma once
#include <chrono>

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
    using clock_t = std::chrono::high_resolution_clock;
    using duration_t = clock_t::duration;
    using time_point_t = clock_t::time_point;
}