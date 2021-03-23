#include <benchmark/benchmark.h>

benchmark::BenchmarkSilo::BenchmarkCont *benchmark::BenchmarkSilo::benchmarks;

namespace benchmark {

void UseCharPointer(char const volatile *)
{
}

} // namespace benchmark
