cmake_minimum_required(VERSION 3.5)
project(tinyfiledialogs C)

add_library(tinyfiledialogs STATIC
  ${CMAKE_CURRENT_LIST_DIR}/src/tinyfiledialogs.c
)

target_include_directories(tinyfiledialogs
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

if(WIN32)
  target_link_libraries(tinyfiledialogs PUBLIC comdlg32 ole32)
endif()

set_target_properties(tinyfiledialogs PROPERTIES FOLDER "Dependencies/tinyfiledialogs")
