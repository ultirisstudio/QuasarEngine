cmake_minimum_required(VERSION 3.5)
project(imguizmo CXX)

add_library(ImGuizmo STATIC)

target_sources(ImGuizmo PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/ImGuizmo.cpp
)

target_include_directories(ImGuizmo
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_compile_features(ImGuizmo PUBLIC cxx_std_17)

target_link_libraries(ImGuizmo PUBLIC ImGui)

target_compile_definitions(ImGuizmo PUBLIC IMGUI_DEFINE_MATH_OPERATORS)

set_target_properties(ImGuizmo PROPERTIES FOLDER "Dependencies/ImGuizmo")
