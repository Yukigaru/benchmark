#include <mutex>
#include <benchmark/benchmark.h>

namespace benchmark {

void UseCharPointer(char const volatile *)
{
}

} // namespace benchmark

std::once_flag Benchmark::warnDebugMode;
