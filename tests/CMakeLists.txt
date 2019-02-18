add_executable(tests tests.cpp)

#enable_testing()
#include(GoogleTest)

if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()
find_package(GTest REQUIRED)

target_link_libraries(tests benchmark GTest::GTest)


#add_test(NAME Tests COMMAND tests)