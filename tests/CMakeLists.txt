find_package(GTest QUIET)
if(NOT GTest_FOUND)
    message(STATUS "GTest not found; benchmark tests will not be built")
    return()
endif()

add_executable(tests tests.cpp)
target_link_libraries(tests benchmark GTest::GTest)
