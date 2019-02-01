cmake_minimum_required(VERSION 3.2)
project(benchmark)

option(WITH_EXAMPLES "Build examples" ON)

add_library(benchmark INTERFACE)
target_include_directories(benchmark INTERFACE include/)

if(WITH_EXAMPLES)
    add_subdirectory(examples)
endif()
