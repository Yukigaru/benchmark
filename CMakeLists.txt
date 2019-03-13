cmake_minimum_required(VERSION 3.2)
project(benchmark)

option(WITH_EXAMPLES "Build examples" ON)
option(WITH_TESTS "Build tests" ON)

add_library(benchmark STATIC
    src/benchmark.cpp
    include/benchmark/benchmark.h
    include/benchmark/detail/benchmark_setup.h
    include/benchmark/detail/config.h
    include/benchmark/detail/cpu_scaling.h
    include/benchmark/detail/dont_optimize.h
    include/benchmark/detail/program_arguments.h
    include/benchmark/detail/state.h
    include/benchmark/detail/statistics.h
    include/benchmark/detail/variables.h)
target_include_directories(benchmark PUBLIC include/)

if(WITH_EXAMPLES)
    add_subdirectory(examples)
endif()

if(WITH_TESTS)
    add_subdirectory(tests)
endif()
