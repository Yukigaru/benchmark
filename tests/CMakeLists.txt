find_package(GTest QUIET)
if(NOT GTest_FOUND)
    message(STATUS "GTest not found; benchmark tests will not be built")
    return()
endif()

add_executable(benchmark_tests tests.cpp statistics_tests.cpp)
target_link_libraries(benchmark_tests benchmark GTest::GTest)
add_test(NAME benchmark_tests COMMAND benchmark_tests)
