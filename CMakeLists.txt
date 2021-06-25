cmake_minimum_required(VERSION 3.10)
project(benchmark LANGUAGES CXX)

if (POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

option(WITH_EXAMPLES "Build examples" ON)
option(WITH_TESTS "Build tests" ON)

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

add_library(benchmark STATIC
    src/benchmark.cpp
    include/benchmark/benchmark.h
    include/benchmark/detail/benchmark_setup.h
    include/benchmark/detail/config.h
    include/benchmark/detail/cpu_info.h
    include/benchmark/detail/dont_optimize.h
    include/benchmark/detail/program_arguments.h
    include/benchmark/detail/state.h
    include/benchmark/detail/statistics.h
    include/benchmark/detail/variables.h
    include/benchmark/detail/colorization.h)
target_compile_features(benchmark PUBLIC cxx_std_17)
set_target_properties(benchmark PROPERTIES CXX_EXTENSIONS OFF)
target_include_directories(benchmark PUBLIC include/)
target_link_libraries(benchmark PUBLIC Threads::Threads)

if(WITH_EXAMPLES)
    add_subdirectory(examples)
endif()

if(WITH_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
