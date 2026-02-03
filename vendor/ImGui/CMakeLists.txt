cmake_minimum_required(VERSION 3.5)
project(imgui CXX)

option(QE_IMGUI_WITH_GLFW   "Enable imgui_impl_glfw platform backend" ON)
option(QE_IMGUI_WITH_WIN32  "Enable imgui_impl_win32 platform backend" OFF)

option(QE_IMGUI_WITH_VULKAN "Enable imgui_impl_vulkan renderer backend" ON)
option(QE_IMGUI_WITH_DX11   "Enable imgui_impl_dx11 renderer backend" OFF)
option(QE_IMGUI_WITH_DX12   "Enable imgui_impl_dx12 renderer backend" OFF)

option(QE_IMGUI_WITH_OPENGL3 "Enable imgui_impl_opengl3 renderer backend" OFF)

add_library(ImGui STATIC)

target_sources(ImGui PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/imgui/imgui.cpp
  ${CMAKE_CURRENT_LIST_DIR}/imgui/imgui_draw.cpp
  ${CMAKE_CURRENT_LIST_DIR}/imgui/imgui_tables.cpp
  ${CMAKE_CURRENT_LIST_DIR}/imgui/imgui_widgets.cpp
  ${CMAKE_CURRENT_LIST_DIR}/imgui/imgui_demo.cpp
  
  ${CMAKE_CURRENT_LIST_DIR}/imgui/misc/cpp/imgui_stdlib.cpp
)

target_sources(ImGui PRIVATE
)

target_include_directories(ImGui PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/imgui
    ${CMAKE_CURRENT_LIST_DIR}/backends
	
    ${CMAKE_CURRENT_LIST_DIR}/misc/cpp
)

target_compile_features(ImGui PUBLIC cxx_std_17)

if(QE_IMGUI_WITH_GLFW)
  target_sources(ImGui PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/backends/imgui_impl_glfw.cpp
  )
  target_link_libraries(ImGui PUBLIC GLFW)
endif()

if(QE_IMGUI_WITH_WIN32)
  if(NOT WIN32)
    message(FATAL_ERROR "QE_IMGUI_WITH_WIN32 requires Windows")
  endif()

  target_sources(ImGui PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/backends/imgui_impl_win32.cpp
  )
  target_link_libraries(ImGui PUBLIC imm32)
endif()

if(QE_IMGUI_WITH_VULKAN)
  target_sources(ImGui PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/backends/imgui_impl_vulkan.cpp
  )
  target_link_libraries(ImGui PUBLIC Vulkan::Vulkan)
endif()

if(QE_IMGUI_WITH_DX11)
  if(NOT WIN32)
    message(FATAL_ERROR "QE_IMGUI_WITH_DX11 requires Windows")
  endif()

  target_sources(ImGui PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/backends/imgui_impl_dx11.cpp
  )
  target_link_libraries(ImGui PUBLIC d3d11 dxgi dxguid d3dcompiler)
endif()

if(QE_IMGUI_WITH_DX12)
  if(NOT WIN32)
    message(FATAL_ERROR "QE_IMGUI_WITH_DX12 requires Windows")
  endif()

  target_sources(ImGui PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/backends/imgui_impl_dx12.cpp
  )
  target_link_libraries(ImGui PUBLIC d3d12 dxgi dxguid d3dcompiler)
endif()

if(QE_IMGUI_WITH_OPENGL3)
  target_sources(ImGui PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/backends/imgui_impl_opengl3.cpp
  )

  target_link_libraries(ImGui PUBLIC Glad)

  if(WIN32)
    target_link_libraries(ImGui PUBLIC opengl32)
  endif()
endif()

set_target_properties(ImGui PROPERTIES FOLDER "Dependencies/ImGui")
