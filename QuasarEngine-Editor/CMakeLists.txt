file(GLOB_RECURSE QE_EDITOR_SOURCES
  CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(QuasarEngine-Editor ${QE_EDITOR_SOURCES})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "src" FILES ${QE_EDITOR_SOURCES})

qe_configure_target(QuasarEngine-Editor "Editor")

target_include_directories(QuasarEngine-Editor PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_definitions(QuasarEngine-Editor PRIVATE
	GLFW_INCLUDE_NONE
	IMGUI_DEFINE_MATH_OPERATORS
)

target_link_libraries(QuasarEngine-Editor PRIVATE
  QuasarEngine-Core

  tinyfiledialogs
  ImGui
  ImGuizmo
  yaml-cpp
  zlib
  TextEditor

  glm
  entt
  Lua::Lua
  sol2::sol2
)

if(QE_BUILD_PHYSX AND WIN32)
  target_link_libraries(QuasarEngine-Editor PRIVATE PhysXSDK)
  add_dependencies(QuasarEngine-Editor PhysX_Build)

  add_custom_command(TARGET QuasarEngine-Editor POST_BUILD
    COMMAND ${CMAKE_COMMAND}
      -Dsrc:PATH=$<IF:$<CONFIG:Debug>,${PHYSX_BIN_CHECKED},${PHYSX_BIN_RELEASE}>
      -Ddst:PATH=$<TARGET_FILE_DIR:QuasarEngine-Editor>
      -P "${CMAKE_SOURCE_DIR}/cmake/CopyPhysXRuntime.cmake"
    VERBATIM
  )
endif()
