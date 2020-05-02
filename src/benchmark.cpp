#include <benchmark/benchmark.h>

BenchmarkSilo::BenchmarkCont *BenchmarkSilo::benchmarks;

namespace benchmark {

void UseCharPointer(char const volatile *)
{
}

} // namespace benchmark
