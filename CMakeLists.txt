cmake_minimum_required(VERSION 3.24...4.1)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not allowed. Use a preset, for example: cmake --preset windows-debug")
endif()

if(NOT DEFINED CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Available configurations")
endif()

project(QuasarEngine
    VERSION 0.1.0
    DESCRIPTION "Cross-platform 3D engine with an integrated editor"
    LANGUAGES C CXX
)

get_property(QUASAR_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT QUASAR_MULTI_CONFIG AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()
if(NOT QUASAR_MULTI_CONFIG)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/$<CONFIG>")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMake")

include(cmake/CompilerOptions.cmake)

option(QUASAR_BUILD_TESTS "Build the unit tests" ON)

add_subdirectory(external)
add_subdirectory(engine)
add_subdirectory(sandbox)
add_subdirectory(editor)

if(QUASAR_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT QuasarEditor)

message(STATUS "${PROJECT_NAME} ${PROJECT_VERSION}")
message(STATUS "  Generator ........... ${CMAKE_GENERATOR}")
message(STATUS "  Compiler ............ ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Warnings as errors .. ${QUASAR_WARNINGS_AS_ERRORS}")
message(STATUS "  Unit tests .......... ${QUASAR_BUILD_TESTS}")
if(NOT QUASAR_MULTI_CONFIG)
    message(STATUS "  Build type .......... ${CMAKE_BUILD_TYPE}")
endif()
