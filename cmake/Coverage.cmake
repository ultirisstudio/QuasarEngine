# Usage: cmake --preset linux-gcc-coverage, or set QUASAR_COVERAGE when configuring with GCC 14 or newer

option(QUASAR_COVERAGE "Measure the code coverage of the tests with gcov" OFF)

if(NOT QUASAR_COVERAGE)
    return()
endif()

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_VERSION VERSION_LESS 14)
    message(FATAL_ERROR "The code coverage needs GCC 14 or newer, whose gcov reads the counters and measures the conditions.")
endif()

target_compile_options(quasar_compile_options INTERFACE --coverage -fcondition-coverage -fprofile-update=atomic)
target_link_options(quasar_compile_options INTERFACE --coverage)
