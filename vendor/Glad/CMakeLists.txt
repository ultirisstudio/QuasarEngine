cmake_minimum_required(VERSION 3.5)
project(glad C)

add_library(Glad STATIC)

target_sources(Glad PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/src/glad.c
)

target_include_directories(Glad
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

set_target_properties(Glad PROPERTIES FOLDER "Dependencies/Glad")
