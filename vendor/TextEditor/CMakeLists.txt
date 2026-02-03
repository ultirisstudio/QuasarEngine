cmake_minimum_required(VERSION 3.5)
project(TextEditor CXX)

add_library(TextEditor STATIC
  ${CMAKE_CURRENT_LIST_DIR}/src/TextEditor.cpp
)

target_include_directories(TextEditor
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(TextEditor PUBLIC ImGui)

target_compile_features(TextEditor PUBLIC cxx_std_17)

set_target_properties(TextEditor PROPERTIES FOLDER "Dependencies/TextEditor")
