add_executable(QuasarSandbox)

target_sources(QuasarSandbox PRIVATE
    src/SandboxApplication.cpp
)

target_link_libraries(QuasarSandbox PRIVATE
    Quasar::EntryPoint
    Quasar::CompileOptions
)

set_target_properties(QuasarSandbox PROPERTIES FOLDER "Applications")
