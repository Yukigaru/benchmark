cmake_minimum_required(VERSION 3.2)
project(benchmark)

option(WITH_EXAMPLES "Build examples" ON)
option(WITH_TESTS "Build tests" ON)

add_library(benchmark STATIC src/benchmark.cpp include/detail/benchmark_setup.h include/detail/state.h include/detail/variables.h)
target_include_directories(benchmark PUBLIC include/)

if(WITH_EXAMPLES)
    add_subdirectory(examples)
endif()

if(WITH_TESTS)
    add_subdirectory(tests)
endif()
