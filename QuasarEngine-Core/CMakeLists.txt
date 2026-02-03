file(GLOB_RECURSE QE_CORE_SOURCES
  CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_library(QuasarEngine-Core STATIC ${QE_CORE_SOURCES})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "src" FILES ${QE_CORE_SOURCES})

qe_configure_target(QuasarEngine-Core "Core")

target_include_directories(QuasarEngine-Core
  PUBLIC  "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_precompile_headers(QuasarEngine-Core PRIVATE
  "src/qepch.h"
)

target_compile_definitions(QuasarEngine-Core PRIVATE
  GLFW_INCLUDE_NONE
)

qe_enable_bigobj(QuasarEngine-Core)

target_link_libraries(QuasarEngine-Core PRIVATE
  Glad
  ImGui
  zlib
  ImGuizmo
  tinyfiledialogs
  assimp::assimp
)

target_link_libraries(QuasarEngine-Core PRIVATE GLFW yaml-cpp assimp::assimp stb)

target_link_libraries(QuasarEngine-Core PUBLIC
  glm
  entt
  PhysXHeaders
  Lua::Lua
  sol2::sol2
)

if(TARGET Vulkan::Vulkan)
  target_link_libraries(QuasarEngine-Core PRIVATE Vulkan::Vulkan)
endif()

if(WIN32)
  target_link_libraries(QuasarEngine-Core PRIVATE d3d11 dxgi dxguid)
endif()
