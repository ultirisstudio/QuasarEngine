cmake_minimum_required(VERSION 3.21)

project(QuasarEngine LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  add_compile_options(/MP)
endif()

set(QE_BIN_DIR     "${CMAKE_SOURCE_DIR}/bin")
set(QE_BIN_INT_DIR "${CMAKE_SOURCE_DIR}/bin-int")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(QuasarCommon)
include(Dependencies)

add_subdirectory(QuasarEngine-Core)
add_subdirectory(QuasarEngine-Editor)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT QuasarEngine-Editor)
