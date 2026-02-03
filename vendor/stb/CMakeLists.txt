cmake_minimum_required(VERSION 3.5)
project(stb CXX)

add_library(stb STATIC)

target_sources(stb PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/src/stb_image.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/stb_rect_pack.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/stb_truetype.cpp
)

target_include_directories(stb
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_compile_features(stb PUBLIC cxx_std_17)

set_target_properties(stb PROPERTIES FOLDER "Dependencies/stb")
