add_executable(QuasarSandbox)

target_sources(QuasarSandbox PRIVATE
    src/SandboxApplication.cpp
    src/SandboxLayer.cpp
    src/SandboxLayer.h
)

target_link_libraries(QuasarSandbox PRIVATE
    Quasar::EntryPoint
    Quasar::CompileOptions
)

add_custom_target(QuasarSandboxResources
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${CMAKE_CURRENT_SOURCE_DIR}/resources" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources/sandbox"
    VERBATIM
)
add_dependencies(QuasarSandbox QuasarSandboxResources)

set_target_properties(QuasarSandbox QuasarSandboxResources PROPERTIES FOLDER "Applications")
