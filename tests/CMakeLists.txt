add_executable(tests tests.cpp)

enable_testing()
include(GoogleTest)

cmake_policy(SET CMP0074 NEW)
find_package(GTest REQUIRED)

target_link_libraries(tests benchmark GTest::GTest)

add_test(NAME Tests COMMAND tests)