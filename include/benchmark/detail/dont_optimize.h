#pragma once
#include "config.h"

namespace benchmark {

    void UseCharPointer(char const volatile *);

// The DoNotOptimize(...) function can be used to prevent a value or
// expression from being optimized away by the compiler. This function is
// intended to add little to no overhead.
// See: https://youtu.be/nXaxk27zwlk?t=2441
#ifndef BENCHMARK_HAS_NO_INLINE_ASSEMBLY
    template <class Tp>
    inline BENCHMARK_ALWAYS_INLINE void DoNotOptimize(Tp const &value)
    {
        asm volatile("" : : "r,m"(value) : "memory");
    }

    template <class Tp>
    inline BENCHMARK_ALWAYS_INLINE void DoNotOptimize(Tp &value)
    {
#if defined(__clang__)
        asm volatile("" : "+r,m"(value) : : "memory");
#else
        asm volatile("" : "+m,r"(value) : : "memory");
#endif
    }

// Force the compiler to flush pending writes to global memory. Acts as an
// effective read/write barrier
    inline BENCHMARK_ALWAYS_INLINE void ClobberMemory()
    {
        asm volatile("" : : : "memory");
    }
#elif defined(_MSC_VER)

    template<class Tp>
    inline BENCHMARK_ALWAYS_INLINE void DoNotOptimize(Tp const &value) {
        benchmark::UseCharPointer(&reinterpret_cast<char const volatile &>(value));
        _ReadWriteBarrier();
    }

    inline BENCHMARK_ALWAYS_INLINE void ClobberMemory() {
        _ReadWriteBarrier();
    }

#else
    template <class Tp>
    inline BENCHMARK_ALWAYS_INLINE void DoNotOptimize(Tp const &value)
    {
        benchmark::UseCharPointer(&reinterpret_cast<char const volatile &>(value));
    }
#endif

} // namespace benchmark