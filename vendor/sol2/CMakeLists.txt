cmake_minimum_required(VERSION 3.20)
project(sol2 CXX)

add_library(sol2 STATIC)

target_compile_features(sol2 PUBLIC cxx_std_17)

target_include_directories(sol2 PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

set(SOL2_DUMMY "${CMAKE_CURRENT_BINARY_DIR}/sol2_dummy.cpp")
file(WRITE "${SOL2_DUMMY}" "#include <sol/sol.hpp>\n")

target_sources(sol2 PRIVATE "${SOL2_DUMMY}")

target_link_libraries(sol2 PUBLIC lua)

set_target_properties(sol2 PROPERTIES FOLDER "Dependencies/sol2")
